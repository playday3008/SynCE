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
/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
*/
extern "C" {
#include "huffman.h"
};
#include <unistd.h>
#include <qthread.h>
#include <ksimpleconfig.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include "rapiwrapper.h"


struct PdaScreenImpl::_keymap PdaScreenImpl::keymap[] = {
        { VK_BACK, Qt::Key_Backspace, "BACK" },
        { VK_TAB, Qt::Key_Tab, "Tab" },
        { VK_CLEAR, Qt::Key_Clear, "Clear" },
        { VK_RETURN, Qt::Key_Return, "Return" },
        { VK_SHIFT, Qt::Key_Shift, "Shift" },
        { VK_CONTROL, Qt::Key_Control, "Control" },
        { VK_MENU, Qt::Key_Alt, "Alt" },
        { VK_PAUSE, Qt::Key_Pause, "Pause" },
        { VK_CAPITAL, Qt::Key_CapsLock, "Capslock" },
        { VK_ESCAPE, Qt::Key_Escape, "Escape" },
        { VK_SPACE, Qt::Key_Space, "Space" },
        { VK_PRIOR, Qt::Key_Prior, "Prior" },
        { VK_NEXT, Qt::Key_Next, "Next" },
        { VK_END, Qt::Key_End, "End" },
        { VK_HOME, Qt::Key_Home, "Home" },
        { VK_LEFT, Qt::Key_Left, "Left" },
        { VK_UP, Qt::Key_Up, "Up" },
        { VK_RIGHT, Qt::Key_Right, "Right" },
        { VK_DOWN, Qt::Key_Down, "Down" },
        { VK_SELECT, UNDEF, "Select" },
        { VK_PRINT, UNDEF, "Print" },
        { VK_EXECUTE, UNDEF, "Execute" },
        { VK_SNAPSHOT, Qt::Key_Print, "Snapshot" },
        { VK_INSERT, Qt::Key_Insert, "Insert" },
        { VK_DELETE, Qt::Key_Delete, "Delete" },
        { VK_HELP, Qt::Key_Help, "Help" },
        { VK_NUMPAD0, UNDEF, "NUM0" },
        { VK_NUMPAD1, UNDEF, "NUM1" },
        { VK_NUMPAD2, UNDEF, "NUM2" },
        { VK_NUMPAD3, UNDEF, "NUM3" },
        { VK_NUMPAD4, UNDEF, "NUM4" },
        { VK_NUMPAD5, UNDEF, "NUM5" },
        { VK_NUMPAD6, UNDEF, "NUM6" },
        { VK_NUMPAD7, UNDEF, "NUM7" },
        { VK_NUMPAD8, UNDEF, "NUM8" },
        { VK_NUMPAD9, UNDEF, "NUM9" },
        { VK_MULTIPLY, Qt::Key_multiply, "*" },
        { VK_ADD, Qt::Key_Plus, "+" },
        { VK_SEPARATOR, UNDEF, "Separator" },
        { VK_SUBTRACT, Qt::Key_Minus, "-" },
        { VK_DECIMAL, Qt::Key_Period, "." },
        { VK_DIVIDE, Qt::Key_Slash, "/" },
        { VK_NUMLOCK, Qt::Key_NumLock, "Numlock" },
        { VK_SCROLL, Qt::Key_ScrollLock, "ScrollLock" },
        { '0', Qt::Key_0, "0" },
        { '1', Qt::Key_1, "1" },
        { '2', Qt::Key_2, "2" },
        { '3', Qt::Key_3, "3" },
        { '4', Qt::Key_4, "4" },
        { '5', Qt::Key_5, "5" },
        { '6', Qt::Key_6, "6" },
        { '7', Qt::Key_7, "7" },
        { '8', Qt::Key_8, "8" },
        { '9', Qt::Key_9, "9" },
        { 'A', Qt::Key_A, "A" },
        { 'B', Qt::Key_B, "B" },
        { 'C', Qt::Key_C, "C" },
        { 'D', Qt::Key_D, "D" },
        { 'E', Qt::Key_E, "E" },
        { 'F', Qt::Key_F, "F" },
        { 'G', Qt::Key_G, "G" },
        { 'H', Qt::Key_H, "H" },
        { 'I', Qt::Key_I, "I" },
        { 'J', Qt::Key_J, "J" },
        { 'K', Qt::Key_K, "K" },
        { 'L', Qt::Key_L, "L" },
        { 'M', Qt::Key_M, "M" },
        { 'N', Qt::Key_N, "N" },
        { 'O', Qt::Key_O, "O" },
        { 'P', Qt::Key_P, "P" },
        { 'Q', Qt::Key_Q, "Q" },
        { 'R', Qt::Key_R, "R" },
        { 'S', Qt::Key_S, "S" },
        { 'T', Qt::Key_T, "T" },
        { 'U', Qt::Key_U, "U" },
        { 'V', Qt::Key_V, "V" },
        { 'W', Qt::Key_W, "W" },
        { 'X', Qt::Key_X, "X" },
        { 'Y', Qt::Key_Y, "Y" },
        { 'Z', Qt::Key_Z, "Z" },
        { -1, 0 }
/*
        { VK_NUMPAD0, Qt::Key_ },
        { VK_NUMPAD1, Qt::Key_ },
        { VK_NUMPAD2, Qt::Key_ },
        { VK_NUMPAD3, Qt::Key_ },
        { VK_NUMPAD4, Qt::Key_ },
        { VK_NUMPAD5, Qt::Key_ },
        { VK_NUMPAD6, Qt::Key_ },
        { VK_NUMPAD7, Qt::Key_ },
        { VK_NUMPAD8, Qt::Key_ },
        { VK_NUMPAD9, Qt::Key_ },
*/

};


PdaScreenImpl::PdaScreenImpl(QString pdaName, KAboutData *aboutData, KAboutApplication *aboutApplication)
        : PdaScreen(NULL, "PdaScreen")
{
    this->setCaption("PDA-Screen:" + pdaName);
}

PdaScreenImpl::~PdaScreenImpl()
{
    delete pdaSocket;
}


QString PdaScreenImpl::getDeviceIp(QString pdaName)
{
    char *path = NULL;
    synce::synce_get_directory(&path);
    QString synceDir = QString(path);
    QString deviceIp = "";

    if (path)
        delete path;

    KSimpleConfig activeConnection(synceDir + "/" + pdaName, true);
    if (activeConnection.hasGroup("device")) {
        activeConnection.setGroup("device");
        deviceIp = activeConnection.readEntry("ip");
    }

    return deviceIp;
}


bool PdaScreenImpl::connectPda(QString pdaAddress, QString pdaName)
{
    synce::PROCESS_INFORMATION info = {0, 0, 0, 0 };
    QString deviceAddress = pdaAddress;

    if (!pdaName.isEmpty()) {
        deviceAddress = getDeviceIp(pdaName);
        if (deviceAddress.isEmpty()) {
            return false;
        }

        Ce::rapiInit(pdaName);
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

        if (!KIO::NetAccess::exists("rapip://" + pdaName + "/Windows/screensnap.exe")) {
            kdDebug(2120) << "Uploading" << endl;
            KStandardDirs *dirs = KApplication::kApplication()->dirs();
            QString screensnap = dirs->findResource("data", "raki/scripts/" + binaryVersion);
            KIO::NetAccess::upload(screensnap,
                    "rapip://" + pdaName + "/Windows/screensnap.exe");
        }
        if (!Ce::createProcess(QString("\\Windows\\screensnap.exe").ucs2(), NULL,
                           NULL, NULL, false, 0, NULL, NULL, NULL, &info)) {}
        Ce::rapiUninit();
    }

    pdaSocket = NULL;

    do {
        if (pdaSocket != NULL) {
            delete pdaSocket;
        }
        pdaSocket = new KSocket(deviceAddress.ascii(), 1234);
        if (pdaSocket->socket() == -1) {
            usleep(20000);
        }
    } while (pdaSocket->socket() == -1);

    pdaSocket->enableRead(true);

    oldData = NULL;

    connect(pdaSocket, SIGNAL(readEvent(KSocket* )), this, SLOT(readSocketRLE(KSocket* )));
    connect(pdaSocket, SIGNAL(closeEvent(KSocket *)), this, SLOT(closeSocket(KSocket* )));
    connect(imageViewer, SIGNAL(wheelRolled(int)), this, SLOT(wheelRolled(int)));
    connect(imageViewer, SIGNAL(keyPressed(int, int)), this, SLOT(keyPressed(int, int)));
    connect(imageViewer, SIGNAL(keyReleased(int, int)), this, SLOT(keyReleased(int, int)));

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
                if (count < size) {
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
//                    kmessagebox
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

            unsigned char *newrleData;
            size_t newrleSize;

            huffman_decode_memory(rleData, rleSize, &newrleData,&newrleSize);
//            rle_decode(bmData + headerSize, rleData, rleSize, oldData + headerSize);
            rle_decode(bmData + headerSize, newrleData, newrleSize, oldData + headerSize);

            delete oldData;
            delete newrleData;
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


void PdaScreenImpl::sendKeyEvent(long int code, long int cmd)
{
    unsigned char buf[4 * sizeof(uint32_t)];

    *(uint32_t *) &buf[sizeof(uint32_t) * 0] = htonl(code);
    *(uint32_t *) &buf[sizeof(uint32_t) * 1] = htonl(cmd);
    *(uint32_t *) &buf[sizeof(uint32_t) * 2] = 0;
    *(uint32_t *) &buf[sizeof(uint32_t) * 3] = 0;

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


void PdaScreenImpl::wheelRolled(int delta)
{
    sendMouseEvent(0, MOUSE_WHEEL, delta, 0);
}


int PdaScreenImpl::mapKey(int code)
{
    int i;

    for (i = 0; PdaScreenImpl::keymap[i].winVkCode != -1; i++) {
        if (PdaScreenImpl::keymap[i].qtVkCode == code) {
            break;
        }
    }

    return i;
}


void PdaScreenImpl::keyPressed(int ascii, int code)
{
    int winVkCode = mapKey(code);

    if (PdaScreenImpl::keymap[winVkCode].winVkCode != -1) {
        kdDebug(2120) << "Found key " << PdaScreenImpl::keymap[winVkCode].name <<
                ", WinCode: " << PdaScreenImpl::keymap[winVkCode].winVkCode <<
                ", QtCode: " << PdaScreenImpl::keymap[winVkCode].qtVkCode << endl;
        sendKeyEvent(PdaScreenImpl::keymap[winVkCode].winVkCode, KEY_PRESSED);
    } else {
        kdDebug(2120) << "Key with code " << code << " not found in map using ascii value " << ascii << endl;
        sendKeyEvent(ascii, KEY_PRESSED);
    }
}


void PdaScreenImpl::keyReleased(int ascii, int code)
{
    int winVkCode = mapKey(code);

    if (PdaScreenImpl::keymap[winVkCode].winVkCode != -1) {
        kdDebug(2120) << "Found key " << PdaScreenImpl::keymap[winVkCode].name <<
                ", WinCode: " << PdaScreenImpl::keymap[winVkCode].winVkCode <<
                ", QtCode: " << PdaScreenImpl::keymap[winVkCode].qtVkCode << endl;
        sendKeyEvent(PdaScreenImpl::keymap[winVkCode].winVkCode, KEY_RELEASED);
    } else {
        kdDebug(2120) << "Key with code " << code << " not found in map using ascii value " << ascii << endl;
        sendKeyEvent(ascii, KEY_RELEASED);
    }
}


void PdaScreenImpl::fileNew()
{
    qWarning( "PdaScreen::fileNew(): Not implemented yet" );
}

void PdaScreenImpl::fileOpen()
{
    qWarning( "PdaScreen::fileOpen(): Not implemented yet" );
}

void PdaScreenImpl::fileSave()
{
    qWarning( "PdaScreen::fileSave(): Not implemented yet" );
}

void PdaScreenImpl::fileSaveAs()
{
    qWarning( "PdaScreen::fileSaveAs(): Not implemented yet" );
}

void PdaScreenImpl::filePrint()
{
    qWarning( "PdaScreen::filePrint(): Not implemented yet" );
}

void PdaScreenImpl::fileExit()
{
    qWarning( "PdaScreen::fileExit(): Not implemented yet" );
}

void PdaScreenImpl::editUndo()
{
    qWarning( "PdaScreen::editUndo(): Not implemented yet" );
}

void PdaScreenImpl::editRedo()
{
    qWarning( "PdaScreen::editRedo(): Not implemented yet" );
}

void PdaScreenImpl::editCut()
{
    qWarning( "PdaScreen::editCut(): Not implemented yet" );
}

void PdaScreenImpl::editPaste()
{
    qWarning( "PdaScreen::editPaste(): Not implemented yet" );
}

void PdaScreenImpl::editFind()
{
    qWarning( "PdaScreen::editFind(): Not implemented yet" );
}

void PdaScreenImpl::helpIndex()
{
    qWarning( "PdaScreen::helpIndex(): Not implemented yet" );
}

void PdaScreenImpl::helpContents()
{
    qWarning( "PdaScreen::helpContents(): Not implemented yet" );
}

void PdaScreenImpl::helpAbout()
{
    qWarning( "PdaScreen::helpAbout(): Not implemented yet" );
}

void PdaScreenImpl::editCopy()
{
    qWarning( "PdaScreen::editCopy(): Not implemented yet" );
}




#include "pdascreenimpl.moc"
