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

#include "runwindow.h"
#include "progressbar.h"
#include "errorevent.h"
#include "management.h"
#include "configdialog.h"


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
  void installCabinetFile(QString file);
  void droppedFile(KURL url);
  void dragEnterEvent(QDragEnterEvent *event);
  void dragLeaveEvent(QDragLeaveEvent *event);
  void customEvent (QCustomEvent *event);

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
  RunWindow *runWindow;
  Manager *managerWindow;
  Progress *progressBar;
  ConfigDialog *configDialog;
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
  
  enum {
    CONNECT_ITEM = 1,
    SHUTDOWN_ITEM,
    EXECUTE_ITEM,
    INSTALL_ITEM,
    CONFIGURE_ITEM,
    STARTDCCM_ITEM,
    STOPDCCM_ITEM
  };
private slots: 
  void showAbout();
};

/*
class Raki : public KPanelApplet, public DCOPObject
{
  Q_OBJECT

public:
  Raki(const QString& configFile, Type t = Normal, int actions = 0,
       QWidget *parent = 0, const char *name = 0);
  ~Raki();
  virtual int widthForHeight(int height) const;
  virtual int heightForWidth(int width) const;
  virtual void about();
  virtual void help();
  virtual void preferences();

  virtual bool process(const QCString &fun, const QByteArray &data,
                       QCString &replyType, QByteArray &replyData);
  QString changeConnectionState(int state);

  void updateIcon() const;

  int actualIconSize;

protected:
  void resizeEvent(QResizeEvent *);

private:
  KConfig *ksConfig;
  QWidget *mainView;

  MyLabel *myLabel;
  pthread_t checkConnection;
  DCOPClient *dcopClient;
  QString appId;
  bool running;
  bool connected;
  pixmap_t connectedIcon;
  pixmap_t disconnectedIcon;
  pixmap_p actualIcon;
  KAboutData *aboutData;
};
*/

#endif
