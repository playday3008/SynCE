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

#include <qdir.h>

#include <kitchensync/addressbooksyncee.h>
#include <kitchensync/calendarsyncee.h>
#include <kitchensync/konnectorinfo.h>

class PocketPCKonnectorFactory : public KRES::PluginFactoryBase
{
public:
    KRES::Resource* resource ( const KConfig* p_config )
    {
        return new KSync::PocketPCKonnector( p_config );
    }

    KRES::ConfigWidget* configWidget ( QWidget* p_parent )
    {
        return new pocketPCHelper::PocketPCKonnectorConfig( p_parent, "PocketPCKonnectorConfig" );
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
            : KSync::Konnector( p_config )
    {
        if ( p_config ) {
            m_pdaName = p_config->readEntry( "PDAName" );
            m_rra = new pocketPCCommunication::Rra( m_pdaName );
            m_rra->setLogLevel( 0 );
        } else {
            m_rra = NULL;
        }

        mBaseDir = storagePath();

        mUidHelper = new KSync::KonnectorUIDHelper(mBaseDir + "/" + m_pdaName);

        mAddrHandler = new pocketPCCommunication::AddressBookHandler( m_rra, mBaseDir, mUidHelper);
        mTodoHandler = new pocketPCCommunication::TodoHandler(m_rra, mBaseDir, mUidHelper);

        mAddressBookSyncee = new AddressBookSyncee();
        mAddressBookSyncee->setTitle("SynCE");

        mCalendarSyncee = new CalendarSyncee( &mCalendar );
        mCalendarSyncee->setTitle("SynCE");

        mSyncees.append(mCalendarSyncee);
        mSyncees.append(mAddressBookSyncee);
    }


    PocketPCKonnector::~PocketPCKonnector()
    {
        kdDebug( 2120 ) << "PocketPCKonnector::~PocketPCKonnector" << endl;

        delete mAddressBookSyncee;
        delete mCalendarSyncee;
        delete mAddrHandler;
        delete mUidHelper;

        if (m_rra != NULL) {
            if ( m_rra.data() ) {
                kdDebug( 2120 ) << "PocketPCKonnector::~PocketPCKonnector: before m_rra->finalDisconnect()" << endl;
                m_rra->finalDisconnect();
            }
            kdDebug( 2120 ) << "PocketPCKonnector::~PocketPCKonnector: m_rra.count(): " << m_rra.count() << endl;
        }
    }


    SynceeList PocketPCKonnector::syncees()
    {
        kdDebug( 2120 ) << "PocketPCKonnector::syncees() returning syncees" << endl;
        return mSyncees;
    }

    bool PocketPCKonnector::readSyncees()
    {
        // clear the internal structures;
        bool firstSync = false;

        clearDataStructures();

        QDir dir;
        QString dirName = mBaseDir + m_pdaName; // check that dir does not exist!!!!!!!
        if ( !dir.exists( dirName ) ) {
            // do something about firstSync!
            firstSync = true;
            dir.mkdir ( dirName );
        }




        if ( !mAddrHandler->connectDevice() ) {
            emit synceeReadError(this);
            return false;
        }
        if (!mAddrHandler->readSyncee(mAddressBookSyncee, firstSync)) {
            emit synceeReadError(this);
            return false;
        }
        mAddrHandler->disconnectDevice();


        if ( !mTodoHandler->connectDevice() ) {
            emit synceeReadError(this);
            return false;
        }
        if (!mTodoHandler->readSyncee(mCalendarSyncee, firstSync)) {
            emit synceeReadError(this);
            return false;
        }
        mTodoHandler->disconnectDevice();

#ifdef A
        m_rra->connect();

        pocketPCCommunication::EventHandler eventHandler(m_rra);
        pocketPCCommunication::TodoHandler todoHandler(m_rra);
        // do just the same for getting the complete calendar (events and todos are separated)
        if ( !eventHandler.getAllEvents( mCalendar, pocketPCCommunication::ALL ) ) {
            emit synceeReadError( this );
            return false;
        }

        m_rra->disconnect(); // think this is necessary.. *hmpf* but it should not
        m_rra->connect();

        if ( !todoHandler.getAllTodos( mCalendar, pocketPCCommunication::ALL ) ) {
            emit synceeReadError( this );
            return false;
        }

        m_rra->disconnect();

        mCalendarSyncee->setTitle("SynCE");
        mCalendarSyncee->setIdentifier(m_pdaName + "-Calendar");
#endif

        emit synceesRead ( this );

        return true;
    }


    bool PocketPCKonnector::writeSyncees()
    {
        if ( mSyncees.empty() ) {
            kdDebug( 2120 ) << "PocketPCKonnector::writeSyncees: m_syncees is empty" << endl;
            return false;
        }

        kdDebug( 2120 ) << "PocketPCKonnector:: m_syncees.addressBookSyncee() read" << endl;

        dumpIds ( mAddressBookSyncee );
        dumpIds ( mCalendarSyncee );


        kdDebug( 2120 ) << "PocketPCKonnector::writeSyncees..." << endl;

        // write m_syncees to the device
        if ( mSyncees.empty() ) {
            kdDebug( 2120 ) << "PocketPCKonnector::writeSyncees: m_syncees is empty" << endl;
            emit synceeWriteError( this );
            return false;
        }

        QString partnerId;

        mAddrHandler->connectDevice();
        mAddrHandler->writeSyncee(mAddressBookSyncee);
        mAddrHandler->disconnectDevice();

        mTodoHandler->connectDevice();
        mTodoHandler->writeSyncee(mCalendarSyncee);
        mTodoHandler->disconnectDevice();

#ifdef A
        m_rra->connect();
        if ( mCalendarSyncee->isValid() ) {
            KCal::Event::List eventsAdded;
            KCal::Event::List eventsRemoved;
            KCal::Event::List eventsModified;

            KCal::Todo::List todosAdded;
            KCal::Todo::List todosRemoved;
            KCal::Todo::List todosModified;

            getEvents( eventsAdded, todosAdded, mCalendarSyncee->added() );
            getEvents( eventsRemoved, todosRemoved, mCalendarSyncee->removed() );
            getEvents( eventsModified, todosModified, mCalendarSyncee->modified() );

            pocketPCCommunication::EventHandler eventHandler( m_rra );

            kdDebug( 2120 ) << "PocketPCKonnector::writeSyncees: writing events to pda" << endl;
            kdDebug( 2120 ) << "    adding events: " << eventsAdded.size() << endl;
            eventHandler.addEvents( eventsAdded );
            kdDebug( 2120 ) << "    removing events" << endl;
            eventHandler.removeEvents( eventsRemoved );
            kdDebug( 2120 ) << "    modifieing events" << endl;
            eventHandler.updateEvents( eventsModified );

            pocketPCCommunication::TodoHandler todoHandler( m_rra );

            kdDebug( 2120 ) << "PocketPCKonnector::writeSyncees: writing todos to pda" << endl;
            kdDebug( 2120 ) << "    adding todos: " << todosAdded.size() << endl;
            todoHandler.addTodos( todosAdded );
            kdDebug( 2120 ) << "    removing todos" << endl;
            todoHandler.removeTodos( todosRemoved );
            kdDebug( 2120 ) << "    modifieing todos" << endl;
            todoHandler.updateTodos( todosModified );

            partnerId = eventHandler.getPartnerId();

        }

        m_rra->disconnect();
#endif

        clearDataStructures();

        emit synceesWritten ( this );

        return true;
    }


    bool PocketPCKonnector::connectDevice()
    {
        m_rra->initRapi();

        return true;
    }


    bool PocketPCKonnector::disconnectDevice()
    {
        m_rra->uninitRapi();

        return true;
    }


    KonnectorInfo PocketPCKonnector::info() const
    {
        if ( !m_rra.data() ) {
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


    void PocketPCKonnector::writeConfig( KConfig* p_config )
    {
        Konnector::writeConfig ( p_config );

        p_config->writeEntry ( "PDAName", m_pdaName );
    }


    void PocketPCKonnector::setPdaName ( const QString& p_pdaName )
    {
        m_pdaName = p_pdaName;
    }


    const QString PocketPCKonnector::getPdaName () const
    {
        return m_pdaName;
    }


    void PocketPCKonnector::dumpIds ( KSync::Syncee* p_syncee )
    {
        if ( p_syncee->isValid() ) {
            kdDebug(2120) << "Entry valid" << endl;
            KSync::SyncEntry * entry = p_syncee->firstEntry();
            while ( entry ) {
                kdDebug( 2120 ) << "PocketPCKonnector:: current id: " << entry->id() << endl;
                kdDebug( 2120 ) << "PocketPCKonnector:: status: " << entry->state() << endl;
                entry = p_syncee->nextEntry();
            }

            QMap<QString, Kontainer::ValueList> idMap = p_syncee->ids();
            QMap<QString, Kontainer::ValueList>::iterator it = idMap.begin();
            for ( ; it != idMap.end(); ++it ) {
                kdDebug( 2120 ) << "   TypeName: " << it.key() << endl;
                Kontainer::ValueList valList = it.data();
                Kontainer::ValueList::iterator valIt = valList.begin();
                for ( ; valIt != valList.end(); ++valIt ) {
                    kdDebug( 2120 ) << "      ids in pair: " << ( *valIt ).first << "    " << ( *valIt ).second << endl;
                }
            }
        }
    }


    void PocketPCKonnector::clearDataStructures()
    {
        mCalendar.deleteAllEvents();
        mCalendar.deleteAllTodos();
        mCalendar.deleteAllJournals();
    }


    void PocketPCKonnector::getEvents ( KCal::Event::List& p_events, KCal::Todo::List& p_todos, KSync::SyncEntry::PtrList p_ptrList )
    {
        KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin();
        for ( ; it != p_ptrList.end(); ++it ) {
            kdDebug( 2120 ) << "PocketPCKonnector::getEvents type of entry: " << ( dynamic_cast<KSync::CalendarSyncEntry*>( *it ) ) ->incidence() ->type() << endl;
            QString type = ( dynamic_cast<KSync::CalendarSyncEntry*>( *it ) ) ->incidence() ->type();
            if ( type == "Todo" )
                p_todos.push_back ( dynamic_cast<KCal::Todo*>( ( dynamic_cast<KSync::CalendarSyncEntry*>( *it ) ) ->incidence() ) );
            else if ( type == "Event" )
                p_events.push_back ( dynamic_cast<KCal::Event*>( ( dynamic_cast<KSync::CalendarSyncEntry*>( *it ) ) ->incidence() ) );
        }
    }
};
