//
// C++ Implementation: todohandler
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "todohandler.h"

#include <kdebug.h>
#include <libkcal/icalformat.h>
#include <libkcal/event.h>
#include <libkcal/calendarnull.h>
#include <libkcal/calendarlocal.h>
#include <qfile.h>

namespace pocketPCCommunication
{
    TodoHandler::TodoHandler( KSharedPtr<Rra> p_rra, QString mBaseDir, KSync::KonnectorUIDHelper *mUidHelper )
            : PimHandler( p_rra )
    {
        initialized = false;
        mTypeId = 0;
        this->mBaseDir = mBaseDir;
        this->mUidHelper = mUidHelper;
    }


    bool TodoHandler::init()
    {
        mTypeId = m_rra->getTypeForName( RRA_SYNCMGR_TYPE_TASK );

        return initialized = mTypeId != 0;
    }


    TodoHandler::~TodoHandler()
    {
        mUidHelper->save();
    }


    int TodoHandler::retrieveTodoListFromDevice(KCal::Todo::List &mTodoList, QValueList<uint32_t> &idList)
    {
        int count = 0;

        KCal::ICalFormat calFormat; // NEEDED FOR TODOS!!!

        QString vCalBegin = "BEGIN:VCALENDAR\nPRODID:-//K Desktop Environment//NONSGML KOrganizer 3.2.1//EN\nVERSION:2.0\n";
        QString vCalEnd = "END:VCALENDAR\n";

        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            count++;
            kdDebug( 2120 ) << " ||| " << endl;

            QString vCal = vCalBegin + m_rra->getVToDo( mTypeId, *it ) + vCalEnd;

            m_rra->markIdUnchanged( mTypeId, *it );

            KCal::Incidence *incidence = calFormat.fromString (vCal);

            QString kdeId;

            if ((kdeId = mUidHelper->kdeId("SynCETodo", incidence->uid(), "---")) != "---") {
                incidence->setUid(kdeId);
            } else {
                mUidHelper->addId("SynCETodo", incidence->uid(), incidence->uid());
            }

            mTodoList.push_back( dynamic_cast<KCal::Todo*> (incidence) );
        }

        return count;
    }


    int TodoHandler::fakeTodoListFromDevice(KCal::Todo::List &mTodoList, QValueList<uint32_t> &idList)
    {
        int count = 0;

        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            count++;
            kdDebug( 2120 ) << " &&& " << endl;
            KCal::Todo *todo = new KCal::Todo();

            QString konId = "RRA-ID-" + QString::number( *it, 16 ).rightJustify( 8, '0' );
            QString kdeId;

            if ((kdeId = mUidHelper->kdeId("SynCETodo", konId, "---")) != "---") {
                todo->setUid(kdeId);
                mUidHelper->removeId("SynCETodo", todo->uid());
                mTodoList.push_back( todo );
            }

        }

        return count;
    }


    bool TodoHandler::getIds()
    {
        if ( !m_rra->getIds( mTypeId, &ids ) ) {
            kdDebug( 2120 ) << "TodoHandler::getIds: could not get the ids.. :(" << endl;
            return false;
        }

        return true;
    }


    int TodoHandler::getTodoListFromDevice(KCal::Todo::List &mTodoList, int mRecType)
    {
        kdDebug( 2120 ) << "[TodoHandler]: got ids.. fetching information" << endl;

        QValueList<uint32_t>::const_iterator begin;
        QValueList<uint32_t>::const_iterator end;

        int count = 0;
        int ret = 0;

        if ( ( mRecType & CHANGED ) && ( ret >= 0 ) ) {
            ret = retrieveTodoListFromDevice( mTodoList, ids.changedIds );
            if ( ret >= 0 ) {
                count += 0;
            }
        }

        if ( ( mRecType & DELETED ) && ( ret >= 0 ) ) {
            ret = fakeTodoListFromDevice( mTodoList, ids.deletedIds );
            if ( ret >= 0 ) {
                count += 0;
            }
        }

        if ( ( mRecType & UNCHANGED ) && ( ret >= 0 ) ) {
            ret = fakeTodoListFromDevice( mTodoList, ids.unchangedIds );
            if ( ret >= 0 ) {
                count += 0;
            }
        }

        if ( ret < 0 ) {
            return -count - 1;
        }

        return count;
    }


    void TodoHandler::insertIntoCalendarSyncee(KSync::CalendarSyncee *mCalendarSyncee, KCal::Todo::List &list, int state)
    {
        kdDebug(2120) << "Begin Inserting into TodoSyncee State: " << state << endl;
        for(KCal::Todo::List::Iterator it = list.begin(); it != list.end(); ++it) {
            KSync::CalendarSyncEntry entry(*it, mCalendarSyncee);
            entry.setState(state);
            mCalendarSyncee->addEntry(entry.clone());
        }
        kdDebug(2120) << "End Inserting into TodoSyncee" << endl;
    }


    bool TodoHandler::readSyncee(KSync::CalendarSyncee *mCalendarSyncee, bool firstSync)
    {
        if (!initialized) {
            if (!init()) {
                kdDebug(2120) << "Could not initialize TodoHandler" << endl;
//              emit synceeReadError(this);
                return false;
            }
        }

        if (!getIds()) {
            kdDebug(2120) << "Could not retriev Todo-IDs" << endl;
//            emit synceeReadError(this);
            return false;
        }

        KCal::Todo::List modifiedList;
        if (firstSync) {
            if (getTodoListFromDevice(modifiedList, pocketPCCommunication::UNCHANGED | pocketPCCommunication::CHANGED) < 0) {
//                emit synceeReadError(this);
                return false;
            }
        } else {
            if (getTodoListFromDevice(modifiedList, pocketPCCommunication::CHANGED) < 0) {
//                emit synceeReadError(this);
                return false;
            }

            KCal::Todo::List removedList;
            if (getTodoListFromDevice(removedList, pocketPCCommunication::DELETED) < 0) {
//                emit synceeReadError(this);
                return false;
            }
            insertIntoCalendarSyncee(mCalendarSyncee, removedList, KSync::SyncEntry::Removed);
        }
        insertIntoCalendarSyncee(mCalendarSyncee, modifiedList, KSync::SyncEntry::Modified);

        mCalendarSyncee->setTitle("SynCETodo");
        mCalendarSyncee->setIdentifier(m_pdaName + "-Todo");

        return true;
    }


    void TodoHandler::getTodos (KCal::Todo::List& p_todos, KSync::SyncEntry::PtrList p_ptrList )
    {
        kdDebug( 2120 ) << "getTodo: " << endl;

        for (KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin(); it != p_ptrList.end(); ++it ) {
            KSync::CalendarSyncEntry *cse = dynamic_cast<KSync::CalendarSyncEntry*>( *it );
            KCal::Todo *todo = dynamic_cast<KCal::Todo*> (cse->incidence() );
            if (todo) {
                p_todos.push_back ( todo );
                kdDebug( 2120 ) << "     " << ( dynamic_cast<KSync::CalendarSyncEntry*>( *it ) ) ->id() << endl;
            }
        }
    }


    void TodoHandler::getTodosAsFakedTodos(KCal::Todo::List& p_todos, KSync::SyncEntry::PtrList p_ptrList )
    {
        kdDebug( 2120 ) << "getTodo: " << endl;

        for (KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin(); it != p_ptrList.end(); ++it ) {
            KSync::CalendarSyncEntry *cse = dynamic_cast<KSync::CalendarSyncEntry*>( *it );
            KCal::Todo *todo = dynamic_cast<KCal::Todo*> (cse->incidence() );
            if (todo) {
                if (mUidHelper->konnectorId("SynCETodo", todo->uid(), "---") != "---") {
                    p_todos.push_back ( todo );
                    kdDebug( 2120 ) << "     " << ( dynamic_cast<KSync::CalendarSyncEntry*>( *it ) ) ->id() << endl;
                }
            }
        }
    }


    void TodoHandler::addTodos(KCal::Todo::List& p_todoList)
    {
        if ( p_todoList.begin() == p_todoList.end() )
            return ;

        KCal::ICalFormat calFormat; // NEEDED FOR TODOS!!!

        for (KCal::Todo::List::Iterator it = p_todoList.begin();
                it != p_todoList.end(); ++it ) {

            QString iCal = calFormat.toString(*it);

            uint32_t newObjectId = m_rra->putVToDo( iCal, mTypeId, 0 );

            mUidHelper->addId("SynCETodo",
                "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ),
                (*it)->uid());
        }
    }


    void TodoHandler::updateTodos (KCal::Todo::List& p_todoList)
    {
        if ( p_todoList.begin() == p_todoList.end() )
            return ;

        KCal::ICalFormat calFormat; // NEEDED FOR TODOS!!!


        for (KCal::Todo::List::Iterator it = p_todoList.begin();
                it != p_todoList.end(); ++it ) {
            QString kUid = mUidHelper->konnectorId("SynCETodo", (*it)->uid(), "---");

            if (kUid != "---") {
                QString iCal = calFormat.toString(*it);
                m_rra->putVToDo( iCal, mTypeId, getOriginalId( kUid ) );
            }
        }
    }


    void TodoHandler::removeTodos (KCal::Todo::List& p_todoList)
    {
        if ( p_todoList.begin() == p_todoList.end() )
            return ;

        for (KCal::Todo::List::Iterator it = p_todoList.begin();
                it != p_todoList.end(); ++it ) {
            QString kUid = mUidHelper->konnectorId("SynCETodo", (*it)->uid(), "---");

            if (kUid != "---") {
                deleteSingleEntry ( mTypeId, getOriginalId( kUid ) );
                mUidHelper->removeId("SynCETodo", kUid);
            }
        }
    }


    bool TodoHandler::writeSyncee(KSync::CalendarSyncee *mCalendarSyncee)
    {
        if (!initialized) {
            if (!init()) {
                kdDebug(2120) << "Could not initialize TodoHandler" << endl;
//              emit synceeReadError(this);
                return false;
            }
        }

        if ( mCalendarSyncee->isValid() ) {
            KCal::Todo::List todoAdded;
            KCal::Todo::List todoRemoved;
            KCal::Todo::List todoModified;

            getTodos( todoAdded, mCalendarSyncee->added() );
            // This is a bad hack - but ksync provides deleted calendar-entries only as todos
            // So lets look if a removed "todo" is actually a removed todo ...
            getTodosAsFakedTodos( todoRemoved, mCalendarSyncee->removed() );
            getTodos( todoModified, mCalendarSyncee->modified() );

            addTodos( todoAdded );
            removeTodos( todoRemoved );
            updateTodos( todoModified );
        }

        return true;
    }


    bool TodoHandler::connectDevice()
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
    bool TodoHandler::disconnectDevice()
    {
        m_rra->disconnect();

        mUidHelper->save();

        return true;
    }
}
