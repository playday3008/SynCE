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

#include <kdebug.h>
#include <qpushbutton.h>
#include <qtable.h>
#include <qptrdict.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

SyncDialogImpl::SyncDialogImpl(Rra *rra, QString& pdaName, QWidget* parent,
                               const char* name, bool modal,
                               WFlags fl)
        : SyncDialog(parent, name, modal, fl)
{
    this->pdaName = pdaName;
    this->rra = rra;
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
{}


void SyncDialogImpl::reject()
{
    RakiWorkerThread::rakiWorkerThread->stop();
    SyncDialog::reject();
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


void SyncDialogImpl::work(QThread */*qt*/, void */*data*/)
{
    SyncTaskListItem *item;

    buttonOk->setDisabled(true);

    for (item = syncItems.first(); item; item = syncItems.next()) {
        if (running()) {
            if (item->isOn()) {
                item->synchronize(this, rra);
            }
        }
    }

    buttonOk->setEnabled(true);
}
