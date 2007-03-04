///////////////////////////////////////////////////////////////////////////////
// MAIN.C
//
// Copies from a compressed RTF file to a raw RTF file, then back to a
// compressed RTF file
//
// ./test <infile> <midfile> <outfile>
//
// Dr J A Gow : 2/2007
//
// This file is distributed under the terms and conditions of the LGPL - please
// see the file LICENCE in the package root directory.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <rtfcomp/rtfcomp.h>

unsigned char inbuf[8192];

int main(int argc, char * argv[])
{
	int rc;
	FILE *fpin, *fpmiddle, *fpout;
	unsigned int inlen;
	unsigned char * out;
	unsigned int outlen;
	unsigned char * recomp;
	unsigned int recmplen;

	if((fpin=fopen(argv[1],"rb"))!=NULL) {
		inlen = fread(inbuf,1,8192,fpin);
		if(inlen>0) {

			// First phase: decompress

			printf("\nhave %d bytes at input - decompressing\n", inlen);

			if((rc=LZRTFDecompress(&out,&outlen,inbuf,inlen))==LZRTF_ERR_NOERROR) {

				printf("decompress done, outlen %d\n",outlen);
		
				printf("saving decompressed data\n");

				if((fpmiddle=fopen(argv[2],"wb"))!=NULL) {
					fwrite(out,1,outlen,fpmiddle);
					fclose(fpmiddle);
				} else {
					printf("failed to open middle file\n");
				}

				// Now save the decompressed file

				printf("Now recompressing\n");

				if((rc=LZRTFCompress(&recomp,&recmplen,out,outlen))==LZRTF_ERR_NOERROR) {

					// Saving recompressed data

					printf("recompress done, writing file %s\n",argv[3]);

					if((fpout=fopen(argv[3],"wb"))!=NULL) {
						fwrite(recomp,1,recmplen,fpout);
						fclose(fpout);
					} else {
						printf("failed to open out file\n");
					}
					free(recomp);
				} else {
					printf("failed to recompress - rc=%d\n",rc);
				}
				free(out);
			} else {
				printf("failed to decompress - rc=%d\n",rc);
			}
		} else {
			printf("no bytes to compress\n");
		}
		fclose(fpin);
	} else {
		printf("unable to open input file\n");
	}
	return 0;
}
