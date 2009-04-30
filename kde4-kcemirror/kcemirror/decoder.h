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
#ifndef DECODER_H
#define DECODER_H

#include <sys/types.h>
#include <QTcpSocket>

/**
@author Volker Christian
*/
class Decoder{
public:
    Decoder();
    Decoder(Decoder *chain);
    virtual bool decode(unsigned char *rawData, size_t rawSize, unsigned char *encData, size_t encSize);
    bool readData(QTcpSocket *s, size_t encSize);
    bool chainDecode(unsigned char *rawData, size_t chainEncSize);
    size_t chainRead(QTcpSocket *s);
    virtual void cleanUp();
    virtual ~Decoder();

private:
    void chainCleanUp();
    bool readSize(QTcpSocket *s);
    bool chainReadSize(QTcpSocket *s);
    Decoder *chain;

protected:
    size_t encSize;
    size_t rawSize;
    unsigned char *encData;
    unsigned char *rawData;
    void cleanUpRawData();
};

#endif
