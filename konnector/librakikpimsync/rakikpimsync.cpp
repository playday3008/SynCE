/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                    Christian Fremgen <cfremgen@users.sourceforge.net>   *
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

#include "rakikpimsync.h"
#include "pimsyncmanager.h"
#include <libkdepim/progressmanager.h>
#include "paireditordialog.h"
#include <kitchensync/multisynk/konnectorpair.h>
#include "syncekonnectorbase.h"


int RakiKPimSync::refcount = 0;

RakiKPimSync::RakiKPimSync() : RakiSyncPlugin()
{
    refcount++;
}


RakiKPimSync::~RakiKPimSync()
{
    refcount--;
    if (!refcount) {
        delete PimSyncManager::self(pdaName );
    }
}


bool RakiKPimSync::postSync( QWidget* parent, bool firstSynchronize, uint32_t partnerId )
{
    RakiSyncPlugin::postSync( parent, firstSynchronize, partnerId );

    KPIM::ProgressManager *pm = KPIM::ProgressManager::instance();
    disconnect ( pm, SIGNAL( progressItemAdded( KPIM::ProgressItem* ) ),
              this, SLOT( progressItemAdded( KPIM::ProgressItem* ) ) );
    disconnect ( pm, SIGNAL( progressItemStatus( KPIM::ProgressItem*, const QString& ) ),
              this, SLOT( progressItemStatus( KPIM::ProgressItem*, const QString& ) ) );
    disconnect ( pm, SIGNAL( progressItemProgress( KPIM::ProgressItem*, unsigned int ) ),
              this, SLOT( progressItemProgress( KPIM::ProgressItem*, unsigned int ) ) );

    return true;
}

bool RakiKPimSync::preSync( QWidget* parent, bool firstSynchronize, uint32_t partnerId )
{
    RakiSyncPlugin::preSync( parent, firstSynchronize, partnerId );

    KPIM::ProgressManager *pm = KPIM::ProgressManager::instance();
    connect ( pm, SIGNAL( progressItemAdded( KPIM::ProgressItem* ) ),
              this, SLOT( progressItemAdded( KPIM::ProgressItem* ) ) );
    connect ( pm, SIGNAL( progressItemStatus( KPIM::ProgressItem*, const QString& ) ),
              this, SLOT( progressItemStatus( KPIM::ProgressItem*, const QString& ) ) );
    connect ( pm, SIGNAL( progressItemProgress( KPIM::ProgressItem*, unsigned int ) ),
              this, SLOT( progressItemProgress( KPIM::ProgressItem*, unsigned int ) ) );

    return true;
}

bool RakiKPimSync::sync()
{
    setTotalSteps(100, true);
    PimSyncManager::self(pdaName)->setActualSyncType(type);
    PimSyncManager::self(pdaName)->startSync();
    return true;
}

int RakiKPimSync::syncContext()
{
    return RakiSyncPlugin::SYNCHRONOUS;
}

void RakiKPimSync::subscribeTo(Rra* rra)
{
    if (rra->getTypeForName(RRA_SYNCMGR_TYPE_CONTACT) == getObjectTypeId()) {
        kdDebug(2120) << "Subscribing Contacts ... " << endl;
        type = CONTACTS;
    } else if (rra->getTypeForName(RRA_SYNCMGR_TYPE_APPOINTMENT) == getObjectTypeId()) {
        kdDebug(2120) << "Subscribing Events ... " << endl;
        type = EVENTS;
    } else if (rra->getTypeForName(RRA_SYNCMGR_TYPE_TASK) == getObjectTypeId()) {
        kdDebug(2120) << "Subscribing Todos ... " << endl;
        type = TODOS;
    }
    PimSyncManager::self(pdaName)->subscribeTo(rra, type);
}


void RakiKPimSync::unsubscribeFrom()
{
    getObjectTypeId();

    int type = 0;

    if (rra->getTypeForName(RRA_SYNCMGR_TYPE_CONTACT) == getObjectTypeId()) {
        kdDebug(2120) << "Unsunscribing Contacts ... " << endl;
        type = CONTACTS;
    } else if (rra->getTypeForName(RRA_SYNCMGR_TYPE_APPOINTMENT) == getObjectTypeId()) {
        kdDebug(2120) << "Unsunscribing Events ... " << endl;
        type = EVENTS;
    } else if (rra->getTypeForName(RRA_SYNCMGR_TYPE_TASK) == getObjectTypeId()) {
        kdDebug(2120) << "Unsunscribing Todos ... " << endl;
        type = TODOS;
    }

    PimSyncManager::self(pdaName)->unsubscribeFrom(type);
}


void RakiKPimSync::configure()
{
    PimSyncManager::self(pdaName)->configure(parent, ksConfig);
}


void RakiKPimSync::createConfigureObject( KConfig* ksConfig )
{
    PimSyncManager::self(pdaName )->loadKonnectors(ksConfig);
    RakiSyncPlugin::createConfigureObject( ksConfig );
}


void RakiKPimSync::init(Rra* rra,  SyncTaskListItem *item, QString pdaName, QWidget *parent,
                         QString serviceName )
{
    kdDebug( 2120 ) << "... init" << endl;
    RakiSyncPlugin::init(rra, item, pdaName, parent, serviceName );
    subscribeTo(rra);
}


void RakiKPimSync::unInit()
{
    kdDebug(2120) << "Uninit" << endl;
    unsubscribeFrom();
}


void RakiKPimSync::progressItemAdded( KPIM::ProgressItem *item )
{
    setTask(item->status().ascii(), true);
}


void RakiKPimSync::progressItemStatus( KPIM::ProgressItem*, const QString &statusMsg )
{
    setTask(statusMsg.ascii(), true);
}


void RakiKPimSync::progressItemProgress( KPIM::ProgressItem*, unsigned int progress )
{
    setProgress(progress, true);
}


#include "rakikpimsync.moc"
