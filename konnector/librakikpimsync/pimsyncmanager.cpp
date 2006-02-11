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

#include "pimsyncmanager.h"

#include <synceengine.h>
#include <kitchensync/multisynk/konnectorpair.h>
#include "syncekonnectorbase.h"
#include "konnectormanager.h"
#include "paireditordialog.h"
#include <kconfig.h>

#include <klocale.h>

QMap<QString, PimSyncManager *> PimSyncManager::pimSyncMap;

PimSyncManager::PimSyncManager(QString pdaName ) : konnectorsLoaded( false ), pair(NULL), pdaName( pdaName )
{}


PimSyncManager::~PimSyncManager()
{
    if ( konnectorsLoaded ) {
        delete mEngine;
        mEngine = 0;
    }

    if (pair) {
        delete pair;
    }

    pimSyncMap.erase( pdaName );
}


PimSyncManager *PimSyncManager::self(QString pdaName )
{
    if ( !pimSyncMap[ pdaName ] ) {
        pimSyncMap[ pdaName ] = new PimSyncManager(pdaName );
    }

    return pimSyncMap[ pdaName ];
}


void PimSyncManager::setActualSyncType(int type)
{
    if (pair) {
        KonnectorManager * pmanager = pair->manager();
        KonnectorManager::Iterator kit;
        for ( kit = pmanager->begin(); kit != pmanager->end(); ++kit ) {
            KSync::SynCEKonnectorBase *k = dynamic_cast<KSync::SynCEKonnectorBase *>( *kit );
            if ( k ) {
                k->actualSyncType( type );
            }
        }
    }
}


void PimSyncManager::subscribeTo(Rra* rra, int type )
{
    if (pair) {
        KonnectorManager * pmanager = pair->manager();
        KonnectorManager::Iterator kit;
        for ( kit = pmanager->begin(); kit != pmanager->end(); ++kit ) {
            KSync::SynCEKonnectorBase *k = dynamic_cast<KSync::SynCEKonnectorBase *>( *kit );
            if ( k ) {
                k->subscribeTo(rra, type);
            }
        }
        pair->save();
    }
}


void PimSyncManager::unsubscribeFrom( int type )
{
    if (pair) {
        KonnectorManager * pmanager = pair->manager();
        KonnectorManager::Iterator kit;
        for ( kit = pmanager->begin(); kit != pmanager->end(); ++kit ) {
            KSync::SynCEKonnectorBase *k = dynamic_cast<KSync::SynCEKonnectorBase *>( *kit );
            if ( k ) {
                k->unsubscribeFrom( type );
            }
        }
        pair->save();
    }
}


bool PimSyncManager::loadKonnectors( KConfig* ksConfig)
{
    if ( !konnectorsLoaded ) {
        ksConfig->setGroup("Pim Synchronizer");
        QString pairUid = ksConfig->readEntry( "PairUid", "---" );
        pair = new KonnectorPair();
        if ( pairUid != "---" ) {
            pair->setUid( pairUid );
            pair->load();
        } else {
            PairEditorDialog pairEditorDialog(0, "PairEditorDialog", pdaName);
            pairEditorDialog.setPair(pair);
            pair->load();
        }
        KonnectorManager * pmanager = pair->manager();
        KonnectorManager::Iterator kit;
        for ( kit = pmanager->begin(); kit != pmanager->end(); ++kit ) {
            KSync::SynCEKonnectorBase *k = dynamic_cast<KSync::SynCEKonnectorBase *>( *kit );
            if ( k ) {
                k->init(pair->uid());
            }
        }
        mEngine = new KSync::SynCEEngine();

        konnectorsLoaded = true;
    }

    return true;
}


void PimSyncManager::startSync()
{
    connect( pair->manager(), SIGNAL( synceesRead( KSync::Konnector* ) ),
             mEngine, SLOT( slotSynceesRead( KSync::Konnector* ) ) );
    connect( pair->manager(), SIGNAL( synceeReadError( KSync::Konnector* ) ),
             mEngine, SLOT( slotSynceeReadError( KSync::Konnector* ) ) );
    connect( pair->manager(), SIGNAL( synceesWritten( KSync::Konnector* ) ),
             mEngine, SLOT( slotSynceesWritten( KSync::Konnector* ) ) );
    connect( pair->manager(), SIGNAL( synceeWriteError( KSync::Konnector* ) ),
             mEngine, SLOT( slotSynceeWriteError( KSync::Konnector* ) ) );
    connect( mEngine, SIGNAL( doneSync() ),
             this, SLOT( syncDone() ) );

    mEngine->go( pair );
}


void PimSyncManager::syncDone()
{
    disconnect( pair->manager(), SIGNAL( synceesRead( KSync::Konnector* ) ),
                mEngine, SLOT( slotSynceesRead( KSync::Konnector* ) ) );
    disconnect( pair->manager(), SIGNAL( synceeReadError( KSync::Konnector* ) ),
                mEngine, SLOT( slotSynceeReadError( KSync::Konnector* ) ) );
    disconnect( pair->manager(), SIGNAL( synceesWritten( KSync::Konnector* ) ),
                mEngine, SLOT( slotSynceesWritten( KSync::Konnector* ) ) );
    disconnect( pair->manager(), SIGNAL( synceeWriteError( KSync::Konnector* ) ),
                mEngine, SLOT( slotSynceeWriteError( KSync::Konnector* ) ) );
    disconnect( mEngine, SIGNAL( doneSync() ),
                this, SLOT( syncDone() ) );

    pair->save();
}


void PimSyncManager::configure(QWidget *parent, KConfig* ksConfig)
{
    PairEditorDialog pairEditorDialog(parent, "PairEditorDialog", pdaName);

    KonnectorPair *tmpPair;

    if (pair) {
        tmpPair = pair;
    } else {
        tmpPair = new KonnectorPair();
    }

    pairEditorDialog.setPair( tmpPair );

    kdDebug(2120) << "PairEditorDialog exec" << endl;
    if (pairEditorDialog.exec()) {
        ksConfig->setGroup("Pim Synchronizer");
        tmpPair = pairEditorDialog.pair();
        ksConfig->writeEntry( "PairUid", tmpPair->uid());
        ksConfig->sync();
        pair = tmpPair;
        KonnectorManager * pmanager = pair->manager();
        KonnectorManager::Iterator kit;
        for ( kit = pmanager->begin(); kit != pmanager->end(); ++kit ) {
            KSync::SynCEKonnectorBase *k = dynamic_cast<KSync::SynCEKonnectorBase *>( *kit );
            if ( k ) {
                k->init(pair->uid());
            }
        }
        pair->save();
        kdDebug(2120) << "Debug: Pair-Manager: " << ( void * ) pair->manager() << endl;
    } else if (!pair) {
        kdDebug(2120) << "Delete tmpPair" << endl;
        delete tmpPair;
        tmpPair = NULL;
    }
}


#include "pimsyncmanager.moc"
