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

#include "syncdialogimpl.h"
#include "synctasklistitem.h"
#include "rakiworkerthread.h"
#include "pdaconfigdialogimpl.h"

#include <kdebug.h>
#include <klocale.h>
#include <qpushbutton.h>
#include <qtable.h>
#include <qptrdict.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

SyncDialogImpl::SyncDialogImpl(Rra *rra, QString& pdaName, QWidget* parent,
        const char* name, bool modal, WFlags fl)
        : SyncDialog(parent, name, modal, fl)
{
    this->pdaName = pdaName;
    this->rra = rra;
    sdtd = new SyncDialogImplThreadData(this);
    objectTypesTable->setLeftMargin(0);
    objectTypesTable->setShowGrid(false);
    objectTypesTable->setReadOnly(true);
    objectTypesTable->setSelectionMode(QTable::NoSelection);
    objectTypesTable->setColumnStretchable(1, true);
    objectTypesTable->setNumRows(0);
    QHeader *header = objectTypesTable->horizontalHeader();
    header->setResizeEnabled(false);
    header->adjustHeaderSize();
}


SyncDialogImpl::~SyncDialogImpl()
{
    if (this->running()) {
        this->setStopRequested(true);
    }
    delete sdtd;
}


void SyncDialogImpl::reject()
{
    if (this->running()) {
        this->setStopRequested(true);
    } else {
        SyncDialog::reject();
    }
}


void SyncDialogImpl::show(QPtrList<SyncTaskListItem>& syncItems)
{
    SyncTaskListItem *item;

    int count = 0;
    this->syncItems = syncItems;

    for (int i = 0; i < objectTypesTable->numRows(); i++) {
        objectTypesTable->clearCellWidget(i, 1);
    }

    for (item = syncItems.first(); item; item = syncItems.next()) {
        if (item->isOn()) {
            count++;
        }
    }

    objectTypesTable->setNumRows(count);

    int j = 0;
    for (item = syncItems.first(); item; item = syncItems.next()) {
        if (item->isOn()) {
            objectTypesTable->setText(j, 0, item->getObjectTypeName());
            objectTypesTable->setCellWidget(j, 2, item->widget());
            objectTypesTable->setCellWidget(j, 1, item->taskLabel());
            item->setTotalSteps(1);
            item->setProgress(0);
            j++;
        }
    }
    SyncDialog::show();
    startWorkerThread(this, &SyncDialogImpl::work, NULL);
}


void *SyncDialogImpl::finishedSynchronization(void */*nothing */)
{
    buttonOk->setEnabled(true);

    if (this->stopRequested()) {
        SyncDialog::reject();
    }

    emit finished();

    return NULL;
}


struct SyncDialogImplDataExchange {
    SyncTaskListItem *item;
    bool success;
};



void *SyncDialogImpl::preSync(void *v_dataExchange)
{
    SyncDialogImplDataExchange *dataExchange = (SyncDialogImplDataExchange *) v_dataExchange;

    if (!dataExchange->item->preSync(this)) {
        dataExchange->success = false;
    }

    return NULL;
}


void *SyncDialogImpl::postSync(void *v_dataExchange)
{
    SyncDialogImplDataExchange *dataExchange = (SyncDialogImplDataExchange *) v_dataExchange;

    if (!dataExchange->item->postSync(this)) {
        dataExchange->success = false;
    }

    return NULL;
}


void SyncDialogImpl::work(QThread */*qt*/, void */*data*/)
{
    SyncDialogImplDataExchange dataExchange;

    buttonOk->setDisabled(true);

    for (dataExchange.item = syncItems.first(); dataExchange.item; dataExchange.item = syncItems.next()) {
        if (running()) {
            if (dataExchange.item->isOn()) {
                dataExchange.success = true;
                setActualSyncItem(dataExchange.item);
                postSyncDialogImplEvent(&SyncDialogImpl::preSync, (void *) &dataExchange, block);
                if (dataExchange.success) {
//                    postSyncDialogImplEvent(&SyncDialogImpl::synSync, (void *) &dataExchange, block);
                    dataExchange.item->synchronize(this);
                    postSyncDialogImplEvent(&SyncDialogImpl::postSync, (void *) &dataExchange, block);
                    if (!dataExchange.success) {
                        dataExchange.item->setTaskLabel("Synchronization not possible");
                        dataExchange.item->setProgress(dataExchange.item->totalSteps());
                        dataExchange.item->setOn(false);
                        dataExchange.item->makePersistent();
                    }
                } else {
                    dataExchange.item->setTaskLabel("Synchronization not possible");
                    dataExchange.item->setProgress(dataExchange.item->totalSteps());
                    dataExchange.item->setOn(false);
                    dataExchange.item->makePersistent();
                }
            }
        }
    }

    postSyncDialogImplEvent(&SyncDialogImpl::finishedSynchronization, NULL, block);
}
