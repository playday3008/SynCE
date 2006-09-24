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
#include <kapplication.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

SyncDialogImpl::SyncDialogImpl(Rra *rra, QString& pdaName, QWidget* parent,
        const char* name, bool modal, WFlags fl)
        : SyncDialog(parent, name, true, fl)
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
    end = true;
}


SyncDialogImpl::~SyncDialogImpl()
{
    if (this->isRunning()) {
//        this->setStopRequested(true);
    }
}


void SyncDialogImpl::reject(bool /* forced */)
{
    if (isRunning()) {
        running = false;
    } else {
        SyncDialog::reject();
    }
}


bool SyncDialogImpl::stopRequested()
{
    return !running;
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
    work(NULL, NULL);
}


void *SyncDialogImpl::finishedSynchronization(void */*nothing */)
{
    buttonOk->setEnabled(true);

    if (!isRunning()) {
        SyncDialog::reject();
    }

    running = false;
    end = true;

    emit finished();

    return NULL;
}


bool SyncDialogImpl::isRunning()
{
    return !end;
}


void SyncDialogImpl::work(QThread */*qt*/, void */*data*/)
{
    SyncTaskListItem *item;

    running = true;
    end = false;
    for (item = syncItems.first(); item; item = syncItems.next()) {
        if (running) {
            if (item->isOn()) {
                if (item->preSync()) {
                    item->synchronize(this);
                    if (!item->postSync()) {
                        item->setTaskLabel("Synchronization not possible");
                        item->setProgress(item->totalSteps());
                        item->setOn(false);
                        item->makePersistent();
                    }
                } else {
                    item->setTaskLabel("Synchronization not possible");
                    item->setProgress(item->totalSteps());
                    item->setOn(false);
                    item->makePersistent();
                }
            }
        }
    }

    finishedSynchronization(NULL);
}
