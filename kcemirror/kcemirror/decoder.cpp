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
#include "decoder.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <kdebug.h>


Decoder::Decoder()
{
    chain = NULL;
    encData = NULL;
    rawData = NULL;
}


Decoder::Decoder(Decoder *chain)
{
    this->chain = chain;
    encData = NULL;
    rawData = NULL;
}


bool Decoder::decode(unsigned char *rawData, size_t /*rawSize*/, unsigned char *encData, size_t encSize)
{
    memcpy(rawData, encData, encSize);
    return true;
}


bool Decoder::chainDecode(unsigned char *rawData, size_t rawSize)
{
    bool ret = true;

    if (chain != NULL) {
        encData = new unsigned char[encSize];
        ret = chain->chainDecode(encData, encSize);
    }

    if (ret) {
        ret = decode(rawData, rawSize, encData, encSize);
    }

    this->rawData = rawData;
    delete[] encData;

    if (chain != NULL) {
        chain->chainCleanUp();
    }

    return ret;
}


bool Decoder::readSize(int s)
{
    uint32_t encSizeN;
    bool ret = false;

    if (recv(s, (void *) &encSizeN, sizeof(uint32_t), 0) == sizeof(uint32_t)) {
        ret = true;
        encSize = ntohl(encSizeN);
    }

    return ret;
}


bool Decoder::chainReadSize(int s)
{
    bool ret;

    ret = readSize(s);

    if (chain != NULL && ret) {
        ret = chain->chainReadSize(s);
    } else if (ret) {
        ret = readData(s, encSize);
        kdDebug(2120) << "--- " << encSize << endl;
    }
    return ret;
}


bool Decoder::readData(int s, size_t encSize)
{
    uint32_t readSize = 0;
    bool ret = false;
    encData = new unsigned char[encSize];
    uint32_t n = 0;

    do {
        n = recv(s, encData + readSize, encSize - readSize, 0);
        readSize += n;
    } while (readSize < encSize && n > 0);

    if (n > 0) {
        ret = true;
    }

    return ret;
}


size_t Decoder::chainRead(int s)
{
    bool ret;
    uint32_t rawSizeN;

    if ((ret = (recv(s, (void *) &rawSizeN, sizeof(uint32_t), 0) == sizeof(uint32_t)))) {
        rawSize = ntohl(rawSizeN);
        if (!(ret = chainReadSize(s))) {
            kdDebug(2120) << "Read sizes error" << endl;
        }
    } else {
        kdDebug(2120) << "Read raw-size error" << endl;
    }

    if (!ret) {
        rawSize = 0;
    }

    return rawSize;
}


void Decoder::chainCleanUp()
{
    if (chain != NULL) {
        chain->chainCleanUp();
    }

    cleanUp();
}


void Decoder::cleanUp()
{
}


Decoder::~Decoder()
{
    if (chain != NULL) {
        delete chain;
    }
}
