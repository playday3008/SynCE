#ifndef RAKI_H
#define RAKI_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kpanelapplet.h>
#include <qstring.h>
#include <kconfig.h>
#include <qpixmap.h>
#include <kpopupmenu.h>
#include <qpopupmenu.h>
#include <dcopclient.h>
#include <dcopobject.h>
#include <kapplication.h>
#include <kaboutapplication.h>
#include <kaboutdata.h>
#include <ksystemtray.h>
#include <kprocess.h>

#include "runwindowimpl.h"
#include "errorevent.h"
#include "managerimpl.h"
#include "configdialogimpl.h"
#include "installer.h"


class Raki : public KSystemTray, public DCOPObject
{
    Q_OBJECT

public:
    Raki(KAboutData* aboutDta, KDialog* d, QWidget* parent=0, const char *name=0);
    ~Raki();
    void restartDccm();

private:
    void setConnectionStatus(bool enable);
    void mousePressEvent(QMouseEvent *);
    void dropEvent(QDropEvent* event);
    void droppedFile(KURL url);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void customEvent (QCustomEvent *event);
    void deleteFile(KURL delFile);
    bool process(const QCString &fun, const QByteArray &data,
                 QCString &replyType, QByteArray &replyData);
    QString changeConnectionState(int state);
    void startDccm();
    void stopDccm();
    void tryStartDccm();
    void postConnect(bool enable);

    DCOPClient *dcopClient;
    KDialog *aboutDialog;
    KPopupMenu *rapiLeMenu;
    KPopupMenu *rapiReMenu;
    RunWindowImpl *runWindow;
    ManagerImpl *managerWindow;
    ConfigDialogImpl *configDialog;
    Installer *installer;
    QPixmap connectedIcon;
    QPixmap disconnectedIcon;
    QPixmap *actualIcon;
    KProcess dccmProc;
    KProcess connectProc;
    KProcess disconnectProc;
    bool entered;
    int startDccmId;
    int stopDccmId;
    int connectId;
    int disconnectId;
    bool dccmRestart;
    bool masqueradeEnabled;
    QString deviceIp;

    enum {
        OPEN_ITEM = 1,
        SHUTDOWN_ITEM,
        EXECUTE_ITEM,
        INSTALL_ITEM,
        CONFIGURE_ITEM,
        STARTDCCM_ITEM,
        STOPDCCM_ITEM,
        CONNECT_ITEM,
        DISCONNECT_ITEM
    };

private slots:
    void showPopupMenu(QPopupMenu *);
    void clickedMenu(int item);
    void showAbout();
    void dccmExited(KProcess *oldDccm);
    void connectExited(KProcess *oldProc);
    void disconnectExited(KProcess *oldProc);
    void quit();
};

#endif
