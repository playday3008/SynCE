///////////////////////////////////////////////////////////////////////////////
// RTFCONVERT.C
//
// Dr J A Gow : 26/2/2007
//              14/3/2007  Applied patch by Robert Jarzmik to significanly 
//                         improve RTF handling
//
// Conversion module with functions allowing RTF data to be converted to and
// from a UTF8 string. Very crude, but designed to work with ANSI encoded
// RTF - not guaranteed to work with much else
//
// This file is distributed under the terms and conditions of the LGPL - please
// see the file LICENCE in the package root directory.
//
///////////////////////////////////////////////////////////////////////////////

#include "rtfconvert.h"
#include <rtfcomp/rtfcomp.h>
#include "sysincludes.h"
#include "utf8conv.h"

// Internal functions

static int RTFCharget(const unsigned char *sin, int maxlen, unsigned int *computed_UTF8);
static int RTFGetControlWordLen(const unsigned char *sin, int maxlen);
static int RTFGetClosingBraceIndex(const unsigned char *sin, int maxlen);

//
// Exported functions

///////////////////////////////////////////////////////////////////////////////
// LZRTFConvertRTFToUTF8
//
// EXPORTED, DLLAPI
//
// Convert an RTF-encoded string to a UTF-8 encoded one. This is crude - we
// just assume that the RTF encoding is ANSI and that all characters are 
// either raw, or escaped out with the backslash/apostrophe sequence
//
///////////////////////////////////////////////////////////////////////////////

int _DLLAPI LZRTFConvertRTFToUTF8(unsigned char ** utfout, unsigned int * utflen,
                                  unsigned char * rtfin, unsigned int rtflen,
                                  RTFOPTS * options)
{
	unsigned char * ip=0;
	unsigned char * strt=0;
	unsigned char   curch;
	int             pNextCh = 0;
	unsigned char * n;
	unsigned int	tosend;
	unsigned int	sflg=0;
	unsigned int    oOut = 0;
	unsigned char * pOut;
	unsigned int	olen;
	unsigned int    ctrllen=0;
	int             brace_level = 0;
	int             cmd_brace_level = 0;
	unsigned char   buf[2048];
	const unsigned char * par = (unsigned char *)"\\par";
	const unsigned char * fonttbl = (unsigned char *)"\\fonttbl";
	RTFOPTS		opts;

	

	enum {
		IDLE,
		GROUP_CMD,
	} state;

	if(!utfout||!rtfin||rtflen==0) {
		return LZRTF_ERR_BADARGS;
	}

	// deal with any options we may have

	memset(&opts,0,sizeof(RTFOPTS));
	if(options) {
		int lencpy = (options->lenOpts>sizeof(RTFOPTS))?sizeof(RTFOPTS):options->lenOpts;
		memcpy(&opts,options,lencpy);
	}

	// are we sucking in compressed data? If so, decompress it first

	if(opts.isCompressed) {

		unsigned char * uncomp;
		unsigned int unclen;
		int rc;
		if((rc=LZRTFDecompress(&uncomp,&unclen,rtfin,rtflen))==LZRTF_ERR_NOERROR) {
			rtfin = uncomp;
			rtflen = unclen;
		} else {
			return rc;
		}
	}
	
	ip=rtfin;

	// start here

	olen = 512;
	if((pOut=(unsigned char *)malloc(olen))==NULL) {
		return LZRTF_ERR_NOMEM;
	}

	brace_level = 0;
	
	while(rtflen>0) {
		
		curch = *ip;
		sflg  = 0;
		pNextCh = -1;
		memset(buf, 0, sizeof(buf));
		
		// First, check if is a character (unicoded, ascii or escaped) 

		pNextCh = RTFCharget(ip, rtflen, &tosend);
		if (pNextCh != -1) {
			sflg = 1;
		}
		
		// Second, check if is a rtf control word 

		if (pNextCh == -1) {
		    pNextCh = RTFGetControlWordLen(ip, rtflen);
		}

		// Third, keep trace of brace level (not needed by now) 

		if (curch == '{') {
			brace_level++;
		}

		if (curch == '}') {
			brace_level--;
		}

		// Fourth, handle special control word \fonttbl 

		if ((pNextCh >= strlen(fonttbl)) && (strncmp(ip, fonttbl, strlen(fonttbl)) == 0)) {
			pNextCh = RTFGetClosingBraceIndex(ip, rtflen);
		}

		// Fifth, handle special control word \par 

		if ((pNextCh >= strlen(par)) && (strncmp(ip, par, strlen(par)) == 0)) {
			tosend = 0x0a;
			sflg = 1;
		}

		// Default: just skip the rtf character 

		if (pNextCh <= 0) {
			pNextCh = 1;
		}

		if(sflg) {
			int u8len;
			
			// one to send: get the UTF8 length
			
			u8len = CV_SizeOfUTF8(tosend);
			
			if(u8len!=-1) {
				
				if((oOut+u8len)>=olen) {
					olen+=256;
					if((n=(unsigned char *)realloc(pOut,olen))==NULL) {
						free(pOut);
						if(opts.isCompressed) {
							free(rtfin);
						}
						return LZRTF_ERR_NOMEM;
					} else {
						pOut=n;
					}
				}
				oOut+=CV_UTF8FromUTF32(tosend,pOut+oOut);
			} 
		}
		
		ip += pNextCh;
		rtflen -= pNextCh;
	}

	*utfout = pOut;

	if(utflen) {
		*utflen = oOut;
	}

	// if we were compressed, free the uncompressed array.

	if(opts.isCompressed) {
		free(rtfin);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// LZRTFConvertUTF8ToRTF
//
// EXPORTED, DLLAPI
//
// Convert an RTF-encoded string to a UTF-8 encoded one. This is crude - we
// just assume that the RTF encoding is ANSI and that all characters are 
// either raw, or escaped out with the backslash/apostrophe sequence
// The header should be without the enclosing group, and  without the \rtf1
// control code. Unicode code points outside of the ANSI range are
// encoded as \uxxxxxxxx
//
///////////////////////////////////////////////////////////////////////////////

int _DLLAPI LZRTFConvertUTF8ToRTF(unsigned char ** rtfout, unsigned int * lenout,
                                  unsigned char * utfin, unsigned int len,
                                  unsigned char * rtfhdr, unsigned int hdrlen,
                                  RTFOPTS * options)
{
	int rc=0;
	unsigned char * out;
	unsigned char * n;
	unsigned int    oIn=0;
	unsigned int	chrIsBad=0;
	unsigned int	uc;
	
	unsigned int 	olen=512;
	unsigned int    oOut=0;
	const unsigned char * pfx = (unsigned char *)"{\\rtf1";
	const unsigned char * par = (unsigned char *)"\x0a\x0d\\par \x0a\x0d";
	RTFOPTS		opts;

	if(!rtfout||!utfin||len==0) {
		return LZRTF_ERR_BADARGS;
	}

	// deal with any options we may have

	memset(&opts,0,sizeof(RTFOPTS));
	if(options) {
		int lencpy = (options->lenOpts>sizeof(RTFOPTS))?sizeof(RTFOPTS):options->lenOpts;
		memcpy(&opts,options,lencpy);
	}

	if((out=(unsigned char *)malloc(olen))==NULL) {
		return LZRTF_ERR_NOMEM;
	}

	memcpy(out,pfx,6);
	oOut += 6;

	// initial preprocess

	if(rtfhdr) {
		olen += hdrlen;
		if((n=(unsigned char *)realloc(out,olen))==NULL) {
			free(out);
			return -1;
		} else {
			out = n;
			memcpy(out+oOut,rtfhdr,hdrlen);
			oOut += hdrlen;
		}
	}

	// ready for the data. Remember to translate 0x0a into /par<space>
	
	while(len) {

		int clen = CV_SizeOfUTF8Data(utfin+oIn);
		if(len>=clen) {
			len-=clen;
		} else {
			chrIsBad = 1;
			break; // last char is bad
		}
		if(clen<=4) {
			uc = CV_UTF32FromUTF8(utfin+oIn);
		} else {
			chrIsBad = 1;
			uc = 0x20;
		}

		oIn+=clen;

		// if we get here, we have a good Unicode code point in uc.
		// Process it. If it is 0x0a, replace with rtf \par. Out
		// of ANSI or 16bits UNICODE replace (for the moment) with 0x20 (space)

		if(uc>65535) {
			uc = 0x20;
		}

		// shouldn't be greater than 8

		if((oOut+12) > olen) {

			olen+=512;

			if((n=(unsigned char *)realloc(out,olen))==NULL) {
				free(out);
				return -1;
			} else {
				out=n;
			}
		}

		if(uc==0x0a) {
			memcpy(out+oOut,par,9);
			oOut += 9;
		} else {
			if (uc<256)
				oOut+=sprintf((char *)(out+oOut),"\\'%2.2x",uc);
			else
				oOut+=sprintf((char *)(out+oOut),"\\u%d\\'3f",uc);
		}
	}

	// final }

	if((oOut+3)>=olen) {
		olen+=2;
		if((n=(unsigned char *)realloc(out,olen))==NULL) {
			free(out);
			return -1;
		} else {
			out=n;
		}
	}
	*(out+oOut++) = 0x0a;
	*(out+oOut++) = 0x0d;
	*(out+oOut++) = '}';

	if(chrIsBad) {
		free(out);
		rc=LZRTF_ERR_BADINPUT;
	}

	// do we want to emit compressed RTF?

	if(opts.isCompressed) {
		unsigned char * compstream;
		unsigned int lencomp;
		if((rc=LZRTFCompress(&compstream,&lencomp,out,oOut))==LZRTF_ERR_NOERROR) {
			free(out);
			out = compstream;
			oOut = lencomp;
		} else {
			free(out);
			return rc;
		}
	}

	if(lenout) {
		*lenout = oOut;
	}
	*rtfout = out;

	return rc;
}

///////////////////////////////////////////////////////////////////////////////
// RTFCharget
//
// INTERNAL
//
// Return len in byte of RTF char in string (-1 if not char)
//        unicode UTF32 char
//
// The len would be : 4 for \'20, greater than 2 for \uNNN, 1 for ANSI
// This is crude - we just assume that the RTF encoding is ANSI and that all 
// characters are either raw, or escaped out with the backslash/apostrophe 
// sequence or unicode rtf sequence \uxxxx (where xxxx is decimal 16bit signed
// value)
///////////////////////////////////////////////////////////////////////////////

static int RTFCharget(const unsigned char *sin, int maxlen, unsigned int *computed_UTF8)
{
	static char *escape_header = "\\'";
	static char *unicode_header = "\\u";
	int header_len = 2;
	int len = -1;
	int skip_next = -1;
	unsigned char c;
	unsigned int skipped_utf;

	if ((maxlen >= (header_len + 2)) && (strncmp(sin, escape_header, header_len) == 0)) {
		len = header_len + 2;

		if (sscanf(sin + header_len, "%2x", computed_UTF8) != 1) {
			len = -1;
		}

		return len;
	}

	if ((maxlen > header_len) && (strncmp(sin, unicode_header, header_len) == 0)) {
		len = header_len;
		c = sin[len];

		while ((len<maxlen) && ((c == '-') || ((c>= '0') && (c <= '9')))) {
			c = sin[++len];
		}

		if (sscanf(sin + header_len, "%d", computed_UTF8) != 1) {
			len = -1;
		}

		// Skip next character after \uN, according to RTF spec

		if ((len >= 1) && (maxlen - len >= 1)) {
			skip_next = RTFCharget(sin+len, maxlen - len, &skipped_utf);
		}

		if (skip_next > 0) {
			len += skip_next;
		}

		return len;
	}

	if ((*sin >= 32) && (*sin <= 127) && (*sin != '\\') && 
            (*sin != '{') && (*sin != '}')) {
		len = 1;
		*computed_UTF8 = *sin;
	}

	return len;		
}

///////////////////////////////////////////////////////////////////////////////
// RTFGetControlWordLen
//
// INTERNAL
//
// Return len in byte of control work
//
// The len would be : 6 for \bold0, -1 for a
///////////////////////////////////////////////////////////////////////////////

static int RTFGetControlWordLen(const unsigned char *sin, int maxlen)
{
	int len = -1;
	int idx = 0;
	unsigned char c;

	if (sin[idx++] != '\\')
		idx = maxlen;

	while (idx < maxlen) {
		if ((sin[idx] >= 'a') && (sin[idx] <= 'z')) {
			idx ++;
		} else {
			break;
		}
	}

	while (idx < maxlen) {
		if ((sin[idx] >= '0') && (sin[idx] <= '9')) {
			idx ++;
		} else {
			break;
		}
	}

	while (idx < maxlen) {
		if ((sin[idx] == ';') || (sin[idx] == ' ')) {
			idx ++;
		} else {
			break;
		}
	}

	if (idx < maxlen) {
		len = idx;
	}

	return len;
}

///////////////////////////////////////////////////////////////////////////////
// RTFGetClosingBraceIndex
//
// INTERNAL
//
// Return string index in bytes to the matching closing brace, or -1 if failure
//
// The index in string would be : 12 for \bold{titi}a}cc, -1 for a
///////////////////////////////////////////////////////////////////////////////

static int RTFGetClosingBraceIndex(const unsigned char *sin, int maxlen)
{
	int len = -1;
	int idx = 0;
	int brace_level = 1;
	unsigned char c;

	while (idx < maxlen) {

		c = sin[idx];

		switch(c) {
			case '{':	brace_level++;
					break;
			
			case '}':	brace_level--;
					break;
		}
		if (brace_level == 0) {
			len = idx;
			break;
		}
		idx ++;
	}

	return len;
}
