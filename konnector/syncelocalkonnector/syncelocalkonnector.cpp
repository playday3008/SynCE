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


class SynCELocalKonnectorFactory : public KRES::PluginFactoryBase
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
        return new SynCELocalKonnectorFactory();
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

        mAddressBookSyncee = new AddressBookSyncee( &mAddressBook );
        mAddressBookSyncee->setTitle( i18n( "Local" ) );

        mTodoSyncee = new TodoSyncee( &mTodoCalendar );
        mTodoSyncee->setTitle( i18n( "Local" ) );

        mEventSyncee = new EventSyncee( &mEventCalendar );
        mEventSyncee->setTitle( i18n( "Local" ) );

        mSyncees.append( mEventSyncee );
        mSyncees.append( mTodoSyncee );
        mSyncees.append( mAddressBookSyncee );
    }


    SynCELocalKonnector::~SynCELocalKonnector()
    {
        kdDebug( 2120 ) << "SynCELocalKonnector::~SynCELocalKonnector" << endl;
        delete mAddressBookSyncee;
        delete mTodoSyncee;
        delete mEventSyncee;
    }


    void SynCELocalKonnector::writeConfig( KConfig *config )
    {
        Konnector::writeConfig( config );

        config->writePathEntry( "CalendarFile", mCalendarFile );
        config->writeEntry( "AddressBookFile", mAddressBookFile );
        config->writeEntry( "BookmarkFile", mAddressBookFile );
    }


    bool SynCELocalKonnector::readSyncees()
    {
        kdDebug(2120) << "SynCELocalKonnector::readSyncee()..." << endl;

        mMd5sumEvent = getPairUid() + "_" + generateMD5Sum( mCalendarFile ) + "_syncelocalkonnector_evt.log";
        mMd5sumTodo = getPairUid() + "_" + generateMD5Sum( mCalendarFile ) + "_syncelocalkonnector_tod.log";
        mMd5sumAbk = getPairUid() + "_" + generateMD5Sum( mAddressBookFile ) + "_syncelocalkonnector_abk.log";

        mTodoCalendar.deleteAllEvents();
        mTodoCalendar.deleteAllTodos();
        mTodoCalendar.deleteAllJournals();

        mEventCalendar.deleteAllEvents();
        mEventCalendar.deleteAllTodos();
        mEventCalendar.deleteAllJournals();

        mAddressBook.clear();

        if ( !mCalendarFile.isEmpty() ) {
            mCalendar.close();
            if ( mCalendar.load( mCalendarFile ) ) {
                if ( _actualSyncType & TODOS ) {
                    KCal::Todo::List todoList = mCalendar.todos();
                    mTodoSyncee->reset();
                    mTodoSyncee->setIdentifier( "Todo" + mCalendarFile );
                    KCal::Todo::List::iterator todoIt;
                    for ( todoIt = todoList.begin(); todoIt != todoList.end(); ++todoIt ) {
                        mCalendar.deleteTodo( *todoIt );
                        mTodoCalendar.addTodo( *todoIt );
                    }
                    TodoSyncHistory c1Helper( mTodoSyncee, storagePath() + mMd5sumTodo );
                    c1Helper.load();
                }

                if ( _actualSyncType & EVENTS ) {
                    KCal::Event::List eventList = mCalendar.events();
                    mEventSyncee->reset();
                    mEventSyncee->setIdentifier( "Event" + mCalendarFile );
                    KCal::Event::List::iterator eventIt;
                    for ( eventIt = eventList.begin(); eventIt != eventList.end(); ++eventIt ) {
                        mCalendar.deleteEvent( *eventIt );
                        mEventCalendar.addEvent(*eventIt);
                    }
                    EventSyncHistory c2Helper( mEventSyncee, storagePath() + mMd5sumEvent );
                    c2Helper.load();
                }
            } else {
                emit synceeReadError( this );
                kdDebug(2120) << "Read failed." << endl;
                return false;
            }
        }

        if ( !mAddressBookFile.isEmpty() ) {
            if ( _actualSyncType & CONTACTS ) {
                mAddressBookResourceFile = new KABC::ResourceFile( mAddressBookFile );
                mAddressBook.addResource( mAddressBookResourceFile );

                if ( !mAddressBook.load() ) {
                    emit synceeReadError( this );
                    kdDebug(2120) << "Read failed." << endl;
                    return false;
                }

                mAddressBookSyncee->reset();
                mAddressBookSyncee->setIdentifier( mAddressBook.identifier() );

                KABC::AddressBook::Iterator it;
                for ( it = mAddressBook.begin(); it != mAddressBook.end(); ++it ) {
                    KSync::AddressBookSyncEntry entry( *it, mAddressBookSyncee );
                    mAddressBookSyncee->addEntry( entry.clone());
                }

                AddressBookSyncHistory aHelper( mAddressBookSyncee, storagePath() + "/" + mMd5sumAbk );
                aHelper.load();
            }
        }

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
        bool ret = false;
        kdDebug( 2120 ) << "SynCELocalKonnector::writeSyncees()..." << endl;

        if ( !mCalendarFile.isEmpty() ) {

            if ( _actualSyncType & TODOS ) {
                purgeRemovedEntries( mTodoSyncee );
                TodoSyncHistory c1Helper( mTodoSyncee, storagePath() + mMd5sumTodo );
                c1Helper.save();
                KCal::Todo::List todoList = mTodoCalendar.todos();
                KCal::Todo::List::iterator todoIt;
                for ( todoIt = todoList.begin(); todoIt != todoList.end(); ++todoIt ) {
                    mTodoCalendar.deleteTodo( *todoIt );
                    mCalendar.addTodo(*todoIt);
                }
            }

            if ( _actualSyncType & EVENTS ) {
                purgeRemovedEntries( mEventSyncee );
                EventSyncHistory c2Helper( mEventSyncee, storagePath() + mMd5sumEvent );
                c2Helper.save();
                KCal::Event::List eventList = mEventCalendar.events();
                KCal::Event::List::iterator eventIt;
                for ( eventIt = eventList.begin(); eventIt != eventList.end(); ++eventIt ) {
                    mEventCalendar.deleteEvent( *eventIt );
                    mCalendar.addEvent(*eventIt);
                }
            }

            if ( !mCalendar.save( mCalendarFile ) )
                goto error;
        }

        if ( !mAddressBookFile.isEmpty() ) {
            if ( _actualSyncType & CONTACTS ) {
                purgeRemovedEntries( mAddressBookSyncee );
                KABC::Ticket *ticket;
                ticket = mAddressBook.requestSaveTicket();
                if ( !ticket ) {
                    kdWarning(2120) << "LocalKonnector::writeSyncees(). Couldn't get ticket for "
                            << "addressbook." << endl;
                    emit synceeWriteError( this );
                    goto error;
                }
                if ( !mAddressBook.save( ticket ) ) {
                    goto error;
                }

                AddressBookSyncHistory aHelper( mAddressBookSyncee, storagePath() + "/" + mMd5sumAbk );
                aHelper.save();
            }
        }


        emit synceesWritten( this );

        ret = true;
error:
        clearDataStructures();
        return ret;
    }


    void SynCELocalKonnector::clearDataStructures()
    {
        if ( mEventSyncee && ( _actualSyncType & EVENTS )) {
            mEventSyncee->reset();
            mEventCalendar.deleteAllEvents();
            mEventCalendar.deleteAllTodos();
            mEventCalendar.deleteAllJournals();
            mCalendar.deleteAllEvents();
            mCalendar.deleteAllTodos();
            mCalendar.deleteAllJournals();
        }

        if ( mTodoSyncee && ( _actualSyncType & TODOS )) {
            mTodoSyncee->reset();
            mTodoCalendar.deleteAllEvents();
            mTodoCalendar.deleteAllTodos();
            mTodoCalendar.deleteAllJournals();
            mCalendar.deleteAllEvents();
            mCalendar.deleteAllTodos();
            mCalendar.deleteAllJournals();
        }

        if ( mAddressBookSyncee && ( _actualSyncType & CONTACTS )) {
            KSync::AddressBookSyncEntry *entry = mAddressBookSyncee->firstEntry();
            while ( entry ) {
                delete entry;
                entry = mAddressBookSyncee->nextEntry();
            }
            mAddressBookSyncee->reset();
            mAddressBook.removeResource(mAddressBookResourceFile);
        }
    }


    void SynCELocalKonnector::actualSyncType( int type )
    {
        _actualSyncType = type;
    }
}
#include "syncelocalkonnector.moc"

