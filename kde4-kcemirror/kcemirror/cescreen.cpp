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

#include "cescreen.h"

#include <unistd.h>
#include <KMessageBox>
#include <KIO/NetAccess>
#include <KStandardDirs>
#include <KApplication>
#include <KMenuBar>
#include <KMenu>
#include <KToolBar>
#include <KStatusBar>
#include <ksocketfactory.h>
#include <arpa/inet.h>
#include <KAboutData>
#include <KStandardAction>
#include <KDebug>

/*
#include <qthread.h>
#include <ksimpleconfig.h>
#include <kiconloader.h>
#include <khelpmenu.h>
#include <kdebug.h>
*/


#include "rapiwrapper.h"
#include "rledecoder.h"
#include "huffmandecoder.h"
#include "xordecoder.h"

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>


CeScreen::CeScreen()
        : KMainWindow(NULL)
{
    pdaSocket = NULL;
    have_header = false;
    pause = false;
    setFixedSize(0, 0);

    imageViewer = new ImageViewer(this);
    setCentralWidget(imageViewer);

    KAction *action;

    /*
     * create toolbar
     */
    KMenu *filemenu = new KMenu();

    action = new KAction(this);
    action->setText(i18n("&Screenshot..."));
    action->setIcon(KIcon("ksnapshot"));
    connect(action, SIGNAL(triggered(bool)),
            this, SLOT(fileSave()));
    filemenu->addAction(action);

    action = KStandardAction::print(this, SLOT(filePrint()), this);
    filemenu->addAction(action);

    filemenu->addSeparator();

    pauseMenuAction = new KAction(this);
    pauseMenuAction->setText(i18n("Pause"));
    pauseMenuAction->setIcon(KIcon("stop"));
    connect(pauseMenuAction, SIGNAL(triggered(bool)),
            this, SLOT(updatePause()));
    filemenu->addAction(pauseMenuAction);

    filemenu->addSeparator();

    action = KStandardAction::quit(kapp, SLOT(quit()), this);
    filemenu->addAction(action);

    filemenu->setTitle("&File");

    menuBar()->addMenu(filemenu);

    menuBar()->addMenu(helpMenu());

    /*
     * create toolbar
     */
    tb = new KToolBar(QString("Utils"), this, Qt::TopToolBarArea, false, true, true);
    tb->setToolButtonStyle(Qt::ToolButtonIconOnly);

    action = KStandardAction::quit(kapp, SLOT(quit()), this);
    tb->addAction(action);

    action = new KAction(this);
    action->setText(i18n("Screenshot"));
    action->setIcon(KIcon("ksnapshot"));
    connect(action, SIGNAL(triggered(bool)),
            this, SLOT(fileSave()));
    tb->addAction(action);

    action = KStandardAction::print(this, SLOT(filePrint()), this);
    tb->addAction(action);

    pauseTbAction = new KAction(this);
    pauseTbAction->setText(i18n("Pause"));
    pauseTbAction->setIcon(KIcon("stop"));
    connect(pauseTbAction, SIGNAL(triggered(bool)),
            this, SLOT(updatePause()));
    tb->addAction(pauseTbAction);

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


void CeScreen::resizeWindow()
{
    adjustSize();
}


void CeScreen::updatePause()
{
    pause = !pause;

    if (pause) {
        statusBar()->insertItem(i18n("Pause"), 3);

        pauseTbAction->setIcon(KIcon("redo"));
        pauseTbAction->setText(QString("Restart"));

        pauseMenuAction->setIcon(KIcon("redo"));
        pauseMenuAction->setText(QString("Restart"));

    } else {
        statusBar()->removeItem(3);

        pauseTbAction->setIcon(KIcon("stop"));
        pauseTbAction->setText(QString("Pause"));

        pauseMenuAction->setIcon(KIcon("stop"));
        pauseMenuAction->setText(QString("Pause"));

        imageViewer->drawImage();
    }
}


bool CeScreen::connectPda(QString pdaName, bool isSynCeDevice, bool forceInstall)
{
    synce::PROCESS_INFORMATION info = {0, 0, 0, 0};
    QString deviceAddress = pdaName;

    if (isSynCeDevice) {
	synce::SynceInfo *dev_info = synce::synce_info_new(pdaName.toAscii().constData());
	if (dev_info)
          deviceAddress = QString(synce::synce_info_get_device_ip(dev_info));
	else
          deviceAddress = QString("");

	if (dev_info) synce::synce_info_destroy(dev_info);

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

        if (!KIO::NetAccess::exists("rapip://" + pdaName + "/Windows/screensnap.exe", KIO::NetAccess::DestinationSide, NULL) 
                || forceInstall) {
            kDebug(2120) << "Uploading" << endl;
            KStandardDirs *dirs = KGlobal::mainComponent().dirs();
            QString screensnap = dirs->findResource("data", "kcemirror/exe/" + binaryVersion);
            KIO::NetAccess::upload(screensnap,
                                   "rapip://" + pdaName + "/Windows/screensnap.exe", NULL);
        }
        if (!Ce::createProcess(QString("\\Windows\\screensnap.exe").utf16(), NULL,
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

        /*
         * FIXME this doesn't return an error immediately, need to listen for 
         * QAbstractSocket signals
         */

        /*
        pdaSocket = KSocketFactory::connectToHost(QString(""), deviceAddress, 1234);
        */

        pdaSocket = KSocketFactory::synchronousConnectToHost(QString(""), deviceAddress, 1234);
        if (pdaSocket->state() == QAbstractSocket::ConnectedState)
                kDebug(2120) << "connected " << endl;


        if (pdaSocket->state() == QAbstractSocket::UnconnectedState) {
            usleep(20000);
        }

    } while (pdaSocket->state() == QAbstractSocket::UnconnectedState && connectcount++ < 10);

    if (pdaSocket->state() == QAbstractSocket::UnconnectedState) {
        return false;
    }

    connect(pdaSocket, SIGNAL(readyRead()), this, SLOT(readSocket()));
    connect(pdaSocket, SIGNAL(aboutToClose()), this, SLOT(closeSocket()));

    connect(imageViewer, SIGNAL(wheelRolled(int)), this, SLOT(wheelRolled(int)));
    connect(imageViewer, SIGNAL(keyPressed(int, int)), this, SLOT(keyPressed(int, int)));
    connect(imageViewer, SIGNAL(keyReleased(int, int)), this, SLOT(keyReleased(int, int)));
    connect(imageViewer, SIGNAL(mousePressed(Qt::MouseButton, int, int )),
            this, SLOT(mousePressed(Qt::MouseButton, int, int )));
    connect(imageViewer, SIGNAL(mouseReleased(Qt::MouseButton, int, int )),
            this, SLOT(mouseReleased(Qt::MouseButton, int, int )));
    connect(imageViewer, SIGNAL(mouseMoved(Qt::MouseButton, int, int )),
            this, SLOT(mouseMoved(Qt::MouseButton, int, int )));
    connect(this, SIGNAL(printContent()), imageViewer, SLOT(printImage()));
    connect(this, SIGNAL(saveContent()), imageViewer, SLOT(saveImage()));
    connect(this, SIGNAL(pdaSize(int, int )), imageViewer, SLOT(setPdaSize(int, int )));

    return true;
}


bool CeScreen::readEncodedImage()
{
    kDebug(2120) << "** entering **" << endl;

    uint32_t rawSize = decoderChain->chainRead(pdaSocket);
    if (rawSize > 0) {
        if (decoderChain->chainDecode(bmpData + headerSize, rawSize)) {

                /*
            FILE* output = fopen("/home/mark/kce4.bmp", "w");
            fwrite(bmpData, bmpSize, 1, output);
            fclose(output);
                */

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


bool CeScreen::readBmpHeader()
{
    uint32_t headerSizeN;
    uint32_t bmpSizeN;
    bool ret = false;

    kDebug(2120) << "Reading Bitmap Header" << endl;

    int n = pdaSocket->read((char*)&bmpSizeN, sizeof(uint32_t));

    if (n == sizeof(uint32_t)) {
        bmpSize = ntohl(bmpSizeN);
    } else {
        KMessageBox::error(this, "Connection to PDA broken", "Error");
        return false;
    }

    n = pdaSocket->read((char*)&headerSizeN, sizeof(uint32_t));

    if (n == sizeof(uint32_t)) {
        headerSize = ntohl(headerSizeN);
        kDebug(2120) << "Header size: " << headerSize << endl;
        if (bmpData != NULL) {
            delete[] bmpData;
        }
        bmpData = new unsigned char[bmpSize];
        uint32_t readSize = 0;
        do {
                int n = pdaSocket->read((char*)bmpData + readSize, headerSize - readSize);
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


bool CeScreen::readSizeMessage()
{
    uint32_t xN;
    uint32_t yN;
    bool ret = true;

    kDebug(2120) << "Reading Size Message" << endl;

    int m = pdaSocket->read((char*)&xN, sizeof(uint32_t));
    int k = pdaSocket->read((char*)&yN, sizeof(uint32_t));
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


void CeScreen::readSocket()
{
    unsigned char packageType;

    if (!have_header) {
            /*
            statusBar()->removeItem(1);
            statusBar()->insertItem(i18n("Connected to ") + pdaName, 1);
            */
            uint32_t xN;
            uint32_t yN;
            int m;
            int k;
            int p;
            unsigned char packageType;

            kDebug(2120) << "bytes available " << pdaSocket->bytesAvailable() << endl;
            p = pdaSocket->read((char*)&packageType, sizeof(unsigned char));
            m = pdaSocket->read((char*)&xN, sizeof(uint32_t));
            k = pdaSocket->read((char*)&yN, sizeof(uint32_t));

            /*
            kDebug(2120) << "for pda " << pdaName.toAscii().constData() << endl;
            */

            if (m > 0 && k > 0) {
                    width = ntohl(xN);
                    height = ntohl(yN);
                    have_header = true;
                    kDebug(2120) << "width = " << width << "height = " << height << endl;
                    emit pdaSize((int) width, (int) height);
            } else {

                    kDebug(2120) << "bumming out " << pdaSocket->errorString() << endl;
            }
            return;

    }

    this->show();

    kDebug(2120) << "bytes available " << pdaSocket->bytesAvailable() << endl;


    int p = pdaSocket->read((char*)&packageType, sizeof(unsigned char));
    if (p != sizeof(unsigned char)) {
        KMessageBox::error(this, "Conection to PDA broken", "Error");
        emit pdaError();
        delete pdaSocket;
        pdaSocket = NULL;
    } else {
        switch(packageType) {
        case XOR_IMAGE:
            kDebug(2120) << "read xor_image" << endl;
            if (!readEncodedImage()) {
                delete pdaSocket;
                pdaSocket = NULL;
                emit pdaError();
            }
            break;
        case SIZE_MESSAGE:
            kDebug(2120) << "read size_message" << endl;
            if (!readSizeMessage()) {
                delete pdaSocket;
                pdaSocket = NULL;
                emit pdaError();
            }
            break;
        case KEY_IMAGE:
            kDebug(2120) << "read key_image" << endl;
            break;
        case BMP_HEADER:
            kDebug(2120) << "read bmp_header" << endl;
            if (!readBmpHeader()) {
                delete pdaSocket;
                pdaSocket = NULL;
                emit pdaError();
            }
            break;
        }
    }
}


void CeScreen::closeSocket()
{
    if (pdaSocket != NULL) {
        delete pdaSocket;
        pdaSocket = NULL;
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

        if (pdaSocket->write((char*)buf, 4 * sizeof(uint32_t)) != (4 * sizeof(uint32_t)))
                kDebug(2120) << "Failure sending mouse event" << endl;
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

        if (pdaSocket->write((char*)buf, 4 * sizeof(uint32_t)) != (4 * sizeof(uint32_t)))
                kDebug(2120) << "Failure sending key event" << endl;
    }
}


void CeScreen::mousePressed(Qt::MouseButton button, int x, int y)
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


void CeScreen::mouseReleased(Qt::MouseButton button, int x, int y)
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


void CeScreen::mouseMoved(Qt::MouseButton button, int x, int y)
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
        kDebug(2120) << "Key with code " << code << " not found in map" << endl;
    }
}


void CeScreen::keyReleased(int ascii, int code)
{
    uint32_t vncCode = this->toKeySym(ascii, code);

    if (vncCode != 0) {
        sendKeyEvent(vncCode, KEY_RELEASED);
    } else {
        kDebug(2120) << "Key with code " << code << " not found in map" << endl;
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

