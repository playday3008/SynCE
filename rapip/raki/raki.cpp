#include <stdlib.h>
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
#include <kfileitem.h>
#include <krun.h>
#include <kstandarddirs.h>

#include "raki.h"
#include "rapiwrapper.h"

#define Icon(x) KGlobal::instance()->iconLoader()->loadIcon(x, KIcon::Toolbar)

Raki::Raki(KAboutData* /*aboutDta*/, KDialog* d, QWidget* parent, 
           const char *name)
        : KSystemTray(parent, name), DCOPObject ("Raki"), aboutDialog(d)
{
    int leCount = 0;
    int reCount = 0;
    
    connectedIcon = Icon("raki");
    disconnectedIcon = Icon("raki_bw");
    setPixmap(disconnectedIcon);

    rapiLeMenu = new KPopupMenu(0, "RakiMenu");
    rapiLeMenu->clear();
    
    rapiLeMenu->insertTitle(SmallIcon("rapip"), i18n("Raki Applet"));
    leCount++;

    rapiLeMenu->insertItem(SmallIcon("run"), i18n("&Execute..."), 
                           EXECUTE_ITEM);
    leCount++;
    
    rapiLeMenu->insertItem(SmallIcon("rotate_cw"), 
                           i18n("&Info && Management..."), INSTALL_ITEM);
    leCount++;
    
    rapiLeMenu->insertItem(SmallIcon("pda_blue"),i18n("&Open rapip:/"), 
                           OPEN_ITEM); 
    leCount++;
    
    rapiLeMenu->insertTearOffHandle(-1, -1);
    leCount++;

    connect(rapiLeMenu, SIGNAL(activated(int)), this, SLOT(clickedMenu(int)));
    rapiLeMenu->setEnabled(false);

    rapiReMenu = new KPopupMenu(0, "RakiMenu");
    rapiReMenu->clear();
    
    rapiReMenu->insertTitle(SmallIcon("rapip"), i18n("Raki Applet"));
    reCount++;
    
    connectId = rapiReMenu->insertItem(SmallIcon("connect_established"), 
                                       i18n("&Connect"), CONNECT_ITEM);
    reCount++;
    
    disconnectId = rapiReMenu->insertItem(SmallIcon("connect_no"), 
                                          i18n("&Disconnect"), 
                                          DISCONNECT_ITEM);
    reCount++;
    
    rapiReMenu->insertSeparator(reCount);
    reCount++;
    
    startDccmId = rapiReMenu->insertItem(SmallIcon("start"), 
                                         i18n("&Start DCCM"), STARTDCCM_ITEM);
    reCount++;
    
    stopDccmId = rapiReMenu->insertItem(SmallIcon("stop"), i18n("S&top DCCM"), 
                                        STOPDCCM_ITEM);
    reCount++;
    
    rapiReMenu->insertSeparator(reCount);
    reCount++;
    
    rapiReMenu->insertItem(SmallIcon("configure"), i18n("&Configure..."), 
                           CONFIGURE_ITEM);
    reCount++;
    
    rapiReMenu->insertSeparator(reCount);
    reCount++;
    
    KHelpMenu *help = new KHelpMenu( this, 0, false );
    connect (help, SIGNAL (showAboutApplication()), this, SLOT (showAbout()));
    rapiReMenu->insertItem(i18n("&Help"), help->menu());
    reCount++; 

    KActionCollection *actionCollection = new KActionCollection(this);
    KAction *quit = KStdAction::quit(this, SLOT (quit()), actionCollection);
    rapiReMenu->insertSeparator(reCount);
    reCount++;
    quit->plug(rapiReMenu, reCount);

    connect(rapiReMenu, SIGNAL(activated(int)), this, SLOT(clickedMenu(int)));
    rapiReMenu->setEnabled(true);
    
    rapiReMenu->setItemEnabled(connectId, true);
    rapiReMenu->setItemEnabled(disconnectId, false);
    
    connect(&dccmProc, SIGNAL(processExited(KProcess *)),
            this, SLOT(dccmExited(KProcess *)));
    connect(&connectProc, SIGNAL(processExited(KProcess *)),
            this, SLOT(connectExited(KProcess *)));
    connect(&disconnectProc, SIGNAL(processExited(KProcess *)),
            this, SLOT(disconnectExited(KProcess *)));

    configDialog = new ConfigDialogImpl(this, "ConfigDialog", true);
    runWindow = new RunWindowImpl(this, "RunWindow", false);
    managerWindow = new ManagerImpl(this, "ManagerWindow", false);
    installer = new Installer(this);
    
    char *path = NULL;
    synce::synce_get_directory(&path);
    
    KStandardDirs *dirs = KGlobal::dirs();
    dirs->addResourceType("synce", "");
    dirs->addPrefix(path);
    
    setAcceptDrops(false);
    entered = false;

    tryStartDccm();
}


Raki::~Raki()
{
    stopDccm();
}


void Raki::tryStartDccm()
{
    dccmRestart = false;
    
    if (configDialog->getStartDccm()) {
        startDccm();
    }
}


void Raki::restartDccm()
{
    dccmRestart = true;
    
    if (dccmProc.isRunning()) {
        stopDccm();
    } else {
        tryStartDccm();
    }
}


void Raki::dccmExited(KProcess */*oldDccm*/)
{
    if (dccmRestart) {
        dccmRestart = false;
        tryStartDccm();
    }   
}


void Raki::startDccm()
{   
    dccmProc.clearArguments();
    
    dccmProc.setExecutable(configDialog->getDccmPath());
    
    dccmProc << "-f";
    
    if (configDialog->getUsePassword()) {
        dccmProc << "-p" << configDialog->getPassword();        
    }
    
    dccmProc.start();
    
    rapiReMenu->setItemEnabled (startDccmId, false);
    rapiReMenu->setItemEnabled (stopDccmId, true);
}


void Raki::stopDccm()
{
    if (dccmProc.isRunning()) {
        dccmProc.kill();
        rapiReMenu->setItemEnabled (startDccmId, true);
        rapiReMenu->setItemEnabled (stopDccmId, false);
    }
}


void Raki::quit()
{
    delete this;
    kapp->quit();
}


void Raki::postConnect(bool enable)
{
    if (enable) {
        KConfig activeConnection("active_connection", true, false, "synce");
    
        activeConnection.setGroup("device");
    
        // activeConnection.readEntry("name");
        // activeConnection.readEntry("class");
        // activeConnection.readEntry("hardware");
        // activeConnection.readEntry("port");
        
        deviceIp = activeConnection.readEntry("ip");
    
        if (configDialog->getMasqueradeEnabled()) {
            KProcess ipTables;
            ipTables.clearArguments();
            ipTables.setExecutable("sudo");
    
            ipTables << "-u" << "root" << configDialog->getIpTables() << "-t" << "nat" 
                     << "-A" << "POSTROUTING" << "-s" << deviceIp  << "-d" 
                     << "0.0.0.0/0" << "-j" << "MASQUERADE";
    
            ipTables.start(KProcess::DontCare);
            
            masqueradeEnabled = true;
        } else {
            masqueradeEnabled = false;
        }
    } else {
        if (masqueradeEnabled) {
            masqueradeEnabled = false;
            KProcess ipTables;
            ipTables.clearArguments();
            ipTables.setExecutable("sudo");
    
            ipTables << "-u" << "root" << configDialog->getIpTables() << "-t" << "nat" 
                     << "-D" << "POSTROUTING" << "-s" << deviceIp  << "-d" 
                     << "0.0.0.0/0" << "-j" << "MASQUERADE";
    
            ipTables.start(KProcess::DontCare);
        }
   }
}


void Raki::setConnectionStatus(bool enable)
{
    rapiLeMenu->setEnabled(enable);
    setAcceptDrops(enable);
    rapiReMenu->setItemEnabled(connectId, !enable);
    rapiReMenu->setItemEnabled(disconnectId, enable);
    postConnect(enable);
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

    if (event->type() == QEvent::MouseButtonDblClick) {
        managerWindow->show();
    } else if (event->button() == LeftButton) {
        showPopupMenu(rapiLeMenu);
    } else if (event->button() == RightButton) {
        showPopupMenu(rapiReMenu);
    }
}


void Raki::customEvent (QCustomEvent *e)
{
    QString msg;

    if (e->type() == QEvent::User) {
        ErrorEvent *event = (ErrorEvent *) e;
        switch(event->getErrorType()) {
        case ErrorEvent::REMOTE_FILE_CREATE_ERROR:
            msg = QString("Could not create ") + 
                          QString(((QString *) event->data())->ascii());
            KMessageBox::error(0, i18n(msg));
            delete ((QString *) event->data());
            break;
        case ErrorEvent::REMOTE_FILE_WRITE_ERROR:
            msg = QString("Could not write to ") + 
                          QString(((QString *) event->data())->ascii());
            KMessageBox::error(0, i18n(msg));
            delete ((QString *) event->data());
            break;
        case ErrorEvent::LOCALE_FILE_OPEN_ERROR:
            msg = QString("Could not open ") + 
                          QString(((QString *) event->data())->ascii());
            KMessageBox::error(0, i18n(msg));
            delete ((QString *) event->data());
            break;
        case ErrorEvent::LOCALE_FILE_READ_ERROR:
            msg = QString("Could not read ") + 
                          QString(((QString *) event->data())->ascii());
            KMessageBox::error(0, i18n(msg));
            delete ((QString *) event->data());
            break;
        case ErrorEvent::REMOTE_FILE_EXECUTE_ERROR:
            msg = QString("Could not execute ") + 
                          QString(((QString *) event->data())->ascii());
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


void Raki::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept(QUrlDrag::canDecode(event));

    if (entered == false) {
        setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    }
    
    entered = true;
}


void Raki::dragLeaveEvent(QDragLeaveEvent * /* event */)
{
    if (entered == true) {
        setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    }
    
    entered = false;
}


void Raki::droppedFile(KURL url)
{
    if (url.path().endsWith(".cab") || url.path().endsWith(".CAB")) {
        installer->installCabinetFile(url);
    }
}


void Raki::dropEvent (QDropEvent *event)
{
    setFrameStyle(QFrame::NoFrame);
    entered=false;
    QStringList list;

    if (QUrlDrag::decodeToUnicodeUris(event, list)) {
        for (QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
            droppedFile(KURL(*it));
        }
    }
}


void Raki::connectExited(KProcess */*oldProc*/)
{ 
}


void Raki::disconnectExited(KProcess */*oldProc*/)
{
}


void Raki::clickedMenu(int item)
{
    switch (item) {
    case -1:
        break;
    case CONNECT_ITEM:
        connectProc.clearArguments();
        connectProc.setExecutable(configDialog->getSynceStart());
        connectProc.start();
        break;
    case DISCONNECT_ITEM:
        disconnectProc.clearArguments();
        disconnectProc.setExecutable(configDialog->getSynceStop());
        disconnectProc.start();
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
        configDialog->show();
        break;
    case STARTDCCM_ITEM:
        startDccm();
        break;
    case STOPDCCM_ITEM:
        stopDccm();
        break;
    case OPEN_ITEM:
        KRun::runURL("rapip:/", QString::fromLatin1("inode/directory"));
        break;
    }
}


QString Raki::changeConnectionState(int state)
{
    switch(state) {
    case 0:
        actualIcon = &disconnectedIcon;
        setConnectionStatus(false);
        break;
    case 1:
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


void Raki::showAbout()
{
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
