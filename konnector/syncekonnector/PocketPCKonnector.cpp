/***************************************************************************
*   Copyright (C) 2004 by Christian Fremgen                               *
*   cfremgen@users.sourceforge.net                                        *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include "PocketPCKonnector.h"
#include "PocketPCKonnectorConfig.h"

#include "todohandler.h"
#include "eventhandler.h"
#include "AddressBookHandler.h"
#include <libkdepim/progressmanager.h>

#include <libkdepim/kpimprefs.h>

#include <qdir.h>

#include <kitchensync/addressbooksyncee.h>
#include <kitchensync/calendarsyncee.h>
#include <kitchensync/konnectorinfo.h>
#include <klocale.h>


class PocketPCKonnectorFactory : public KRES::PluginFactoryBase
{
public:
    KRES::Resource* resource ( const KConfig* p_config )
    {
        return new KSync::PocketPCKonnector( p_config );
    }

    KRES::ConfigWidget* configWidget ( QWidget* p_parent )
    {
        return new KSync::PocketPCKonnectorConfig( p_parent, "PocketPCKonnectorConfig" );
    }
};


extern "C"
{
    void* init_libsyncekonnector() {
        return new PocketPCKonnectorFactory();
    }
}


namespace KSync
{
    PocketPCKonnector::PocketPCKonnector( const KConfig* p_config )
            : KSync::SynCEKonnectorBase( p_config ), mEventCalendar(KPimPrefs::timezone()), mTodoCalendar(KPimPrefs::timezone()), m_rra(0), subscribtions(0)
    {
        contactsEnabled = true;
        contactsFirstSync = true;
        todosEnabled = true;
        todosFirstSync = true;
        eventsEnabled = true;
        eventsFirstSync = true;
        initialized = false;
        idsRead = false;
        m_rra = NULL;

        if ( p_config ) {
            m_pdaName = p_config->readEntry( "PDAName" );
            contactsEnabled = p_config->readBoolEntry("ContactsEnabled", true);
            contactsFirstSync = p_config->readBoolEntry("ContactsFirstSync", true);
            todosEnabled = p_config->readBoolEntry("TodosEnabled", true);
            todosFirstSync = p_config->readBoolEntry("TodosFirstSync", true);
            eventsEnabled = p_config->readBoolEntry("EventsEnabled", true);
            eventsFirstSync = p_config->readBoolEntry("EventsFirstSync", true);
            init();
        }
    }

    void PocketPCKonnector::init()
    {
        if (!initialized) {
            mBaseDir = storagePath();

            QDir dir;
            QString dirName = mBaseDir + m_pdaName;

            if ( !dir.exists( dirName ) ) {
                dir.mkdir ( dirName );
            }

            mAddressBookSyncee = new AddressBookSyncee();
            mAddressBookSyncee->setTitle("SynCE");

            mEventSyncee = new EventSyncee( &mEventCalendar );
            mEventSyncee->setTitle("SynCE");

            mTodoSyncee = new TodoSyncee( &mTodoCalendar );
            mTodoSyncee->setTitle("SynCE");

            mSyncees.append(mEventSyncee);
            mSyncees.append(mTodoSyncee);
            mSyncees.append(mAddressBookSyncee);

            subscribtionCount = 0;

            m_rra = new Rra( m_pdaName );
            m_rra->setLogLevel( 0 );
            mUidHelper = new KSync::KonnectorUIDHelper(mBaseDir + "/" + m_pdaName);
            mAddrHandler = new pocketPCCommunication::AddressBookHandler( m_rra, mBaseDir, mUidHelper);
            mTodoHandler = new pocketPCCommunication::TodoHandler(m_rra, mBaseDir, mUidHelper);
            mEventHandler = new pocketPCCommunication::EventHandler(m_rra, mBaseDir, mUidHelper);
        }

        initialized = true;
    }


    PocketPCKonnector::~PocketPCKonnector()
    {
        kdDebug( 2120 ) << "PocketPCKonnector::~PocketPCKonnector" << endl;

        if (mAddressBookSyncee) {
            delete mAddressBookSyncee;
        }
        if (mTodoSyncee) {
            delete mTodoSyncee;
        }
        if (mEventSyncee) {
            delete mEventSyncee;
        }
        if (mAddrHandler) {
            delete mAddrHandler;
        }
        if (mTodoHandler) {
            delete mTodoHandler;
        }
        if (mEventHandler) {
            delete mEventHandler;
        }
        if (mUidHelper) {
            delete mUidHelper;
        }
    }


    SynceeList PocketPCKonnector::syncees()
    {
        kdDebug( 2120 ) << "PocketPCKonnector::syncees() returning syncees" << endl;
        return mSyncees;
    }

    bool PocketPCKonnector::readSyncees()
    {
        if ( mSyncees.empty() ) {
            kdDebug( 2120 ) << "syncekonnector not configured - please configure and sync again" << endl;
            emit synceeReadError( this );
            return false;
        }

        clearDataStructures();

        mProgressItem->setStatus("Start loading data from Windows CE");

        if (!idsRead) {
            if (mAddrHandler && contactsEnabled /*&& (subscribtions & CONTACTS)*/) {
                m_rra->subscribeForType(mAddrHandler->getTypeId());
                subscribtionCount++;
            }

            if (mTodoHandler && todosEnabled /*&& (subscribtions & TODOS)*/) {
                m_rra->subscribeForType(mTodoHandler->getTypeId());
                subscribtionCount++;
            }

            if (mEventHandler && eventsEnabled /*&& (subscribtions & EVENTS)*/) {
                m_rra->subscribeForType(mEventHandler->getTypeId());
                subscribtionCount++;
            }

            if (!m_rra->getIds()) {
                emit synceeReadError(this);
            }
            idsRead = true;
        }

        if (mAddrHandler && contactsEnabled && (_actualSyncType & CONTACTS)) {
            mAddrHandler->setProgressItem( mProgressItem);
            if (!mAddrHandler->readSyncee(mAddressBookSyncee, contactsFirstSync)) {
                emit synceeReadError(this);
                return false;
            }
        }

        if (mTodoHandler && todosEnabled && (_actualSyncType & TODOS)) {
            mTodoHandler->setProgressItem( mProgressItem);
            if (!mTodoHandler->readSyncee(mTodoSyncee, todosFirstSync)) {
                emit synceeReadError(this);
                return false;
            }
        }

        mProgressItem->setProgress(60);

        if (mEventHandler && eventsEnabled && (_actualSyncType & EVENTS)) {
            mEventHandler->setProgressItem( mProgressItem);
            if (!mEventHandler->readSyncee(mEventSyncee, eventsFirstSync)) {
                emit synceeReadError(this);
                return false;
            }
        }

        emit synceesRead ( this );

        return true;
    }


    bool PocketPCKonnector::writeSyncees()
    {
        kdDebug( 2120 ) << "PocketPCKonnector::writeSyncees..." << endl;

        // write m_syncees to the device
        if ( mSyncees.empty() ) {
            kdDebug( 2120 ) << "PocketPCKonnector::writeSyncees: m_syncees is empty" << endl;
            emit synceeWriteError( this );
            return false;
        }

        QString partnerId;

        if (mAddrHandler && contactsEnabled && (_actualSyncType & CONTACTS)) {
            mAddrHandler->writeSyncee(mAddressBookSyncee);
            m_rra->unsubscribeType(mAddrHandler->getTypeId());
            contactsFirstSync = false;
            subscribtionCount--;
        }

        if (mTodoHandler && todosEnabled && (_actualSyncType & TODOS)) {
            mTodoHandler->writeSyncee(mTodoSyncee);
            m_rra->unsubscribeType(mTodoHandler->getTypeId());
            todosFirstSync = false;
            subscribtionCount--;
        }

        if(mEventHandler && eventsEnabled && (_actualSyncType & EVENTS)) {
            mEventHandler->writeSyncee(mEventSyncee);
            m_rra->unsubscribeType(mEventHandler->getTypeId());
            eventsFirstSync = false;
            subscribtionCount--;
        }

        if (subscribtionCount == 0) {
            idsRead = false;
        }

        clearDataStructures();

        emit synceesWritten ( this );

        return true;
    }


    bool PocketPCKonnector::connectDevice()
    {
        mProgressItem = progressItem( i18n( "Start loading data from Windows CE..." ) );
        mProgressItem->setStatus("Connecting to " + m_pdaName);

        if (!m_pdaName.isEmpty()) {
            m_rra->connect();
        } else {
            kdDebug(2120) << "You have didn't configure syncekonnector well - please repeat the configuration and start again" << endl;
            return false;
        }

        return true;
    }


    bool PocketPCKonnector::disconnectDevice()
    {
        if (mUidHelper) {
            mUidHelper->save();
        }
        m_rra->disconnect();

        mProgressItem->setComplete();
        mProgressItem = 0;

        return true;
    }


    KonnectorInfo PocketPCKonnector::info() const
    {
        if (m_rra) {
            return KonnectorInfo ( QString ( "PocketPC (WinCE) Konnector" ),
                                QIconSet(),  //iconSet(),
                                QString ( "WinCE 3.0 up" ),
                                /*                              "0", //metaId(),
                                                                "", //iconName(),*/
                                false );
        } else {
            return KonnectorInfo ( QString ( "PocketPC (WinCE) Konnector" ),
                                   QIconSet(),  //iconSet(),
                                   QString ( "WinCE 3.0 up" ),
                                   /*                              "0", //metaId(),
                                                                 "", //iconName(),*/
                                   m_rra->isConnected() );
        }

    }


    void PocketPCKonnector::writeConfig(KConfig* p_config )
    {
        p_config->writeEntry ( "PDAName", m_pdaName );
        p_config->writeEntry ("ContactsEnabled", contactsEnabled);
        p_config->writeEntry ("EventsEnabled", eventsEnabled);
        p_config->writeEntry ("TodosEnabled", todosEnabled);
        p_config->writeEntry ("ContactsFirstSync", contactsFirstSync);
        p_config->writeEntry ("EventsFirstSync", eventsFirstSync);
        p_config->writeEntry ("TodosFirstSync", todosFirstSync);

        Konnector::writeConfig ( p_config );
    }


    void PocketPCKonnector::setPdaName ( const QString& p_pdaName )
    {
        m_pdaName = p_pdaName;
        init();
    }


    const QString PocketPCKonnector::getPdaName () const
    {
        return m_pdaName;
    }


    bool PocketPCKonnector::getContactsEnabled()
    {
        return contactsEnabled;
    }


    bool PocketPCKonnector::getContactsFirstSync()
    {
        return contactsFirstSync;
    }


    bool PocketPCKonnector::getEventsEnabled()
    {
        return eventsEnabled;
    }


    bool PocketPCKonnector::getEventsFirstSync()
    {
        return eventsFirstSync;
    }


    bool PocketPCKonnector::getTodosEnabled()
    {
        return todosEnabled;
    }


    bool PocketPCKonnector::getTodosFirstSync()
    {
        return todosFirstSync;
    }


    void PocketPCKonnector::setContactsState(bool enabled, bool firstSync)
    {
        contactsEnabled = enabled;
        contactsFirstSync = firstSync;
    }


    void PocketPCKonnector::setEventsState(bool enabled, bool firstSync)
    {
        eventsEnabled = enabled;
        eventsFirstSync = firstSync;
    }


    void PocketPCKonnector::setTodosState(bool enabled, bool firstSync)
    {
        todosEnabled = enabled;
        todosFirstSync = firstSync;
    }


    void PocketPCKonnector::clearDataStructures()
    {
        if (mEventSyncee) {
            mEventSyncee->reset();
        }

        if (mTodoSyncee) {
            mTodoSyncee->reset();
        }

        if (mAddressBookSyncee) {
            mAddressBookSyncee->reset();
        }

        mTodoCalendar.deleteAllEvents();
        mTodoCalendar.deleteAllTodos();
        mTodoCalendar.deleteAllJournals();

        mEventCalendar.deleteAllEvents();
        mEventCalendar.deleteAllTodos();
        mEventCalendar.deleteAllJournals();
    }

    void PocketPCKonnector::subscribeTo( int type )
    {
        if (type & CONTACTS) {
            contactsEnabled = true;
        } else if (type & EVENTS) {
            eventsEnabled = true;
        } else if (type & TODOS) {
            todosEnabled = true;
        }
    }

    void PocketPCKonnector::unsubscribeFrom( int type )
    {
        if (type & CONTACTS) {
            contactsEnabled = false;
        } else if (type & EVENTS) {
            eventsEnabled = false;
        } else if (type & TODOS) {
            todosEnabled = false;
        }
    }

    void PocketPCKonnector::actualSyncType(int type)
    {
        kdDebug(2120) << "Actual Sync Type: " << type << endl;
        _actualSyncType = type;
    }
}
