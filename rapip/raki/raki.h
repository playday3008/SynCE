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
  
  void setConnectionStatus(bool enable);
  
protected slots:
  void showPopupMenu(QPopupMenu *);

public:
  void restartDccm();

protected:
  void mousePressEvent(QMouseEvent *);
  void dropEvent(QDropEvent* event);
  void droppedFile(KURL url);
  void dragEnterEvent(QDragEnterEvent *event);
  void dragLeaveEvent(QDragLeaveEvent *event);
  void customEvent (QCustomEvent *event);
  void deleteFile(KURL delFile);

private slots:
  void quit();
  
private slots:
  void clickedMenu(int item);

private:  
  bool process(const QCString &fun, const QByteArray &data,
                       QCString &replyType, QByteArray &replyData);
  QString changeConnectionState(int state);
  void startDccm();
  void stopDccm();
  void tryStartDccm();
  KDialog *aboutDialog;
  DCOPClient *dcopClient;
  KPopupMenu *rapiLeMenu;
  KPopupMenu *rapiReMenu;
  RunWindowImpl *runWindow;
  ManagerImpl *managerWindow;
  ConfigDialogImpl *configDialog;
  Installer *installer;
  QString appId;
  bool entered;
  bool running;
  bool connected;
  QPixmap connectedIcon;
  QPixmap disconnectedIcon;
  QPixmap *actualIcon;
  bool dccmRunning;
  int startDccmId;
  int stopDccmId; 
  KProcess *dccmProc;
  
  enum {
    CONNECT_ITEM = 1,
    OPEN_ITEM,
    SHUTDOWN_ITEM,
    EXECUTE_ITEM,
    INSTALL_ITEM,
    CONFIGURE_ITEM,
    STARTDCCM_ITEM,
    STOPDCCM_ITEM
  };
private slots: 
  void showAbout();
  void dccmExited(KProcess *oldDccm);
};

#endif
