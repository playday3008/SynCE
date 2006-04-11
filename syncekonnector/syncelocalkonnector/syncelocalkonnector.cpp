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
#include <kmessagebox.h>

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
            : SynCEKonnectorBase( config ), mConfigWidget( 0 ), mCalendar( KPimPrefs::timezone() )
    {
        if ( config ) {
            mCalendarFile = config->readPathEntry( "CalendarFile" );
            mAddressBookFile = config->readPathEntry( "AddressBookFile" );
            mBookmarkFile = config->readPathEntry( "BookmarkFile" );
        }

        mAddressBookSyncee = new AddressBookSyncee( &mAddressBook );
        mAddressBookSyncee->setTitle( i18n( "Local" ) );

        mTodoSyncee = new TodoSyncee( &mCalendar );
        mTodoSyncee->setTitle( i18n( "Local" ) );

        mEventSyncee = new EventSyncee( &mCalendar );
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

        mMd5sumEvent = getPairUid() + "/" + generateMD5Sum( mCalendarFile ) + "_syncelocalkonnector_evt.log";
        mMd5sumTodo = getPairUid() + "/" + generateMD5Sum( mCalendarFile ) + "_syncelocalkonnector_tod.log";
        mMd5sumAbk = getPairUid() + "/" + generateMD5Sum( mAddressBookFile ) + "_syncelocalkonnector_abk.log";

        mAddressBook.clear();

        if ( _actualSyncType & TODOS ) {
            if ( !mCalendarFile.isEmpty() ) {
                mCalendar.close();
                if ( mCalendar.load( mCalendarFile ) ) {
                    TodoSyncHistory c1Helper( mTodoSyncee, storagePath() + mMd5sumTodo );
                    c1Helper.load();
                }
            } else {
                KMessageBox::error(0, "You didn't configure the Task-Synchronizer. Please do so and synchronize again.",
                                   QString("Error reading from local calendar"));
                emit synceeReadError( this );
                kdDebug(2120) << "Read failed." << endl;
                return false;
            }
        }

        if ( _actualSyncType & EVENTS ) {
            if ( !mCalendarFile.isEmpty() ) {
                mCalendar.close();
                if ( mCalendar.load( mCalendarFile ) ) {
                    EventSyncHistory c2Helper( mEventSyncee, storagePath() + mMd5sumEvent );
                    c2Helper.load();
                }
            } else {
                KMessageBox::error(0, "You didn't configure the Appointment-Synchronizer. Please do so and synchronize again.",
                                   QString("Error reading from local calendar"));
                emit synceeReadError( this );
                kdDebug(2120) << "Read failed." << endl;
                return false;
            }
        }

        if ( _actualSyncType & CONTACTS ) {
            if ( !mAddressBookFile.isEmpty() ) {
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
            } else {
                KMessageBox::error(0, "You didn't configure the Contact-Synchronizer. Please do so and synchronize again.",
                                   QString("Error reading from local addressbook"));
                emit synceeReadError( this );
                kdDebug(2120) << "Read failed." << endl;
                return false;
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

        if ( _actualSyncType & TODOS ) {
            if ( !mCalendarFile.isEmpty() ) {
                purgeRemovedEntries( mTodoSyncee );
                TodoSyncHistory c1Helper( mTodoSyncee, storagePath() + mMd5sumTodo );
                c1Helper.save();
                if ( !mCalendar.save( mCalendarFile ) ) {
                    KMessageBox::error(0, "Error writing to calendar " + mCalendarFile + ". Please check permissions.",
                                       QString("Error writing to local calendar"));
                    emit synceeWriteError( this );
                    goto error;
                }
            } else {
                KMessageBox::error(0, "You didn't configure the Task-Synchronizer. Please do so and synchronize again.",
                                   QString("Error writing to local calendar"));
                emit synceeWriteError( this );
                goto error;
            }
        }

        if ( _actualSyncType & EVENTS ) {
            if ( !mCalendarFile.isEmpty() ) {
                purgeRemovedEntries( mEventSyncee );
                EventSyncHistory c2Helper( mEventSyncee, storagePath() + mMd5sumEvent );
                c2Helper.save();
                if ( !mCalendar.save( mCalendarFile ) ) {
                    KMessageBox::error(0, "Error writing to calendar " + mCalendarFile + ". Please check permissions.",
                                       QString("Error writing to local calendar"));
                    emit synceeWriteError( this );
                    goto error;
                }
            } else {
                KMessageBox::error(0, "You didn't configure the Appointment-Synchronizer. Please do so and synchronize again.",
                                   QString("Error writing to local calendar"));
                emit synceeWriteError( this );
                goto error;
            }
        }

        if ( _actualSyncType & CONTACTS ) {
            if ( !mAddressBookFile.isEmpty() ) {
                purgeRemovedEntries( mAddressBookSyncee );
                KABC::Ticket *ticket;
                ticket = mAddressBook.requestSaveTicket();
                if ( !ticket ) {
                    KMessageBox::error(0, "Error during ticket-request to save " + mAddressBookFile + ".",
                                       QString("Error writing to local addressbook"));
                    emit synceeWriteError( this );
                    goto error;
                }
                if ( !mAddressBook.save( ticket ) ) {
                    KMessageBox::error(0, "Error writing to addressbook " + mAddressBookFile + ". Please check permissions.",
                                       QString("Error writing to local addressbook"));
                    emit synceeWriteError( this );
                    goto error;
                }

                AddressBookSyncHistory aHelper( mAddressBookSyncee, storagePath() + "/" + mMd5sumAbk );
                aHelper.save();
            } else {
                KMessageBox::error(0, "You didn't configure the Contact-Synchronizer. Please do so and synchronize again.",
                                   QString("Error writing to local addressbook"));
                emit synceeWriteError( this );
                goto error;
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
        if ( !mCalendarFile.isEmpty() ) {
            if ( mEventSyncee && ( _actualSyncType & EVENTS )) {
                mEventSyncee->reset();
                mCalendar.deleteAllEvents();
                mCalendar.deleteAllTodos();
                mCalendar.deleteAllJournals();
            }

            if ( mTodoSyncee && ( _actualSyncType & TODOS )) {
                mTodoSyncee->reset();
                mCalendar.deleteAllEvents();
                mCalendar.deleteAllTodos();
                mCalendar.deleteAllJournals();
            }
        }

        if ( !mAddressBookFile.isEmpty() ) {
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
    }


    void SynCELocalKonnector::actualSyncType( int type )
    {
        _actualSyncType = type;
    }
}
#include "syncelocalkonnector.moc"

