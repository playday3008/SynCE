/*
    This file is part of KitchenSync.

    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "synceengine.h"

#include <konnector.h>
#include <kitchensync/multisynk/konnectormanager.h>
#include <konnectorinfo.h>
#include <klocale.h>

#include <addressbooksyncee.h>
#include <calendarsyncee.h>
#include <synceelist.h>
#include <syncuikde.h>


#include <qdatetime.h>

#include <kitchensync/multisynk/konnectorpair.h>
#include <kitchensync/multisynk/stdsyncui.h>


using namespace KSync;


SynCEEngine::SynCEEngine()
    : mManager( 0 ), mSyncUi( 0 )
{
}

SynCEEngine::~SynCEEngine()
{
    delete mSyncUi;
    mSyncUi = 0;
}

void SynCEEngine::logMessage( const QString &message )
{
    QString text = QTime::currentTime().toString() + ": ";
    text += message;

    kdDebug() << "LOG: " << text << endl;
}

void SynCEEngine::logError( const QString &message )
{
    QString text = QTime::currentTime().toString() + ": ";
    text += message;

    kdDebug() << "ERR: " << text << endl;

    emit error( message );
}

void SynCEEngine::setResolveStrategy( int strategy )
{
    delete mSyncUi;

    switch ( strategy ) {
        case KonnectorPair::ResolveFirst:
            mSyncUi = new SyncUiFirst();
            break;
        case KonnectorPair::ResolveSecond:
            mSyncUi = new SyncUiSecond();
            break;
        case KonnectorPair::ResolveBoth:
            mSyncUi = new KSync::SyncUi();
            break;
        default:
            mSyncUi = new SyncUiKde( 0, true, true );
    }

    mCalendarSyncer.setSyncUi( mSyncUi );
    mAddressBookSyncer.setSyncUi( mSyncUi );
}

void SynCEEngine::go( KonnectorPair *pair )
{
    kdDebug() << "Engine::gooooooooooo():" << endl;

    logMessage( i18n("Sync Action triggered") );

    setResolveStrategy( pair->resolveStrategy() );

    mOpenedKonnectors.clear();
    mProcessedKonnectors.clear();
    mKonnectorCount = 0;

    mKonnectors.clear();

    if ( mManager )
        disconnect( this, SIGNAL( doneSync() ), mManager, SLOT( emitFinished() ) );

    mManager = pair->manager();
    connect( this, SIGNAL( doneSync() ), mManager, SLOT( emitFinished() ) );

    KonnectorManager::Iterator it;
    for ( it = mManager->begin(); it != mManager->end(); ++it )
        mKonnectors.append( *it );

    Konnector *k;
    for( k = mKonnectors.first(); k; k = mKonnectors.next() ) {
        logMessage( i18n("Connecting '%1'").arg( k->resourceName() ) );
        if ( !k->connectDevice() ) {
            logError( i18n("Cannot connect device '%1'.").arg( k->resourceName() ) );
        } else {
            mOpenedKonnectors.append( k );
            ++mKonnectorCount;
        }
    }

    for ( k = mOpenedKonnectors.first(); k; k = mOpenedKonnectors.next() ) {
        logMessage( i18n("Request Syncees") );
        if ( !k->readSyncees() ) {
            logError( i18n("Cannot read data from '%1'.").arg( k->resourceName() ) );
        }
    }
}


void SynCEEngine::slotSynceesRead( Konnector *k )
{
    logError( "in my do sync" );
    logMessage( i18n("Syncees read from '%1'").arg( k->resourceName() ) );

    mProcessedKonnectors.append( k );

    SynceeList syncees = k->syncees();

    if ( syncees.count() == 0 ) {
        logMessage( i18n("Syncee list is empty.") );
        return;
    }

    tryExecuteActions();
}

void SynCEEngine::tryExecuteActions()
{
    kdDebug() << "Engine::tryExecuteActions()" << endl;


    kdDebug() << "  konnectorCount: " << mKonnectorCount << endl;
    kdDebug() << "  processedKonnectorsCount: " << mProcessedKonnectors.count()
            << endl;

    Konnector *k;
    for( k = mProcessedKonnectors.first(); k; k = mProcessedKonnectors.next() )
        logMessage( i18n("Processed '%1'").arg( k->resourceName() ) );

    if ( mKonnectorCount == mProcessedKonnectors.count() ) {
        executeActions();
    }
}

void SynCEEngine::executeActions()
{
    logMessage( i18n("Execute Actions") );

    Konnector *konnector;
    for ( konnector = mOpenedKonnectors.first(); konnector;
          konnector = mOpenedKonnectors.next() )
        konnector->applyFilters( KSync::Konnector::FilterBeforeSync );

    doSync();

    mProcessedKonnectors.clear();

    for( konnector = mOpenedKonnectors.first(); konnector;
         konnector = mOpenedKonnectors.next() ) {
             konnector->applyFilters( KSync::Konnector::FilterAfterSync );

             if ( !konnector->writeSyncees() )
                 logError( i18n("Cannot write data back to '%1'.").arg( konnector->resourceName() ) );
         }
}


void SynCEEngine::slotSynceeReadError( Konnector *k )
{
    logError( i18n("Error reading Syncees from '%1'").arg( k->resourceName() ) );

    --mKonnectorCount;

    tryExecuteActions();
}

void SynCEEngine::slotSynceesWritten( Konnector *k )
{
    logMessage( i18n("Syncees written to '%1'").arg( k->resourceName() ) );

    mProcessedKonnectors.append( k );

    disconnectDevice( k );

    tryFinish();
}

void SynCEEngine::slotSynceeWriteError( Konnector *k )
{
    logError( i18n("Error writing Syncees to '%1'").arg( k->resourceName() ) );

    --mKonnectorCount;

    disconnectDevice( k );

    tryFinish();
}

void SynCEEngine::disconnectDevice( Konnector *k )
{
    if ( !k->disconnectDevice() )
        logError( i18n("Error disconnecting device '%1'").arg( k->resourceName() ) );
}

void SynCEEngine::tryFinish()
{
    if ( mKonnectorCount == mProcessedKonnectors.count() ) {
        finish();
    }
}

void SynCEEngine::finish()
{
    logMessage( i18n("Synchronization finished.") );
    emit doneSync();
}



template<class T>
        T  *SynCEEngine::templateSyncee(SynceeList *synceeList)const
{
  T *syncee;

  SynceeList::ConstIterator it;
  for( it = synceeList->begin(); it != synceeList->end(); ++it ) {
    syncee = dynamic_cast<T*>( *it );
    if ( syncee ) return syncee;
  }

  return 0;
}


#include <kdebug.h>
void SynCEEngine::doSync()
{
  mCalendarSyncer.clear();
  mEventSyncer.clear();
  mTodoSyncer.clear();
  mAddressBookSyncer.clear();

  logError( "in my do sync" );
  Konnector *k;
  for( k = mKonnectors.first(); k; k = mKonnectors.next() ) {
    SynceeList syncees = k->syncees();

    if ( syncees.count() == 0 )
      continue;

    CalendarSyncee *calendarSyncee = syncees.calendarSyncee();
    if ( calendarSyncee )
      mCalendarSyncer.addSyncee( calendarSyncee );

    EventSyncee *eventSyncee = templateSyncee<EventSyncee>(&syncees);
    if (eventSyncee)
        mEventSyncer.addSyncee(eventSyncee);

    TodoSyncee *todoSyncee = templateSyncee<TodoSyncee>(&syncees);
    if (todoSyncee)
        mTodoSyncer.addSyncee(todoSyncee);

    AddressBookSyncee *addressBookSyncee = syncees.addressBookSyncee();
    if ( addressBookSyncee )
      mAddressBookSyncer.addSyncee( addressBookSyncee );
  }

  mCalendarSyncer.sync();
  mAddressBookSyncer.sync();
  mTodoSyncer.sync();
  mEventSyncer.sync();
}

#include "synceengine.moc"
