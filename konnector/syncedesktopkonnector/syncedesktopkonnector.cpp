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

#include "syncedesktopkonnector.h"

#include "syncedesktopkonnectorconfig.h"

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
#include <kapplication.h>

using namespace KSync;

typedef SyncHistory<KSync::TodoSyncee, KSync::TodoSyncEntry > TodoSyncHistory;
typedef SyncHistory<KSync::EventSyncee, KSync::EventSyncEntry > EventSyncHistory;


class SynCEDesktopKonnectorFactory : public KRES::PluginFactoryBase
{
    public:
        KRES::Resource* resource ( const KConfig* p_config )
        {
            return new KSync::SynCEDesktopKonnector( p_config );
        }

        KRES::ConfigWidget* configWidget ( QWidget* p_parent )
        {
            return new KSync::SynCEDesktopKonnectorConfig( p_parent, "DesktopKonnectorConfig" );
        }
};


extern "C"
{
    void* init_libsyncedesktopkonnector() {
        return new SynCEDesktopKonnectorFactory();
    }
}


namespace KSync
{

    SynCEDesktopKonnector::SynCEDesktopKonnector( const KConfig *config )
    : SynCEKonnectorBase( config ), mConfigWidget( 0 ), mCalendar( KPimPrefs::timezone() ), mCalendarResource(0), mContactResource(0)
    {
        mCalendar.readConfig();
        KRES::Manager<KCal::ResourceCalendar> *calendarManager = mCalendar.resourceManager();
        KRES::Manager<KCal::ResourceCalendar>::ActiveIterator kcalIt;
        kdDebug(2120) << "Active Resources: " << endl;

        for ( kcalIt = calendarManager->activeBegin(); kcalIt != calendarManager->activeEnd(); ++kcalIt ) {
            kdDebug(2120) << "Calendar Resource: Id: " << (*kcalIt)->identifier() << "   Name: " << (*kcalIt)->resourceName() << endl;
        }

        mAddressBook.readConfig();
        KRES::Manager<KABC::Resource> *contactManager = mAddressBook.resourceManager();
        KRES::Manager<KABC::Resource>::ActiveIterator kabcIt;

        for ( kabcIt = contactManager->activeBegin(); kabcIt != contactManager->activeEnd(); ++kabcIt) {
            kdDebug(2120) << "Addressbook Resource: Id: " << (*kabcIt)->identifier() << "   Name: " << (*kabcIt)->resourceName() << endl;
        }

        if ( config ) {
            mCalendarResourceIdentifier = config->readEntry( "CurrentCalendarResource" );
            for ( kcalIt = calendarManager->activeBegin(); kcalIt != calendarManager->activeEnd(); ++kcalIt ) {
                if ((*kcalIt)->identifier() == mCalendarResourceIdentifier) {
                    mCalendarResource = *kcalIt;
                    kdDebug(2120) << "Found standard resource to mCalendar- Id: " << mCalendarResource->identifier() << "   Name: " << mCalendarResource->resourceName() << endl;
                    mCalendar.resourceManager()->setStandardResource( *kcalIt );
                    connect( calendarManager->standardResource(), SIGNAL( resourceLoaded( ResourceCalendar* ) ),
                             SLOT( loadingFinished() ) );
                    connect( calendarManager->standardResource(), SIGNAL( resourceSaved( ResourceCalendar* ) ),
                             SLOT( savingFinished() ) );
                }
            }

            mContactResourceIdentifier = config->readEntry( "CurrentAddressBookResource" );
            for (kabcIt = contactManager->activeBegin(); kabcIt != contactManager->activeEnd(); ++kabcIt) {
                if ((*kabcIt)->identifier() == mContactResourceIdentifier) {
                    mContactResource = *kabcIt;
                    mContactResourceIdentifier = mContactResourceIdentifier;
                    kdDebug(2120) << "Found standard resource for mAddressBook - Id: " << mContactResource->identifier() << "   Name: " << mContactResource->resourceName() << endl;
                    contactManager->setStandardResource(*kabcIt);
                    mContactResource->setAddressBook(&mAddressBook);
                }
            }
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


    SynCEDesktopKonnector::~SynCEDesktopKonnector()
    {
        kdDebug( 2120 ) << "SynCELocalKonnector::~SynCELocalKonnector" << endl;
        delete mAddressBookSyncee;
        delete mTodoSyncee;
        delete mEventSyncee;
    }


    void SynCEDesktopKonnector::writeConfig( KConfig *config )
    {
        Konnector::writeConfig( config );

        config->writeEntry( "CurrentCalendarResource", mCalendarResourceIdentifier );
        config->writeEntry( "CurrentAddressBookResource", mContactResourceIdentifier );
    }


    bool SynCEDesktopKonnector::readSyncees()
    {
        kdDebug(2120) << "SynCEDesktopKonnector::readSyncee()..." << endl;

        mMd5sumEvent = getPairUid() + "/" + "syncedesktopkonnector_evt.log";
        mMd5sumTodo = getPairUid() + "/" + "syncedesktopkonnector_tod.log";
        mMd5sumAbk = getPairUid() + "/" + "syncedesktopkonnector_abk.log";

        if ( _actualSyncType & TODOS ) {
            if ( mCalendarResource ) {
                mTodoSyncee->reset();
                mCalendar.close();
                mCalendar.load( );
            } else {
                KMessageBox::error(0, "You didn't configure the Task-Synchronizer. Please do so and synchronize again.",
                                   QString("Error reading from local calendar"));
                emit synceeReadError( this );
                kdDebug(2120) << "Read failed." << endl;
                return false;
            }
        }

        if ( _actualSyncType & EVENTS ) {
            if ( mCalendarResource ) {
                mEventSyncee->reset();
                mCalendar.close();
                mCalendar.load( );
            } else {
                KMessageBox::error(0, "You didn't configure the Appointment-Synchronizer. Please do so and synchronize again.",
                                   QString("Error reading from local calendar"));
                emit synceeReadError( this );
                kdDebug(2120) << "Read failed." << endl;
                return false;
            }
        }

        if ( _actualSyncType & CONTACTS ) {
            if ( mContactResource ) {
                mAddressBook.clear();
                mAddressBookSyncee->reset();
                mAddressBookSyncee->setIdentifier( mAddressBook.identifier() );

                if (!mContactResource->open()) {
                    emit synceeReadError(this);
                    kdDebug(2120) << "Opening resource failed" << endl;
                    return false;
                }

                if ( !mContactResource->load() ) {
                    emit synceeReadError( this );
                    kdDebug(2120) << "Read failed." << endl;
                    return false;
                }

            } else {
                KMessageBox::error(0, "You didn't configure the Contact-Synchronizer. Please do so and synchronize again.",
                                   QString("Error reading from local addressbook"));
                emit synceeReadError( this );
                kdDebug(2120) << "Read failed." << endl;
                return false;
            }
        }

        loadingFinished();

        return true;
    }


    void SynCEDesktopKonnector::loadingFinished()
    {
        kdDebug(2120) << "Loading finished" << endl;
        if ( _actualSyncType & TODOS ) {
            kdDebug(2120) << "mCalendar loaded" << endl;
            mTodoSyncee->setIdentifier( "Todo" + mCalendarResourceIdentifier );
            TodoSyncHistory c1Helper( mTodoSyncee, storagePath() + mMd5sumTodo );
            c1Helper.load();
        }

        if ( _actualSyncType & EVENTS ) {
            kdDebug(2120) << "mCalendar loaded" << endl;
            mEventSyncee->setIdentifier( "Event" + mCalendarResourceIdentifier );
            EventSyncHistory c2Helper( mEventSyncee, storagePath() + mMd5sumEvent );
            c2Helper.load();
        }

        if (_actualSyncType & CONTACTS) {
            KABC::AddressBook::Iterator it;
            for ( it = mAddressBook.begin(); it != mAddressBook.end(); ++it ) {
                KSync::AddressBookSyncEntry entry( *it, mAddressBookSyncee );
                mAddressBookSyncee->addEntry( entry.clone());
            }
            AddressBookSyncHistory aHelper( mAddressBookSyncee, storagePath() + "/" + mMd5sumAbk );
            aHelper.load();
        }

        loaded = true;
        emit synceesRead( this );
    }


    bool SynCEDesktopKonnector::connectDevice()
    {
        return true;
    }


    bool SynCEDesktopKonnector::disconnectDevice()
    {
        return true;
    }


    KSync::KonnectorInfo SynCEDesktopKonnector::info() const
    {
        return KonnectorInfo( i18n( "Dummy Konnector" ),
                              QIconSet(),
                              "agenda",     // icon name
                              false );
    }


    bool SynCEDesktopKonnector::writeSyncees()
    {
        bool ret = false;
        kdDebug( 2120 ) << "SynCEDesktopKonnector::writeSyncees()..." << endl;

        if ( _actualSyncType & TODOS ) {
            if ( mCalendarResource ) {
                purgeRemovedEntries( mTodoSyncee );
                TodoSyncHistory c1Helper( mTodoSyncee, storagePath() + mMd5sumTodo );
                c1Helper.save();
                KCal::CalendarResources::Ticket *ticket = mCalendar.requestSaveTicket( mCalendarResource );
                if ( !ticket ) {
                    KMessageBox::error(0, "Error during ticket-request to save Calendar.",
                                       QString("Error writing to Calendar"));
                    emit synceeWriteError( this );
                    goto error;
                }
                if (!mCalendar.save( ticket )) {
                    KMessageBox::error(0, "Error writing Calendar.",
                                       QString("Error writing to Calendar"));
                    mCalendar.releaseSaveTicket(ticket);
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
            if ( mCalendarResource ) {
                purgeRemovedEntries( mEventSyncee );
                EventSyncHistory c2Helper( mEventSyncee, storagePath() + mMd5sumEvent );
                c2Helper.save();
                KCal::CalendarResources::Ticket *ticket = mCalendar.requestSaveTicket( mCalendarResource );
                if ( !ticket ) {
                    KMessageBox::error(0, "Error during ticket-request to save Calendar.",
                                       QString("Error writing to Calendar"));
                    emit synceeWriteError( this );
                    goto error;
                }
                if (!mCalendar.save( ticket )) {
                    KMessageBox::error(0, "Error writing Calendar.",
                                       QString("Error writing to Calendar"));
                    mCalendar.releaseSaveTicket(ticket);
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
            if ( mContactResource ) {
                purgeRemovedEntries( mAddressBookSyncee );
                AddressBookSyncHistory aHelper( mAddressBookSyncee, storagePath() + "/" + mMd5sumAbk );
                aHelper.save();
                KABC::Ticket *ticket = mAddressBook.requestSaveTicket(mContactResource);
                if ( !ticket ) {
                    KMessageBox::error(0, "Error during ticket-request to save Addressbook.",
                                       QString("Error writing to Addressbook"));
                    emit synceeWriteError( this );
                    mContactResource->close();
                    goto error;
                }
                if ( !mAddressBook.save( ticket ) ) {
                    KMessageBox::error(0, "Error writing to Addressbook.",
                                       QString("Error writing to Addressbook"));
                    emit synceeWriteError( this );
                    mAddressBook.releaseSaveTicket (ticket);
                    mContactResource->close();
                    goto error;
                }
                mContactResource->close();
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


    void SynCEDesktopKonnector::savingFinished()
    {
        kdDebug(2120) << "Saving finished" << endl;
        emit synceesWritten( this );
        clearDataStructures();
    }


    void SynCEDesktopKonnector::clearDataStructures()
    {
        if ( mCalendarResource ) {
            if ( mEventSyncee && ( _actualSyncType & EVENTS )) {
                mEventSyncee->reset();
                mCalendarResource->close();
            }

            if ( mTodoSyncee && ( _actualSyncType & TODOS )) {
                mTodoSyncee->reset();
                mCalendarResource->close();
            }
        }

        if ( mContactResource ) {
            if ( mAddressBookSyncee && ( _actualSyncType & CONTACTS )) {
                KSync::AddressBookSyncEntry *entry = mAddressBookSyncee->firstEntry();
                while ( entry ) {
                    delete entry;
                    entry = mAddressBookSyncee->nextEntry();
                }
                mAddressBookSyncee->reset();
            }
        }
    }


    void SynCEDesktopKonnector::setCurrentContactResource( const QString &identifier )
    {
        if (mContactResourceIdentifier != identifier) {
            KRES::Manager<KABC::Resource> *contactManager = mAddressBook.resourceManager();
            KRES::Manager<KABC::Resource>::Iterator kabcIt;

            for (kabcIt = contactManager->begin(); kabcIt != contactManager->end(); ++kabcIt) {
                if ((*kabcIt)->identifier() == identifier) {
                    mContactResource = *kabcIt;
                    mContactResourceIdentifier = identifier;
                    kdDebug(2120) << "Found standard resource for mAddressBook" << endl;
                    kdDebug(2120) << "Std: Id: " << mContactResource->identifier() << "   Name: " << mContactResource->resourceName() << endl;
                    contactManager->setStandardResource(*kabcIt);
                    mContactResource->setAddressBook(&mAddressBook);
                    mContactResource->setActive(true);
                }
            }
        }
    }


    void SynCEDesktopKonnector::setCurrentCalendarResource( const QString &identifier )
    {
        if (mCalendarResourceIdentifier != identifier) {
            KRES::Manager<KCal::ResourceCalendar> *calendarManager = mCalendar.resourceManager();
            KRES::Manager<KCal::ResourceCalendar>::Iterator kcalIt;
            for ( kcalIt = calendarManager->begin(); kcalIt != calendarManager->end(); ++kcalIt ) {
                if ((*kcalIt)->identifier() == identifier) {
                    mCalendarResource = *kcalIt;
                    mCalendarResourceIdentifier = identifier;
                    kdDebug(2120) << "Found standard resource for mCalendar" << endl;
                    kdDebug(2120) << "Std: Id: " << mCalendarResource->identifier() << "   Name: " << mCalendarResource->resourceName() << endl;
                    mCalendarResource->setActive(true);
                    disconnect( calendarManager->standardResource(), SIGNAL( resourceLoaded( ResourceCalendar* ) ),
                                this, SLOT( loadingFinished() ) );
                    disconnect( calendarManager->standardResource(), SIGNAL( resourceSaved( ResourceCalendar* ) ),
                                this, SLOT( savingFinished() ) );
                    mCalendar.resourceManager()->setStandardResource( *kcalIt );
                    connect( calendarManager->standardResource(), SIGNAL( resourceLoaded( ResourceCalendar* ) ),
                             SLOT( loadingFinished() ) );
                    connect( calendarManager->standardResource(), SIGNAL( resourceSaved( ResourceCalendar* ) ),
                             SLOT( savingFinished() ) );
                }
            }
        }
    }

    void SynCEDesktopKonnector::actualSyncType( int type )
    {
        _actualSyncType = type;
    }


}
#include "syncedesktopkonnector.moc"

