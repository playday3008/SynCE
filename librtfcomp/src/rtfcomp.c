///////////////////////////////////////////////////////////////////////////////
// RTFCOMP.C
//
// LZRTF compression module
//
// Dr J A Gow : 24/2/2007
//
// This file is distributed under the terms and conditions of the LGPL - please
// see the file LICENCE in the package root directory.
//
//
///////////////////////////////////////////////////////////////////////////////

#include <rtfcomp/rtfcomp.h>
#include "sysincludes.h"
#include "constants.h"
#include "crc32.h"

//
// Sample compressed RTF coder.

#define LZRTF_TYPE_LITERAL	0
#define LZRTF_TYPE_REFERENCE	1
#define LZRTF_TYPE_EOBMARKER	2

//
// Internal data structures

// Table entry

typedef struct _tag_ENTRY {

	unsigned int		type;	// 1 = ref, 0 = literal
	unsigned int            offset;	// offset of ref
	unsigned int            len;	// length of ref
	
	struct _tag_ENTRY *	next;	

} ENTRY;

typedef ENTRY * PENTRY;

// Coder object

typedef struct _tag_RTFCODE {

	PENTRY 		pTable;
	PENTRY 		pLastEntry;

	unsigned char *	pSrc;
	unsigned int	len;

	unsigned char *	EncoderWin;	 // offset to start of encoder window
	unsigned int	EncodedLen;	 // length of already encoded string

	unsigned int	SearchStrOffset; // start of string to search for (current encoding pos)
	unsigned int	SearchLen;	 // length of string to search for.
	
	unsigned int	fitOffset;	 // offset of last match from start of encoder win

	unsigned char * response;	 // response.

} RTFCODE;

typedef RTFCODE *  PRTFCODE;

//
// Internal function prototypes

static int LZRTFChunkResponse(PRTFCODE pRtfCode);
static PENTRY LZRTFAddNode(PRTFCODE  pRtfCode);
static int LZRTFFindEntry(PRTFCODE  pRtfCode);
static int LZRTFDestroyTable(PRTFCODE  pRtfCode);

#define LZRTF_WINDOW	4096

//
// Exported functions

///////////////////////////////////////////////////////////////////////////////
// LZRTFCompress
//
// EXPORTED, DLLAPI
//
// Compress an RTF string. Feed it a pointer to receive the result string
// pointer, the pointer to the source string and the length of the source
// string. The function will return error codes as documented in errorcodes.h
//
// Note, you (as the caller) takes ownership of the destination byte buffer
// which is dynamically allocated: so better make sure you free it when done
// with it or memory leaks will ensue. Note that the length of the output string
// can be had from the first little-endian DWORD in the output buffer, but
// it is returned anyway for convenience.
//
///////////////////////////////////////////////////////////////////////////////

int _DLLAPI LZRTFCompress(unsigned char ** dest, unsigned int * outlen,
                          unsigned char * src, int len)
{
	int rc=0;
	
	RTFCODE		coder;
	PENTRY		entry;
	unsigned int	win=0;	// window number
	unsigned int	bcount;
	unsigned char *	srcwithhdr;

	if(!dest||!src) {
		return LZRTF_ERR_BADARGS;
	}

	memset(&coder,0,sizeof(RTFCODE));

	// need loop to build entries, then to chunk-ize them
	// We build this over a 4096 byte window, then reconstruct
	// the table for the next window, and so on.

	// Ok, let's start at the very beginning. We append the header string so that
	// references can be built.

	if((srcwithhdr=(unsigned char *)malloc(len+LZRTF_HDR_LEN))!=NULL) {
		memcpy(srcwithhdr,LZRTF_HDR_DATA,LZRTF_HDR_LEN);
		memcpy(srcwithhdr+LZRTF_HDR_LEN,src,len);
	} else {
		return LZRTF_ERR_NOMEM;
	}

	// iterate over each character in the buffer, taking into account
	// the window.

	coder.pSrc = srcwithhdr;	// with header
	coder.len = len;		// with out header
	coder.EncoderWin = coder.pSrc;
	coder.EncodedLen = LZRTF_HDR_LEN;	// start at end of hdr
	entry = NULL;				// current entry NULL
	win=0;					// change window
	bcount=0;				// byte count

	// for the first window only

	coder.SearchStrOffset = LZRTF_HDR_LEN;
	coder.SearchLen = 0;

	while(bcount<len) {
	
		do {  // over each character

			coder.SearchLen++;

			if(LZRTFFindEntry(&coder)) {

				// matches. Proceed to the next one.

				if(!entry) {

					// Need a new reference node

					if((entry = LZRTFAddNode(&coder))==NULL) {
						LZRTFDestroyTable(&coder);
						free(srcwithhdr);
						return LZRTF_ERR_NOMEM;
					}
					entry->type = LZRTF_TYPE_REFERENCE;
					entry->len = 0;
					entry->offset = coder.fitOffset;
					entry->len++;
					coder.EncodedLen++;

				} else {

					if(entry->type == LZRTF_TYPE_REFERENCE) {

						// We can't have a reference greater than 17 bytes

						if((entry->len) >= 17) {

							coder.SearchStrOffset += entry->len;
							coder.SearchLen = 0;	// new search
							entry=NULL;
							bcount--;		// do this chr again.

						} else {
							entry->offset = coder.fitOffset;
							entry->len++;
							coder.EncodedLen++;
						}
					
					} else {

						// the previous type is LITERAL: close it and make a new
						// reference

						if((entry=LZRTFAddNode(&coder))==NULL) {
							LZRTFDestroyTable(&coder);
							free(srcwithhdr);
							return LZRTF_ERR_NOMEM;
						}
					
						entry->type = LZRTF_TYPE_REFERENCE;
						entry->len = 0;
						entry->offset = coder.fitOffset;
						entry->len++;
						coder.EncodedLen++;
					}
				}
			} else {

				// no match - make decision

				// If our current entry is a NULL, we make a new LITERAL entry

				if(!entry) {
					if((entry = LZRTFAddNode(&coder))==NULL) {
						LZRTFDestroyTable(&coder);
						free(srcwithhdr);
						return LZRTF_ERR_NOMEM;
					}
					entry->type = LZRTF_TYPE_LITERAL;
					entry->offset = LZRTF_WINDOW*win + coder.SearchStrOffset;
					entry->len = 1;
					coder.SearchStrOffset++;
					coder.EncodedLen++;
					coder.SearchLen=0;

				} else {
					if(entry->type == LZRTF_TYPE_REFERENCE) {

						// if the reference is 1 or less bytes long, we
						// reencode as a literal. Remember offsets into the
						// literal are offsets into the source buffer. However,
						// we must restart the referencing process from the new starting
						// point.

						if(entry->len < 2) {

							entry->offset = LZRTF_WINDOW*win + coder.SearchStrOffset;
							entry->type = LZRTF_TYPE_LITERAL;
						}
					
						// otherwise just close the reference and reset the search
						// offset to the new char. We have not yet encoded this char.
						// so don't increment EncodedLen

						bcount--; // go back one and check for matches.
						coder.SearchStrOffset+=entry->len;
						entry=NULL;

							
					} else {
		
						// entry type is already a literal: just add the new char.

						entry->len++;
						coder.EncodedLen++;
						coder.SearchStrOffset++;
	
						// we have decoded one more char, and we start 
						// searching on the next char
					}

					coder.SearchLen = 0;
				}
			}

		} while((coder.EncodedLen<4096) && (++bcount<len));

		// Handle dangling <2 byte references.

		if(entry) {
			if(entry->type == LZRTF_TYPE_REFERENCE) {

				if(entry->len < 2) {

					entry->offset = LZRTF_WINDOW*win + coder.SearchStrOffset;
					entry->type = LZRTF_TYPE_LITERAL;
				}
			}
		}

		// our last entry will always be valid.

		if(coder.EncodedLen>=4096) {
			win++;
			coder.EncodedLen = 0;
			coder.EncoderWin += 4096;
			entry = NULL;
		}
	} 

	// add the end of block marker

	if((entry = LZRTFAddNode(&coder))==NULL) {
		LZRTFDestroyTable(&coder);
		free(srcwithhdr);
		return LZRTF_ERR_NOMEM;
	}
	entry->type = LZRTF_TYPE_EOBMARKER;
	entry->offset = LZRTF_WINDOW*win + coder.SearchStrOffset + 1;
	entry->len = 2;

	// Now chunk the response

	if((rc=LZRTFChunkResponse(&coder))==LZRTF_ERR_NOERROR) {
		// send it back.
		*dest = coder.response;
		if(outlen) {
			*outlen = *((unsigned int *)coder.response)+4;	// add the four back in for real string len
		}
	}
	
	LZRTFDestroyTable(&coder);
	free(coder.pSrc);
	return rc;
}

///////////////////////////////////////////////////////////////////////////////
// LZRTFChunkResponse
//
// INTERNAL
//
// Dump the table out in chunked format
//
///////////////////////////////////////////////////////////////////////////////

static int LZRTFChunkResponse(PRTFCODE pRtfCode)
{
	// The response is chunked into 8-unit chunks, prefixed by a flag bit. However, we can
	// get the compressed data length easily as we can scan through the blocks.

	unsigned char * response = NULL;

	unsigned int	rsplen = 0;
	unsigned int 	rspcnt = 0;

	unsigned int	ucnt = 0;
	unsigned int    oCurFlag = 0;
	unsigned short	refUnit;
	unsigned int    litOffset=0;

	PENTRY cur = pRtfCode->pTable;

	// Now, set up the header points

	if((response=(unsigned char *)malloc(512))!=NULL) {
		rsplen = 512;
		rspcnt = 16;	// space for header
	} else {
		return LZRTF_ERR_NOMEM;
	}

	while(cur) {

		// check that we always have enough space for at least one unit
		// a flag byte, and the last ref.

		if(rsplen < (rspcnt+5)) {
			unsigned char * c;
			rsplen += 512;
			if((c = (unsigned char *)realloc(response,rsplen))!=NULL) {
				response=c;
			} else {
				free(response);
				return LZRTF_ERR_NOMEM;
			}
		}

		if(ucnt==0) {

			// this is a flag byte

			oCurFlag = rspcnt;
			response[rspcnt++]=0;
			ucnt=1;

		} else {

			// this is a unit code

			if((cur->type == LZRTF_TYPE_REFERENCE)||(cur->type==LZRTF_TYPE_EOBMARKER)) {

				// add the reference:
	
				response[oCurFlag] |= 0x01 << (ucnt-1);
				refUnit = (cur->offset << 4) & 0xfff0;
				refUnit |= (cur->len - 2) & 0x000f;
				response[rspcnt++] = refUnit>>8;
				response[rspcnt++] = refUnit&0x00ff;
		
				cur=cur->next;

			} else {
				
				// we are a literal.
				
				response[rspcnt++] = pRtfCode->pSrc[cur->offset+litOffset++];
				if(litOffset>=cur->len) {
					cur=cur->next;
					litOffset=0;
				}
			}

			if(ucnt++>=8) {
				ucnt=0;
			}
		}
	}
	
	// add the details to the header.

	*(unsigned int *)(&response[0]) = rspcnt-4; //not incl.size field
	*(unsigned int *)(&response[4]) = pRtfCode->len;
	*(unsigned int *)(&response[8]) = 0x75465a4c;
	*(unsigned int *)(&response[12]) = LZRTFCalcCRC32(response,16,rspcnt-16);

	pRtfCode->response = response;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// LZRTFAddNode
//
// INTERNAL
//
// Add a new node to the table
//
///////////////////////////////////////////////////////////////////////////////

static PENTRY LZRTFAddNode(PRTFCODE pRtfCode)
{
	PENTRY entry;
	if((entry=(PENTRY)malloc(sizeof(ENTRY)))!=NULL) {

		// Add and return the node.

		entry->type   = 0;
		entry->offset = 0;
		entry->len    = 0;
		entry->next   = NULL;
		if(!pRtfCode->pTable) {
			pRtfCode->pTable      = entry;
			pRtfCode->pLastEntry  = entry;
		} else {
			pRtfCode->pLastEntry->next = entry;
			pRtfCode->pLastEntry = entry;
		}
	}
	return entry;
}

///////////////////////////////////////////////////////////////////////////////
// LZRTFFindEntry
//
// INTERNAL
//
// Find a reference in the already-encoded portion of the block
//
///////////////////////////////////////////////////////////////////////////////

static int LZRTFFindEntry(PRTFCODE pRtfCode)
{
	int rc=0;

	if(pRtfCode->SearchLen <= pRtfCode->EncodedLen) {

		// it might fit.

		int SearchMax = pRtfCode->EncodedLen - pRtfCode->SearchLen;
		int i;

		// so go find it.

		for(i=0;i<SearchMax;i++) {
			
			if(memcmp((pRtfCode->EncoderWin+i),
                                   (pRtfCode->EncoderWin + pRtfCode->SearchStrOffset),
                                   pRtfCode->SearchLen)==0) {

				pRtfCode->fitOffset = i;
				rc=1;
				break;
			} 
		}
	}

	return rc;
}

///////////////////////////////////////////////////////////////////////////////
// LZRTFDestroyTable
//
// INTERNAL
//
// Clean up after us
//
///////////////////////////////////////////////////////////////////////////////

static int LZRTFDestroyTable(PRTFCODE pRtfCode)
{
	PENTRY e=pRtfCode->pTable;
	PENTRY del=NULL;

	while(e) {
		del = e;
		e=e->next;
		free(del);
	}
	return 0;
}
