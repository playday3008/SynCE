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

#include "raki.h"
#include "rapiwrapper.h"
#include "configdialogimpl.h"
#include "rakiworkerthread.h"
#include "pda.h"
#include "installer.h"

#include <kglobal.h>
#include <klocale.h>
#include <kaction.h>
#include <kapplication.h>
#include <kuniqueapplication.h>
#include <khelpmenu.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kfileitem.h>
#include <krun.h>
#include <kdebug.h>
#include <ktip.h>
#include <qcursor.h>
#include <qdragobject.h>
#include <kstandarddirs.h>
#include <qtooltip.h>
#include <qstringlist.h>

#include <unistd.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

#define Icon(x) KGlobal::instance()->iconLoader()->loadIcon(x, KIcon::Toolbar)

Raki::Raki(KAboutData *, KDialog* d, QWidget* parent, const char *name)
        : KSystemTray(parent, name), DCOPObject ("Raki"), aboutDialog(d)
{
    initialized = true;
    int leCount = 0;
    int reCount = 0;

    connectedIcon = Icon("raki");
    disconnectedIcon = Icon("raki_bw");

    rapiLeMenu = new KPopupMenu(0, "RakiMenu");
    rapiLeMenu->setCaption("Connected Devices");
    rapiLeMenu->clear();

    rapiLeMenu->insertTitle(SmallIcon("rapip"), i18n("Connected Devices"));
    leCount++;

    rapiLeMenu->insertTearOffHandle(-1, -1);
    leCount++;

    rapiReMenu = new KPopupMenu(0, "RakiMenu");
    rapiReMenu->clear();

    rapiReMenu->insertTitle(SmallIcon("rapip"), i18n("Raki Applet"));
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

    connect(&dccmProc, SIGNAL(processExited(KProcess *)),
            this, SLOT(dccmExited(KProcess *)));
    connect(&dccmProc, SIGNAL(receivedStdout(KProcess*, char *, int)),
            this, SLOT(dccmStdout(KProcess *, char *, int)));
    connect(&dccmProc, SIGNAL(receivedStderr(KProcess*, char *, int)),
            this, SLOT(dccmStderr(KProcess *, char *, int)));

    configDialog = new ConfigDialogImpl(this, "ConfigDialog", true);
    connect(configDialog, SIGNAL(restartDccm()), this, SLOT(restartDccm()));
    connect(configDialog, SIGNAL(configError()), this, SLOT(quit()));

    installer = new Installer(this, &pdaList);

    entered = false;

    setConnectionStatus(false);

    dccmConnection = NULL;
    openDccmConnection();

    configDialog->checkRunningVersion();
    
    KTipDialog::showTip(KApplication::kApplication()->dirs()->findResource("data", "raki/tips"));

    startTimer(1000);
}


Raki::~Raki()
{
    QDictIterator<PDA> it(pdaList);

    for (; it.current(); ++it ) {
        delete *it;
    }
    stopDccm();
    dccmProc.detach();

    if (dccmConnection)
        delete dccmConnection;
}


void Raki::timerEvent(QTimerEvent */*e*/)
{
    initializePda();
}


void Raki::initializePda()
{
    PDA *pda = NULL;

    if (!pendingPdaList.empty()) {
        QString commandAndPda = *pendingPdaList.begin();

        char cmd = (commandAndPda)[0].latin1();
        QString pdaName = (commandAndPda).mid(1);

        if (!pdaName.isEmpty())
            pda = pdaList.find(pdaName);

        switch(cmd) {
        case 'C':
            if (!RakiWorkerThread::running()) {
                pendingPdaList.remove(commandAndPda);
                if (!pda) {
                    pda = new PDA(this, pdaName);
                    pdaList.insert(pdaName, pda);
                }
                connect(pda, SIGNAL(initialized(PDA *)), this, SLOT(pdaInitialized(PDA*)));
                pda->init();
            }
            break;
        case 'D':
            pendingPdaList.remove(commandAndPda);
            if (pda) {
                bool showAgain = false;
                pda->setDisconnected();
                if (rapiLeMenu->isVisible()) {
                    rapiLeMenu->hide();
                    showAgain = true;
                }
                rapiLeMenu->removeItem(pda->getMenuIndex());
                if (showAgain && rapiLeMenu->count() > 3) {
                    rapiLeMenu->show();
                }
                pdaList.take(pdaName);
                delete pda;
                if (pdaList.count() == 0)
                    setConnectionStatus(false);
                else
                    setConnectionStatus(true);
            }
            break;
        case 'P':
            pendingPdaList.remove(commandAndPda);
            if (!pda) {
                pda = new PDA(this, pdaName);
                pdaList.insert(pdaName, pda);
            }
            connect(pda, SIGNAL(resolvedPassword(QString, QString, KSocket *)),
                    this, SLOT(resolvedPassword(QString, QString, KSocket *)));
            pda->requestPassword(dccmConnection);
            break;
        case 'R':
            pendingPdaList.remove(commandAndPda);
            if (pda) {
                KMessageBox::error(0, QString("Password for PDA \"") + pda->getName() + QString("\" invalid. Password cleared!"));
                pda->passwordInvalid();
                pdaList.take(pdaName);
                delete pda;
            }
            break;
        case 'S':
            pendingPdaList.remove(commandAndPda);
            // dccm stoped
            break;
        default:
            pendingPdaList.remove(commandAndPda);
            break;
        }
    }
}


void Raki::connectionRequest(KSocket *dccmSocket)
{
    char buffer[256];

    int n = read(dccmSocket->socket(), buffer, 256);

    if (n > 0) {
        buffer[n] = '\0';

        QStringList pdaNames = QStringList::split(";", buffer);

        for (QStringList::Iterator it = pdaNames.begin(); it != pdaNames.end(); ++it) {
            pendingPdaList += *it;
        }
    }

    initializePda();
}


void Raki::dccmConnect()
{
    char *path = NULL;
    synce::synce_get_directory(&path);

    dccmConnection = new KSocket((QString(path) + QString("/csock")).ascii());
    synce::wstr_free_string(path);  // use this wstr function to free a "normal" cstring. 
                                    // It does the same.and doesn't require stdlib.h to
                                    // be included.

    if (dccmConnection->socket() > 0) {
        dccmConnection->enableRead(true);
        connect(dccmConnection, SIGNAL(readEvent(KSocket *)),
                this, SLOT(connectionRequest(KSocket *)));
        connect(dccmConnection, SIGNAL(closeEvent(KSocket *)),
                this, SLOT(closeDccmConnection(KSocket *)));
    } else {
        delete dccmConnection;
        dccmConnection = NULL;
    }
}


void Raki::openDccmConnection()
{
    if (dccmConnection == NULL) {
        dccmConnect();
    } else if (dccmConnection->socket() < 0) {
        delete dccmConnection;
        dccmConnect();
    }
}


void Raki::closeDccmConnection(KSocket */*dccmSocket*/)
{
    if (dccmConnection) {
        delete dccmConnection;
        dccmConnection = NULL;
    }
}


void Raki::pdaInitialized(PDA *pda)
{
    bool showAgain = false;

    pda->setConnected();
    if (rapiLeMenu->isVisible()) {
        rapiLeMenu->hide();
        showAgain = true;
    }
    rapiLeMenu->insertItem(SmallIcon("pda_blue"),
                           pda->getName(),
                           pda->getMenu());
    if (showAgain) {
        rapiLeMenu->show();
    }
    setConnectionStatus(true);
    pda->synchronize(false);
}


void Raki::resolvedPassword(QString pdaName, QString password, KSocket *dccmSocket)
{
    char passBuffer[100];
    const char *pdaNamec = pdaName.ascii();

    sprintf(passBuffer, "R%s=%s", pdaNamec, password.ascii());
    write(dccmSocket->socket(), passBuffer, strlen(passBuffer));
}


void Raki::tryStartDccm()
{
    dccmRestart = false;

    if (configDialog->getStartDccm()) {
        startDccm();
    } else {
        rapiReMenu->setItemEnabled (startDccmId, true);
        rapiReMenu->setItemEnabled (stopDccmId, false);
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
    rapiReMenu->setItemEnabled (startDccmId, true);
    rapiReMenu->setItemEnabled (stopDccmId, false);

    if (dccmRestart) {
        dccmRestart = false;
        tryStartDccm();
    }

    if (dccmShouldRun) {
        KMessageBox::error(0, "Could not start dccm or dccm has exited.\n"
                              "Maybe there is already a dccm running");
    }
}


void Raki::dccmStdout(KProcess *, char *buf, int len)
{
    if (buf != NULL && len > 0) {
        buf[len] = 0;
        kdDebug(2120) << "stdout::dccm: " << buf << endl;
    }
}


void Raki::dccmStderr(KProcess *, char *buf, int len)
{
    if (buf != NULL && len > 0) {
        buf[len] = 0;
        kdDebug(2120) << "stderr::dccm: " << buf << endl;
    }
}


void Raki::startDccm()
{
    dccmProc.clearArguments();
    dccmProc.setExecutable(configDialog->getDccmPath());
    dccmProc << "-f" /* << "-d" << "3"*/;
    
    dccmShouldRun = true;

    dccmProc.start(KProcess::NotifyOnExit,  (KProcess::Communication)
                   (KProcess::Stdout | KProcess::Stderr));

    if (dccmProc.isRunning()) {
        rapiReMenu->setItemEnabled (startDccmId, false);
        rapiReMenu->setItemEnabled (stopDccmId, true);
    } else {
        rapiReMenu->setItemEnabled (startDccmId, true);
        rapiReMenu->setItemEnabled (stopDccmId, false);
    }
}


void Raki::stopDccm()
{
    dccmShouldRun = false;

    if (dccmProc.isRunning()) {
        dccmProc.kill();
    }
}


void Raki::shutDown()
{
    initialized = false;
    delete this;
}


void Raki::quit()
{
    shutDown();
    kapp->quit();
}


bool Raki::isInitialized()
{
    return initialized;
}


void Raki::setConnectionStatus(bool enable)
{
    QToolTip::remove(this);

    rapiLeMenu->setEnabled(enable);
    setAcceptDrops(enable);

    if (enable) {
        connected = true;
        actualIcon = &connectedIcon;
        QDictIterator<PDA> it(pdaList);

        QString str = QString("<b><center><u>Devices</u></center></b>\n");

        for (; it.current(); ++it ) {
            str = str + "Name: <b>" + (*it)->getName() +
                  "</b><br>";
        }
        QToolTip::add(this, str);

    } else {
        connected = false;
        actualIcon = &disconnectedIcon;
        QToolTip::add(this, "No Device connected");
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
    if (event->button() == LeftButton) {
        showPopupMenu(rapiLeMenu);
    } else if (event->button() == RightButton) {
        showPopupMenu(rapiReMenu);
    }
}


void Raki::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept(QUrlDrag::canDecode(event));

    if (entered == false) {
        setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    }

    entered = true;
}


void Raki::dragLeaveEvent(QDragLeaveEvent * /* event */)
{
    if (entered == true) {
        setFrameStyle(QFrame::NoFrame | QFrame::Plain);
    }

    entered = false;
}


void Raki::dropEvent (QDropEvent *event)
{
    setFrameStyle(QFrame::NoFrame);
    entered=false;

    QStringList list;

    if (QUrlDrag::decodeToUnicodeUris(event, list)) {
        installer->show(list);
    }
}


void Raki::clickedMenu(int item)
{
    switch (item) {
    case -1:
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
    }
}


QString Raki::changeConnectionState(int state)
{
    PDA *pda;
    bool newPda = false;
    QString pdaName = "active_connection";

    if (!(pda = pdaList.find("active_connection"))) {
        pda = new PDA(this, pdaName);
        newPda = true;
    }

    switch(state) {
    case 0:
        rapiLeMenu->removeItem(pda->getMenuIndex());
        pda->setDisconnected();
        pdaList.take("active_connection");
        delete pda;
        if (pdaList.count() == 0)
            setConnectionStatus(false);
        else
            setConnectionStatus(true);
        break;
    case 1:
        if (newPda)
            pdaList.insert("active_connection", pda);
        pda->setConnected();
        rapiLeMenu->insertItem(SmallIcon("pda_blue"), "Anonymous",
                               pda->getMenu());
        setConnectionStatus(true);
        break;
    }
    return QString("Switched");
}


void Raki::dccmNotification(QString signal)
{
    if (signal == "start") {
        openDccmConnection();
    } else if (signal == "stop") {
        // nothing to do jet
    } else if (signal == "connect") {
        // nothing to do jet
    } else if (signal == "disconnect") {
        // nothing to do jet
    }
}


static const char* const Raki_ftable[4][3] =
    {
        { "void", "dccmNotification(QString)", "dccmNotification(QString)" },
        { "void", "setConnectionStatus(int)", "setConnectionStatus(int)" },
        { "void", "installCabFile(QString)", "installCabFile(QString)" },
        { 0, 0, 0 }
    };


bool Raki::process(const QCString &fun, const QByteArray &data, QCString& replyType, QByteArray &replyData)
{
    if ( fun == Raki_ftable[0][1] ) { // void dccmNotification(QString)
        QString arg0;
        QDataStream arg( data, IO_ReadOnly );
        arg >> arg0;
        replyType = Raki_ftable[0][0];
        dccmNotification(arg0 );
    } else if ( fun == Raki_ftable[1][1] ) { // void setConnectionStatus(int)
        int arg0;
        QDataStream arg( data, IO_ReadOnly );
        arg >> arg0;
        replyType = Raki_ftable[1][0];
        changeConnectionState(arg0 );
    } else if (fun == Raki_ftable[2][1] ) { // void installCabFile(QString)
        QString arg0;
        QDataStream arg( data, IO_ReadOnly );
        arg >> arg0;
        replyType = Raki_ftable[2][0];
        QStringList list;
        list << arg0;
        if (!pdaList.isEmpty())
            installer->show(list);
    } else {
        return DCOPObject::process( fun, data, replyType, replyData );
    }
    return TRUE;
}


QCStringList Raki::interfaces()
{
    QCStringList ifaces = DCOPObject::interfaces();
    ifaces += "Raki";
    return ifaces;
}


QCStringList Raki::functions()
{
    QCStringList funcs = DCOPObject::functions();
    for ( int i = 0; Raki_ftable[i][2]; i++ ) {
        QCString func = Raki_ftable[i][0];
        func += ' ';
        func += Raki_ftable[i][2];
        funcs << func;
    }
    return funcs;
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
                                    "ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT\n"
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
                         "(c) 2003, Volker Christian (voc)", 0, "http://synce.sourceforge.net/synce/kde/",
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
    aboutData.addCredit("Michael Jarrett",
                        I18N_NOOP("ported the AvantGo support from Malsync to SynCE."),
                        "JudgeBeavis@hotmail.com");
    aboutData.setLicenseText(MITlicense);

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options );

#ifdef NOTUNIQUE
    KApplication a;
#else
    if (!KUniqueApplication::start()) {
        fprintf(stderr, "Raki is already running!\n");
    } else {
        KUniqueApplication a;
#endif

        Raki *raki = new Raki(&aboutData, new KAboutApplication(&aboutData));

        if (raki->isInitialized()) {
            a.setMainWidget(raki);
            raki->connect (&a, SIGNAL (aboutToQuit()), raki, SLOT (shutDown()));
            raki->show();

            return a.exec();
        }
#ifndef NOUNIQUE
    }
#endif
}
