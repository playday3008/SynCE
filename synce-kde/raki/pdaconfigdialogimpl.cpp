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

#include "pdaconfigdialogimpl.h"
#include "synctasklistitem.h"
#include "removepartnershipdialogimpl.h"


#include <qcheckbox.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>
#include <klocale.h>
#include <kdebug.h>

#include <unistd.h>
#include <stdlib.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

PdaConfigDialogImpl::PdaConfigDialogImpl(QString pdaName, QWidget* parent,
        const char* name, bool modal, WFlags fl)
        : PdaConfigDialog(parent, name, modal, fl)
{
    ksConfig = new KConfig("raki/" + pdaName + ".cfg", false, false, "data");
    this->pdaName = pdaName;
    partnershipCreated.setTime_t(0);
    readConfig();
    updateFields();
    buttonCancel->setEnabled(false);
    objectTypeList->setFullWidth(true);
    syncTaskItemList.setAutoDelete(true);
}


PdaConfigDialogImpl::~PdaConfigDialogImpl()
{
    writeConfig();
    delete ksConfig;
}


void PdaConfigDialogImpl::updateFields()
{
    SyncTaskListItem *item;

    passwordEdit->setText(password);
    masqEnabledCheckbox->setChecked(masqEnabled);
    syncAtConnectCheckbox->setChecked(syncAtConnect);
    closeWhenCompletedCheckbox->setChecked(closeWhenCompleted);
    disconnectWhenCompletedCheckbox->setChecked(disconnectWhenCompleted);
    for (item = syncTaskItemList.first(); item; item = syncTaskItemList.next()) {
        item->undo();
    }

    buttonCancel->setDisabled(true);
}


QString PdaConfigDialogImpl::getPassword()
{
    return password;
}


bool PdaConfigDialogImpl::getMasqueradeEnabled()
{
    return masqEnabled;
}


bool PdaConfigDialogImpl::getSyncAtConnect()
{
    return syncAtConnect;
}


void PdaConfigDialogImpl::writeConfig()
{
    SyncTaskListItem *syncTaskListItem;

    ksConfig->setGroup("MainConfig");
    ksConfig->writeEntry("Password", password);
    ksConfig->writeEntry("Masquerade", masqEnabled);
    ksConfig->writeEntry("SyncAtConnect", syncAtConnect);
    ksConfig->writeEntry("CloseWhenCompleted", closeWhenCompleted);
    ksConfig->writeEntry("DisconnectWhenCompleted", disconnectWhenCompleted);
    ksConfig->writeEntry("PartnerName", partnerName);
    ksConfig->writeEntry("PartnerId", partnerId);
    ksConfig->writeEntry("PartnershipCreated", partnershipCreated);
    for (syncTaskListItem = syncTaskItemList.first(); syncTaskListItem;
            syncTaskListItem = syncTaskItemList.next()) {
        ksConfig->setGroup("Synchronizer-" + QString::number(
                syncTaskListItem->getObjectType()->id));
        ksConfig->writeEntry("Name", syncTaskListItem->getObjectType()->name2);
        ksConfig->writeEntry("Active", syncTaskListItem->isOn());
        ksConfig->writeEntry("PreferedLibrary",
                syncTaskListItem->getPreferedLibrary());
        ksConfig->writeEntry("PreferedOffer",
                syncTaskListItem->getPreferedOffer());
        ksConfig->writeEntry("LastSynchronized",
                syncTaskListItem->getLastSynchronized());
    }
    ksConfig->sync();
}


void PdaConfigDialogImpl::readConfig()
{
    if (ksConfig->hasGroup("MainConfig")) {
        ksConfig->setGroup("MainConfig");
        masqEnabled = ksConfig->readBoolEntry("Masquerade");
        password = ksConfig->readEntry("Password");
        syncAtConnect = ksConfig->readBoolEntry("SyncAtConnect");
        closeWhenCompleted = ksConfig->readBoolEntry("CloseWhenCompleted");
        disconnectWhenCompleted = ksConfig->readBoolEntry("DisconnectWhenCompleted");
        partnerName = ksConfig->readEntry("PartnerName");
        partnerId = ksConfig->readUnsignedLongNumEntry("PartnerId", 0);
        partnershipCreated = ksConfig->readDateTimeEntry("PartnershipCreated",
                &partnershipCreated);
        newPda = false;
    } else {
        masqEnabled = false;
        password = QString::null;
        syncAtConnect = false;
        partnerName = QString::null;
        partnerId = 0;
        partnershipCreated.setTime_t(0);
        newPda = true;
    }
}


bool PdaConfigDialogImpl::readConnectionFile()
{
    char *path = NULL;
    synce::synce_get_directory(&path);
    QString synceDir = QString(path);

    KSimpleConfig activeConnection(synceDir + "/" + pdaName, true);
    activeConnection.setGroup("device");
    deviceIp = activeConnection.readEntry("ip");
    osVersion = activeConnection.readUnsignedLongNumEntry("os_version");

    if (path)
        free(path);

    return true;
}


void PdaConfigDialogImpl::clearConfig()
{
    QStringList groups = ksConfig->groupList();

    for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it ) {
        ksConfig->deleteGroup(*it);
    }
    syncTaskItemList.clear();

    ksConfig->sync();
    readConfig();
}


void PdaConfigDialogImpl::applySlot()
{
    SyncTaskListItem *syncTaskListItem;

    if (buttonCancel->isEnabled()) {
        this->password = passwordEdit->text();
        masqEnabled = masqEnabledCheckbox->isChecked();
        syncAtConnect = syncAtConnectCheckbox->isChecked();
        closeWhenCompleted = closeWhenCompletedCheckbox->isChecked();
        disconnectWhenCompleted = disconnectWhenCompletedCheckbox->isChecked();
        for (syncTaskListItem = syncTaskItemList.first(); syncTaskListItem;
                syncTaskListItem = syncTaskItemList.next()) {
            syncTaskListItem->makePersistent();
        }
        writeConfig();
        buttonCancel->setDisabled(true);
    }
}


void PdaConfigDialogImpl::masqChangedSlot()
{
    buttonCancel->setEnabled(true);
}


void PdaConfigDialogImpl::disableApply()
{
    updateFields();
}


void PdaConfigDialogImpl::changedSlot()
{
    buttonCancel->setEnabled(true);
}


QString PdaConfigDialogImpl::getDeviceIp()
{
    return deviceIp;
}


int PdaConfigDialogImpl::getOsVersion()
{
    return osVersion;
}


void PdaConfigDialogImpl::objectTypeList_rightButtonClicked(
        QListViewItem *item, const QPoint &, int )
{
    if (item != NULL) {
        ((SyncTaskListItem *) item)->openPopup();
    }
}


void PdaConfigDialogImpl::setConfigButton()
{
    SyncTaskListItem *item = (SyncTaskListItem *) objectTypeList->selectedItem();
    if (item != 0) {
        kPushButton2->setEnabled(((SyncTaskListItem *)item)->QCheckListItem::isOn());
    } else {
        kPushButton2->setEnabled(false);
    }
}


bool PdaConfigDialogImpl::isNewPda()
{
    return newPda;
}


void PdaConfigDialogImpl::setPartner(QString partnerName, uint32_t partnerId)
{
    this->partnerName = partnerName;
    this->partnerId = partnerId;
}


void PdaConfigDialogImpl::setNewPartner(QString partnerName,
        uint32_t partnerId)
{
    setPartner(partnerName, partnerId);
    partnershipCreated = QDateTime(QDate::currentDate(), QTime::currentTime());
    kdDebug(2120) << i18n("Partnership created:") << " " << partnershipCreated.toString() << endl;
}


QString PdaConfigDialogImpl::getPartnerName()
{
    return partnerName;
}


uint32_t PdaConfigDialogImpl::getPartnerId()
{
    return partnerId;
}


void PdaConfigDialogImpl::addSyncTask(Rra *rra, uint32_t objectType,
        uint32_t partnerId)
{
    SyncTaskListItem *item;
    QDateTime lastSynchronized;

    item = new SyncTaskListItem(rra, pdaName, objectType, objectTypeList, partnerId);
    ksConfig->setGroup("Synchronizer-" + QString::number(
            item->getObjectType()->id));
    item->setPreferedLibrary(ksConfig->readEntry("PreferedLibrary"));
    item->setPreferedOffer(ksConfig->readEntry("PreferedOffer"));
    lastSynchronized = item->getLastSynchronized();
    item->setLastSynchronized(ksConfig->readDateTimeEntry("LastSynchronized",
            &lastSynchronized));
    item->setFirstSynchronization((
            partnershipCreated > item->getLastSynchronized()));
    item->setOn(ksConfig->readBoolEntry("Active"));
    connect((const QObject *) item, SIGNAL(stateChanged(bool)), this,
            SLOT(changedSlot()));
    connect((const QObject *) item, SIGNAL(stateChanged(bool)), this,
            SLOT(setConfigButton()));
    connect((const QObject *) item, SIGNAL(serviceChanged()), this,
            SLOT(changedSlot()));
    objectTypeList->insertItem(item);
    syncTaskItemList.append(item);
    item->makePersistent();
}


QPtrList<SyncTaskListItem>& PdaConfigDialogImpl::getSyncTaskItemList()
{
    return syncTaskItemList;
}


void PdaConfigDialogImpl::kPushButton2_clicked()
{
    SyncTaskListItem *item = (SyncTaskListItem *) objectTypeList->currentItem();

    if (item != NULL) {
        item->configure();
    }
}


void PdaConfigDialogImpl::show()
{
    setConfigButton();
    PdaConfigDialog::show();
}


bool PdaConfigDialogImpl::getCloseWhenCompleted()
{
    return closeWhenCompleted;
}


bool PdaConfigDialogImpl::getDisconnectWhenCompleted()
{
    return disconnectWhenCompleted;
}
