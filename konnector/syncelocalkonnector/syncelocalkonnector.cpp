/*
This file is part of KitchenSync.

Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

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

#include "syncelocalkonnector.h"

#include "syncelocalkonnectorconfig.h"

#include <calendarsyncee.h>
#include <addressbooksyncee.h>
#include <bookmarksyncee.h>
#include <synchistory.h>

#include <libkcal/todo.h>
#include <libkcal/event.h>

#include <libkdepim/kpimprefs.h>

#include <kabc/resourcefile.h>

#include <konnectorinfo.h>

#include <kconfig.h>
#include <kgenericfactory.h>

using namespace KSync;

typedef SyncHistory<KSync::TodoSyncee, KSync::TodoSyncEntry > TodoSyncHistory;
typedef SyncHistory<KSync::EventSyncee, KSync::EventSyncEntry > EventSyncHistory;


class LocalKonnectorFactory : public KRES::PluginFactoryBase
{
    public:
        KRES::Resource* resource ( const KConfig* p_config )
        {
            return new KSync::SynCELocalKonnector( p_config );
        }

        KRES::ConfigWidget* configWidget ( QWidget* p_parent )
        {
            return new KSync::SynCELocalKonnectorConfig( p_parent, "LocalKonnectorConfig" );
        }
};


extern "C"
{
    void* init_libsyncelocalkonnector() {
        return new LocalKonnectorFactory();
    }
}


namespace KSync
{

    SynCELocalKonnector::SynCELocalKonnector( const KConfig *config )
            : SynCEKonnectorBase( config ), mConfigWidget( 0 ),
            mCalendar( KPimPrefs::timezone() ), mTodoCalendar( KPimPrefs::timezone() ), mEventCalendar( KPimPrefs::timezone() )
    {
        if ( config ) {
            mCalendarFile = config->readPathEntry( "CalendarFile" );
            mAddressBookFile = config->readPathEntry( "AddressBookFile" );
            mBookmarkFile = config->readPathEntry( "BookmarkFile" );
        }

        mMd5sumEvent = generateMD5Sum( mCalendarFile ) + "_syncelocalkonnector_evt.log";
        mMd5sumTodo = generateMD5Sum( mCalendarFile ) + "_syncelocalkonnector_tod.log";
        mMd5sumAbk = generateMD5Sum( mAddressBookFile ) + "_localkonnector_abk.log";
        mMd5sumBkm = generateMD5Sum( mBookmarkFile ) + "_localkonnector_bkm.log";

        mAddressBookSyncee = new AddressBookSyncee( &mAddressBook );
        mAddressBookSyncee->setTitle( i18n( "Local" ) );

        mTodoSyncee = new TodoSyncee( &mTodoCalendar );
        mTodoSyncee->setTitle( i18n( "Local" ) );

        mEventSyncee = new EventSyncee( &mEventCalendar );
        mEventSyncee->setTitle( i18n( "Local" ) );

        mSyncees.append( mEventSyncee );
        mSyncees.append( mTodoSyncee );
        mSyncees.append( mAddressBookSyncee );
        mSyncees.append( new BookmarkSyncee( &mBookmarkManager ) );
    }

    SynCELocalKonnector::~SynCELocalKonnector()
    {}

    void SynCELocalKonnector::writeConfig( KConfig *config )
    {
        Konnector::writeConfig( config );

        config->writePathEntry( "CalendarFile", mCalendarFile );
        config->writeEntry( "AddressBookFile", mAddressBookFile );
        config->writeEntry( "BookmarkFile", mAddressBookFile );
    }

    bool SynCELocalKonnector::readSyncees()
    {
        kdDebug() << "LocalKonnector::readSyncee()" << endl;

        mTodoCalendar.deleteAllEvents();
        mTodoCalendar.deleteAllTodos();
        mTodoCalendar.deleteAllJournals();

        mEventCalendar.deleteAllEvents();
        mEventCalendar.deleteAllTodos();
        mEventCalendar.deleteAllJournals();

        if ( !mCalendarFile.isEmpty() ) {
            kdDebug() << "LocalKonnector::readSyncee(): calendar: " << mCalendarFile
            << endl;
            mCalendar.close();
            if ( mCalendar.load( mCalendarFile ) ) {
                kdDebug() << "Read succeeded." << endl;

                if ( _actualSyncType & TODOS ) {
                    kdDebug() << "****************Syncing todos" << endl;
                    KCal::Todo::List todoList = mCalendar.todos();
                    mTodoSyncee->reset();
                    mTodoSyncee->setIdentifier( "Todo" + mCalendarFile );
                    kdDebug() << "IDENTIFIER: " << mTodoSyncee->identifier() << endl;
                    KCal::Todo::List::iterator todoIt;
                    for ( todoIt = todoList.begin(); todoIt != todoList.end(); ++todoIt ) {
                        mCalendar.deleteTodo( *todoIt );
                        kdDebug( 2120 ) << "------- Todo: " << ( *todoIt ) ->dtStartTimeStr() << endl;
                        mTodoCalendar.addTodo( ( *todoIt ) ->clone() );
                    }
                    TodoSyncHistory c1Helper( mTodoSyncee, storagePath() + "/todo" + mMd5sumTodo );
                    c1Helper.load();
                }

                if ( _actualSyncType & EVENTS ) {
                    kdDebug() << "****************Syncing events" << endl;
                    KCal::Event::List eventList = mCalendar.events();
                    mEventSyncee->reset();
                    mEventSyncee->setIdentifier( "Event" + mCalendarFile );
                    kdDebug() << "IDENTIFIER: " << mEventSyncee->identifier() << endl;
                    KCal::Event::List::iterator eventIt;
                    for ( eventIt = eventList.begin(); eventIt != eventList.end(); ++eventIt ) {
                        mCalendar.deleteEvent( *eventIt );
                        kdDebug( 2120 ) << "------- Event: " << ( *eventIt ) ->dtStartTimeStr() << endl;
                        mEventCalendar.addEvent( ( *eventIt ) ->clone() );
                    }
                    EventSyncHistory c2Helper( mEventSyncee, storagePath() + "/event" + mMd5sumEvent );
                    c2Helper.load();
                }
            } else {
                emit synceeReadError( this );
                kdDebug() << "Read failed." << endl;
                return false;
            }
        }

        if ( !mAddressBookFile.isEmpty() ) {
            kdDebug() << "LocalKonnector::readSyncee(): addressbook: "
            << mAddressBookFile << endl;

            mAddressBookResourceFile = new KABC::ResourceFile( mAddressBookFile );
            mAddressBook.addResource( mAddressBookResourceFile );

            if ( !mAddressBook.load() ) {
                emit synceeReadError( this );
                kdDebug() << "Read failed." << endl;
                return false;
            }

            kdDebug() << "Read succeeded." << endl;

            if ( _actualSyncType & CONTACTS ) {
                kdDebug() << "****************Syncing contacts" << endl;
                mAddressBookSyncee->reset();
                mAddressBookSyncee->setIdentifier( mAddressBook.identifier() );

                KABC::AddressBook::Iterator it;
                for ( it = mAddressBook.begin(); it != mAddressBook.end(); ++it ) {
                    KSync::AddressBookSyncEntry entry( *it, mAddressBookSyncee );
                    mAddressBookSyncee->addEntry( entry.clone() );
                }

                /* let us apply Sync Information */
                AddressBookSyncHistory aHelper( mAddressBookSyncee, storagePath() + "/" + mMd5sumAbk );
                aHelper.load();
            }
        }

        // TODO: Read Bookmarks

        emit synceesRead( this );

        return true;
    }

    bool SynCELocalKonnector::connectDevice()
    {
        return true;
    }

    bool SynCELocalKonnector::disconnectDevice()
    {
        return true;
    }

    KSync::KonnectorInfo SynCELocalKonnector::info() const
    {
        return KonnectorInfo( i18n( "Dummy Konnector" ),
                              QIconSet(),
                              "agenda",     // icon name
                              false );
    }


    bool SynCELocalKonnector::writeSyncees()
    {
        if ( !mCalendarFile.isEmpty() ) {

            if ( _actualSyncType & TODOS ) {
                kdDebug() << "****************Syncing todos" << endl;
                purgeRemovedEntries( mTodoSyncee );
                TodoSyncHistory c1Helper( mTodoSyncee, storagePath() + "/todo" + mMd5sumTodo );
                c1Helper.save();
                KCal::Todo::List todoList = mTodoCalendar.todos();
                KCal::Todo::List::iterator todoIt;
                for ( todoIt = todoList.begin(); todoIt != todoList.end(); ++todoIt ) {
                    mTodoCalendar.deleteTodo( *todoIt );
                    kdDebug( 2120 ) << "------- Todo: " << ( *todoIt ) ->dtStartTimeStr() << endl;
                    mCalendar.addTodo( ( *todoIt ) ->clone() );
                }
            }

            if ( _actualSyncType & EVENTS ) {
                kdDebug() << "****************Syncing events" << endl;
                purgeRemovedEntries( mEventSyncee );
                EventSyncHistory c2Helper( mEventSyncee, storagePath() + "/event" + mMd5sumEvent );
                c2Helper.save();
                KCal::Event::List eventList = mEventCalendar.events();
                KCal::Event::List::iterator eventIt;
                for ( eventIt = eventList.begin(); eventIt != eventList.end(); ++eventIt ) {
                    mEventCalendar.deleteEvent( *eventIt );
                    kdDebug( 2120 ) << "------- Event: " << ( *eventIt ) ->dtStartTimeStr() << endl;
                    mCalendar.addEvent( ( *eventIt ) ->clone() );
                }
            }

            if ( !mCalendar.save( mCalendarFile ) )
                return false;
        }

        if ( !mAddressBookFile.isEmpty() ) {
            if ( _actualSyncType & CONTACTS ) {
                kdDebug() << "****************Syncing contacts" << endl;
                purgeRemovedEntries( mAddressBookSyncee );
                KABC::Ticket *ticket;
                ticket = mAddressBook.requestSaveTicket( mAddressBookResourceFile );
                if ( !ticket ) {
                    kdWarning() << "LocalKonnector::writeSyncees(). Couldn't get ticket for "
                    << "addressbook." << endl;
                    emit synceeWriteError( this );
                    return false;
                }
                if ( !mAddressBook.save( ticket ) ) {
                    mAddressBook.removeResource(mAddressBookResourceFile);
                    return false;
                }

                mAddressBook.removeResource(mAddressBookResourceFile);

                AddressBookSyncHistory aHelper( mAddressBookSyncee, storagePath() + "/" + mMd5sumAbk );
                aHelper.save();
            }
        }

        // TODO: Write Bookmarks

        emit synceesWritten( this );

        return true;
    }

    void SynCELocalKonnector::actualSyncType( int type )
    {
        kdDebug( 2120 ) << "Actual Sync Type: " << type << endl;
        _actualSyncType = type;
    }
}
#include "syncelocalkonnector.moc"
