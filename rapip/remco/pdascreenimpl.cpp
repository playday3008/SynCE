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
#include "pdascreenimpl.h"

#include <unistd.h>

PdaScreenImpl::PdaScreenImpl(QString pdaName, KAboutData *aboutData, KAboutApplication *aboutApplication)
        : PdaScreen(NULL, "PdaScreen")
{
    this->setCaption("PDA-Screen:" + pdaName);
}

PdaScreenImpl::~PdaScreenImpl()
{
    delete pdaSocket;
}


bool PdaScreenImpl::connectPda(const char *pdaAddress, const char *synceName)
{
    pdaSocket = new KSocket(pdaAddress, 1234);
    if (pdaSocket->socket() == -1) {
        return false;
    }
    pdaSocket->enableRead(true);
    oldData = NULL;
    connect(pdaSocket, SIGNAL(readEvent(KSocket* )), this, SLOT(readSocketRLE(KSocket* )));
//    connect(pdaSocket, SIGNAL(readEvent(KSocket* )), this, SLOT(readSocket(KSocket* )));
    connect(pdaSocket, SIGNAL(closeEvent(KSocket *)), this, SLOT(closeSocket(KSocket* )));
    connect(imageViewer, SIGNAL(mousePressed(ButtonState, int, int )), this,
            SLOT(mousePressed(ButtonState, int, int )));
    connect(imageViewer, SIGNAL(mouseReleased(ButtonState, int, int )), this,
            SLOT(mouseReleased(ButtonState, int, int )));
    connect(imageViewer, SIGNAL(mouseMoved(ButtonState, int, int )), this,
            SLOT(mouseMoved(ButtonState, int, int )));

    return true;
}


size_t PdaScreenImpl::rle_decode(unsigned char *target, unsigned char *source, size_t size, unsigned char *oldData)
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
            *tmp_target++ = val1 ^ *oldData++;
            *tmp_target++ = val2 ^ *oldData++;
            *tmp_target++ = val3 ^ *oldData++;
            act1 += 3;
            act2 += 3;
            act3 += 3;
            count += 3;
            if (count < size) {
                if (samcount == 2) {
                    unsigned char samruncount = *act1;
                    while (samruncount) {
                        *tmp_target++ = val1 ^ *oldData++;
                        *tmp_target++ = val2 ^ *oldData++;
                        *tmp_target++ = val3 ^ *oldData++;
                        samruncount--;
                    }
                    act1 += 1;
                    act2 += 1;
                    act3 += 1;
                    count += 1;
                    val1 = *act1;
                    val2 = *act2;
                    val3 = *act3;
                    samcount = 0;
                }
            }
        } else {
            samcount = 0;
            *tmp_target++ = (val1 = *act1) ^ *oldData++;
            *tmp_target++ = (val2 = *act2) ^ *oldData++;
            *tmp_target++ = (val3 = *act3) ^ *oldData++;
            act1 += 3;
            act2 += 3;
            act3 += 3;
            count += 3;
        }
    } while (count < size);

    return tmp_target - target;
}


void PdaScreenImpl::readSocketRLE(KSocket *socket)
{
    uint32_t headerSizeN;
    uint32_t rleSizeN;
    uint32_t bmpSizeN;

    if (socket != NULL) {
        int n = read(socket->socket(), &headerSizeN, sizeof(long));
        int m = read(socket->socket(), &rleSizeN, sizeof(long));
        int k = read(socket->socket(), &bmpSizeN, sizeof(long));
        if (n > 0 && m > 0 && k > 0) {
            uint32_t headerSize = ntohl(headerSizeN);
            uint32_t rleSize = ntohl(rleSizeN);
            uint32_t bmpSize = ntohl(bmpSizeN);

            uchar *bmData = new uchar[bmpSize];
            uchar *rleData = new uchar[rleSize];

            kdDebug(2120) << "H: " << headerSize << ", D: " << rleSize << ", S: " << bmpSize << endl;

            uint32_t rsize = 0;
            int rsize_tmp;
            do {
                rsize_tmp = read(socket->socket(), bmData + rsize, headerSize - rsize);
                if (rsize_tmp == 0) {
                    kdDebug(2120) << "<Header> Unexpected end of file - " << rsize <<
                        " Bytes read instead of " << headerSize << endl;
                    delete socket;
                    socket = NULL;
                    break;
                }
                if (rsize_tmp < 0) {
                    kdDebug(2120) << "Error during read" << endl;
                    delete socket;
                    socket = NULL;
                    break;
                }
                rsize += rsize_tmp;
            } while (rsize < headerSize);

            rsize = 0;
            do {
                rsize_tmp = read(socket->socket(), rleData + rsize, rleSize - rsize);
                if (rsize_tmp == 0) {
                    kdDebug(2120) << "<RLE-Data> Unexpected end of file - " << rsize <<
                        " Bytes read instead of " << rleSize << endl;
                    delete socket;
                    socket = NULL;
                    break;
                }
                if (rsize_tmp < 0) {
                    kdDebug(2120) << "Error during read" << endl;
                    delete socket;
                    socket = NULL;
                    break;
                }
                rsize += rsize_tmp;
            } while (rsize < rleSize);

            kdDebug(2120) << "Read ready" << endl;

            if (oldData == NULL) {
                oldData = new unsigned char[bmpSize];
                memset(oldData, 0, bmpSize);
            }

            rle_decode(bmData + headerSize, rleData, rleSize, oldData + headerSize);

            delete oldData;
            oldData = bmData;

            imageViewer->drawImage(oldData, bmpSize);

            delete rleData;
        }
    }
}


void PdaScreenImpl::readSocket(KSocket *socket)
{
    uint32_t sizeN;

    if (socket != NULL) {
        int n = read(socket->socket(), &sizeN, sizeof(long));

        if (n > 0) {
            uint32_t size = ntohl(sizeN);
            uchar *bmData = new uchar[size];
            uint32_t rsize = 0;
            int rsize_tmp;
            do {
                rsize_tmp = read(socket->socket(), bmData + rsize, size - rsize);
                if (rsize_tmp == 0) {
                    kdDebug(2120) << "Zero Bytes read but not at end of input - should not happen" << endl;
                    delete socket;
                    socket = NULL;
                    break;
                }
                if (rsize_tmp < 0) {
                    kdDebug(2120) << "Error during read" << endl;
                    delete socket;
                    socket = NULL;
                    break;
                }
                rsize += rsize_tmp;
            } while (rsize < size);

            if (rsize == size) {
                imageViewer->drawImage(bmData, rsize);
            }

            delete bmData;
        }
    }
}


void PdaScreenImpl::closeSocket(KSocket *socket)
{
    if (socket != NULL) {
        delete socket;
        socket = NULL;
    }
}


void PdaScreenImpl::sendMouseEvent(long int button, long int cmd, long int x, long int y)
{
    unsigned char buf[4 * sizeof(uint32_t)];

    *(uint32_t *) &buf[sizeof(uint32_t) * 0] = htonl(button);
    *(uint32_t *) &buf[sizeof(uint32_t) * 1] = htonl(cmd);
    *(uint32_t *) &buf[sizeof(uint32_t) * 2] = htonl((long) (65535 * x / 240));
    *(uint32_t *) &buf[sizeof(uint32_t) * 3] = htonl((long) (65535 * y / 320));

    write(pdaSocket->socket(), buf, 4 * sizeof(uint32_t));
}


void PdaScreenImpl::mousePressed(ButtonState button, int x, int y)
{
    long int buttonNumber;

    switch(button) {
    case Qt::LeftButton:
        buttonNumber = LEFT_BUTTON;
        break;
    case Qt::RightButton:
        buttonNumber = RIGHT_BUTTON;
        break;
    case Qt::MidButton:
        buttonNumber = MID_BUTTON;
        break;
    default:
        buttonNumber = 0;
        break;
    }

    sendMouseEvent(buttonNumber, MOUSE_PRESSED, x, y);
}


void PdaScreenImpl::mouseReleased(ButtonState button, int x, int y)
{
    long int buttonNumber;

    switch(button) {
    case Qt::LeftButton:
        buttonNumber = LEFT_BUTTON;
        break;
    case Qt::RightButton:
        buttonNumber = RIGHT_BUTTON;
        break;
    case Qt::MidButton:
        buttonNumber = MID_BUTTON;
        break;
    default:
        buttonNumber = 0;
        break;
    }

    sendMouseEvent(buttonNumber, MOUSE_RELEASED, x, y);
}


void PdaScreenImpl::mouseMoved(ButtonState button, int x, int y)
{
    long int buttonNumber;

    switch(button) {
    case Qt::LeftButton:
        buttonNumber = LEFT_BUTTON;
        break;
    case Qt::RightButton:
        buttonNumber = RIGHT_BUTTON;
        break;
    case Qt::MidButton:
        buttonNumber = MID_BUTTON;
        break;
    default:
        buttonNumber = 0;
        break;
    }

    sendMouseEvent(buttonNumber, MOUSE_MOVED, x, y);
}


#include "pdascreenimpl.moc"
