#include <unistd.h>
#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <kapplication.h>
#include <kuniqueapplication.h>
#include <khelpmenu.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <qcursor.h>
#include <qdragobject.h>

#include <syslog.h>

#include "raki.h"
#include "rapiwrapper.h"

#define Icon(x) KGlobal::instance()->iconLoader()->loadIcon(x, KIcon::Toolbar)

Raki::Raki(KAboutData* /*aboutDta*/, KDialog* d, QWidget* parent, const char *name)
  : KSystemTray(parent, name), DCOPObject ("Raki"), aboutDialog(d)
{
  connectedIcon = Icon("raki");
  disconnectedIcon = Icon("raki_bw");
  setPixmap(disconnectedIcon);

  rapiLeMenu = new KPopupMenu(0, "RakiMenu");
  rapiLeMenu->clear();
  rapiLeMenu->insertTitle(SmallIcon("rapip"), i18n("Raki Applet"));

  rapiLeMenu->insertItem(SmallIcon("run"), i18n("&Execute..."), EXECUTE_ITEM);
  rapiLeMenu->insertItem(SmallIcon("rotate_cw"), i18n("&Info && Management..."), INSTALL_ITEM);
  rapiLeMenu->insertTearOffHandle(-1, -1);
  
  rapiLeMenu->setEnabled(false);
  
  rapiReMenu = new KPopupMenu(0, "RakiMenu");
  rapiReMenu->clear();
  rapiReMenu->insertTitle(SmallIcon("rapip"), i18n("Raki Applet"));
  
  startDccmId = rapiReMenu->insertItem(SmallIcon("start"), i18n("&Start DCCM"), STARTDCCM_ITEM);
  stopDccmId = rapiReMenu->insertItem(SmallIcon("stop"), i18n("S&top DCCM"), STOPDCCM_ITEM);
   rapiReMenu->insertSeparator(3);
 
  rapiReMenu->insertItem(SmallIcon("configure"), i18n("&Configure..."), CONFIGURE_ITEM);
  rapiReMenu->insertSeparator(5);
  KHelpMenu *help = new KHelpMenu( this, 0, false );
  rapiReMenu->insertItem(i18n("&Help"), help->menu());
  connect (help, SIGNAL (showAboutApplication()), this, SLOT (showAbout()));
  
  KActionCollection *actionCollection = new KActionCollection(this);
  KAction *quit = KStdAction::quit(/*kapp*/ this, SLOT (quit()), actionCollection);
  rapiReMenu->insertSeparator(7);
  quit->plug(rapiReMenu, 8);
  
  rapiReMenu->setEnabled(true);
  
  configDialog = new ConfigDialog(this, "ConfigDialog", true);
  runWindow = new RunWindow(this, "RunWindow", false);
  progressBar = new Progress(this, "ProgressWindow", false);
  managerWindow = new Manager(this, "ManagerWindow", false);

  connect(rapiLeMenu, SIGNAL(activated(int)), this, SLOT(clickedMenu(int)));
  connect(rapiReMenu, SIGNAL(activated(int)), this, SLOT(clickedMenu(int)));
  
  tryStartDccm();
  
  setAcceptDrops(false);
  entered = false;
}


Raki::~Raki()
{
    stopDccm();
}


void Raki::restartDccm()
{
  if (dccmRunning) {
    stopDccm();
    tryStartDccm();
  }
}


void Raki::tryStartDccm()
{

  if (configDialog->getStartDccm()) {
    startDccm();
  }
}


void Raki::startDccm()
{
  QString cmd;
  
  dccmRunning = false;
  
  if (configDialog->getUsePassword()) {
    cmd = QString("/usr/local/bin/dccm -p " + configDialog->getPassword());
  } else {
    cmd = QString("/usr/local/bin/dccm");
  }

  system(cmd.ascii());
    
  rapiReMenu->setItemEnabled (startDccmId, false); 
  rapiReMenu->setItemEnabled (stopDccmId, true); 
    
  dccmRunning = true;
}


void Raki::stopDccm()
{
    if (dccmRunning) {
        system("killall dccm");
        dccmRunning = false;
        
        rapiReMenu->setItemEnabled (startDccmId, true); 
        rapiReMenu->setItemEnabled (stopDccmId, false);
    }
}


void Raki::quit()
{
    delete this;
    kapp->quit();
}


void Raki::setConnectionStatus(bool enable)
{
  rapiLeMenu->setEnabled(enable);
  setAcceptDrops(enable);
}


void Raki::showPopupMenu(QPopupMenu *menu)
{
  menu->move(-1000,-1000);
  menu->show();
  menu->hide();

  QPoint g = QCursor::pos();

  if (menu->height() < g.y())
    menu->popup(QPoint( g.x(), g.y() - menu->height()));
  else
    menu->popup(QPoint(g.x(), g.y()));
}


void Raki::mousePressEvent(QMouseEvent *event)
{
    
  if (event->type() == QEvent::MouseButtonDblClick){
    managerWindow->show();
  } else if (event->button() == LeftButton) {
    showPopupMenu(rapiLeMenu);
  } else if (event->button() == RightButton) {
    showPopupMenu(rapiReMenu);
  }
}


void Raki::customEvent (QCustomEvent *e)
{
  syslog(LOG_INFO, "EVent received\n");
  QString msg;

  if (e->type() == QEvent::User) {
    ErrorEvent *event = (ErrorEvent *) e;
    switch(event->getErrorType()) {
    case ErrorEvent::REMOTE_FILE_CREATE_ERROR:
      msg = QString("Could not create ") + QString(((QString *) event->data())->ascii());
      KMessageBox::error(0, i18n(msg));
      delete ((QString *) event->data());
      break;
    case ErrorEvent::REMOTE_FILE_WRITE_ERROR:
      msg = QString("Could not write to ") + QString(((QString *) event->data())->ascii());
      KMessageBox::error(0, i18n(msg));
      delete ((QString *) event->data());
      break;
    case ErrorEvent::LOCALE_FILE_OPEN_ERROR:
      msg = QString("Could not open ") + QString(((QString *) event->data())->ascii());
      KMessageBox::error(0, i18n(msg));
      delete ((QString *) event->data());
      break;
    case ErrorEvent::LOCALE_FILE_READ_ERROR:
      msg = QString("Could not read ") + QString(((QString *) event->data())->ascii());
      KMessageBox::error(0, i18n(msg));
      delete ((QString *) event->data());
      break;
    case ErrorEvent::REMOTE_FILE_EXECUTE_ERROR:
      msg = QString("Could not execute ") + QString(((QString *) event->data())->ascii());
      KMessageBox::error(0, i18n(msg));
      delete ((QString *) event->data());
      break;
    case ErrorEvent::NO_FILENAME_ERROR:
      msg = QString("No filename");
      KMessageBox::error(0, i18n(msg));
      delete ((QString *) event->data());
      break;
    }
  }
}


void Raki::installCabinetFile(QString file)
{
  progressBar->setInstallFile(file);
  progressBar->showRequest();
}


void Raki::droppedFile(KURL url)
{
  if (url.path().endsWith(".cab") || url.path().endsWith(".CAB")) {
    installCabinetFile(url.path());
  }
}


void Raki::dragEnterEvent(QDragEnterEvent *event)
{
  event->accept(QUrlDrag::canDecode(event));

  if (entered==false)
    setFrameStyle(QFrame::StyledPanel | QFrame::Plain);

  entered=true;
}


void Raki::dragLeaveEvent(QDragLeaveEvent * /* event */)
{
  if (entered == true)
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

  entered=false;
}


void Raki::dropEvent (QDropEvent *event)
{
  setFrameStyle(QFrame::NoFrame);
  entered=false;
  QStringList list;

  if (QUrlDrag::decodeToUnicodeUris(event, list))
    droppedFile(KURL(list.first()));
}


void Raki::clickedMenu(int item)
{
  switch (item) {
  case -1:
    break;
  case CONNECT_ITEM:
    break;
  case SHUTDOWN_ITEM:
    break;
  case EXECUTE_ITEM:
    runWindow->show();
    break;
  case INSTALL_ITEM:
    managerWindow->show();
    break;
  case CONFIGURE_ITEM:
    configDialog->updateFields();
    configDialog->show();
    break;
  case STARTDCCM_ITEM:
    startDccm();
    break;
  case STOPDCCM_ITEM:
    stopDccm();
    break;
  }
}


QString Raki::changeConnectionState(int state)
{
  switch(state) {
  case 0:
    connected = false;
    actualIcon = &disconnectedIcon;
    setConnectionStatus(false);
    break;
  case 1:
    connected = true;
    actualIcon = &connectedIcon;
    setConnectionStatus(true);
    break;
  }
  setPixmap(*actualIcon);
  return QString("Switched");
}


bool Raki::process(const QCString &fun, const QByteArray &data,
                   QCString &replyType, QByteArray &replyData)
{
  if (fun == "setConnectionStatus(int)") {
    QDataStream arg(data, IO_ReadOnly);
    int i; // parameter
    arg >> i;
    QString result = this->changeConnectionState (i);
    QDataStream reply(replyData, IO_WriteOnly);
    reply << result;
    replyType = "QString";
    return true;
  } else {
    qDebug("unknown function call to BarObject::process()");
    return false;
  }

  return true;
}


void Raki::showAbout(){
    aboutDialog->exec();
}

static const char *description = I18N_NOOP("Raki, a PocketPC-Management Tool");
static KCmdLineOptions options[] =
{
    { 0, 0, 0 }
    // INSERT YOUR COMMANDLINE OPTIONS HERE
};


int main(int argc, char *argv[])
{
    KAboutData aboutData( "raki", I18N_NOOP("Raki"),
        VERSION, description, KAboutData::License_GPL,
        "(c) 2003, Volker Christian (voc)", 0, 0, "voc@users.sourceforge.net");
    aboutData.addAuthor("Volker Christian",0, "voc@users.sourceforge.net");
    aboutData.addCredit("Ludovic Lange", I18N_NOOP("is the Initiator of the SynCE-Project."), "llange@users.sourceforge.net");
    aboutData.addCredit("David Eriksson", I18N_NOOP("is the current Project Manager."), "twogood@users.sourceforge.net");
    aboutData.addCredit("Ganesh Varadarajan", I18N_NOOP("has developed the serial-over-USB driver."), "vganesh@users.sourceforge.net");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
    
    if (!KUniqueApplication::start()) {
       fprintf(stderr, "Raki is already running!\n");
       exit(0);
    }
    
    KUniqueApplication a;

    Raki *raki = new Raki(&aboutData, new KAboutApplication(&aboutData));
    a.setMainWidget(raki);

    raki->connect (&a, SIGNAL (shutDown()), raki, SLOT (quit()));
    raki->show();

    return a.exec();
}
