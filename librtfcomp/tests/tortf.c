///////////////////////////////////////////////////////////////////////////////
// TORTF.C
//
// Copies from an RTF file to a UTF8 file, either compressed or uncompressed
//
// ./tortf <infile> <outfile> isCompressed
//
// isCompressed = 0 for uncompressed, 1 for compressed
//
// Dr J A Gow : 2/2007
//
// This file is distributed under the terms and conditions of the LGPL - please
// see the file LICENCE in the package root directory.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtfcomp/rtfcomp.h>


unsigned char * header = (unsigned char *)
			 "\\ansi \\deff0{\\fonttbl{\\f0\\fnil\\fcharset0\\fprq0 Tahoma;}{\\f1\\froman\\fcharset2\\fprq2"
			 "Symbol;}{\\f2\\fswiss\\fcharset204\\fprq2;}}{\\colortbl;\\red0\\green0\\blue0;\\red128\\green128"
                         "\\blue128;\\red192\\green192\\blue192;\\red255\\green255\\blue255;\\red255\\green0\\blue0;\\red0"
                         "\\green255\\blue0;\\red0\\green0\\blue255;\\red0\\green255\\blue255;\\red255\\green0\\blue255;"
                         "\\red255\\green255\\blue0;\\red128\\green0\\blue0;\\red0\\green128\\blue0;\\red0\\green0"
                         "\\blue128;\\red0\\green128\\blue128;\\red128\\green0\\blue128;\\red128\\green128\\blue0;}\x0a";


unsigned char inbuf[8192];

int main(int argc, char * argv[])
{

	FILE *fpin, *fpmiddle;
	unsigned int inlen;
	unsigned char * out;
	unsigned int outlen;
	int rc=0;
	RTFOPTS options = { sizeof(RTFOPTS), 0 };

	if(argc>3) {
		if(!strcmp(argv[3],"1")) {
			options.isCompressed=1;
		}
	}

	if((fpin=fopen(argv[1],"rb"))!=NULL) {
		inlen = fread(inbuf,1,8192,fpin);
		if(inlen>0) {

			// First phase: decompress

			printf("\nhave %d bytes at input - converting to RTF \n", inlen);

			if((rc=LZRTFConvertUTF8ToRTF(&out,&outlen,inbuf,inlen,header,strlen((char *)header),&options))==0) {

				printf("conversion done, outlen %d\n",outlen);
		
				printf("saving converted data\n");

				if((fpmiddle=fopen(argv[2],"wb"))!=NULL) {
					fwrite(out,1,outlen,fpmiddle);
					fclose(fpmiddle);
				} else {
					printf("failed to open middle file\n");
				}

				free(out);
			} else {
				printf("failed to convert rc=%d\n",rc);
			}
		} else {
			printf("no bytes to convert\n");
		}
		fclose(fpin);
	} else {
		printf("unable to open input file\n");
	}
	return 0;
}
