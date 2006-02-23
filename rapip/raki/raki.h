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

#ifndef RAKI_H
#define RAKI_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kpanelapplet.h>
#include <kpopupmenu.h>
#include <khelpmenu.h>
#include <kaboutapplication.h>
#include <kaboutdata.h>
#include <ksystemtray.h>
#include <kprocess.h>
#include <ksock.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qdict.h>
#include <qpopupmenu.h>
#include <dcopclient.h>
#include <dcopobject.h>

class ConfigDialogImpl;
class PDA;
class Installer;
class ErrorEvent;

/**
@author Volker Christian,,,
*/

class Raki : public KSystemTray, public DCOPObject
{
    Q_OBJECT

public:
    Raki(KAboutData* aboutDta, KDialog* d, QWidget* parent=0,
            const char *name=0);
    ~Raki();
    ConfigDialogImpl *configDialog;
    bool isInitialized();

private:
    void setConnectionStatus(bool enable);
    void mousePressEvent(QMouseEvent *);
    void dropEvent(QDropEvent* event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    bool process(const QCString &fun, const QByteArray &data,
                 QCString &replyType, QByteArray &replyData);
    QString changeConnectionState(int state);
    bool dccmConnect();
    bool openDccmConnection();
    void dccmNotification(QString signal);
    void startDccm();
    void stopDccm();
    void tryStartDccm();
    QCStringList interfaces();
    QCStringList functions();
    void timerEvent(QTimerEvent *e);
    void startMasquerading(PDA *pda);
    void stopMasquerading(PDA *pda);

    DCOPClient *dcopClient;
    KDialog *aboutDialog;
    KPopupMenu *rapiLeMenu;
    KPopupMenu *rapiReMenu;
    KHelpMenu *help;
    Installer *installer;
    QPixmap connectedIcon;
    QPixmap disconnectedIcon;
    QPixmap *actualIcon;
    KProcess dccmProc;
    bool entered;
    bool connected;
    int startDccmId;
    int stopDccmId;
    bool dccmRestart;
    bool dccmShouldRun;
    KSocket *dccmConnection;
    QDict<PDA> pdaList;
    bool initialized;
    QStringList pendingPdaList;
    KProcess ipTablesProc;
    int timerId;

    enum {
        CONFIGURE_ITEM = 1,
        STARTDCCM_ITEM,
        STOPDCCM_ITEM
    };

private slots:
    void showPopupMenu(QPopupMenu *);
    void clickedMenu(int item);
    void showAbout();
    void dccmExited(KProcess *oldDccm);
    void dccmStdout(KProcess *, char *buf, int len);
    void dccmStderr(KProcess *, char *buf, int len);
    void connectionRequest();
    void initializePda();
    void closeDccmConnection();
    void resolvedPassword(QString pdaName, QString password);
    void disconnectPda(QString pdaName);
    void quit();
    void shutDown();
    void restartDccm();
    void pdaInitialized(PDA *pda, int);
    void ipTablesExited(KProcess *);
    void ipTablesStdout(KProcess *, char *, int);
    void ipTablesStderr(KProcess *, char *, int);
};

#endif
