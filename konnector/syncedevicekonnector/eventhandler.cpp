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

#include "eventhandler.h"

#include <kdebug.h>
#include <kapplication.h>
#include <libkcal/icalformat.h>
#include <libkcal/event.h>
#include <libkcal/todo.h>
#include <libkcal/calendarnull.h>
#include <libkcal/calendarlocal.h>
#include <qfile.h>
#include <qregexp.h>

namespace PocketPCCommunication
{
    EventHandler::EventHandler() : PimHandler()
    {
        mTypeId = 0;

        QFile f( "/etc/timezone" );
        if ( f.open( IO_ReadOnly ) ) {
            QTextStream ts( &f );
            ts >> sCurrentTimeZone;
        }
        f.close();
    }


    bool EventHandler::init()
    {
        mTypeId = m_rra->getTypeForName( RRA_SYNCMGR_TYPE_APPOINTMENT );

        return mTypeId != 0;
    }


    EventHandler::~EventHandler()
    {}


    bool EventHandler::retrieveEventListFromDevice( KCal::Event::List &mEventList, QValueList<uint32_t> &idList )
    {
        KCal::ICalFormat calFormat;
        calFormat.setTimeZone( sCurrentTimeZone, false );
        bool ret = true;

        QString vCalBegin = "BEGIN:VCALENDAR\nPRODID:-//K Desktop Environment//NONSGML KOrganizer 3.2.1//EN\nVERSION:2.0\n";
        QString vCalEnd = "END:VCALENDAR\n";

        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            incrementSteps();

            kdDebug( 2120 ) << "Retrieving Event from device: " << "RRA-ID-" +
            QString::number ( *it, 16 ).rightJustify( 8, '0' ) << endl;

            QString vEvent = m_rra->getVEvent( mTypeId, *it );
            if ( vEvent.isEmpty() ) {
                addErrorEntry("RRA-ID-" + QString::number ( *it, 16 ).rightJustify( 8, '0' ));
                ret = false;
            }
            QString vCal = vCalBegin + vEvent + vCalEnd;

            // TODO delete the incidence created by fromString
            KCal::Incidence *incidence = calFormat.fromString ( vCal );

            QString kdeId;
            if ( ( kdeId = mUidHelper->kdeId( "SynCEEvent", incidence->uid(), "---" ) ) != "---" ) {
                incidence->setUid( kdeId );
            } else {
                mUidHelper->addId( "SynCEEvent", incidence->uid(), incidence->uid() );
            }

            kdDebug( 2120 ) << "    ID-Pair: KDEID: " << incidence->uid() << " DeviceID: " <<
            "RRA-ID-" + QString::number ( *it, 16 ).rightJustify( 8, '0' ) << endl;

            mEventList.push_back( dynamic_cast<KCal::Event*> ( incidence ) );

            KApplication::kApplication() ->processEvents();
        }

        return ret;
    }


    void EventHandler::fakeEventListFromDevice( KCal::Event::List &mEventList, QValueList<uint32_t> &idList )
    {
        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            KCal::Event *event = new KCal::Event();

            QString konId = "RRA-ID-" + QString::number( *it, 16 ).rightJustify( 8, '0' );
            QString kdeId;

            if ( ( kdeId = mUidHelper->kdeId( "SynCEEvent", konId, "---" ) ) != "---" ) {

                kdDebug( 2120 ) << "Faking Event for device: " << konId << endl;

                event->setUid( kdeId );
                mUidHelper->removeId( "SynCEEvent", event->uid() );
                kdDebug( 2120 ) << "    ID-Pair: KDEID: " << event->uid() << " DeviceID: " << konId << endl;
                mEventList.push_back( event );
            }

        }
    }


    void EventHandler::getIds()
    {
        m_rra->getIdsForType( mTypeId, &ids );
    }


    bool EventHandler::getEventListFromDevice( KCal::Event::List &mEventList, int mRecType )
    {
        bool ret = true;

        if ( ( mRecType & CHANGED ) ) {
            setStatus( "Reading changed Events" );
            ret = retrieveEventListFromDevice( mEventList, ids.changedIds );
        }

        if ( ( mRecType & DELETED ) && ret ) {
            setStatus( "Creating dummys for deleted Events" );
            fakeEventListFromDevice( mEventList, ids.deletedIds );
        }

        if ( ( mRecType & UNCHANGED ) && ret ) {
            setStatus( "Reading unchanged Events" );
            ret = retrieveEventListFromDevice( mEventList, ids.unchangedIds );
        }

        return ret;
    }


    void EventHandler::insertIntoCalendarSyncee( KSync::EventSyncee *mCalendarSyncee, KCal::Event::List &list, int state )
    {
        for ( KCal::Event::List::Iterator it = list.begin(); it != list.end(); ++it ) {
            KSync::EventSyncEntry entry( *it, mCalendarSyncee );
            entry.setState( state );
            // TODO delete the cloned entries somewhere
            mCalendarSyncee->addEntry( entry.clone() );
        }
    }


    bool EventHandler::readSyncee( KSync::EventSyncee *mCalendarSyncee, bool firstSync )
    {
        bool ret = false;

        getIds();

        KCal::Event::List modifiedList;
        if ( firstSync ) {
            this->setMaximumSteps((ids.changedIds.size() + ids.unchangedIds.size()));
            if ( !getEventListFromDevice( modifiedList, PocketPCCommunication::UNCHANGED | PocketPCCommunication::CHANGED ) ) {
                setError("Can not retrieve unchanged Events from the Device");
                goto error;
            }
        } else {
            this->setMaximumSteps(ids.changedIds.size());
            if ( !getEventListFromDevice( modifiedList, PocketPCCommunication::CHANGED ) ) {
                setError("Can not retrieve changed Events from the Device");
                goto error;
            }

            KCal::Event::List removedList;
            if ( !getEventListFromDevice( removedList, PocketPCCommunication::DELETED ) ) {
                setError("Can not retrieve deleted Events from the Device");
                goto error;
            }
            insertIntoCalendarSyncee( mCalendarSyncee, removedList, KSync::SyncEntry::Removed );
        }
        insertIntoCalendarSyncee( mCalendarSyncee, modifiedList, KSync::SyncEntry::Modified );

        mCalendarSyncee->setTitle( "SynCEEvent" );
        mCalendarSyncee->setIdentifier( m_pdaName + "-Event" );

        ret = true;

    error:
        return ret;
    }


    void EventHandler::getEvents ( KCal::Event::List& p_events, KSync::SyncEntry::PtrList p_ptrList )
    {
        for ( KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin(); it != p_ptrList.end(); ++it ) {
            KSync::EventSyncEntry *cse = dynamic_cast<KSync::EventSyncEntry*>( *it );
            KCal::Event *event = dynamic_cast<KCal::Event*> ( cse->incidence() );
            if ( event ) {
                p_events.push_back ( event );
            }
        }
    }


    void EventHandler::getTodosAsFakedEvents( KCal::Event::List& p_events, KSync::SyncEntry::PtrList p_ptrList )
    {
        for ( KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin(); it != p_ptrList.end(); ++it ) {
            KSync::EventSyncEntry *cse = dynamic_cast<KSync::EventSyncEntry*>( *it );
            KCal::Todo *todo = dynamic_cast<KCal::Todo*> ( cse->incidence() );
            if ( todo ) {
                if ( mUidHelper->konnectorId( "SynCEEvent", todo->uid(), "---" ) != "---" ) {
                    KCal::Event * event = new KCal::Event(); // This event is never deleted yet ... memory whole ... FIXME
                    event->setUid( todo->uid() );
                    p_events.push_back ( event );
                }
            }
        }
    }


    bool EventHandler::addEvents( KCal::Event::List& p_eventList )
    {
        bool ret = true;
        KCal::ICalFormat calFormat;
        calFormat.setTimeZone( sCurrentTimeZone, false );

        RRA_Uint32Vector* added_ids = rra_uint32vector_new();

        if ( p_eventList.begin() == p_eventList.end() ) {
            goto finish;
        }

        for ( KCal::Event::List::Iterator it = p_eventList.begin();
                it != p_eventList.end(); ++it ) {
            incrementSteps();

            QString iCal = calFormat.toString( *it );
            iCal.stripWhiteSpace();
            iCal.replace( QRegExp( "END:VALARM\n" ), "END:VALARM" );

            kdDebug( 2120 ) << "Adding Event on Device: " << ( *it ) ->uid() << endl;

            uint32_t newObjectId = m_rra->putVEvent( iCal, mTypeId, 0 );
            if ( newObjectId == 0 ) {
                addErrorEntry((*it)->summary());
                ret = false;
            }

            m_rra->markIdUnchanged( mTypeId, newObjectId );

            mUidHelper->addId( "SynCEEvent",
                               "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ),
                               ( *it ) ->uid() );

            kdDebug( 2120 ) << "    ID-Pair: KDEID: " << ( *it ) ->uid() << " DeviceID: " <<
            "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ) << endl;

            rra_uint32vector_add( added_ids, newObjectId );

            KApplication::kApplication() ->processEvents();
        }

    finish:
        m_rra->registerAddedObjects( mTypeId, added_ids );
        rra_uint32vector_destroy( added_ids, true );

        return ret;
    }


    bool EventHandler::updateEvents ( KCal::Event::List& p_eventList )
    {
        bool ret = true;
        KCal::ICalFormat calFormat; // NEEDED FOR TODOS!!!
        calFormat.setTimeZone( sCurrentTimeZone, false );

        if ( p_eventList.begin() == p_eventList.end() ) {
            goto finish;
        }

        for ( KCal::Event::List::Iterator it = p_eventList.begin();
                it != p_eventList.end(); ++it ) {
            incrementSteps();
            QString kUid = mUidHelper->konnectorId( "SynCEEvent", ( *it ) ->uid(), "---" );

            if ( kUid != "---" ) {
                kdDebug( 2120 ) << "Updating Event on Device: " << "ID-Pair: KDEID: " <<
                ( *it ) ->uid() << " DeviceId: " << kUid << endl;
                QString iCal = calFormat.toString( *it );
                iCal.replace( QRegExp( "END:VALARM\n" ), "END:VALARM" );

                uint32_t retId = m_rra->putVEvent( iCal, mTypeId, getOriginalId( kUid ) );
                if ( retId == 0 ) {
                    addErrorEntry((*it)->summary());
                    ret = false;
                }

                m_rra->markIdUnchanged( mTypeId, getOriginalId( kUid ) );
            }

            KApplication::kApplication() ->processEvents();
        }

    finish:
        return ret;
    }


    bool EventHandler::removeEvents ( KCal::Event::List& p_eventList )
    {
//        int errorCount = 0;
        bool ret = false;
        RRA_Uint32Vector * deleted_ids = rra_uint32vector_new();

        if ( p_eventList.begin() == p_eventList.end() ) {
            goto success;
        }

        for ( KCal::Event::List::Iterator it = p_eventList.begin();
                it != p_eventList.end(); ++it ) {
            incrementSteps();
            QString kUid = mUidHelper->konnectorId( "SynCEEvent", ( *it ) ->uid(), "---" );

            if ( kUid != "---" ) {
                kdDebug( 2120 ) << "Removing Event on Device: " << "ID-Pair: KDEID: " <<
                ( *it ) ->uid() << " DeviceId: " << kUid << endl;

                if (!m_rra->deleteObject (mTypeId, getOriginalId( kUid ))) {
//                    if (errorCount++ == -1) {
//                        goto error;
//                    }
                }

                mUidHelper->removeId( "SynCEEvent", kUid );
                rra_uint32vector_add( deleted_ids, getOriginalId( kUid ) );
            }

            KApplication::kApplication() ->processEvents();
        }


    success:
        ret = true;

//    error:
        m_rra->removeDeletedObjects( mTypeId, deleted_ids );
        rra_uint32vector_destroy( deleted_ids, true );

        return ret;
    }


    bool EventHandler::writeSyncee( KSync::EventSyncee *mCalendarSyncee )
    {
        bool ret = true;

        if ( mCalendarSyncee->isValid() ) {
            KCal::Event::List eventAdded;
            KCal::Event::List eventRemoved;
            KCal::Event::List eventModified;

            setMaximumSteps(mCalendarSyncee->added().count() + mCalendarSyncee->removed().count() + mCalendarSyncee->modified().count());
            resetSteps();
            getEvents( eventAdded, mCalendarSyncee->added() );
            getEvents( eventRemoved, mCalendarSyncee->removed() );
            // This is a bad hack - but ksync provides deleted calendar-entries only as todos
            // So lets look if a removed "todo" is actually a removed event ...
            getTodosAsFakedEvents( eventRemoved, mCalendarSyncee->removed() );
            getEvents( eventModified, mCalendarSyncee->modified() );

            setStatus( "Writing added Events" );
            if (ret = addEvents( eventAdded )) {
                setStatus( "Erasing deleted Events" );
                if (ret = removeEvents( eventRemoved )) {
                    setStatus( "Writing changed Events" );
                    if (! (ret = updateEvents( eventModified ))) {
                        setError("Can not write back updated Events to the Device");
                    }
                } else {
                    setError("Can not erase deleted Events on the Device");
                }
            } else {
                setError("Can not added Events on the Device");
            }
        }

        return ret;
    }
}
