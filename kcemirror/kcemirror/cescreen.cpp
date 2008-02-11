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

#include "cescreen.h"

#include <unistd.h>
#include <qthread.h>
#include <ksimpleconfig.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kmenubar.h>
#include <kpopupmenu.h>
#include <ktoolbar.h>
#include <kiconloader.h>
#include <kstatusbar.h>
#include <khelpmenu.h>
#include <kmessagebox.h>

#include "rapiwrapper.h"
#include "rledecoder.h"
#include "huffmandecoder.h"
#include "xordecoder.h"

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>


CeScreen::CeScreen()
        : KMainWindow(NULL, "KCeScreen")
{
    pdaSocket = NULL;
    pause = false;
    setFixedSize(0, 0);

    aboutApplication = new KAboutApplication(this, "about", false);
    aboutKDE = new KAboutKDE(this, "aboutkde", false);

    imageViewer = new ImageViewer(this);
    setCentralWidget(imageViewer);

    filemenu = new KPopupMenu();
    filemenu->insertItem(SmallIcon("ksnapshot"), i18n("&Screenshot..."),
                         this, SLOT(fileSave()), 0, 1, 1);
    filemenu->insertItem(SmallIcon("fileprint"), i18n("&Print..."),
                         this, SLOT(filePrint()), 0, 2, 2);
    filemenu->insertSeparator(3);
    pauseItem = filemenu->insertItem(SmallIcon("stop"), i18n("&Pause"),
                                     this, SLOT(updatePause()), 0, 3, 4);
    filemenu->insertSeparator(5);
    filemenu->insertItem(SmallIcon("exit"), i18n("&Quit"),
                         kapp, SLOT(quit()), 0, 4, 6);
    menuBar()->insertItem(i18n("&File"), filemenu);

    KPopupMenu *helpmenu = new KPopupMenu();
    helpmenu->insertItem(SmallIcon("contents"), i18n("KCeMirror &Handbook"),
                         this, SLOT(appHelpActivated()));
    helpmenu->insertSeparator();
    helpmenu->insertItem(kapp->miniIcon(), i18n("&About KCeMirror"),
                         this, SLOT(showAboutApplication()));
    helpmenu->insertItem(SmallIcon("about_kde"), i18n("About &KDE"),
                         this, SLOT(showAboutKDE()));
    menuBar()->insertItem(i18n("&Help"), helpmenu);

    tb = new KToolBar(this, QMainWindow::Top, false, "Utils", true, true);
    tb->insertButton("exit", 1, SIGNAL(clicked()), kapp, SLOT(quit()), true, "Quit");
    tb->insertButton("ksnapshot", 2, SIGNAL(clicked()), this, SLOT(fileSave()), true, "Screenshot");
    tb->insertButton("fileprint", 3, SIGNAL(clicked()), this, SLOT(filePrint()), true, "Print");
    tb->insertButton("stop", 4, SIGNAL(clicked()), this, SLOT(updatePause()), true, "Pause");

    connect(tb, SIGNAL(placeChanged(QDockWindow::Place )), this, SLOT(resizeWindow()));
    connect(tb, SIGNAL(modechange()), this, SLOT(resizeWindow()));

    statusBar()->insertItem(i18n("Connecting..."), 1);

    this->decoderChain = NULL;
    this->decoderChain = new HuffmanDecoder(this->decoderChain);
    this->decoderChain = new RleDecoder(this->decoderChain);
    this->decoderChain = new XorDecoder(this->decoderChain);

    bmpData = NULL;
}


CeScreen::~CeScreen()
{
    if (pdaSocket != NULL) {
        delete pdaSocket;
    }

    delete decoderChain;
}


void CeScreen::showAboutApplication()
{
    aboutApplication->show();
}


void CeScreen::showAboutKDE()
{
    aboutKDE->show();
}


void CeScreen::resizeWindow()
{
    adjustSize();
}


void CeScreen::updatePause()
{
    pause = !pause;

    if (pause) {
        statusBar()->insertItem(i18n("Pause"), 3);
        tb->removeItem(4);
        tb->insertButton("redo", 4, SIGNAL(clicked()), this, SLOT(updatePause()), true, "Restart");
        filemenu->removeItem(pauseItem);
        pauseItem = filemenu->insertItem(SmallIcon("redo"), i18n("&Restart"),
                                         this, SLOT(updatePause()), 0, 3, 3);
    } else {
        statusBar()->removeItem(3);
        tb->removeItem(4);
        tb->insertButton("stop", 4, SIGNAL(clicked()), this, SLOT(updatePause()), true, "Pause");
        filemenu->removeItem(pauseItem);
        pauseItem = filemenu->insertItem(SmallIcon("stop"), i18n("&Pause"),
                                         this, SLOT(updatePause()), 0, 3, 3);
        imageViewer->drawImage();
    }
}


QString CeScreen::getDeviceIp(QString pdaName)
{
    char *path = NULL;
    synce::synce_get_directory(&path);
    QString synceDir = QString(path);
    QString deviceIp = "";

    if (path) {
        delete path;
    }

    KSimpleConfig activeConnection(synceDir + "/" + pdaName, true);
    if (activeConnection.hasGroup("device")) {
        activeConnection.setGroup("device");
        deviceIp = activeConnection.readEntry("ip");
    }

    return deviceIp;
}


bool CeScreen::connectPda(QString pdaName, bool isSynCeDevice, bool forceInstall)
{
    synce::PROCESS_INFORMATION info = {0, 0, 0, 0};
    QString deviceAddress = pdaName;

    if (isSynCeDevice) {
        deviceAddress = getDeviceIp(pdaName);
        if (deviceAddress.isEmpty()) {
            return false;
        }

        if (!Ce::rapiInit(pdaName)) {
            return false;
        }

        synce::SYSTEM_INFO system;
        Ce::getSystemInfo(&system);
        QString arch;

        switch(system.wProcessorArchitecture) {
        case 1: // Mips
            arch = ".mips";
            break;
        case 4: // SHx
            arch = ".shx";
            break;
        case 5: // Arm
            arch = ".arm";
            break;
        }

        QString binaryVersion = "screensnap.exe" + arch;

#if KDE_VERSION < KDE_MAKE_VERSION(3,2,0)
        if (!KIO::NetAccess::exists("rapip://" + pdaName + "/Windows/screensnap.exe") 
                || forceInstall) {
#else
        if (!KIO::NetAccess::exists("rapip://" + pdaName + "/Windows/screensnap.exe", false, NULL) 
                || forceInstall) {
#endif
            kdDebug(2120) << "Uploading" << endl;
            KStandardDirs *dirs = KApplication::kApplication()->dirs();
            QString screensnap = dirs->findResource("data", "kcemirror/exe/" + binaryVersion);
#if KDE_VERSION < KDE_MAKE_VERSION(3,2,0)
            KIO::NetAccess::upload(screensnap,
                                   "rapip://" + pdaName + "/Windows/screensnap.exe");
#else
            KIO::NetAccess::upload(screensnap,
                                   "rapip://" + pdaName + "/Windows/screensnap.exe", NULL);
#endif
        }
        if (!Ce::createProcess(QString("\\Windows\\screensnap.exe").ucs2(), NULL,
                               NULL, NULL, false, 0, NULL, NULL, NULL, &info)) {
            return false;
        }
        Ce::rapiUninit();
    }

    pdaSocket = NULL;

    int connectcount = 0;

    do {
        if (pdaSocket != NULL) {
            delete pdaSocket;
        }
        pdaSocket = new KSocket(deviceAddress.ascii(), 1234);
        if (pdaSocket->socket() == -1) {
            usleep(20000);
        }
    } while (pdaSocket->socket() == -1 && connectcount++ < 10);

    if (pdaSocket->socket() == -1) {
        return false;
    }

    pdaSocket->enableRead(true);

    connect(pdaSocket, SIGNAL(readEvent(KSocket* )), this, SLOT(readSocket(KSocket* )));
    connect(pdaSocket, SIGNAL(closeEvent(KSocket *)), this, SLOT(closeSocket(KSocket* )));
    connect(imageViewer, SIGNAL(wheelRolled(int)), this, SLOT(wheelRolled(int)));
    connect(imageViewer, SIGNAL(keyPressed(int, int)), this, SLOT(keyPressed(int, int)));
    connect(imageViewer, SIGNAL(keyReleased(int, int)), this, SLOT(keyReleased(int, int)));
    connect(imageViewer, SIGNAL(mousePressed(ButtonState, int, int )),
            this, SLOT(mousePressed(ButtonState, int, int )));
    connect(imageViewer, SIGNAL(mouseReleased(ButtonState, int, int )),
            this, SLOT(mouseReleased(ButtonState, int, int )));
    connect(imageViewer, SIGNAL(mouseMoved(ButtonState, int, int )),
            this, SLOT(mouseMoved(ButtonState, int, int )));
    connect(this, SIGNAL(printContent()), imageViewer, SLOT(printImage()));
    connect(this, SIGNAL(saveContent()), imageViewer, SLOT(saveImage()));
    connect(this, SIGNAL(pdaSize(int, int )), imageViewer, SLOT(setPdaSize(int, int )));

    statusBar()->removeItem(1);
    statusBar()->insertItem(i18n("Connected to ") + pdaName, 1);

    uint32_t xN;
    uint32_t yN;
    int m;
    int k;
    int p;
    unsigned char packageType;

    p = read(pdaSocket->socket(), &packageType, sizeof(unsigned char));
    m = read(pdaSocket->socket(), &xN, sizeof(uint32_t));
    k = read(pdaSocket->socket(), &yN, sizeof(uint32_t));

    if (m > 0 && k > 0) {
        width = ntohl(xN);
        height = ntohl(yN);
        emit pdaSize((int) width, (int) height);
    } else {
        return false;
    }
    
    return true;
}


bool CeScreen::readEncodedImage(KSocket *socket)
{
    uint32_t rawSize = decoderChain->chainRead(socket->socket());
    if (rawSize > 0) {
        if (decoderChain->chainDecode(bmpData + headerSize, rawSize)) {
            imageViewer->loadImage(bmpData, bmpSize);
            if (!pause) {
                imageViewer->drawImage();
            }
        } else {
            KMessageBox::error(this, "Decoding error", "Error");
            return false;
        }
    } else {
        KMessageBox::error(this, "Conection to PDA broken", "Error");
        return false;
    }

    return true;
}


bool CeScreen::readBmpHeader(KSocket *socket)
{
    uint32_t headerSizeN;
    uint32_t bmpSizeN;
    bool ret = false;

    kdDebug(2120) << "Reading Bitmap Header" << endl;

    int n = read(socket->socket(), &bmpSizeN, sizeof(uint32_t));

    if (n == sizeof(uint32_t)) {
        bmpSize = ntohl(bmpSizeN);
    } else {
        KMessageBox::error(this, "Connection to PDA broken", "Error");
        return false;
    }

    n = read(socket->socket(), &headerSizeN, sizeof(uint32_t));

    if (n == sizeof(uint32_t)) {
        headerSize = ntohl(headerSizeN);
        kdDebug(2120) << "Header size: " << headerSize << endl;
        if (bmpData != NULL) {
            delete[] bmpData;
        }
        bmpData = new unsigned char[bmpSize];
        uint32_t readSize = 0;
        do {
            int n = read(socket->socket(), bmpData + readSize, headerSize - readSize);
            readSize += n;
            if (n <= 0) {
                KMessageBox::error(this, "Conection to PDA broken", "Error");
                return false;
            }
        } while (readSize < headerSize);
        ret = true;
    } else {
        KMessageBox::error(this, "Conection to PDA broken", "Error");
        return false;
    }

    return ret;
}


bool CeScreen::readSizeMessage(KSocket *socket)
{
    uint32_t xN;
    uint32_t yN;
    bool ret = true;

    kdDebug(2120) << "Reading Size Message" << endl;

    int m = read(socket->socket(), &xN, sizeof(uint32_t));
    int k = read(socket->socket(), &yN, sizeof(uint32_t));
    if (m == sizeof(long) && k == sizeof(uint32_t)) {
        uint32_t x = ntohl(xN);
        uint32_t y = ntohl(yN);
        emit pdaSize((int) x, (int) y);
    } else {
        KMessageBox::error(this, "Conection to PDA broken", "Error");
        ret = false;
    }

    return ret;
}


void CeScreen::readSocket(KSocket *socket)
{
    unsigned char packageType;

    int p = read(socket->socket(), &packageType, sizeof(unsigned char));
    if (p != sizeof(unsigned char)) {
        KMessageBox::error(this, "Conection to PDA broken", "Error");
        emit pdaError();
        delete socket;
    } else {
        switch(packageType) {
        case XOR_IMAGE:
            if (!readEncodedImage(socket)) {
                delete socket;
                socket = NULL;
                emit pdaError();
            }
            break;
        case SIZE_MESSAGE:
            if (!readSizeMessage(socket)) {
                delete socket;
                socket = NULL;
                emit pdaError();
            }
            break;
        case KEY_IMAGE:
            break;
        case BMP_HEADER:
            if (!readBmpHeader(socket)) {
                delete socket;
                socket = NULL;
                emit pdaError();
            }
            break;
        }
    }
}


void CeScreen::closeSocket(KSocket *socket)
{
    if (socket != NULL) {
        delete socket;
        socket = NULL;
    }
}


void CeScreen::sendMouseEvent(uint32_t button, uint32_t cmd,
                              uint32_t x, uint32_t y)
{
    if (!pause) {
        unsigned char buf[4 * sizeof(uint32_t)];

        *(uint32_t *) &buf[sizeof(uint32_t) * 0] = htonl(cmd);
        *(uint32_t *) &buf[sizeof(uint32_t) * 1] = htonl(button);
        *(uint32_t *) &buf[sizeof(uint32_t) * 2] = htonl((long) (65535 * x / width));
        *(uint32_t *) &buf[sizeof(uint32_t) * 3] = htonl((long) (65535 * y / height));

        write(pdaSocket->socket(), buf, 4 * sizeof(uint32_t));
    }
}


void CeScreen::sendKeyEvent(uint32_t code, uint32_t cmd)
{
    if (!pause) {
        unsigned char buf[4 * sizeof(uint32_t)];

        *(uint32_t *) &buf[sizeof(uint32_t) * 0] = htonl(cmd);
        *(uint32_t *) &buf[sizeof(uint32_t) * 1] = htonl(code);
        *(uint32_t *) &buf[sizeof(uint32_t) * 2] = 0;
        *(uint32_t *) &buf[sizeof(uint32_t) * 3] = 0;

        write(pdaSocket->socket(), buf, 4 * sizeof(uint32_t));
    }
}


void CeScreen::mousePressed(ButtonState button, int x, int y)
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


void CeScreen::mouseReleased(ButtonState button, int x, int y)
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


void CeScreen::mouseMoved(ButtonState button, int x, int y)
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


void CeScreen::wheelRolled(int delta)
{
    sendMouseEvent(0, MOUSE_WHEEL, delta, 0);
}


uint32_t CeScreen::toKeySym(int ascii, int code)
{
    if ( (ascii >= 'a') && (ascii <= 'z') ) {
        ascii = code;
        ascii = ascii + 0x20;
        return ascii;
    }

    if ( ( code >= 0x0a0 ) && code <= 0x0ff )
        return code;

    if ( ( code >= 0x20 ) && ( code <= 0x7e ) )
        return code;

    switch( code ) {
    case SHIFT:
        return XK_Shift_L;
    case CTRL:
        return XK_Control_L;
    case ALT:
        return XK_Alt_L;

    case Qt::Key_Escape:
        return  XK_Escape;
    case Qt::Key_Tab:
        return XK_Tab;
    case Qt::Key_Backspace:
        return XK_BackSpace;
    case Qt::Key_Return:
        return XK_Return;
    case Qt::Key_Enter:
        return XK_Return;
    case Qt::Key_Insert:
        return XK_Insert;
    case Qt::Key_Delete:
        return XK_Delete;
    case Qt::Key_Pause:
        return XK_Pause;
    case Qt::Key_Print:
        return XK_Print;
    case Qt::Key_SysReq:
        return XK_Sys_Req;
    case Qt::Key_Home:
        return XK_Home;
    case Qt::Key_End:
        return XK_End;
    case Qt::Key_Left:
        return XK_Left;
    case Qt::Key_Up:
        return XK_Up;
    case Qt::Key_Right:
        return XK_Right;
    case Qt::Key_Down:
        return XK_Down;
    case Qt::Key_Prior:
        return XK_Prior;
    case Qt::Key_Next:
        return XK_Next;

    case Qt::Key_Shift:
        return XK_Shift_L;
    case Qt::Key_Control:
        return XK_Control_L;
    case Qt::Key_Meta:
        return XK_Meta_L;
    case Qt::Key_Alt:
        return XK_Alt_L;
    case Qt::Key_CapsLock:
        return XK_Caps_Lock;
    case Qt::Key_NumLock:
        return XK_Num_Lock;
    case Qt::Key_ScrollLock:
        return XK_Scroll_Lock;

    case Qt::Key_F1:
        return XK_F1;
    case Qt::Key_F2:
        return XK_F2;
    case Qt::Key_F3:
        return XK_F3;
    case Qt::Key_F4:
        return XK_F4;
    case Qt::Key_F5:
        return XK_F5;
    case Qt::Key_F6:
        return XK_F6;
    case Qt::Key_F7:
        return XK_F7;
    case Qt::Key_F8:
        return XK_F8;
    case Qt::Key_F9:
        return XK_F9;
    case Qt::Key_F10:
        return XK_F10;
    case Qt::Key_F11:
        return XK_F11;
    case Qt::Key_F12:
        return XK_F12;
    case Qt::Key_F13:
        return XK_F13;
    case Qt::Key_F14:
        return XK_F14;
    case Qt::Key_F15:
        return XK_F15;
    case Qt::Key_F16:
        return XK_F16;
    case Qt::Key_F17:
        return XK_F17;
    case Qt::Key_F18:
        return XK_F18;
    case Qt::Key_F19:
        return XK_F19;
    case Qt::Key_F20:
        return XK_F20;
    case Qt::Key_F21:
        return XK_F21;
    case Qt::Key_F22:
        return XK_F22;
    case Qt::Key_F23:
        return XK_F23;
    case Qt::Key_F24:
        return XK_F24;

    case Qt::Key_unknown:
        return 0;
    default:
        return 0;
    }

    return 0;
}

void CeScreen::keyPressed(int ascii, int code)
{
    uint32_t vncCode = this->toKeySym(ascii, code);

    if (vncCode != 0) {
        sendKeyEvent(vncCode, KEY_PRESSED);
    } else {
        kdDebug(2120) << "Key with code " << code << " not found in map" << endl;
    }
}


void CeScreen::keyReleased(int ascii, int code)
{
    uint32_t vncCode = this->toKeySym(ascii, code);

    if (vncCode != 0) {
        sendKeyEvent(vncCode, KEY_RELEASED);
    } else {
        kdDebug(2120) << "Key with code " << code << " not found in map" << endl;
    }
}


void CeScreen::fileSave()
{
    emit saveContent();
}


void CeScreen::filePrint()
{
    emit printContent();
}

#include "cescreen.moc"

