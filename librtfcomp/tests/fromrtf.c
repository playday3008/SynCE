///////////////////////////////////////////////////////////////////////////////
// FROMRTF.C
//
// Copies from a UTF8 file to an RTF file, either compressed or uncompressed
//
// ./fromrtf <infile> <outfile> isCompressed
//
// isCompressed = 1 for compressed, 0 for uncompressed
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

			printf("\nhave %d bytes at input - converting to UTF8 \n", inlen);

			if((rc=LZRTFConvertRTFToUTF8(&out,&outlen,inbuf,inlen,&options))==0) {

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
