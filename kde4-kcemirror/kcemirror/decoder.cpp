/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 * Copyright (c) 2009 Mark Ellis <mark@mpellis.org.uk>                     *
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
    kDebug(2120) << "** entering **" << endl;

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


bool Decoder::readSize(QTcpSocket *s)
{
    uint32_t encSizeN;
    bool ret = false;

    uint32_t readSize = 0;
    int32_t n = 0;

    do {
        n = s->read((char *)&encSizeN + readSize, sizeof(uint32_t) - readSize);
        readSize += n;
        if (readSize < sizeof(uint32_t) && n == 0)
                s->waitForReadyRead(3000);
    } while (readSize < sizeof(uint32_t) && s->state() == QAbstractSocket::ConnectedState);

    if (readSize == sizeof(uint32_t)) {
        ret = true;
        encSize = ntohl(encSizeN);
    }

    return ret;
}


bool Decoder::chainReadSize(QTcpSocket *s)
{
    bool ret;

    ret = readSize(s);

    if (chain != NULL && ret) {
        ret = chain->chainReadSize(s);
    } else if (ret) {
        ret = readData(s, encSize);
        kDebug(2120) << "--- " << encSize << endl;
    }
    return ret;
}


bool Decoder::readData(QTcpSocket *s, size_t encSize)
{
    kDebug(2120) << "encSize" << encSize << endl;


    uint32_t readSize = 0;
    bool ret = false;
    encData = new unsigned char[encSize];
    int32_t n = 0;

    kDebug(2120) << "bytes available " << s->bytesAvailable() << endl;
    do {
        n = s->read((char*)encData + readSize, encSize - readSize);
        readSize += n;

        if (readSize < encSize && n == 0)
                s->waitForReadyRead(3000);

    } while (readSize < encSize && s->state() == QAbstractSocket::ConnectedState);

    /*
    kDebug(2120) << "encSize" << encSize << "readSize" << readSize << "n" << n << endl;
    */

    if (n >= 0) {
        ret = true;
    }

    return ret;
}


size_t Decoder::chainRead(QTcpSocket *s)
{
    int ret = 0;
    uint32_t rawSizeN = 0;
    uint32_t readSize = 0;
    int32_t n = 0;

    do {
        n = s->read((char *)&rawSizeN + readSize, sizeof(uint32_t) - readSize);
        readSize += n;
        if (readSize < sizeof(uint32_t) && n == 0)
                s->waitForReadyRead(3000);
    } while (readSize < sizeof(uint32_t) && s->state() == QAbstractSocket::ConnectedState);

    ret = readSize;

    if (ret == sizeof(uint32_t)) {
        rawSize = ntohl(rawSizeN);
        if (!(ret = chainReadSize(s))) {
            kDebug(2120) << "Read sizes error" << endl;
        }
    } else {
        kDebug(2120) << "Read raw-size error: " << ret << " read" << endl;
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
