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

#include "pda.h"
#include "configdialogimpl.h"
#include "pdaconfigdialogimpl.h"
#include "syncdialogimpl.h"
#include "runwindowimpl.h"
#include "managerimpl.h"
#include "passworddialogimpl.h"
#include "raki.h"
#include "rra.h"
#include "removepartnershipdialogimpl.h"

#include <qcheckbox.h>
#include <kiconloader.h>
#include <klocale.h>
#include <krun.h>
#include <kapplication.h>
#include <kaudioplayer.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include <stdlib.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

PDA::PDA(Raki *raki, QString pdaName)
        : QObject()
{
    int menuCount = 0;
    currentInstalled = 0;
    installed = 0;
    installCounter = 0;
    masqueradeEnabled = false;
    partnerOk = false;
    this->raki = raki;
    this->pdaName = pdaName;
    this->rra = new Rra(pdaName);

    runWindow = new RunWindowImpl(pdaName, raki, "RunWindow", false);
    runWindow->setCaption("Execute on " + pdaName);

    managerWindow = new ManagerImpl(pdaName, raki, "ManagerWindow", false);
    managerWindow->setCaption("Info and Management for " + pdaName);

    passwordDialog = new PasswordDialogImpl(pdaName, raki,
                                            "PasswordWindow", true);
    connect(passwordDialog, SIGNAL(password(QString)), this,
            SLOT(setPassword(QString)));
    passwordDialog->setCaption("Password for " + pdaName);

    configDialog = new PdaConfigDialogImpl(this, pdaName, raki, "ConfigDialog",
                                           false);
    configDialog->setCaption("Configuration for " + pdaName);
    
    syncDialog = new SyncDialogImpl(rra, pdaName, raki, "SynchronizeDialog", true);
    syncDialog->setCaption("Synchronize " + pdaName);

    associatedMenu = new KPopupMenu(raki, "PDA-Menu");

    associatedMenu->clear();
    associatedMenu->setCaption(pdaName);

    associatedMenu->insertTitle(SmallIcon("rapip"), pdaName);
    menuCount++;

    syncItem = associatedMenu->insertItem(SmallIcon("connect_established"), i18n("&Synchronize"),
                               this, SLOT(synchronize()));
    associatedMenu->setItemEnabled(syncItem, false);
    menuCount++;

    associatedMenu->insertItem(SmallIcon("rotate_cw"),
                               i18n("&Info && Management..."), this,
                               SLOT(manage()));
    menuCount++;

    associatedMenu->insertSeparator(menuCount);
    menuCount++;

    associatedMenu->insertItem(SmallIcon("folder"), i18n("&Open rapip://") +
                               QString(pdaName) + "/", this, SLOT(openFs()));
    menuCount++;

    associatedMenu->insertItem(SmallIcon("run"), i18n("&Execute..."), this,
                               SLOT(execute()));
    menuCount++;

    associatedMenu->insertSeparator(menuCount);
    menuCount++;

    associatedMenu->insertItem(SmallIcon("configure"), i18n("Configure ") +
                               QString(pdaName), this, SLOT(configurePda()));
    menuCount++;

    associatedMenu->insertTearOffHandle(-1, -1);
    menuCount++;

    connect(&ipTablesProc, SIGNAL(processExited (KProcess *)), this,
            SLOT(ipTablesExited(KProcess *)));
    connect(&ipTablesProc, SIGNAL(receivedStdout(KProcess*, char *, int)),
            this, SLOT(ipTablesStdout(KProcess *, char *, int)));
    connect(&ipTablesProc, SIGNAL(receivedStderr(KProcess*, char *, int)),
            this, SLOT(ipTablesStderr(KProcess *, char *, int)));
}


PDA::~PDA()
{
    delete syncDialog;
    delete passwordDialog;
    delete runWindow;
    delete managerWindow;
    delete configDialog;
    delete associatedMenu;

    slaveDict.setAutoDelete(true);

    if (masqueradeEnabled) {
        startMasquerading(false);
    }
}


QPtrDict<ObjectType> PDA::getSynchronizationTypes()
{
        return rra->getTypes();
}


void PDA::setMenuIndex(int menuIndex)
{
    this->menuIndex = menuIndex;
}


int PDA::getMenuIndex()
{
    return menuIndex;
}


const char *PDA::getName()
{
    return pdaName.ascii();
}


KPopupMenu *PDA::getMenu()
{
    return associatedMenu;
}


void PDA::execute()
{
    runWindow->show();
}


void PDA::manage()
{
    managerWindow->show();
}


void PDA::openFs()
{
    KRun::runURL("rapip://" + QString(pdaName) + "/",
                 QString::fromLatin1("inode/directory"));
}


void PDA::synchronize(bool forced)
{
    if ((forced || configDialog->getSyncAtConnect()) && isPartner()) {
        QPtrList<SyncTaskListItem>& syncItems = configDialog->syncronizationTasks();
        syncDialog->show(syncItems);
    }
}


void PDA::configurePda()
{
    configDialog->show();
}


void PDA::requestPassword(KSocket *dccmSocket)
{
    this->dccmSocket = dccmSocket;
    KAudioPlayer::play(raki->configDialog->getPasswordRequestNotify());

    if (configDialog->getPassword().isEmpty()) {
        passwordDialog->exec();
    } else {
        emit resolvedPassword(pdaName, configDialog->getPassword(), dccmSocket);
    }
}


void PDA::setPassword(QString password)
{
    if (passwordDialog->rememberPasswordCheck->isChecked()) {
        configDialog->passwordEdit->setText(password);
        configDialog->applySlot();
        configDialog->writeConfig();
    }
    emit resolvedPassword(pdaName, password, dccmSocket);
}


void PDA::passwordInvalid()
{
    KAudioPlayer::play(raki->configDialog->getPasswordRejectNotify());
    configDialog->passwordEdit->setText("");
    configDialog->applySlot();
    configDialog->writeConfig();
}


void PDA::ipTablesExited(KProcess *ipTablesProc)
{
    int exitStatus;

    if (ipTablesProc->normalExit()) {
        exitStatus = ipTablesProc->exitStatus();
        if (exitStatus != 0) {
            KMessageBox::error(raki, "Could not create masqueraded route",
                               "iptables", KMessageBox::Notify);
        } else {
            kdDebug(2120) << "Masqueraded Route created for \"" << pdaName.ascii() << "\"" << endl;
        }
    } else {
        KMessageBox::error(raki, "iptables terminated due to a signal",
                           "iptables", KMessageBox::Notify);
    }
    disconnect(ipTablesProc, SIGNAL(processExited (KProcess *)), this,
               SLOT(ipTablesExited(KProcess *)));
}


void PDA::ipTablesStdout(KProcess *, char *buf, int len)
{
    if (buf != NULL && len > 0) {
        buf[len] = 0;
        kdDebug(2120) << "stdout::iptables: " << buf << endl;
    }
}


void PDA::ipTablesStderr(KProcess *, char *buf, int len)
{
    if (buf != NULL && len > 0) {
        buf[len] = 0;
        kdDebug(2120) << "stderr::iptables: " << buf << endl;
    }
}


bool PDA::startMasquerading(bool start)
{
    bool enabled = false;

    if ((!start && masqueradeEnabled) || start) {
        ipTablesProc.clearArguments();
        ipTablesProc.setExecutable("sudo");

        ipTablesProc << "-u" << "root" << raki->configDialog->getIpTables()
        << "-t" << "nat" << ((start) ? "-A" : "-D")
        << "POSTROUTING" << "-s" << configDialog->getDeviceIp() << "-d"
        << "0.0.0.0/0" << "-j" << "MASQUERADE";

        if (ipTablesProc.start(KProcess::NotifyOnExit, (KProcess::Communication)
                               (KProcess::Stdout | KProcess::Stderr))) {
            enabled = start;
        } else {
            KMessageBox::error(raki, "Could not start iptables", "iptables",
                               KMessageBox::Notify);
        }
    }

    return enabled;
}


void PDA::setConnected()
{
    if (configDialog->getMasqueradeEnabled()) {
        masqueradeEnabled = startMasquerading(true);
    }
    KAudioPlayer::play(raki->configDialog->getConnectNotify());
}


void PDA::setDisconnected()
{
    if (masqueradeEnabled) {
        startMasquerading(false);
        masqueradeEnabled = false;
    }
    KAudioPlayer::play(raki->configDialog->getDisconnectNotify());
}


void PDA::registerCopyJob(KIO::CopyJob *copyJob)
{
    slaveDict.insert(copyJob, new KURL::List::List());
}


void PDA::addURLByCopyJob(KIO::CopyJob *copyJob, KURL& url)
{
    KURL::List *list = slaveDict.find(copyJob);
    KURL::List::Iterator it;

    it = list->find(url);

    if (it == list->end()) {
        list->append(url);
    }
}


void PDA::unregisterCopyJob(KIO::CopyJob *copyJob)
{
    delete slaveDict.take(copyJob);
}


KURL::List& PDA::getURLListByCopyJob(KIO::CopyJob *copyJob)
{
    KURL::List *list = slaveDict.find(copyJob);

    return *list;
}


void *PDA::advanceProgressEvent(void *data)
{
    int advance = (int) data;

    progressBar->advance(advance);

    return NULL;
}


void *PDA::advanceTotalStepsEvent(void *data)
{
    int advance = (int) data;

    progressBar->setTotalSteps(progressBar->totalSteps() + advance);

    return NULL;
}


#define advanceProgress(a) \
    postThreadEvent(&PDA::advanceProgressEvent, a, noBlock);

#define advanceTotalSteps(a) \
    postThreadEvent(&PDA::advanceTotalStepsEvent, a, noBlock);


void *PDA::removePartnershipDialog(void *data)
{
    struct Rra::Partner * partner = (struct Rra::Partner *) data;

    return (void *) RemovePartnershipDialogImpl::showDialog(QString(partner[0].name),
            QString(partner[1].name) , 0, "Remove Partnership", true, 0);
}


void *PDA::alreadyTwoPartnershipsDialog(void *)
{
    KMessageBox::error((QWidget *) parent(),
                       "There are already two partnerships configured on the device. Using guest",
                       "Error configuring partnership");

    return NULL;
}


void PDA::trySetPartner()
{
    struct Rra::Partner partner[2];
    struct Rra::Partner newPartner;
    char hostname[256];
    partnerOk = false;

    srandom(QDateTime::currentDateTime().toTime_t());

    kdDebug(2120) << "Tryping to set a partnership" << endl;

    if (gethostname(hostname, 256) != 0) {
        return;
    }

    newPartner.name = hostname;
    newPartner.id = random();

    partner[0] = rra->getPartner(1);
    advanceTotalSteps(1);
    advanceProgress(1);
    kdDebug(2120) << "   Trying partnership 1" << endl;
    if (partner[0].name.isEmpty() || partner[0].id == 0) {
        kdDebug(2120) << "      Partnership 1 free ... using this index" << endl;
        newPartner.index = 1;
        rra->setPartner(newPartner);
        rra->setCurrentPartner(1);
        advanceProgress(1);
        partnerName = newPartner.name;
        partnerId = newPartner.id;
        partnerOk = true;
        associatedMenu->setItemEnabled(syncItem, true);
    } else {
        partner[1] = rra->getPartner(2);
        advanceTotalSteps(1);
        advanceProgress(1);
        kdDebug(2120) << "   Trying partnership 2" << endl;
        if (partner[1].name.isEmpty() || partner[1].id == 0) {
            kdDebug(2120) << "      Partnership 2 free ... using this index" << endl;
            newPartner.index = 2;
            rra->setPartner(newPartner);
            rra->setCurrentPartner(2);
            advanceProgress(1);
            partnerName = newPartner.name;
            partnerId = newPartner.id;
            partnerOk = true;
            associatedMenu->setItemEnabled(syncItem, true);
        }
    }

    if (partnerOk) {
        configDialog->setPartner(partnerName, partnerId);
        return;
    }

    int deletedItems = (int) postThreadEvent(&PDA::removePartnershipDialog, partner, block);

    if (deletedItems != 0) {
        if (deletedItems & 1 | 2) { 
            advanceTotalSteps(2);
        } else { 
            advanceTotalSteps(1);
        }
        if (deletedItems & 1) {
            newPartner.name = "";
            newPartner.id = 0;
            newPartner.index = 1;
            rra->setPartner(newPartner);
            advanceProgress(1);
        }
        if (deletedItems & 2) {
            newPartner.name = "";
            newPartner.id = 0;
            newPartner.index = 2;
            rra->setPartner(newPartner);
            advanceProgress(1);
        }
        trySetPartner();
    } else {
        postThreadEvent(&PDA::alreadyTwoPartnershipsDialog, NULL, block);

        partnerName = "";
        partnerId = 0;
        rra->setCurrentPartner(0);
        advanceProgress(1);
    }
}


bool PDA::isPartner()
{
    return partnerOk;
}


void *PDA::noPartnershipQuestion(void *)
{
    KGuiItem yesItem("Set Partnership");
    KGuiItem noItem("Use Guest");

    return (void *) KMessageBox::questionYesNo((QWidget *) parent(), "No valid Partnership found. "
            "Do you wan't to set one "
            "or connect as guest?", "Partnership", yesItem, noItem);
}


void PDA::checkPartner()
{
    struct Rra::Partner partner;

    kdDebug(2120) << "Checking partner" << endl;

    if (configDialog->getPartnerId() == 0) {
        kdDebug(2120) << "No previous partnership found ... " << endl;
    } else {
        kdDebug(2120) << "Previous partnership found ... trying to match with pda" << endl;
        advanceTotalSteps(1);
        partner = rra->getPartner(1);
        advanceProgress(1);
        kdDebug(2120) << "Checking: " << partner.name << " " << partner.id << " " <<
        configDialog->getPartnerName() << " " << configDialog->getPartnerId() << endl;
        if (partner.name == configDialog->getPartnerName() &&
                partner.id == configDialog->getPartnerId() && partner.id != 0 && !partner.name.isEmpty()) {
            kdDebug(2120) << "Partnership 1 ... ok" << endl;
            partnerOk = true;
            rra->setCurrentPartner(1);
            advanceProgress(1);
            associatedMenu->setItemEnabled(syncItem, true);
            return;
        }

        advanceTotalSteps(1); // 5
        partner = rra->getPartner(2);
        advanceProgress(1);
        kdDebug(2120) << "Checking: " << partner.name << " " << partner.id << " " <<
        configDialog->getPartnerName() << " " << configDialog->getPartnerId() << endl;
        if (partner.name == configDialog->getPartnerName() &&
                partner.id == configDialog->getPartnerId() && partner.id != 0 && !partner.name.isEmpty()) {
            kdDebug(2120) << "Partnership 2 ... ok" << endl;
            partnerOk = true;
            rra->setCurrentPartner(2);
            advanceProgress(1);
            associatedMenu->setItemEnabled(syncItem, true);
            return;
        }
        kdDebug(2120) << "No match found ... " << endl;
    }

    kdDebug(2120) << "trying to set one" << endl;

    int answer = (int) postThreadEvent(&PDA::noPartnershipQuestion, NULL, block);

    if (answer == KMessageBox::Yes) {
        trySetPartner();
    } else {
        partnerName = "";
        partnerId = 0;
        rra->setCurrentPartner(0);
        advanceProgress(1);
    }
}


void *PDA::progressDialogCancel(void *)
{
    progressDialog->hide();
    emit initialized(this);
    delete progressDialog;
    configDialog->writeConfig();

    return NULL;
}


void *PDA::rraConnectionError(void *)
{
    KMessageBox::error(0, "Could not create a RRA-connection");

    return NULL;
}


void *PDA::initializationStarted(void *)
{
    return NULL;
}


void PDA::initThread(QThread *, void *)
{
    postThreadEvent(&PDA::initializationStarted, NULL, noBlock);
    
    if (!configDialog->isNewPda()) {
        if (rra->connect()) {
            advanceProgress(1);
            checkPartner();
            rra->disconnect();
        } else {
            postThreadEvent(&PDA::rraConnectionError, NULL, block);
        }
    } else {
        if (rra->connect()) {
            advanceProgress(1);
            trySetPartner();
            rra->disconnect();
        } else {
            postThreadEvent(&PDA::rraConnectionError, NULL, block);
        }
    }

    postThreadEvent(&PDA::progressDialogCancel, NULL, noBlock);
}


void PDA::init()
{
    kdDebug(2120) << "in pda-init" << endl;
    progressDialog = new KProgressDialog(raki, "Connecting",
                                         "Connecting...", "Connecting " + pdaName + "...", true);
    progressDialog->setAllowCancel(false);
    progressBar = progressDialog->progressBar();
    progressDialog->setMinimumDuration(0);
    progressBar->setTotalSteps(3);
    progressBar->advance(1);
    startWorkerThread(this, &PDA::initThread, NULL);
}
