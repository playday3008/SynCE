//
// C++ Implementation: eventhandler
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "eventhandler.h"

#include <kdebug.h>
#include <libkcal/icalformat.h>
#include <libkcal/event.h>
#include <libkcal/todo.h>
#include <libkcal/calendarnull.h>
#include <libkcal/calendarlocal.h>
#include <qfile.h>
#include <qregexp.h>

namespace pocketPCCommunication
{
    EventHandler::EventHandler( KSharedPtr<Rra> p_rra, QString mBaseDir, KSync::KonnectorUIDHelper *mUidHelper )
            : PimHandler( p_rra )
    {
        initialized = false;
        mTypeId = 0;
        this->mBaseDir = mBaseDir;
        this->mUidHelper = mUidHelper;

        QFile f("/etc/timezone");
        if(f.open(IO_ReadOnly)) {
            QTextStream ts(&f);
            ts >> sCurrentTimeZone;
        }
        f.close();
    }


    bool EventHandler::init()
    {
        mTypeId = m_rra->getTypeForName( RRA_SYNCMGR_TYPE_APPOINTMENT );

        return initialized = mTypeId != 0;
    }


    EventHandler::~EventHandler()
    {
        mUidHelper->save();
    }


    int EventHandler::retrieveEventListFromDevice(KCal::Event::List &mEventList, QValueList<uint32_t> &idList)
    {
        int count = 0;

        KCal::ICalFormat calFormat; // NEEDED FOR TODOS!!!
        calFormat.setTimeZone(sCurrentTimeZone, false);

        QString vCalBegin = "BEGIN:VCALENDAR\nPRODID:-//K Desktop Environment//NONSGML KOrganizer 3.2.1//EN\nVERSION:2.0\n";
        QString vCalEnd = "END:VCALENDAR\n";

        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            count++;
            kdDebug( 2120 ) << " ||| " << endl;

            QString vCal = vCalBegin + m_rra->getVEvent( mTypeId, *it ) + vCalEnd;

            m_rra->markIdUnchanged( mTypeId, *it );

            KCal::Incidence *incidence = calFormat.fromString (vCal);

            QString kdeId;
            if ((kdeId = mUidHelper->kdeId("SynCEEvent", incidence->uid(), "---")) != "---") {
                incidence->setUid(kdeId);
            } else {
                mUidHelper->addId("SynCEEvent", incidence->uid(), incidence->uid());
            }

            mEventList.push_back( dynamic_cast<KCal::Event*> (incidence) );
        }

        return count;
    }


    int EventHandler::fakeEventListFromDevice(KCal::Event::List &mEventList, QValueList<uint32_t> &idList)
    {
        int count = 0;

        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            count++;
            kdDebug( 2120 ) << " &&& " << endl;
            KCal::Event *event = new KCal::Event();

            QString konId = "RRA-ID-" + QString::number( *it, 16 ).rightJustify( 8, '0' );
            QString kdeId;

            if ((kdeId = mUidHelper->kdeId("SynCEEvent", konId, "---")) != "---") {
                event->setUid(kdeId);
                mUidHelper->removeId("SynCEEvent", event->uid());
                mEventList.push_back( event );
            }

        }

        return count;
    }


    bool EventHandler::getIds()
    {
        if ( !m_rra->getIds( mTypeId, &ids ) ) {
            kdDebug( 2120 ) << "EventHandler::getIds: could not get the ids.. :(" << endl;
            return false;
        }

        return true;
    }


    int EventHandler::getEventListFromDevice(KCal::Event::List &mEventList, int mRecType)
    {
        kdDebug( 2120 ) << "[EventHandler]: got ids.. fetching information" << endl;

        QValueList<uint32_t>::const_iterator begin;
        QValueList<uint32_t>::const_iterator end;

        int count = 0;
        int ret = 0;

        if ( ( mRecType & CHANGED ) && ( ret >= 0 ) ) {
            ret = retrieveEventListFromDevice( mEventList, ids.changedIds );
            if ( ret >= 0 ) {
                count += 0;
            }
        }

        if ( ( mRecType & DELETED ) && ( ret >= 0 ) ) {
            ret = fakeEventListFromDevice( mEventList, ids.deletedIds );
            if ( ret >= 0 ) {
                count += 0;
            }
        }

        if ( ( mRecType & UNCHANGED ) && ( ret >= 0 ) ) {
            ret = retrieveEventListFromDevice( mEventList, ids.unchangedIds );
            if ( ret >= 0 ) {
                count += 0;
            }
        }

        if ( ret < 0 ) {
            return -count - 1;
        }

        return count;
    }


    void EventHandler::insertIntoCalendarSyncee(KSync::CalendarSyncee *mCalendarSyncee, KCal::Event::List &list, int state)
    {
        kdDebug(2120) << "Begin Inserting into EventSyncee State: " << state << endl;
        for(KCal::Event::List::Iterator it = list.begin(); it != list.end(); ++it) {
            KSync::CalendarSyncEntry entry(*it, mCalendarSyncee);
            entry.setState(state);
            mCalendarSyncee->addEntry(entry.clone());
        }
        kdDebug(2120) << "End Inserting into EventSyncee" << endl;
    }


    bool EventHandler::readSyncee(KSync::CalendarSyncee *mCalendarSyncee, bool firstSync)
    {
        if (!initialized) {
            if (!init()) {
                kdDebug(2120) << "Could not initialize EventHandler" << endl;
//              emit synceeReadError(this);
                return false;
            }
        }

        if (!getIds()) {
            kdDebug(2120) << "Could not retriev Event-IDs" << endl;
//            emit synceeReadError(this);
            return false;
        }

        KCal::Event::List modifiedList;
        if (firstSync) {
            if (getEventListFromDevice(modifiedList, pocketPCCommunication::UNCHANGED | pocketPCCommunication::CHANGED) < 0) {
//                emit synceeReadError(this);
                return false;
            }
        } else {
            if (getEventListFromDevice(modifiedList, pocketPCCommunication::CHANGED) < 0) {
//                emit synceeReadError(this);
                return false;
            }

            KCal::Event::List removedList;
            if (getEventListFromDevice(removedList, pocketPCCommunication::DELETED) < 0) {
//                emit synceeReadError(this);
                return false;
            }
            insertIntoCalendarSyncee(mCalendarSyncee, removedList, KSync::SyncEntry::Removed);
        }
        insertIntoCalendarSyncee(mCalendarSyncee, modifiedList, KSync::SyncEntry::Modified);

        mCalendarSyncee->setTitle("SynCEEvent");
        mCalendarSyncee->setIdentifier(m_pdaName + "-Event");

        return true;
    }


    void EventHandler::getEvents (KCal::Event::List& p_events, KSync::SyncEntry::PtrList p_ptrList )
    {
        kdDebug( 2120 ) << "getEvent: " << endl;

        for (KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin(); it != p_ptrList.end(); ++it ) {
            KSync::CalendarSyncEntry *cse = dynamic_cast<KSync::CalendarSyncEntry*>( *it );
            KCal::Event *event = dynamic_cast<KCal::Event*> (cse->incidence() );
            if (event) {
                p_events.push_back ( event );
                kdDebug( 2120 ) << "     " << ( dynamic_cast<KSync::CalendarSyncEntry*>( *it ) ) ->id() << endl;
            }
        }
    }


    void EventHandler::getTodosAsFakedEvents(KCal::Event::List& p_events, KSync::SyncEntry::PtrList p_ptrList )
    {
        kdDebug(2120) << "getTodosAsEvents: " << endl;

        for (KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin(); it != p_ptrList.end(); ++it ) {
            KSync::CalendarSyncEntry *cse = dynamic_cast<KSync::CalendarSyncEntry*>( *it );
            KCal::Todo *todo = dynamic_cast<KCal::Todo*> (cse->incidence() );
            if (todo) {
                if (mUidHelper->konnectorId("SynCEEvent", todo->uid(), "---") != "---") {
                    KCal::Event *event = new KCal::Event(); // This event is never deleted yet ... memory whole ... FIXME
                    event->setUid(todo->uid());
                    p_events.push_back ( event );
                    kdDebug( 2120 ) << "     " << ( dynamic_cast<KSync::CalendarSyncEntry*>( *it ) ) ->id() << endl;
                }
            }
        }
    }


    void EventHandler::addEvents(KCal::Event::List& p_eventList)
    {
        if ( p_eventList.begin() == p_eventList.end() )
            return ;

        KCal::ICalFormat calFormat; // NEEDED FOR TODOS!!!
        calFormat.setTimeZone(sCurrentTimeZone, false);

        for (KCal::Event::List::Iterator it = p_eventList.begin();
                it != p_eventList.end(); ++it ) {

            QString iCal = calFormat.toString(*it);
            iCal.stripWhiteSpace();
            iCal.replace(QRegExp("END:VALARM\n"), "END:VALARM");

            uint32_t newObjectId = m_rra->putVEvent( iCal, mTypeId, 0 );

            mUidHelper->addId("SynCEEvent",
                "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ),
                (*it)->uid());
        }
    }


    void EventHandler::updateEvents (KCal::Event::List& p_eventList)
    {
        if ( p_eventList.begin() == p_eventList.end() )
            return ;

        KCal::ICalFormat calFormat; // NEEDED FOR TODOS!!!
        calFormat.setTimeZone(sCurrentTimeZone, false);

        for (KCal::Event::List::Iterator it = p_eventList.begin();
                it != p_eventList.end(); ++it ) {
            QString kUid = mUidHelper->konnectorId("SynCEEvent", (*it)->uid(), "---");

            if (kUid != "---") {
                QString iCal = calFormat.toString(*it);
                iCal.replace(QRegExp("END:VALARM\n"), "END:VALARM");

                m_rra->putVEvent( iCal, mTypeId, getOriginalId( kUid ) );
            }
        }
    }


    void EventHandler::removeEvents (KCal::Event::List& p_eventList)
    {
        if ( p_eventList.begin() == p_eventList.end() )
            return ;

        for (KCal::Event::List::Iterator it = p_eventList.begin();
                it != p_eventList.end(); ++it ) {
            QString kUid = mUidHelper->konnectorId("SynCEEvent", (*it)->uid(), "---");

            if (kUid != "---") {
                deleteSingleEntry ( mTypeId, getOriginalId( kUid ) );
                mUidHelper->removeId("SynCEEvent", kUid);
            }
        }
    }


    bool EventHandler::writeSyncee(KSync::CalendarSyncee *mCalendarSyncee)
    {
        if (!initialized) {
            if (!init()) {
                kdDebug(2120) << "Could not initialize EventHandler" << endl;
//              emit synceeReadError(this);
                return false;
            }
        }

        if ( mCalendarSyncee->isValid() ) {
            KCal::Event::List eventAdded;
            KCal::Event::List eventRemoved;
            KCal::Event::List eventModified;

            getEvents( eventAdded, mCalendarSyncee->added() );
            getEvents( eventRemoved, mCalendarSyncee->removed() );
            // This is a bad hack - but ksync provides deleted calendar-entries only as todos
            // So lets look if a removed "todo" is actually a removed event ...
            getTodosAsFakedEvents( eventRemoved, mCalendarSyncee->removed() );
            getEvents( eventModified, mCalendarSyncee->modified() );

            addEvents( eventAdded );
            removeEvents( eventRemoved );
            updateEvents( eventModified );
        }

        return true;
    }


    bool EventHandler::connectDevice()
    {
        if ( !m_rra->connect() ) {
            kdDebug( 2120 ) << "PocketPCKonnector: could not connect to device!" << endl;
            return false;
        } else {
            kdDebug( 2120 ) << "PocketPCKonnector: connected to device!" << endl;;
        }

        return true;
    }


    /** Disconnect the device.
     * @see KSync::Konnector::disconnectDevice()
     * @return true if device can be disconnect. false otherwise
     */
    bool EventHandler::disconnectDevice()
    {
        m_rra->disconnect();

        mUidHelper->save();

        return true;
    }

}
