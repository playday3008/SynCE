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
#include "rapiwrapper.h"
#include "removepartnershipdialogimpl.h"
#include "initprogress.h"

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
    delete rra;

    slaveDict.setAutoDelete(true);

    if (masqueradeEnabled) {
        startMasquerading(false);
    }
}


bool PDA::getSynchronizationTypes(QPtrDict<ObjectType> *types)
{
    return rra->getTypes(types);
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

    if (advance == 1) {
        progressBar->advance(advance);
    } else {
        progressBar->setProgress(advance);
    }

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
    int removedPartners;

    initProgress->hide();

    removedPartners = RemovePartnershipDialogImpl::showDialog(QString(partner[0].name),
            QString(partner[1].name) , 0, "Remove Partnership", true, 0);

    initProgress->show();

    return (void *) removedPartners;
}


void *PDA::alreadyTwoPartnershipsDialog(void *)
{
    initProgress->hide();
    KMessageBox::error((QWidget *) parent(),
                       "There are already two partnerships configured on the device. Using guest",
                       "Error configuring partnership");
    initProgress->show();
    return NULL;
}


bool PDA::isPartner()
{
    return partnerOk;
}


void *PDA::progressDialogCancel(void *init)
{
    initProgress->hide();
    emit initialized(this, (int ) init);
    delete initProgress;
    if (init)
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


bool PDA::removePartnership(int *removedPartnerships)
{
    struct Rra::Partner partners[2];
    bool removePartnershipOk = true;

    advanceProgress(1);
    if (rra->getPartner(1, &partners[0])) {
        advanceProgress(1);
        if (rra->getPartner(2, &partners[1])) {
            int deletedItems = (int) postThreadEvent(&PDA::removePartnershipDialog, partners, block);
            if (deletedItems > 0) {
                if (deletedItems != 0) {
                    struct Rra::Partner deletedPartner;
                    if (deletedItems & 1) {
                        deletedPartner.name = "";
                        deletedPartner.id = 0;
                        deletedPartner.index = 1;
                        advanceProgress(1);
                        if (!rra->setPartner(deletedPartner)) {
                            removePartnershipOk = false;
                        }
                    }
                    if ((deletedItems & 2) && removePartnershipOk) {
                        deletedPartner.name = "";
                        deletedPartner.id = 0;
                        deletedPartner.index = 2;
                        advanceProgress(1);
                        if (!rra->setPartner(deletedPartner)) {
                            removePartnershipOk = false;
                        }
                    }
                }
                *removedPartnerships = deletedItems;
            }
        } else {
            removePartnershipOk = false;
        }
    } else {
        removePartnershipOk = false;
    }

    return removePartnershipOk;
}

bool PDA::setPartnershipThread()
{
    kdDebug(2120) << "in PDE::init" << endl;
    bool setPartnerOk = true;
    unsigned int index;

    advanceProgress(1);
    if (!rra->partnerCreate(& index)) {
        int removedPartnerships = 0;
        if (removePartnership(&removedPartnerships)) {
            if (removedPartnerships > 0) {
                setPartnerOk = setPartnershipThread();
            } else {
                postThreadEvent(&PDA::alreadyTwoPartnershipsDialog, NULL, block);
                kdDebug(2120) << "Using Guest" << endl;
                partnerName = "";
                partnerId = 0;
                advanceProgress(7);
                if (rra->setCurrentPartner(0)) {
                    setPartnerOk = false;
                }
            }
        } else {
            setPartnerOk = false;
        }
    } else {
        struct Rra::Partner partner;
        advanceProgress(7);
        if (rra->getPartner(index, &partner)) {
            partnerOk = true;
            partnerName = partner.name;
            partnerId = partner.id;
            associatedMenu->setItemEnabled(syncItem, true);
        } else {
            setPartnerOk = false;
        }
    }

    return setPartnerOk;
}


void PDA::setPartnership(QThread */*thread*/, void *)
{
    bool setPartnerOk = false;

    if (Ce::rapiInit(pdaName)) {
        setPartnerOk = setPartnershipThread();
        Ce::rapiUninit();
    }

    if (setPartnerOk) {
        configDialog->setPartner(partnerName, partnerId);
        postThreadEvent(&PDA::progressDialogCancel, 1, block);
    } else {
        postThreadEvent(&PDA::progressDialogCancel, 0, block);
    }
}


void PDA::init()
{
    kdDebug(2120) << "in pda-init" << endl;

    initProgress = new InitProgress(raki, "InitProgress", true,
                WStyle_Customize | WStyle_NoBorder | WStyle_Tool | WX11BypassWM);

    progressBar = initProgress->progressBar;
    progressBar->setTotalSteps(7);
    initProgress->pdaName->setText(pdaName);
    initProgress->show();

    startWorkerThread(this, &PDA::setPartnership, NULL);
}
