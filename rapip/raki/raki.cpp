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
#include <qtooltip.h>

#include "raki.h"
#include "rapiwrapper.h"

#define Icon(x) KGlobal::instance()->iconLoader()->loadIcon(x, KIcon::Toolbar)

Raki::Raki(KAboutData *, KDialog* d, QWidget* parent, const char *name)
        : KSystemTray(parent, name), DCOPObject ("Raki"), aboutDialog(d)
{
    int leCount = 0;
    int reCount = 0;
    
    connectedIcon = Icon("raki");
    disconnectedIcon = Icon("raki_bw");

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
    synceDir = QString(path);

    entered = false;
    
    masqueradeStarted = false;

    setConnectionStatus(false);
    
    tryStartDccm();
}


Raki::~Raki()
{
    startMasquerading(false);
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


void Raki::startMasquerading(bool start)
{
    if (configDialog->getMasqueradeEnabled() || masqueradeStarted) {
        KProcess ipTables;
        ipTables.clearArguments();
        ipTables.setExecutable("sudo");
    
        ipTables << "-u" << "root" << configDialog->getIpTables() << "-t" << "nat"
                 << ((start) ? "-A" : "-D") << "POSTROUTING" << "-s" << deviceIp 
                 << "-d" << "0.0.0.0/0" << "-j" << "MASQUERADE";
    
        ipTables.start(KProcess::DontCare);
        
        if (start) {
            masqueradeStarted = true;
        } else {
            masqueradeStarted = false;
        }
    }
}


void Raki::setConnectionStatus(bool enable)
{
    QToolTip::remove(this);
    
    rapiLeMenu->setEnabled(enable);
    setAcceptDrops(enable);
    rapiReMenu->setItemEnabled(connectId, !enable);
    rapiReMenu->setItemEnabled(disconnectId, enable);
    
    if (enable) {
        connected = true;
        actualIcon = &connectedIcon;
        KSimpleConfig activeConnection(synceDir + "/active_connection", true);
        activeConnection.setGroup("device");
        QToolTip::add(this, "<b><center><u>Device Info</u></center></b><table>"
            "<tr><td><b>Name:</b></td><td>" + 
                    activeConnection.readEntry("name") 
            + "</td></tr><tr><td><b>Class:</b></td><td>" + 
                    activeConnection.readEntry("class") 
            + "</td></tr><tr><td><b>Hardware:</b></td><td>" + 
                    activeConnection.readEntry("hardware") 
            + "</td></tr>");
        // activeConnection.readEntry("port");
        
        deviceIp = activeConnection.readEntry("ip");
        startMasquerading(true);
    } else {
        connected = false;
        actualIcon = &disconnectedIcon;
        QToolTip::add(this, "No Device connected");
        startMasquerading(false);
    }
    setPixmap(*actualIcon);
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
        setConnectionStatus(false);
        break;
    case 1:
        setConnectionStatus(true);
        break;
    }
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
static const char *MITlicense = I18N_NOOP(
"Copyright (c) 2003 Volker Christian\n"
"\n"
"Permission is hereby granted, free of charge, to\n"
"any person obtaining a copy of this software and\n"
"associated documentation files (the \"Software\"), to\n"
"deal in the Software without restriction, including\n"
"without limitation the rights to use, copy, modify,\n"
"merge, publish, distribute, sublicense, and/or sell\n"
"copies of the Software, and to permit persons to whom\n"
"the Software is furnished to do so, subject to the\n"
"following conditions:\n"
"\n"
"The above copyright notice and this permission notice\n"
"shall be included in all copies or substantial portions\n"
"of the Software.\n"
"\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF\n"
"ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED\n"
"TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A\n"
"PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT\n"
"SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR\n"
"ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN\n"
"ACTION OF CONTRACT,TORT OR OTHERWISE, ARISING FROM, OUT\n"
"OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
"OTHER DEALINGS IN THE SOFTWARE.");

static KCmdLineOptions options[] =
    {
        { 0, 0, 0 }
        // INSERT YOUR COMMANDLINE OPTIONS HERE
    };


int main(int argc, char *argv[])
{
    KAboutData aboutData("raki", I18N_NOOP("Raki"),
                         VERSION, description, KAboutData::License_Custom,
                         "(c) 2003, Volker Christian (voc)", 0, 0, 
                         "voc@users.sourceforge.net");
    aboutData.addAuthor("Volker Christian", 0, "voc@users.sourceforge.net");
    aboutData.addCredit("Ludovic Lange",
                        I18N_NOOP("is the Initiator of the SynCE-Project."), 
                        "llange@users.sourceforge.net");
    aboutData.addCredit("David Eriksson", 
                        I18N_NOOP("is the current Project Manager."), 
                        "twogood@users.sourceforge.net");
    aboutData.addCredit("Ganesh Varadarajan", 
                        I18N_NOOP("has developed the serial-over-USB driver."), 
                        "vganesh@users.sourceforge.net");
    aboutData.setLicenseText(MITlicense);

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options );

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
