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

#include <librra.h>

#include <qcheckbox.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>
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
    buttonApply->setEnabled(false);
    objectTypeList->setFullWidth(true);
    syncTaskItemList.setAutoDelete(true);
    syncAtConnectCheckbox->setEnabled(false);
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
    for (item = syncTaskItemList.first();
            item; item = syncTaskItemList.next()) {
        item->undo();
    }

    buttonApply->setDisabled(true);
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
    ksConfig->writeEntry("PartnerName", partnerName);
    ksConfig->writeEntry("PartnerId", partnerId);
    ksConfig->writeEntry("PartnershipCreated", partnershipCreated);
    for (syncTaskListItem = syncTaskItemList.first(); syncTaskListItem;
            syncTaskListItem = syncTaskItemList.next()) {
        ksConfig->setGroup("Synchronizer-" + QString::number(
                syncTaskListItem->getObjectType()->id));
        ksConfig->writeEntry("Name", syncTaskListItem->getObjectType()->name);
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
        partnerName = ksConfig->readEntry("PartnerName", "");
        partnerId = ksConfig->readUnsignedLongNumEntry("PartnerId", 0);
        partnershipCreated = ksConfig->readDateTimeEntry("PartnershipCreated",
                &partnershipCreated);
        newPda = false;
    } else {
        masqEnabled = false;
        password = "";
        syncAtConnect = false;
        newPda = true;
    }
}


void PdaConfigDialogImpl::applySlot()
{
    SyncTaskListItem *syncTaskListItem;

    if (buttonApply->isEnabled()) {
        this->password = passwordEdit->text();
        masqEnabled = masqEnabledCheckbox->isChecked();
        syncAtConnect = syncAtConnectCheckbox->isChecked();
        for (syncTaskListItem = syncTaskItemList.first(); syncTaskListItem;
                syncTaskListItem = syncTaskItemList.next()) {
            syncTaskListItem->makePersistent();
        }
        writeConfig();
        buttonApply->setDisabled(true);
    }
}


void PdaConfigDialogImpl::masqChangedSlot()
{
    buttonApply->setEnabled(true);
}


void PdaConfigDialogImpl::disableApply()
{
    updateFields();
}


void PdaConfigDialogImpl::changedSlot()
{
    buttonApply->setEnabled(true);
}


QString PdaConfigDialogImpl::getDeviceIp()
{
    char *path = NULL;
    synce::synce_get_directory(&path);
    QString synceDir = QString(path);

    if (path)
        free(path);

    KSimpleConfig activeConnection(synceDir + "/" + pdaName, true);
    activeConnection.setGroup("device");
    QString deviceIp = activeConnection.readEntry("ip");

    return deviceIp;
}


void PdaConfigDialogImpl::objectTypeList_rightButtonClicked(
        QListViewItem *item, const QPoint &, int )
{
    ((SyncTaskListItem *) item)->openPopup();
}


bool PdaConfigDialogImpl::isNewPda()
{
    return newPda;
}


void PdaConfigDialogImpl::setPartner(QString partnerName, uint32_t partnerId)
{
    this->partnerName = partnerName;
    this->partnerId = partnerId;

    if (partnerId != 0) {
        syncAtConnectCheckbox->setEnabled(true);
    }
}


void PdaConfigDialogImpl::setNewPartner(QString partnerName,
        uint32_t partnerId)
{
    setPartner(partnerName, partnerId);
    partnershipCreated = QDateTime(QDate::currentDate(), QTime::currentTime());
    kdDebug(2120) << "Partnership created: " << partnershipCreated.toString();
}


QString PdaConfigDialogImpl::getPartnerName()
{
    return partnerName;
}


uint32_t PdaConfigDialogImpl::getPartnerId()
{
    return partnerId;
}


void PdaConfigDialogImpl::addSyncTask(ObjectType *objectType,
        uint32_t partnerId)
{
    SyncTaskListItem *item;
    QDateTime lastSynchronized;

    item = new SyncTaskListItem(pdaName, objectType, objectTypeList, partnerId);
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
