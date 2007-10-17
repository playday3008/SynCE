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

#include "imageviewer.h"

#include <kmainwindow.h>
#include <ksock.h>
#include <kaboutdata.h>
#include <kaboutapplication.h>
#include <kaboutkde.h>
#include <kdebug.h>
#include "decoder.h"

#define MOUSE_PRESSED   1
#define MOUSE_RELEASED  2
#define MOUSE_MOVED     3
#define MOUSE_WHEEL     4
#define KEY_PRESSED     5
#define KEY_RELEASED    6

#define LEFT_BUTTON     1
#define RIGHT_BUTTON    2
#define MID_BUTTON      3

#define SIZE_MESSAGE    1
#define XOR_IMAGE       2
#define KEY_IMAGE       3
#define BMP_HEADER      4

/**
@author Volker Christian
*/

class CeScreen : public KMainWindow
{
    Q_OBJECT

public:
    CeScreen();
    ~CeScreen();
    bool connectPda(QString pdaName, bool isSynCeDevice = true, bool forceInstall = false);

public slots:
    virtual void fileSave();
    virtual void filePrint();
    virtual void showAboutApplication();
    virtual void showAboutKDE();

private slots:
    void readSocket(KSocket *socket);
    void closeSocket(KSocket *socket);
    void mousePressed(ButtonState button, int x, int y);
    void mouseReleased(ButtonState button, int x, int y);
    void mouseMoved(ButtonState button, int x, int y);
    void wheelRolled(int delta);
    void keyPressed(int ascii, int code);
    void keyReleased(int ascii, int code);
    void resizeWindow();
    void updatePause();

signals:
    void printContent();
    void saveContent();
    void pdaSize(int, int );
    void pdaError();

private:
    KSocket *pdaSocket;

    void sendMouseEvent(unsigned long int button, unsigned long int cmd,
            unsigned long int x, unsigned long int y);
    void sendKeyEvent(unsigned long int code, unsigned long int cmd);
    bool readEncodedImage(KSocket *socket);
    bool readSizeMessage(KSocket *socket);
    bool readBmpHeader(KSocket *socket);
    QString getDeviceIp(QString pdaAddress);
    unsigned long toKeySym(int ascii, int code);
    ImageViewer *imageViewer;
    uint32_t width;
    uint32_t height;
    bool pause;
    KToolBar *tb;
    KPopupMenu *filemenu;
    KAboutApplication *aboutApplication;
    KAboutKDE *aboutKDE;
    int pauseItem;
    Decoder *decoderChain;
    unsigned char *bmpData;
    uint32_t headerSize;
    uint32_t bmpSize;

    static struct _keymap {
        int winVkCode;
        int qtVkCode;
        QString name;
    } keymap[];
};

#endif

