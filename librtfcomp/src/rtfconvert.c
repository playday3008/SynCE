///////////////////////////////////////////////////////////////////////////////
// RTFCONVERT.C
//
// Dr J A Gow : 26/2/2007
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
	unsigned char * n;
	unsigned int	tosend;
	unsigned int	sflg=0;
	unsigned int    oOut = 0;
	unsigned char * pOut;
	unsigned int	olen;
	unsigned int    ctrllen=0;
	const unsigned char * par = (unsigned char *)"par";
	const unsigned char * fonttbl = (unsigned char *)"fonttbl";
	RTFOPTS		opts;

	enum {
		IDLE,
		START_CTRL,
		IN_CTRL,
		GROUP_CMD
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

	state = IDLE;

	while(rtflen--) {

		curch = *ip;
		sflg  = 0;

		switch(state) {

			case IDLE:		if(curch=='\\') {
							state = START_CTRL;
						} else {
							if((curch!=0x0a)&&(curch!=0x0d)&&(curch!='{')&&(curch!='}')) {
								tosend = curch;
								sflg=1;
							}
						}
						break;
	
			case START_CTRL:	ctrllen=1;
						strt = ip;
						state = IN_CTRL;
						break;

			case IN_CTRL:		if((curch == '\\')||(curch<=0x20)||(curch=='{')||(curch=='}')||(curch==';')) {

							// process control code

							if(ctrllen >= 3) {

								if(!memcmp(strt,par,3)) {

									// send newline

									tosend = 0x0a;
									sflg = 1;
									state = IDLE;

								}

								if(!memcmp(strt,fonttbl,ctrllen)) {

									state=GROUP_CMD;
								}

								if(*strt == 0x27) {

									// we have two digits of hex
									if(sscanf((char *)strt+1,"%2x",&tosend)==1) {
										sflg=1;
									}
									state = IDLE;
								}
							}
							if(curch=='\\') {
								state = START_CTRL;
							} 

						} else {
							ctrllen++;
						}
						break;

			case GROUP_CMD:		if(curch=='}') {
							state = IDLE;
						}
						break;

			default:		break;
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
		ip++;
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
		// of ANSI range, replace (for the moment) with 0x20 (space)

		if(uc>255) {
			uc = 0x20;
		}

		// shouldn't be greater than 7

		if((oOut+7) > olen) {

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
			oOut+=sprintf((char *)(out+oOut),"\\'%2.2x",uc);
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






