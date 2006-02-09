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
#include <kabc/resourcefile.h>
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
            m_pdaName = p_config->readEntry( "PDAName" );
            contactsEnabled = p_config->readBoolEntry( "ContactsEnabled", true );
            contactsFirstSync = p_config->readBoolEntry( "ContactsFirstSync", true );
            todosEnabled = p_config->readBoolEntry( "TodosEnabled", true );
            todosFirstSync = p_config->readBoolEntry( "TodosFirstSync", true );
            eventsEnabled = p_config->readBoolEntry( "EventsEnabled", true );
            eventsFirstSync = p_config->readBoolEntry( "EventsFirstSync", true );
            init();
        }

        mAddressBookSyncee = new AddressBookSyncee( &mAddressBook);
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

    void SynCEDeviceKonnector::init()
    {
        if ( !initialized ) {
            m_rra = new Rra( m_pdaName );
            m_rra->setLogLevel( 0 );

            mAddrHandler = new PocketPCCommunication::AddressbookHandler(m_rra);
            mTodoHandler = new PocketPCCommunication::TodoHandler(m_rra);
            mEventHandler = new PocketPCCommunication::EventHandler(m_rra);
        }

        initialized = true;
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

        if (m_rra) {
            delete m_rra;
        }
    }


    SynceeList SynCEDeviceKonnector::syncees()
    {
        kdDebug( 2120 ) << "PocketPCKonnector::syncees() returning syncees" << endl;
        return mSyncees;
    }

    bool SynCEDeviceKonnector::readSyncees()
    {
        if ( mSyncees.empty() ) {
            kdDebug( 2120 ) << "syncekonnector not configured - please configure and sync again" << endl;
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
            mAddressBookResourceFile = new KABC::ResourceFile( "/home/voc/addressfile.vcf" );
            mAddressBook.addResource( mAddressBookResourceFile );
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
        kdDebug( 2120 ) << "PocketPCKonnector::writeSyncees..." << endl;

        // write m_syncees to the device
        if ( mSyncees.empty() ) {
            kdDebug( 2120 ) << "PocketPCKonnector::writeSyncees: m_syncees is empty" << endl;
            emit synceeWriteError( this );
            return false;
        }

        QString partnerId;

        if ( mAddrHandler && contactsEnabled && ( _actualSyncType & CONTACTS ) ) {
            mAddrHandler->writeSyncee( mAddressBookSyncee );
            m_rra->unsubscribeType( mAddrHandler->getTypeId() );

            KSync::AddressBookSyncEntry *entry = mAddressBookSyncee->firstEntry();
            while(entry) {
                delete entry;
                entry = mAddressBookSyncee->nextEntry();
            }
            mAddressBook.removeResource(mAddressBookResourceFile);
            mAddressBookSyncee->reset();
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

        clearDataStructures();

        emit synceesWritten ( this );

        return true;
    }


    bool SynCEDeviceKonnector::connectDevice()
    {
        mProgressItem = progressItem( i18n( "Start loading data from Windows CE..." ) );
        mProgressItem->setStatus( "Connecting to " + m_pdaName );

        if ( !m_pdaName.isEmpty() ) {
            m_rra->connect();
        } else {
            kdDebug( 2120 ) << "You have didn't configure syncekonnector well - please repeat the configuration and start again" << endl;
            return false;
        }

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
                                   QIconSet(),   //iconSet(),
                                   QString ( "WinCE 3.0 up" ),
                                   /*                              "0", //metaId(),
                                                                   "", //iconName(),*/
                                   false );
        } else {
            return KonnectorInfo ( QString ( "PocketPC (WinCE) Konnector" ),
                                   QIconSet(),   //iconSet(),
                                   QString ( "WinCE 3.0 up" ),
                                   /*                              "0", //metaId(),
                                                                 "", //iconName(),*/
                                   m_rra->isConnected() );
        }

    }


    void SynCEDeviceKonnector::writeConfig( KConfig* p_config )
    {
        p_config->writeEntry ( "PDAName", m_pdaName );
        p_config->writeEntry ( "ContactsEnabled", contactsEnabled );
        p_config->writeEntry ( "EventsEnabled", eventsEnabled );
        p_config->writeEntry ( "TodosEnabled", todosEnabled );
        p_config->writeEntry ( "ContactsFirstSync", contactsFirstSync );
        p_config->writeEntry ( "EventsFirstSync", eventsFirstSync );
        p_config->writeEntry ( "TodosFirstSync", todosFirstSync );

        Konnector::writeConfig ( p_config );
    }


    void SynCEDeviceKonnector::setPdaName ( const QString& p_pdaName )
    {
        m_pdaName = p_pdaName;
        init();
    }


    const QString SynCEDeviceKonnector::getPdaName () const
    {
        return m_pdaName;
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


    void SynCEDeviceKonnector::clearDataStructures()
    {
        if ( mEventSyncee ) {
            mEventSyncee->reset();
        }

        if ( mTodoSyncee ) {
            mTodoSyncee->reset();
        }

        if ( mAddressBookSyncee ) {
            mAddressBookSyncee->reset();
        }

        mTodoCalendar.deleteAllEvents();
        mTodoCalendar.deleteAllTodos();
        mTodoCalendar.deleteAllJournals();

        mEventCalendar.deleteAllEvents();
        mEventCalendar.deleteAllTodos();
        mEventCalendar.deleteAllJournals();

        mAddressBook.clear();
    }

    void SynCEDeviceKonnector::subscribeTo( int type )
    {
        if ( type & CONTACTS ) {
            contactsEnabled = true;
        } else if ( type & EVENTS ) {
            eventsEnabled = true;
        } else if ( type & TODOS ) {
            todosEnabled = true;
        }
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
        kdDebug( 2120 ) << "Actual Sync Type: " << type << endl;
        _actualSyncType = type;
    }

    void SynCEDeviceKonnector::setPairUid( const QString &pairUid )
    {
        if ( this->pairUid != pairUid ) {
            this->pairUid = pairUid;

            mBaseDir = storagePath();

            QDir dir;
            QString dirName = mBaseDir + m_pdaName + "_" + pairUid;

            if ( !dir.exists( dirName ) ) {
                dir.mkdir ( dirName );
            }

            if (mUidHelper != NULL) {
                mUidHelper->save();
                delete mUidHelper;
            }
            mUidHelper = new KSync::KonnectorUIDHelper( mBaseDir + "/" + m_pdaName + "_" + pairUid );

            mAddrHandler->setUidHelper(mUidHelper);
            mTodoHandler->setUidHelper(mUidHelper);
            mEventHandler->setUidHelper(mUidHelper);
        }
    }
}
