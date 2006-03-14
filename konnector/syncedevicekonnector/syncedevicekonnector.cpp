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

#include "syncedevicekonnector.h"
#include "syncedevicekonnectorconfig.h"

#include "todohandler.h"
#include "eventhandler.h"
#include "addressbookhandler.h"
#include <libkdepim/progressmanager.h>

#include <libkdepim/kpimprefs.h>

#include <qdir.h>

#include <kitchensync/addressbooksyncee.h>
#include <kitchensync/calendarsyncee.h>
#include <kitchensync/konnectorinfo.h>
#include <klocale.h>
#include <kmessagebox.h>

class SynCEDeviceKonnectorFactory : public KRES::PluginFactoryBase
{
    public:
        KRES::Resource* resource ( const KConfig* p_config )
        {
            return new KSync::SynCEDeviceKonnector( p_config );
        }

        KRES::ConfigWidget* configWidget ( QWidget* p_parent )
        {
            return new KSync::SynCEDeviceKonnectorConfig( p_parent, "PocketPCKonnectorConfig" );
        }
};


extern "C"
{
    void* init_libsyncedevicekonnector() {
        return new SynCEDeviceKonnectorFactory();
    }
}


namespace KSync
{
    SynCEDeviceKonnector::SynCEDeviceKonnector( const KConfig* p_config )
            : KSync::SynCEKonnectorBase( p_config ), mEventCalendar( KPimPrefs::timezone() ), mTodoCalendar( KPimPrefs::timezone() ), m_rra( 0 ), subscribtions( 0 )
    {
        contactsEnabled = false;
        contactsFirstSync = true;
        todosEnabled = false;
        todosFirstSync = true;
        eventsEnabled = false;
        eventsFirstSync = true;
        initialized = false;
        error = false;
        m_rra = NULL;

        mUidHelper = NULL;
        mAddrHandler = NULL;
        mTodoHandler = NULL;
        mEventHandler = NULL;

        if ( p_config ) {
            contactsEnabled = p_config->readBoolEntry( "ContactsEnabled", true );
            contactsFirstSync = p_config->readBoolEntry( "ContactsFirstSync", true );
            todosEnabled = p_config->readBoolEntry( "TodosEnabled", true );
            todosFirstSync = p_config->readBoolEntry( "TodosFirstSync", true );
            eventsEnabled = p_config->readBoolEntry( "EventsEnabled", true );
            eventsFirstSync = p_config->readBoolEntry( "EventsFirstSync", true );
        }

        mAddressBookSyncee = new AddressBookSyncee();
        mAddressBookSyncee->setTitle( "SynCE" );

        mEventSyncee = new EventSyncee( &mEventCalendar );
        mEventSyncee->setTitle( "SynCE" );

        mTodoSyncee = new TodoSyncee( &mTodoCalendar );
        mTodoSyncee->setTitle( "SynCE" );

        mSyncees.append( mEventSyncee );
        mSyncees.append( mTodoSyncee );
        mSyncees.append( mAddressBookSyncee );

        subscribtionCount = 0;

        PocketPCCommunication::PimHandler::setProgressItem(progressItem( i18n( "Start loading data from Windows CE..." )));
    }


    void SynCEDeviceKonnector::init( const QString &pairUid )
    {
        if ( !initialized ) {
            SynCEKonnectorBase::init( pairUid );
            initialized = true;

            mAddrHandler = new PocketPCCommunication::AddressbookHandler();
            mTodoHandler = new PocketPCCommunication::TodoHandler();
            mEventHandler = new PocketPCCommunication::EventHandler();

            QString mBaseDir = storagePath();

            QDir dir;
            QString dirName = mBaseDir + getPairUid();

            if ( !dir.exists( dirName ) ) {
                dir.mkdir ( dirName );
            }

            if ( mUidHelper != NULL ) {
                mUidHelper->save();
                delete mUidHelper;
            }
            mUidHelper = new KSync::KonnectorUIDHelper( mBaseDir + getPairUid() );

            mAddrHandler->setUidHelper( mUidHelper );
            mTodoHandler->setUidHelper( mUidHelper );
            mEventHandler->setUidHelper( mUidHelper );
        }
    }


    SynCEDeviceKonnector::~SynCEDeviceKonnector()
    {
        kdDebug( 2120 ) << "SynCEDeviceKonnector::~SynCEDeviceKonnector" << endl;
        delete mAddressBookSyncee;
        delete mTodoSyncee;
        delete mEventSyncee;

        if ( mAddrHandler ) {
            delete mAddrHandler;
        }
        if ( mTodoHandler ) {
            delete mTodoHandler;
        }
        if ( mEventHandler ) {
            delete mEventHandler;
        }
        if ( mUidHelper ) {
            delete mUidHelper;
        }
        PocketPCCommunication::PimHandler::progressItemSetCompleted();
    }


    SynceeList SynCEDeviceKonnector::syncees()
    {
        return mSyncees;
    }


    bool SynCEDeviceKonnector::readSyncees()
    {
        kdDebug( 2120 ) << "SynCEDeviceKonnector::readSyncees()..." << endl;
        if ( mSyncees.empty() ) {
            kdDebug( 2120 ) << "SynCEDeviceKonnector not configured - please configure and sync again" << endl;
            emit synceeReadError( this );
            error = true;
            goto error;
        }

        clearDataStructures();

        if ( subscribtionCount == 0 ) {
            if ( mAddrHandler && contactsEnabled ) {
                m_rra->subscribeForType( mAddrHandler->getTypeId() );
                subscribtionCount++;
            }

            if ( mTodoHandler && todosEnabled ) {
                m_rra->subscribeForType( mTodoHandler->getTypeId() );
                subscribtionCount++;
            }

            if ( mEventHandler && eventsEnabled ) {
                m_rra->subscribeForType( mEventHandler->getTypeId() );
                subscribtionCount++;
            }

            if ( error = !m_rra->getIds() ) {
                KMessageBox::error(0, QString("Failur during ID-request from device"),
                                   QString("Error reading from") + m_rra->getTypeForId(mAddrHandler->getTypeId())->name2 +
                                   " synchronizer");
                emit synceeReadError(this);
                goto error;
            }
        }

        if ( mAddrHandler && contactsEnabled && ( _actualSyncType & CONTACTS ) ) {
            if ( error = !mAddrHandler->readSyncee( mAddressBookSyncee, contactsFirstSync ) ) {
                KMessageBox::errorList(0, PocketPCCommunication::PimHandler::getError(),
                                       PocketPCCommunication::PimHandler::getErrorEntries(),
                                    QString("Error reading from ") +
                                    m_rra->getTypeForId(mAddrHandler->getTypeId())->name2 +
                                    " synchronizer");
                emit synceeReadError( this );
                goto error;
            }
        }

        if ( mTodoHandler && todosEnabled && ( _actualSyncType & TODOS ) ) {
            if (error = !mTodoHandler->readSyncee( mTodoSyncee, todosFirstSync ) ) {
                KMessageBox::errorList(0, PocketPCCommunication::PimHandler::getError(),
                                   PocketPCCommunication::PimHandler::getErrorEntries(),
                                    QString("Error reading from ") +
                                    m_rra->getTypeForId(mTodoHandler->getTypeId())->name2 +
                                    " synchronizer");
                emit synceeReadError( this );
                goto error;
            }
        }

        if ( mEventHandler && eventsEnabled && ( _actualSyncType & EVENTS ) ) {
            if (error = !mEventHandler->readSyncee( mEventSyncee, eventsFirstSync ) ) {
                KMessageBox::errorList(0, PocketPCCommunication::PimHandler::getError(),
                                   PocketPCCommunication::PimHandler::getErrorEntries(),
                                    QString("Error reading from ") +
                                    m_rra->getTypeForId(mEventHandler->getTypeId())->name2 +
                                    " synchronizer");
                emit synceeReadError( this );
                goto error;
            }
        }

        emit synceesRead ( this );

    error:
        return !error;
    }


    bool SynCEDeviceKonnector::writeSyncees()
    {
        kdDebug( 2120 ) << "SynCEDeviceKonnector::writeSyncees()..." << endl;

        if ( mSyncees.empty() ) {
            kdDebug( 2120 ) << "SynCEDeviceKonnector::writeSyncees: m_syncees is empty" << endl;
            emit synceeWriteError( this );
            goto error;
        }

        if ( mAddrHandler && contactsEnabled && ( _actualSyncType & CONTACTS ) ) {
            error = !mAddrHandler->writeSyncee( mAddressBookSyncee );
            contactsFirstSync = false;
            m_rra->unsubscribeType( mAddrHandler->getTypeId() );
            subscribtionCount--;
            if (error) {
                KMessageBox::errorList(0, PocketPCCommunication::PimHandler::getError(),
                                   PocketPCCommunication::PimHandler::getErrorEntries(),
                                   QString("Error writing to ") +
                                   m_rra->getTypeForId(mAddrHandler->getTypeId())->name2 +
                                   " synchronizer");
                emit synceeWriteError(this);
                goto error;
            }
        }

        if ( mTodoHandler && todosEnabled && ( _actualSyncType & TODOS ) ) {
            error = !mTodoHandler->writeSyncee( mTodoSyncee );
            todosFirstSync = false;
            m_rra->unsubscribeType( mTodoHandler->getTypeId() );
            subscribtionCount--;
            if (error) {
                KMessageBox::errorList(0, PocketPCCommunication::PimHandler::getError(),
                                   PocketPCCommunication::PimHandler::getErrorEntries(),
                                   QString("Error writing to ") +
                                   m_rra->getTypeForId(mTodoHandler->getTypeId())->name2 +
                                   " synchronizer");
                emit synceeWriteError(this);
                goto error;
            }
        }

        if ( mEventHandler && eventsEnabled && ( _actualSyncType & EVENTS ) ) {
            error = !mEventHandler->writeSyncee( mEventSyncee );
            eventsFirstSync = false;
            m_rra->unsubscribeType( mEventHandler->getTypeId() );
            subscribtionCount--;
            if (error) {
                KMessageBox::errorList(0, PocketPCCommunication::PimHandler::getError(),
                                       PocketPCCommunication::PimHandler::getErrorEntries(),
                                   QString("Error writing to ") +
                                   m_rra->getTypeForId(mEventHandler->getTypeId())->name2 +
                                           " synchronizer");
                emit synceeWriteError(this);
                goto error;
            }
        }

        emit synceesWritten ( this );

    error:
        clearDataStructures();

        return !error;
    }


    void SynCEDeviceKonnector::clearDataStructures()
    {
        if ( mEventSyncee && ( _actualSyncType & EVENTS ) ) {
            mEventSyncee->reset();
            mEventCalendar.deleteAllEvents();
            mEventCalendar.deleteAllTodos();
            mEventCalendar.deleteAllJournals();
        }

        if ( mTodoSyncee && ( _actualSyncType & TODOS ) ) {
            mTodoSyncee->reset();
            mTodoCalendar.deleteAllEvents();
            mTodoCalendar.deleteAllTodos();
            mTodoCalendar.deleteAllJournals();
        }

        if ( mAddressBookSyncee && ( _actualSyncType & CONTACTS ) ) {
            KSync::AddressBookSyncEntry * entry = mAddressBookSyncee->firstEntry();
            while ( entry ) {
                delete entry;
                entry = mAddressBookSyncee->nextEntry();
            }
            mAddressBookSyncee->reset();
        }
    }


    bool SynCEDeviceKonnector::connectDevice()
    {
        PocketPCCommunication::PimHandler::resetError();
        error = false;
        if (subscribtionCount == 0) {
            m_rra->connect();
        }

        return true;
    }


    bool SynCEDeviceKonnector::disconnectDevice()
    {
        if ( mUidHelper ) {
            mUidHelper->save();
        }

        if (subscribtionCount == 0) {
            m_rra->disconnect();
        }

        PocketPCCommunication::PimHandler::resetError();

        return true;
    }


    KonnectorInfo SynCEDeviceKonnector::info() const
    {
        if ( m_rra ) {
            return KonnectorInfo ( QString ( "PocketPC (WinCE) Konnector" ),
                                   QIconSet(),     //iconSet(),
                                   QString ( "WinCE 3.0 up" ),
                                   /*                              "0", //metaId(),
                                                                   "", //iconName(),*/
                                   false );
        } else {
            return KonnectorInfo ( QString ( "PocketPC (WinCE) Konnector" ),
                                   QIconSet(),     //iconSet(),
                                   QString ( "WinCE 3.0 up" ),
                                   /*                              "0", //metaId(),
                                                                 "", //iconName(),*/
                                   m_rra->isConnected() );
        }

    }


    void SynCEDeviceKonnector::writeConfig( KConfig* p_config )
    {
        p_config->writeEntry ( "ContactsEnabled", contactsEnabled );
        p_config->writeEntry ( "EventsEnabled", eventsEnabled );
        p_config->writeEntry ( "TodosEnabled", todosEnabled );
        p_config->writeEntry ( "ContactsFirstSync", contactsFirstSync );
        p_config->writeEntry ( "EventsFirstSync", eventsFirstSync );
        p_config->writeEntry ( "TodosFirstSync", todosFirstSync );

        Konnector::writeConfig ( p_config );
    }


    bool SynCEDeviceKonnector::getContactsEnabled()
    {
        return contactsEnabled;
    }


    bool SynCEDeviceKonnector::getEventsEnabled()
    {
        return eventsEnabled;
    }


    bool SynCEDeviceKonnector::getTodosEnabled()
    {
        return todosEnabled;
    }


    void SynCEDeviceKonnector::setContactsState( bool enabled)
    {
        contactsEnabled = enabled;
    }


    void SynCEDeviceKonnector::setEventsState( bool enabled)
    {
        eventsEnabled = enabled;
    }


    void SynCEDeviceKonnector::setTodosState( bool enabled)
    {
        todosEnabled = enabled;
    }


    void SynCEDeviceKonnector::actualSyncType( int type )
    {
        _actualSyncType = type;
    }


    void SynCEDeviceKonnector::subscribeTo( Rra* rra, int type )
    {
        if ( type & CONTACTS ) {
            contactsEnabled = true;
        } else if ( type & EVENTS ) {
            eventsEnabled = true;
        } else if ( type & TODOS ) {
            todosEnabled = true;
        }

        m_rra = rra;
        mAddrHandler->setRra( rra );
        mTodoHandler->setRra( rra );
        mEventHandler->setRra( rra );
    }


    void SynCEDeviceKonnector::unsubscribeFrom( int type )
    {
        if ( type & CONTACTS ) {
            contactsEnabled = false;
        } else if ( type & EVENTS ) {
            eventsEnabled = false;
        } else if ( type & TODOS ) {
            todosEnabled = false;
        }
    }
}
