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

#include "synctasklistitem.h"
#include "syncthread.h"
#include "rakisyncfactory.h"
#include "rakisyncplugin.h"
#include "rra.h"

#include <kprogress.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <qcursor.h>
#include <qpoint.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

SyncTaskListItem::SyncTaskListItem(QString pdaName, RRA_SyncMgrType *objectType,
                                   QListView* listView, uint32_t partnerId)
        : QCheckListItem(listView, objectType->name2, QCheckListItem::CheckBox)
{
    this->objectType = objectType;
    this->partnerId = partnerId;
    this->syncPlugin = NULL;
    this->pdaName = pdaName;
    this->lastSynchronized.setTime_t(0);
    this->isOnStore = false;

    connect(&itemMenu, SIGNAL(activated(int)), this, SLOT(clickedMenu(int)));
}


SyncTaskListItem::~SyncTaskListItem()
{
    if (syncPlugin != NULL) {
        delete syncPlugin;
    }
}


void SyncTaskListItem::stateChange(bool state)
{
    createSyncPlugin(state);
    emit stateChanged(state);
}


void SyncTaskListItem::undo()
{
    if (preferedOffer != preferedOfferTemp ||
            preferedLibrary != preferedLibraryTemp ||
            isOnStore != QCheckListItem::isOn()) {
        QCheckListItem::setOn(isOnStore);
        preferedOfferTemp = preferedOffer;
        preferedLibraryTemp = preferedLibrary;
    }
}


void SyncTaskListItem::makePersistent()
{
    if (preferedOffer != preferedOfferTemp ||
            preferedLibrary != preferedLibraryTemp ||
            isOnStore != QCheckListItem::isOn()) {
        isOnStore = QCheckListItem::isOn();
        preferedOffer = preferedOfferTemp;
        preferedLibrary = preferedLibraryTemp;
    }
}


bool SyncTaskListItem::isOn()
{
    return isOnStore;
}


void SyncTaskListItem::setOn(bool state)
{
    QCheckListItem::setOn(state);
}


void SyncTaskListItem::setLastSynchronized(QDateTime lastSynchronized)
{
    this->lastSynchronized = lastSynchronized;
}


QDateTime & SyncTaskListItem::getLastSynchronized()
{
    return lastSynchronized;
}


void SyncTaskListItem::setFirstSynchronization(bool firstSynchronization)
{
    this->firstSynchronization = firstSynchronization;
}


bool SyncTaskListItem::isFirstSynchronization()
{
    return firstSynchronization;
}


RRA_SyncMgrType* SyncTaskListItem::getObjectType()
{
    return objectType;
}


QString SyncTaskListItem::getObjectTypeName()
{
    return objectType->name2;
}


void SyncTaskListItem::setObjectType(RRA_SyncMgrType *objectType)
{
    this->objectType = objectType;
}


void SyncTaskListItem::setTotalSteps(int totalSteps)
{
    progress->setTotalSteps(totalSteps);
}


void SyncTaskListItem::setProgress(int value)
{
    progress->setProgress(value);
}


void SyncTaskListItem::advance(int offset)
{
    progress->advance(offset);
}


int SyncTaskListItem::totalSteps()
{
    return progress->totalSteps();
}


void SyncTaskListItem::setTaskLabel(QString task)
{
    taskLabelWidget->setText(task);
}


QWidget *SyncTaskListItem::widget()
{
    progress = new KProgress();
    return progress;
}


QWidget *SyncTaskListItem::taskLabel()
{
    taskLabelWidget = new QLabel(0, "TaskLabel");
    taskLabelWidget->setText("Waiting...");

    return taskLabelWidget;
}


KTrader::OfferList SyncTaskListItem::getOffers()
{
    return KTrader::self()->query("Raki/Synchronizer",
                                  "[X-Raki-Synchronizer] == '" + text() + "'");
}

QString SyncTaskListItem::getPreferedOffer()
{
    return preferedOffer;
}


QString SyncTaskListItem::getPreferedLibrary()
{
    return preferedLibrary;
}


void SyncTaskListItem::setPreferedOffer(QString preferedOffer)
{
    this->preferedOffer = preferedOffer;
    this->preferedOfferTemp = preferedOffer;
}


void SyncTaskListItem::setPreferedLibrary(QString preferedLibrary)
{
    this->preferedLibrary = preferedLibrary;
    this->preferedLibraryTemp = preferedLibrary;
}


void SyncTaskListItem::clickedMenu(int item)
{
    KTrader::OfferList::Iterator it;

    if (offers.begin() != offers.end()) {
        for (it = offers.begin(); it != offers.end(); ++it) {
            KService::Ptr service = *it;
            kdDebug(2120) << "Select Name: " << service->name() + "; Library: " <<
                    service->library() << endl;
            if (service->name() == itemMenu.text(item)) {
                if (preferedOffer != service->name() ||
                        preferedLibrary != service->library()) {
                    preferedOfferTemp = service->name();
                    preferedLibraryTemp = service->library();
                    itemMenu.setItemChecked(item, true);
                }
            } else {
                itemMenu.setItemChecked(item, false);
            }
        }
    }
    
    createSyncPlugin(QCheckListItem::isOn());
    emit serviceChanged();
}


void SyncTaskListItem::openPopup()
{
    bool preferedFound = false;
    offers = getOffers();
    KTrader::OfferList::ConstIterator it;

    itemMenu.clear();
    itemMenu.setCaption("Services for " + text());

    itemMenu.insertTitle("Services for " + text());
    itemMenu.setCheckable(true);

    itemMenu.setEnabled(true);

    if (offers.begin() != offers.end()) {
        for (it = offers.begin(); it != offers.end(); ++it) {
            KService::Ptr service = *it;
            kdDebug(2120) << "Open Name: " << service->name() + "; Library: " <<
                    service->library() << endl;
            int item = itemMenu.insertItem(service->name());
            if (service->name() == preferedOfferTemp) {
                itemMenu.setItemChecked(item, true);
                preferedFound = true;
            }
        }

        if (!preferedFound) {
            preferedOffer = "";
            preferedLibrary = "";
        }

        itemMenu.move(-1000,-1000);
        itemMenu.show();
        itemMenu.hide();

        QPoint g = QCursor::pos();

        if (itemMenu.height() < g.y()) {
            itemMenu.popup(QPoint( g.x(), g.y() - itemMenu.height()));
        } else {
            itemMenu.popup(QPoint(g.x(), g.y()));
        }
    }
}


int SyncTaskListItem::createSyncPlugin(bool state)
{
    int ret = 0;

    if (syncPlugin != NULL) {
        delete syncPlugin;
        syncPlugin = NULL;
    }

    if (/*isOn()*/ state) {
        KTrader::OfferList offers;

        QString library = getPreferedLibrary();
        QString offer = getPreferedOffer();

        if (library.isEmpty()) {
            offers = getOffers();
            if (offers.begin() != offers.end()) {
                KService::Ptr service = *offers.begin();
                library = service->library();
                offer = service->name();
            }
        }

        if (!library.isEmpty()) {
            kdDebug(2120) << "Name: " << offer + "; Library: " << library << endl;
            KLibFactory *factory = KLibLoader::self()->factory(library.ascii());
            if (!factory) {
                QString errorMessage = KLibLoader::self()->lastErrorMessage();
                kdDebug(2120) << "There was an error: " << offer << errorMessage <<
                    endl;
                ret = ERROR_NOFACTORY;
            } else {
                if (factory->inherits("RakiSyncFactory")) {
                    RakiSyncFactory *syncFactory =
                        static_cast<RakiSyncFactory*> (factory);
                    syncPlugin = static_cast<RakiSyncPlugin*>
                                 (syncFactory->create());
                    syncPlugin->init(objectType, pdaName, this->listView(), offer);
                    syncFactory->callme(); // Fake call to link correct.
                } else {
                    kdDebug(2120) << "Library no Raki-Plugin" << endl;
                    ret = ERROR_WRONGLIBRARYTYPE;
                }
            }
        } else {
            ret = ERROR_NOSYNCHRONIZER;
        }
    }

    switch(ret) {
    case ERROR_NOSYNCHRONIZER:
        KMessageBox::information(this->listView(), "<p>No Synchronizer found for <b>" +
                QString(objectType->name2) + "</b></p>", QString(objectType->name2) + pdaName);
        this->setOn(false);
        this->makePersistent();
        break;
    case ERROR_WRONGLIBRARYTYPE:
        KMessageBox::error(this->listView(), "<p>Wrong library type for <b>" +
                QString(objectType->name2) + "</b></p>", QString(objectType->name2) + pdaName);
        this->setOn(false);
        this->makePersistent();
        break;
    case ERROR_NOFACTORY:
        KMessageBox::error(this->listView(), "<p>Wrong library type for <b>" +
                QString(objectType->name2) + "</b></p>", QString(objectType->name2) + pdaName);
        this->setOn(false);
        this->makePersistent();
        break;
    }

    return ret;
}


bool SyncTaskListItem::synchronize(SyncThread *syncThread, Rra *rra)
{
    bool ret = false;

    postSyncThreadEvent(SyncThread::setTask, (void *) qstrdup("Started"));
    postSyncThreadEvent(SyncThread::setTotalSteps, (void *) 1);

    if (syncPlugin != NULL) {
        kdDebug(2120) << "Started syncing with " << syncPlugin->serviceName() << endl;

        ret = syncPlugin->doSync(syncThread, rra, this, firstSynchronization,
                partnerId);

        kdDebug(2120) << "Finished syncing with " << syncPlugin->serviceName() << endl;
        postSyncThreadEvent(SyncThread::setProgress, totalSteps());

        if (ret) {
            lastSynchronized = QDateTime(QDate::currentDate(),
                    QTime::currentTime());
            firstSynchronization = false;
            postSyncThreadEvent(SyncThread::setTask,
                    (void *) qstrdup("Finished"));
        } else {
            postSyncThreadEvent(SyncThread::setTask, (void *)
                    qstrdup("Error during synchronization"));
        }
    }

    syncThread->synchronizeGui();

    return ret;
}


bool SyncTaskListItem::preSync(SyncThread *syncThread, Rra *rra)
{
    bool ret;
    
    ret = syncPlugin->preSync(syncThread, rra, firstSynchronization, partnerId);
    
    return ret;
}


bool SyncTaskListItem::postSync(SyncThread *syncThread, Rra *rra)
{
    bool ret;
    
    ret = syncPlugin->postSync(syncThread, rra, firstSynchronization, partnerId);
    
    return ret;
}


void SyncTaskListItem::configure()
{
    if (syncPlugin != NULL) {
        syncPlugin->configure();
    }
}
