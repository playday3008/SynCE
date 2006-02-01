//
// C++ Implementation: rakikpimsync
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
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

    setTotalSteps(100, true);
    PimSyncManager::self(pdaName)->setActualSyncType(type);
    PimSyncManager::self(pdaName)->startSync();

    return true;
}

bool RakiKPimSync::sync()
{
    // We don't sync in a thread - we do a synchronous sync.
    return true;
}


void RakiKPimSync::subscribeTo()
{
    kdDebug(2120) << "RRA_SYNCMGR_TYPE_CONTACT: " << rra->getTypeForName(RRA_SYNCMGR_TYPE_CONTACT) << endl;
    kdDebug(2120) << "RRA_SYNCMGR_TYPE_APPOINTMENT: " << rra->getTypeForName(RRA_SYNCMGR_TYPE_APPOINTMENT) << endl;
    kdDebug(2120) << "RRA_SYNCMGR_TYPE_TASK: " << rra->getTypeForName(RRA_SYNCMGR_TYPE_TASK) << endl;

    kdDebug(2120) << "Effective type: " << getObjectTypeId() << endl;
    if (rra->getTypeForName(RRA_SYNCMGR_TYPE_CONTACT) == getObjectTypeId()) {
        kdDebug(2120) << "Contacts ... " << endl;
        type = CONTACTS;
    } else if (rra->getTypeForName(RRA_SYNCMGR_TYPE_APPOINTMENT) == getObjectTypeId()) {
        kdDebug(2120) << "Events ... " << endl;
        type = EVENTS;
    } else if (rra->getTypeForName(RRA_SYNCMGR_TYPE_TASK) == getObjectTypeId()) {
        kdDebug(2120) << "Todos ... " << endl;
        type = TODOS;
    }
    PimSyncManager::self(pdaName)->subscribeTo(type);
}


void RakiKPimSync::unsubscribeFrom()
{
    getObjectTypeId();

    int type = 0;

    if (rra->getTypeForName(RRA_SYNCMGR_TYPE_CONTACT) == getObjectTypeId()) {
        kdDebug(2120) << "Contacts ... " << endl;
        type = CONTACTS;
    } else if (rra->getTypeForName(RRA_SYNCMGR_TYPE_APPOINTMENT) == getObjectTypeId()) {
        kdDebug(2120) << "Events ... " << endl;
        type = EVENTS;
    } else if (rra->getTypeForName(RRA_SYNCMGR_TYPE_TASK) == getObjectTypeId()) {
        kdDebug(2120) << "Todos ... " << endl;
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
    kdDebug( 2120 ) << "... loading connectors" << endl;
    PimSyncManager::self(pdaName ) ->loadKonnectors( ksConfig);
    RakiSyncPlugin::createConfigureObject( ksConfig );
}


void RakiKPimSync::init(Rra* rra,  SyncTaskListItem *item, QString pdaName, QWidget *parent,
                         QString serviceName )
{
    kdDebug( 2120 ) << "... init" << endl;
    RakiSyncPlugin::init(rra, item, pdaName, parent, serviceName );
    subscribeTo();
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
