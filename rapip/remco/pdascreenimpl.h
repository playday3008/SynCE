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
#ifndef PDASCREENIMPL_H
#define PDASCREENIMPL_H


#include "pdascreen.h"
#include "imageviewer.h"

#include <kmainwindow.h>
#include <ksock.h>
#include <kaboutdata.h>
#include <kaboutapplication.h>
#include <kdebug.h>

#define MOUSE_PRESSED   1
#define MOUSE_RELEASED  2
#define MOUSE_MOVED     3

#define LEFT_BUTTON     1
#define RIGHT_BUTTON    2
#define MID_BUTTON      3

/**
@author Volker Christian
*/

class PdaScreenImpl : public PdaScreen
{
Q_OBJECT
public:
    PdaScreenImpl(QString pdaName, KAboutData *aboutData, KAboutApplication *aboutApplication);
    ~PdaScreenImpl();
    bool connectPda(const char *pdaAddress, const char *synceName = NULL);

private slots:
    void readSocket(KSocket *socket);
    void readSocketRLE(KSocket *socket);
    void closeSocket(KSocket *socket);
    void mousePressed(ButtonState button, int x, int y);
    void mouseReleased(ButtonState button, int x, int y);
    void mouseMoved(ButtonState button, int x, int y);
    size_t rle_decode(unsigned char *target, unsigned char *source, size_t size, unsigned char *oldData);

private:
    KSocket *pdaSocket;
    unsigned char *oldData;

    void sendMouseEvent(long int button, long int cmd, long int x, long int y);
};

#endif
