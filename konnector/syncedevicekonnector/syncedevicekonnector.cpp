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
        idsRead = false;
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
            return false;
        }

        clearDataStructures();

        mProgressItem->setStatus( "Start loading data from Windows CE" );

        if ( !idsRead ) {
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

            if ( !m_rra->getIds() ) {
                emit synceeReadError( this );
            }
            idsRead = true;
        }

        if ( mAddrHandler && contactsEnabled && ( _actualSyncType & CONTACTS ) ) {
            mAddrHandler->setProgressItem( mProgressItem );
            if ( !mAddrHandler->readSyncee( mAddressBookSyncee, contactsFirstSync ) ) {
                emit synceeReadError( this );
                return false;
            }
        }

        if ( mTodoHandler && todosEnabled && ( _actualSyncType & TODOS ) ) {
            mTodoHandler->setProgressItem( mProgressItem );
            if ( !mTodoHandler->readSyncee( mTodoSyncee, todosFirstSync ) ) {
                emit synceeReadError( this );
                return false;
            }
        }

        mProgressItem->setProgress( 60 );

        if ( mEventHandler && eventsEnabled && ( _actualSyncType & EVENTS ) ) {
            mEventHandler->setProgressItem( mProgressItem );
            if ( !mEventHandler->readSyncee( mEventSyncee, eventsFirstSync ) ) {
                emit synceeReadError( this );
                return false;
            }
        }

        emit synceesRead ( this );

        return true;
    }


    bool SynCEDeviceKonnector::writeSyncees()
    {
        bool ret = false;

        kdDebug( 2120 ) << "SynCEDeviceKonnector::writeSyncees()..." << endl;

        // write m_syncees to the device
        if ( mSyncees.empty() ) {
            kdDebug( 2120 ) << "SynCEDeviceKonnector::writeSyncees: m_syncees is empty" << endl;
            emit synceeWriteError( this );
            goto error;
        }

        if ( mAddrHandler && contactsEnabled && ( _actualSyncType & CONTACTS ) ) {
            mAddrHandler->writeSyncee( mAddressBookSyncee );
            m_rra->unsubscribeType( mAddrHandler->getTypeId() );
            contactsFirstSync = false;
            subscribtionCount--;
        }

        if ( mTodoHandler && todosEnabled && ( _actualSyncType & TODOS ) ) {
            mTodoHandler->writeSyncee( mTodoSyncee );
            m_rra->unsubscribeType( mTodoHandler->getTypeId() );
            todosFirstSync = false;
            subscribtionCount--;
        }

        if ( mEventHandler && eventsEnabled && ( _actualSyncType & EVENTS ) ) {
            mEventHandler->writeSyncee( mEventSyncee );
            m_rra->unsubscribeType( mEventHandler->getTypeId() );
            eventsFirstSync = false;
            subscribtionCount--;
        }

        if ( subscribtionCount == 0 ) {
            idsRead = false;
        }

        emit synceesWritten ( this );

        ret = true;
error:
        clearDataStructures();
        return ret;
    }


    void SynCEDeviceKonnector::clearDataStructures()
    {
        if ( mEventSyncee && ( _actualSyncType & EVENTS )) {
            mEventSyncee->reset();
        }

        if ( mTodoSyncee && ( _actualSyncType & TODOS )) {
            mTodoSyncee->reset();
        }

        if ( mAddressBookSyncee && ( _actualSyncType & CONTACTS )) {
            KSync::AddressBookSyncEntry *entry = mAddressBookSyncee->firstEntry();
            while ( entry ) {
                delete entry;
                entry = mAddressBookSyncee->nextEntry();
            }
            mAddressBookSyncee->reset();
        }

        mTodoCalendar.deleteAllEvents();
        mTodoCalendar.deleteAllTodos();
        mTodoCalendar.deleteAllJournals();

        mEventCalendar.deleteAllEvents();
        mEventCalendar.deleteAllTodos();
        mEventCalendar.deleteAllJournals();
    }


    bool SynCEDeviceKonnector::connectDevice()
    {
        mProgressItem = progressItem( i18n( "Start loading data from Windows CE..." ) );
        m_rra->connect();

        return true;
    }


    bool SynCEDeviceKonnector::disconnectDevice()
    {
        if ( mUidHelper ) {
            mUidHelper->save();
        }
        m_rra->disconnect();

        mProgressItem->setComplete();
        mProgressItem = 0;

        return true;
    }


    KonnectorInfo SynCEDeviceKonnector::info() const
    {
        if ( m_rra ) {
            return KonnectorInfo ( QString ( "PocketPC (WinCE) Konnector" ),
                                   QIconSet(),    //iconSet(),
                                   QString ( "WinCE 3.0 up" ),
                                   /*                              "0", //metaId(),
                                                                   "", //iconName(),*/
                                   false );
        } else {
            return KonnectorInfo ( QString ( "PocketPC (WinCE) Konnector" ),
                                   QIconSet(),    //iconSet(),
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


    bool SynCEDeviceKonnector::getContactsFirstSync()
    {
        return contactsFirstSync;
    }


    bool SynCEDeviceKonnector::getEventsEnabled()
    {
        return eventsEnabled;
    }


    bool SynCEDeviceKonnector::getEventsFirstSync()
    {
        return eventsFirstSync;
    }


    bool SynCEDeviceKonnector::getTodosEnabled()
    {
        return todosEnabled;
    }


    bool SynCEDeviceKonnector::getTodosFirstSync()
    {
        return todosFirstSync;
    }


    void SynCEDeviceKonnector::setContactsState( bool enabled, bool firstSync )
    {
        contactsEnabled = enabled;
        contactsFirstSync = firstSync;
    }


    void SynCEDeviceKonnector::setEventsState( bool enabled, bool firstSync )
    {
        eventsEnabled = enabled;
        eventsFirstSync = firstSync;
    }


    void SynCEDeviceKonnector::setTodosState( bool enabled, bool firstSync )
    {
        todosEnabled = enabled;
        todosFirstSync = firstSync;
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


    void SynCEDeviceKonnector::actualSyncType( int type )
    {
        _actualSyncType = type;
    }


    void SynCEDeviceKonnector::init(const QString &pairUid )
    {
        if ( !initialized ) {
            SynCEKonnectorBase::init(pairUid );
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
}
