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
#include <qdatetime.h>
#include <kiconloader.h>
#include <klocale.h>
#include <krun.h>
#include <kapplication.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kdebug.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

PDA::PDA(Raki *raki, QString pdaName)
        : QObject()
{
    int menuCount = 0;
    _masqueradeStarted = false;
    partnerOk = false;
    typesRead = false;
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

    configDialog = new PdaConfigDialogImpl(pdaName, raki, "ConfigDialog",
            false);
    configDialog->setCaption("Configuration for " + pdaName);

    syncDialog = new SyncDialogImpl(rra, pdaName, raki, "SynchronizeDialog",
            true);
    syncDialog->setCaption("Synchronize " + pdaName);

    associatedMenu = new KPopupMenu(raki, "PDA-Menu");

    associatedMenu->clear();
    associatedMenu->setCaption(pdaName);

    associatedMenu->insertTitle(SmallIcon("rapip"), pdaName);
    menuCount++;

    syncItem = associatedMenu->insertItem(SmallIcon("connect_established"),
            i18n("&Synchronize"), this, SLOT(synchronize()));
    associatedMenu->setItemEnabled(syncItem, false);
    menuCount++;

    associatedMenu->insertItem(SmallIcon("rotate_cw"),
            i18n("&Info && Management..."), this, SLOT(manage()));
    menuCount++;

    associatedMenu->insertSeparator(menuCount);
    menuCount++;

    associatedMenu->insertItem(SmallIcon("folder"), i18n("&Open rapip://") +
            QString(pdaName) + "/", this, SLOT(openFs()));
    menuCount++;

    pdaMirrorItem = associatedMenu->insertItem(SmallIcon("pda_blue"),
            i18n("Run &KCeMirror"), this, SLOT(startPdaMirror()));
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

    connect(syncDialog, SIGNAL(finished()), configDialog, SLOT(writeConfig()));
    connect(&pdaMirror, SIGNAL(processExited(KProcess* )), this, SLOT(pdaMirrorExited(KProcess* )));
}


PDA::~PDA()
{
    if (syncDialog->running()) {
        syncDialog->setDelayedDelete(true);
        syncDialog->setStopRequested(true);
    } else {
        delete syncDialog;
    }
    delete passwordDialog;
    delete runWindow;
    if (managerWindow->running()) {
        managerWindow->setDelayedDelete(true);
        managerWindow->WorkerThreadInterface::setStopRequested(true);
    } else {
        delete managerWindow;
    }
    delete configDialog;
    delete associatedMenu;
    delete rra;

    slaveDict.setAutoDelete(true);
}


void PDA::pdaMirrorExited(KProcess* )
{
    associatedMenu->setItemEnabled(pdaMirrorItem, true);
}


void PDA::startPdaMirror()
{
    pdaMirror.clearArguments();
    pdaMirror.setExecutable("kcemirror");
    pdaMirror << pdaName;

    associatedMenu->setItemEnabled(pdaMirrorItem, false);

    pdaMirror.start(KProcess::NotifyOnExit, (KProcess::Communication)
                   (KProcess::Stdout | KProcess::Stderr));
}


bool PDA::running()
{
    return (WorkerThreadInterface::running() || managerWindow->running() ||
            syncDialog->running());
}


void PDA::setStopRequested(bool isStopRequested)
{
    if (managerWindow->running()) {
        managerWindow->setStopRequested(isStopRequested);
    } else if (syncDialog->running()) {
        syncDialog->setStopRequested(isStopRequested);
    } else if (WorkerThreadInterface::running()) {
        WorkerThreadInterface::setStopRequested(isStopRequested);
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
        QPtrList<SyncTaskListItem>& syncItems =
                configDialog->getSyncTaskItemList();
        syncDialog->show(syncItems);
    }
}


void PDA::configurePda()
{
    configDialog->show();
}


void PDA::requestPassword()
{
    if (configDialog->getPassword().isEmpty()) {
        passwordDialog->exec();
    } else {
        emit resolvedPassword(pdaName, configDialog->getPassword());
    }
}


void PDA::setPassword(QString password)
{
    if (passwordDialog->rememberPasswordCheck->isChecked()) {
        configDialog->passwordEdit->setText(password);
        configDialog->applySlot();
        configDialog->writeConfig();
    }
    emit resolvedPassword(pdaName, password);
}


void PDA::passwordInvalid()
{
    configDialog->passwordEdit->setText("");
    configDialog->applySlot();
    configDialog->writeConfig();
}


bool PDA::isMasqueradeEnabled()
{
    return configDialog->getMasqueradeEnabled();
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


KURL::List PDA::getURLListByCopyJob(KIO::CopyJob *copyJob)
{
    KURL::List *list = slaveDict.find(copyJob);

    return *list;
}


unsigned int PDA::getNumberOfCopyJobs()
{
    return slaveDict.count();
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


QString PDA::getDeviceIp()
{
    return configDialog->getDeviceIp();
}


#define advanceInitProgress(a) \
    postThreadEvent(&PDA::advanceProgressEvent, a, noBlock);

#define advanceInitTotalSteps(a) \
    postThreadEvent(&PDA::advanceTotalStepsEvent, a, noBlock);


void *PDA::removePartnershipDialog(void *data)
{
    struct Rra::Partner * partner = (struct Rra::Partner *) data;
    int removedPartners;

    initProgress->hide();

    removedPartners = RemovePartnershipDialogImpl::showDialog(
            QString(partner[0].name), QString(partner[1].name) , 0,
            "Remove Partnership", true, 0);

    initProgress->show();

    return (void *) removedPartners;
}


void *PDA::alreadyTwoPartnershipsDialog(void *)
{
    initProgress->hide();
    KMessageBox::error((QWidget *) parent(),
                       "There are already two partnerships configured on the "
                       "device. Using guest", "Error configuring partnership");
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
    if (init) {
        configDialog->writeConfig();
    }

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

    advanceInitProgress(1);
    if (rra->getPartner(1, &partners[0])) {
        advanceInitProgress(1);
        if (rra->getPartner(2, &partners[1])) {
            int deletedItems = (int) postThreadEvent(
                    &PDA::removePartnershipDialog, partners, block);
            if (deletedItems > 0) {
                if (deletedItems != 0) {
                    struct Rra::Partner deletedPartner;
                    if (deletedItems & 1) {
                        deletedPartner.name = "";
                        deletedPartner.id = 0;
                        deletedPartner.index = 1;
                        advanceInitProgress(1);
                        if (!rra->setPartner(deletedPartner)) {
                            removePartnershipOk = false;
                        }
                    }
                    if ((deletedItems & 2) && removePartnershipOk) {
                        deletedPartner.name = "";
                        deletedPartner.id = 0;
                        deletedPartner.index = 2;
                        advanceInitProgress(1);
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

    advanceInitProgress(1);
    if (!rra->partnerCreate(& index)) {
        int removedPartnerships = 0;
        if (removePartnership(&removedPartnerships)) {
            if (removedPartnerships > 0) {
                setPartnerOk = setPartnershipThread();
            } else {
                postThreadEvent(&PDA::alreadyTwoPartnershipsDialog, NULL,
                        block);
                kdDebug(2120) << "Using Guest" << endl;
                partnerName = "";
                partnerId = 0;
                advanceInitProgress(7);
                if (rra->setCurrentPartner(0)) {
                    setPartnerOk = false;
                }
            }
        } else {
            setPartnerOk = false;
        }
    } else {
        struct Rra::Partner partner;
        advanceInitProgress(7);
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
        if (partnerId != configDialog->getPartnerId() && partnerId != 0) {
            configDialog->setNewPartner(partnerName, partnerId);
            kdDebug(2120) << "New Partnership" << endl;
        } else if (partnerId != 0) {
            configDialog->setPartner(partnerName, partnerId);
            kdDebug(2120) << "Already known Partnership" << endl;
        } else {
            configDialog->setPartner("Guest", 0);
            kdDebug(2120) << "Guest Partnership" << endl;
        }

        if (partnerOk) {
            postThreadEvent(&PDA::synchronizationTasks, 0, noBlock);
        }

        postThreadEvent(&PDA::progressDialogCancel, 1, noBlock);
    } else {
        postThreadEvent(&PDA::progressDialogCancel, 0, noBlock);
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

    kapp->processEvents();

    startWorkerThread(this, &PDA::setPartnership, NULL);

//    kdDebug(2120) << "Ende PDA::init()" << endl;
}


bool PDA::synchronizationTasks(void *)
{
    ObjectType *objectType;
    QDateTime lastSynchronized;
    bool ret = true;

    if (!typesRead) {
        typesRead = true;
        QPtrDict<ObjectType> types;
        if (getSynchronizationTypes(&types)) {
            QPtrDictIterator<ObjectType> it(types);
            for( ; it.current(); ++it ) {
                objectType = it.current();
                configDialog->addSyncTask(objectType, partnerId);
            }
        } else {
            ret = false;
        }
    }

    kdDebug(2120) << "Ende PDA::syncronizationTasks()" << endl;

    return ret;
}


void PDA::setMasqueradeStarted()
{
    _masqueradeStarted = true;
}


bool PDA::masqueradeStarted()
{
    return _masqueradeStarted;
}
