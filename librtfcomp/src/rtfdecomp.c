///////////////////////////////////////////////////////////////////////////////
// RTFDECOMP.C
//
// Decompressor module for LZRTF
//
// Dr J A Gow 24/2/2007
//
// Basic decompressor ported from some Java code and optimized.
//
// This file is distributed under the terms and conditions of the LGPL - please
// see the file LICENCE in the package root directory.
//
///////////////////////////////////////////////////////////////////////////////

#include <rtfcomp/rtfcomp.h>
#include "sysincludes.h"
#include "constants.h"
#include "crc32.h"

///////////////////////////////////////////////////////////////////////////////
// LZRTFDecompress
//
// EXPORTED, DLLAPI
//
// Decompress the RTF data block.
//
///////////////////////////////////////////////////////////////////////////////

int _DLLAPI LZRTFDecompress(unsigned char ** dest, unsigned int * outlen,
                            unsigned char * src, unsigned int len)
{

        unsigned char * dst; // destination for uncompressed bytes
        int in = 0;          // current position in src array
        int out = 0;         // current position in dest array
	int csi=0;
	unsigned int cs;

	if(!dest||!src||(len<16)) {
		return LZRTF_ERR_BADARGS;
	} 

	// FIXME - Endian sensitive.

        unsigned int compressedSize = *((unsigned int *)(src+in));
        in += 4;
        unsigned int uncompressedSize = *((unsigned int *)(src+in));
        in += 4;
        unsigned int magic = *((unsigned int *)(src+in));
        in += 4;
        unsigned int crc32 = *((unsigned int *)(src+in));
        in += 4;

        if (compressedSize != (len-4)) { // check size excluding the size field itself
            	return LZRTF_ERR_BADCOMPRESSEDSIZE;
	}

        if (crc32 != LZRTFCalcCRC32(src,16,len-16)) {
		return LZRTF_ERR_BADCRC;
	}

        // process the data

        if (magic == 0x414c454d) { // magic number that identifies the stream as a uncompressed stream

		if((dst = (unsigned char *)malloc(uncompressedSize))==NULL) {
			return LZRTF_ERR_NOMEM;
		}
		memcpy(dst,src,uncompressedSize);
		*outlen = uncompressedSize;

        } else {

		if (magic == 0x75465a4c) { // magic number that identifies the stream as a compressed stream
			
			unsigned int oblen = LZRTF_HDR_LEN + uncompressedSize;
			if((dst = (unsigned char *)malloc(oblen))==NULL) {
				return LZRTF_ERR_NOMEM;
			}
			memcpy(dst,LZRTF_HDR_DATA,LZRTF_HDR_LEN);
            		out = LZRTF_HDR_LEN;
            		int flagCount = 0;
            		int flags = 0;

			// beneath largely ported straight from the Java (yuck) code.

            		while (out < oblen) {

		                // each flag byte flags 8 literals/references, 1 per bit
        
        			flags = (flagCount++ % 8 == 0) ? src[in++] : flags >> 1;
                
				if ((flags & 1) == 1) { // each flag bit is 1 for reference, 0 for literal
                    		
					int offset = src[in++];
                    			int length = src[in++];

                    			offset = (offset << 4) | (length >> 4); // the offset relative to block start
                    			length = (length & 0xF) + 2; // the number of bytes to copy
                    			
					// the decompression buffer is supposed to wrap around back
                    			// to the beginning when the end is reached. we save the
                    			// need for such a buffer by pointing straight into the data
                    			// buffer, and simulating this behaviour by modifying the
                    			// pointers appropriately.

                    			offset = (out / 4096) * 4096 + offset;
                    			
					if (offset >= out) // take from previous block
                        			offset -= 4096;

                    			int end = offset + length;
                    			
					while (offset < end)
                        			dst[out++] = dst[offset++];

                		} else { // literal
                    
					dst[out++] = src[in++];
                		}
            		}
	    
			// copy it back without the prebuffered data

            		src = dst;
            		if((dst = (unsigned char *)malloc(uncompressedSize))==NULL) {
				free(src);
				return LZRTF_ERR_NOMEM;
			}
			memcpy(dst,src+LZRTF_HDR_LEN,uncompressedSize);
			*dest = dst;
			if(outlen) {
				*outlen=uncompressedSize;
			}
			free(src);

		} else {
            		return LZRTF_ERR_BADMAGIC;
        	}
	}

        return LZRTF_ERR_NOERROR;
}





