/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/
#include "rledecoder.h"

RleDecoder::RleDecoder(Decoder* chain): Decoder(chain)
{
}


RleDecoder::RleDecoder(): Decoder()
{
}


RleDecoder::~RleDecoder()
{
}


bool RleDecoder::decode(unsigned char *target, size_t rawSize, unsigned char *source, size_t size)
{
    unsigned char *act1 = source;
    unsigned char *act2 = source + 1;
    unsigned char *act3 = source + 2;

    unsigned char val1 = *act1;
    unsigned char val2 = *act2;
    unsigned char val3 = *act3;

    unsigned char *tmp_target = target;

    size_t count = 0;
    size_t samcount = 0;
    do {
        if ((val1 == *act1) && (val2 == *act2) && (val3 == *act3)) {
            samcount++;
            *tmp_target++ = val1;
            *tmp_target++ = val2;
            *tmp_target++ = val3;
            act1 += 3;
            act2 += 3;
            act3 += 3;
            count += 3;
            if (samcount == 2) {
                samcount = 0;
                unsigned char samruncount = *act1;
                while (samruncount) {
                    *tmp_target++ = val1;
                    *tmp_target++ = val2;
                    *tmp_target++ = val3;
                    samruncount--;
                }
                act1 += 1;
                act2 += 1;
                act3 += 1;
                count += 1;
                if (count < size) {
                    val1 = *act1;
                    val2 = *act2;
                    val3 = *act3;
                }
            }
        } else {
            samcount = 0;
            *tmp_target++ = (val1 = *act1);
            *tmp_target++ = (val2 = *act2);
            *tmp_target++ = (val3 = *act3);
            act1 += 3;
            act2 += 3;
            act3 += 3;
            count += 3;
        }
    } while (count < size);

    return ((size_t) (tmp_target - target)) == rawSize;
}
