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
#include <kapplication.h>
#include <libkcal/icalformat.h>
#include <libkcal/event.h>
#include <libkcal/todo.h>
#include <libkcal/calendarnull.h>
#include <libkcal/calendarlocal.h>
#include <qfile.h>
#include <qregexp.h>

namespace pocketPCCommunication
{
    EventHandler::EventHandler( Rra* p_rra, QString mBaseDir, KSync::KonnectorUIDHelper *mUidHelper)
            : PimHandler( p_rra, mUidHelper )
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
    {}


    int EventHandler::retrieveEventListFromDevice(KCal::Event::List &mEventList, QValueList<uint32_t> &idList)
    {
        int count = 0;

        KCal::ICalFormat calFormat;
        calFormat.setTimeZone(sCurrentTimeZone, false);

        QString vCalBegin = "BEGIN:VCALENDAR\nPRODID:-//K Desktop Environment//NONSGML KOrganizer 3.2.1//EN\nVERSION:2.0\n";
        QString vCalEnd = "END:VCALENDAR\n";

        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            count++;

            kdDebug(2120) << "Retrieving Event from device: " << "RRA-ID-" +
                    QString::number ( *it, 16 ).rightJustify( 8, '0' ) << endl;

            QString vCal = vCalBegin + m_rra->getVEvent( mTypeId, *it ) + vCalEnd;

            // TODO delete the incidence created by fromString
            KCal::Incidence *incidence = calFormat.fromString (vCal);

            QString kdeId;
            if ((kdeId = mUidHelper->kdeId("SynCEEvent", incidence->uid(), "---")) != "---") {
                incidence->setUid(kdeId);
            } else {
                mUidHelper->addId("SynCEEvent", incidence->uid(), incidence->uid());
            }

            kdDebug(2120) << "    ID-Pair: KDEID: " << incidence->uid() << " DeviceID: " <<
                "RRA-ID-" + QString::number ( *it, 16 ).rightJustify( 8, '0' ) << endl;

            mEventList.push_back( dynamic_cast<KCal::Event*> (incidence) );

            KApplication::kApplication()->processEvents();
        }

        return count;
    }


    int EventHandler::fakeEventListFromDevice(KCal::Event::List &mEventList, QValueList<uint32_t> &idList)
    {
        int count = 0;

        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            count++;

            KCal::Event *event = new KCal::Event();

            QString konId = "RRA-ID-" + QString::number( *it, 16 ).rightJustify( 8, '0' );
            QString kdeId;

            if ((kdeId = mUidHelper->kdeId("SynCEEvent", konId, "---")) != "---") {

                kdDebug(2120) << "Faking Event for device: " << konId << endl;

                event->setUid(kdeId);
                mUidHelper->removeId("SynCEEvent", event->uid());
                kdDebug(2120) << "    ID-Pair: KDEID: " << event->uid() << " DeviceID: " << konId << endl;
                mEventList.push_back( event );
            }

        }

        return count;
    }


    bool EventHandler::getIds()
    {
        m_rra->getIdsForType( mTypeId, &ids );

        return true;
    }


    int EventHandler::getEventListFromDevice(KCal::Event::List &mEventList, int mRecType)
    {
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


    void EventHandler::insertIntoCalendarSyncee(KSync::EventSyncee *mCalendarSyncee, KCal::Event::List &list, int state)
    {
        for(KCal::Event::List::Iterator it = list.begin(); it != list.end(); ++it) {
            KSync::EventSyncEntry entry(*it, mCalendarSyncee);
            entry.setState(state);
            // TODO delete the cloned entries somewhere
            mCalendarSyncee->addEntry(entry.clone());
        }
    }


    bool EventHandler::readSyncee(KSync::EventSyncee *mCalendarSyncee, bool firstSync)
    {
        getIds();

        KCal::Event::List modifiedList;
        if (firstSync) {
            if (getEventListFromDevice(modifiedList, pocketPCCommunication::UNCHANGED | pocketPCCommunication::CHANGED) < 0) {
                return false;
            }
        } else {
            if (getEventListFromDevice(modifiedList, pocketPCCommunication::CHANGED) < 0) {
                return false;
            }

            KCal::Event::List removedList;
            if (getEventListFromDevice(removedList, pocketPCCommunication::DELETED) < 0) {
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
        for (KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin(); it != p_ptrList.end(); ++it ) {
            KSync::EventSyncEntry *cse = dynamic_cast<KSync::EventSyncEntry*>( *it );
            KCal::Event *event = dynamic_cast<KCal::Event*> (cse->incidence() );
            if (event) {
                p_events.push_back ( event );
            }
        }
    }


    void EventHandler::getTodosAsFakedEvents(KCal::Event::List& p_events, KSync::SyncEntry::PtrList p_ptrList )
    {
        for (KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin(); it != p_ptrList.end(); ++it ) {
            KSync::EventSyncEntry *cse = dynamic_cast<KSync::EventSyncEntry*>( *it );
            KCal::Todo *todo = dynamic_cast<KCal::Todo*> (cse->incidence() );
            if (todo) {
                if (mUidHelper->konnectorId("SynCEEvent", todo->uid(), "---") != "---") {
                    KCal::Event *event = new KCal::Event(); // This event is never deleted yet ... memory whole ... FIXME
                    event->setUid(todo->uid());
                    p_events.push_back ( event );
                }
            }
        }
    }


    void EventHandler::addEvents(KCal::Event::List& p_eventList)
    {
        RRA_Uint32Vector* added_ids = rra_uint32vector_new();

        if ( p_eventList.begin() == p_eventList.end() ) {
            rra_uint32vector_destroy(added_ids, true);
            return ;
        }

        KCal::ICalFormat calFormat;
        calFormat.setTimeZone(sCurrentTimeZone, false);

        for (KCal::Event::List::Iterator it = p_eventList.begin();
                it != p_eventList.end(); ++it ) {

            QString iCal = calFormat.toString(*it);
            iCal.stripWhiteSpace();
            iCal.replace(QRegExp("END:VALARM\n"), "END:VALARM");

            kdDebug(2120) << "Adding Event on Device: " << (*it)->uid() << endl;

            uint32_t newObjectId = m_rra->putVEvent( iCal, mTypeId, 0 );
            m_rra->markIdUnchanged( mTypeId, newObjectId );

            mUidHelper->addId("SynCEEvent",
                "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ),
                (*it)->uid());

            kdDebug(2120) << "    ID-Pair: KDEID: " << (*it)->uid() << " DeviceID: " <<
                "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ) << endl;

            rra_uint32vector_add(added_ids, newObjectId);

            KApplication::kApplication()->processEvents();
        }
        m_rra->registerAddedObjects(mTypeId, added_ids);

        rra_uint32vector_destroy(added_ids, true);
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
                kdDebug(2120) << "Updating Event on Device: " << "ID-Pair: KDEID: " <<
                    (*it)->uid() << " DeviceId: " << kUid << endl;
                QString iCal = calFormat.toString(*it);
                iCal.replace(QRegExp("END:VALARM\n"), "END:VALARM");

                m_rra->putVEvent( iCal, mTypeId, getOriginalId( kUid ) );
                m_rra->markIdUnchanged( mTypeId, getOriginalId( kUid ) );
            }

            KApplication::kApplication()->processEvents();
        }
    }


    void EventHandler::removeEvents (KCal::Event::List& p_eventList)
    {
        RRA_Uint32Vector* deleted_ids = rra_uint32vector_new();

        if ( p_eventList.begin() == p_eventList.end() )
            return ;

        for (KCal::Event::List::Iterator it = p_eventList.begin();
                it != p_eventList.end(); ++it ) {
            QString kUid = mUidHelper->konnectorId("SynCEEvent", (*it)->uid(), "---");

            if (kUid != "---") {
                kdDebug(2120) << "Removing Event on Device: " << "ID-Pair: KDEID: " <<
                    (*it)->uid() << " DeviceId: " << kUid << endl;
                deleteSingleEntry ( mTypeId, getOriginalId( kUid ) );
                mUidHelper->removeId("SynCEEvent", kUid);
                rra_uint32vector_add(deleted_ids, getOriginalId( kUid ));
            }

            KApplication::kApplication()->processEvents();
        }

        m_rra->removeDeletedObjects(mTypeId, deleted_ids);

        rra_uint32vector_destroy(deleted_ids, true);
    }


    bool EventHandler::writeSyncee(KSync::EventSyncee *mCalendarSyncee)
    {
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
}
