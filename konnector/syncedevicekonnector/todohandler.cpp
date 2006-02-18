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

#include "todohandler.h"

#include <kdebug.h>
#include <kapplication.h>
#include <libkcal/icalformat.h>
#include <libkcal/event.h>
#include <libkcal/calendarnull.h>
#include <libkcal/calendarlocal.h>
#include <qfile.h>

namespace PocketPCCommunication
{
    TodoHandler::TodoHandler() : PimHandler()
    {
        initialized = false;
        mTypeId = 0;
    }


    bool TodoHandler::init()
    {
        mTypeId = m_rra->getTypeForName( RRA_SYNCMGR_TYPE_TASK );

        return initialized = mTypeId != 0;
    }


    TodoHandler::~TodoHandler()
    {}


    bool TodoHandler::retrieveTodoListFromDevice( KCal::Todo::List &mTodoList, QValueList<uint32_t> &idList )
    {
        KCal::ICalFormat calFormat; // NEEDED FOR TODOS!!!
        bool ret = false;

        QString vCalBegin = "BEGIN:VCALENDAR\nPRODID:-//K Desktop Environment//NONSGML KOrganizer 3.2.1//EN\nVERSION:2.0\n";
        QString vCalEnd = "END:VCALENDAR\n";

        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            incrementSteps();

            kdDebug( 2120 ) << "Retrieving Todo from device: " << "RRA-ID-" +
            QString::number ( *it, 16 ).rightJustify( 8, '0' ) << endl;

            QString vTodo = m_rra->getVToDo( mTypeId, *it );
            if ( vTodo.isEmpty() ) {
                goto error;
            }

            QString vCal = vCalBegin + vTodo + vCalEnd;

            // TOTO delete incidence created fromString somewhere
            KCal::Incidence *incidence = calFormat.fromString ( vCal );

            QString kdeId;

            if ( ( kdeId = mUidHelper->kdeId( "SynCETodo", incidence->uid(), "---" ) ) != "---" ) {
                incidence->setUid( kdeId );
            } else {
                mUidHelper->addId( "SynCETodo", incidence->uid(), incidence->uid() );
            }

            kdDebug( 2120 ) << "    ID-Pair: KDEID: " << incidence->uid() << " DeviceID: " <<
            "RRA-ID-" + QString::number ( *it, 16 ).rightJustify( 8, '0' ) << endl;

            mTodoList.push_back( dynamic_cast<KCal::Todo*> ( incidence ) );

            KApplication::kApplication() ->processEvents();
        }

        ret = true;

    error:
        return ret;
    }


    void TodoHandler::fakeTodoListFromDevice( KCal::Todo::List &mTodoList, QValueList<uint32_t> &idList )
    {
        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            KCal::Todo *todo = new KCal::Todo();

            QString konId = "RRA-ID-" + QString::number( *it, 16 ).rightJustify( 8, '0' );
            QString kdeId;

            if ( ( kdeId = mUidHelper->kdeId( "SynCETodo", konId, "---" ) ) != "---" ) {

                kdDebug( 2120 ) << "Faking Todo for device: " << konId << endl;

                todo->setUid( kdeId );
                mUidHelper->removeId( "SynCETodo", todo->uid() );
                kdDebug( 2120 ) << "    ID-Pair: KDEID: " << todo->uid() << " DeviceID: " << konId << endl;
                mTodoList.push_back( todo );
            }
        }
    }


    bool TodoHandler::getIds()
    {
        m_rra->getIdsForType( mTypeId, &ids );

        return true;
    }


    int TodoHandler::getTodoListFromDevice( KCal::Todo::List &mTodoList, int mRecType )
    {
        bool ret = true;

        if ( ( mRecType & CHANGED ) ) {
            setStatus( "Reading changed Todos" );
            ret = retrieveTodoListFromDevice( mTodoList, ids.changedIds );
        }

        if ( ( mRecType & DELETED ) && ret ) {
            setStatus( "Creating dummys for deleted Todos" );
            fakeTodoListFromDevice( mTodoList, ids.deletedIds );
        }

        if ( ( mRecType & UNCHANGED ) && ret ) {
            setStatus( "Reading unchanged Todos" );
            ret = retrieveTodoListFromDevice( mTodoList, ids.unchangedIds );
        }

        return ret;
    }


    void TodoHandler::insertIntoCalendarSyncee( KSync::TodoSyncee *mCalendarSyncee, KCal::Todo::List &list, int state )
    {
        for ( KCal::Todo::List::Iterator it = list.begin(); it != list.end(); ++it ) {
            KSync::TodoSyncEntry entry( *it, mCalendarSyncee );
            entry.setState( state );
            mCalendarSyncee->addEntry( entry.clone() );
        }
    }


    bool TodoHandler::readSyncee( KSync::TodoSyncee *mCalendarSyncee, bool firstSync )
    {
        bool ret = false;

        getIds();

        KCal::Todo::List modifiedList;
        if ( firstSync ) {
            this->setMaximumSteps((ids.changedIds.size() + ids.unchangedIds.size()));
            if ( !getTodoListFromDevice( modifiedList, PocketPCCommunication::UNCHANGED | PocketPCCommunication::CHANGED ) ) {
                goto error;
            }
        } else {
            this->setMaximumSteps(ids.changedIds.size());
            if ( !getTodoListFromDevice( modifiedList, PocketPCCommunication::CHANGED ) ) {
                goto error;
            }

            KCal::Todo::List removedList;
            if ( !getTodoListFromDevice( removedList, PocketPCCommunication::DELETED ) ) {
                goto error;
            }
            insertIntoCalendarSyncee( mCalendarSyncee, removedList, KSync::SyncEntry::Removed );
        }
        insertIntoCalendarSyncee( mCalendarSyncee, modifiedList, KSync::SyncEntry::Modified );

        mCalendarSyncee->setTitle( "SynCETodo" );
        mCalendarSyncee->setIdentifier( m_pdaName + "-Todo" );

        ret = true;

    error:
        return ret;
    }


    void TodoHandler::getTodos ( KCal::Todo::List& p_todos, KSync::SyncEntry::PtrList p_ptrList )
    {
        for ( KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin(); it != p_ptrList.end(); ++it ) {
            KSync::TodoSyncEntry *cse = dynamic_cast<KSync::TodoSyncEntry*>( *it );
            KCal::Todo *todo = dynamic_cast<KCal::Todo*> ( cse->incidence() );
            if ( todo ) {
                p_todos.push_back ( todo );
            }
        }
    }


    void TodoHandler::getTodosAsFakedTodos( KCal::Todo::List& p_todos, KSync::SyncEntry::PtrList p_ptrList )
    {
        for ( KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin(); it != p_ptrList.end(); ++it ) {
            KSync::TodoSyncEntry *cse = dynamic_cast<KSync::TodoSyncEntry*>( *it );
            KCal::Todo *todo = dynamic_cast<KCal::Todo*> ( cse->incidence() );
            if ( todo ) {
                if ( mUidHelper->konnectorId( "SynCETodo", todo->uid(), "---" ) != "---" ) {
                    p_todos.push_back ( todo );
                }
            }
        }
    }


    bool TodoHandler::addTodos( KCal::Todo::List& p_todoList )
    {
        bool ret = false;
        RRA_Uint32Vector * added_ids = rra_uint32vector_new();
        KCal::ICalFormat calFormat;

        if ( p_todoList.begin() == p_todoList.end() ) {
            goto success;
        }

        for ( KCal::Todo::List::Iterator it = p_todoList.begin();
                it != p_todoList.end(); ++it ) {
            incrementSteps();

            QString iCal = calFormat.toString( *it );

            kdDebug( 2120 ) << "Adding Todo on Device: " << ( *it ) ->uid() << endl;

            uint32_t newObjectId = m_rra->putVToDo( iCal, mTypeId, 0 );
            if ( newObjectId == 0 ) {
                goto error;
            }

            m_rra->markIdUnchanged( mTypeId, newObjectId );

            mUidHelper->addId( "SynCETodo",
                               "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ),
                               ( *it ) ->uid() );

            kdDebug( 2120 ) << "    ID-Pair: KDEID: " << ( *it ) ->uid() << " DeviceID: " <<
            "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ) << endl;

            rra_uint32vector_add( added_ids, newObjectId );

            KApplication::kApplication() ->processEvents();
        }

    success:
        ret = true;

    error:
        m_rra->registerAddedObjects( mTypeId, added_ids );
        rra_uint32vector_destroy( added_ids, true );

        return ret;
    }


    bool TodoHandler::updateTodos ( KCal::Todo::List& p_todoList )
    {
        bool ret = false;
        KCal::ICalFormat calFormat; // NEEDED FOR TODOS!!!

        if ( p_todoList.begin() == p_todoList.end() ) {
            goto success;
        }

        for ( KCal::Todo::List::Iterator it = p_todoList.begin();
                it != p_todoList.end(); ++it ) {
            incrementSteps();
            QString kUid = mUidHelper->konnectorId( "SynCETodo", ( *it ) ->uid(), "---" );

            if ( kUid != "---" ) {
                kdDebug( 2120 ) << "Updating Todo on Device: " << "ID-Pair: KDEID: " <<
                ( *it ) ->uid() << " DeviceId: " << kUid << endl;
                QString iCal = calFormat.toString( *it );
                uint32_t retId = m_rra->putVToDo( iCal, mTypeId, getOriginalId( kUid ) );
                if (retId == 0) {
                    goto error;
                }

                m_rra->markIdUnchanged( mTypeId, getOriginalId( kUid ) );
            }

            KApplication::kApplication() ->processEvents();
        }

success:
        ret = true;

error:
        return ret;
    }


    bool TodoHandler::removeTodos ( KCal::Todo::List& p_todoList )
    {
//        int errorCount = 0;
        bool ret = false;
        RRA_Uint32Vector * deleted_ids = rra_uint32vector_new();

        if ( p_todoList.begin() == p_todoList.end() ) {
            goto success;
        }

        for ( KCal::Todo::List::Iterator it = p_todoList.begin();
                it != p_todoList.end(); ++it ) {
            incrementSteps();
            QString kUid = mUidHelper->konnectorId( "SynCETodo", ( *it ) ->uid(), "---" );

            if ( kUid != "---" ) {
                kdDebug( 2120 ) << "Removing Event on Device: " << "ID-Pair: KDEID: " <<
                ( *it ) ->uid() << " DeviceId: " << kUid << endl;

                if (!m_rra->deleteObject (mTypeId, getOriginalId( kUid ))) {
//                    if (errorCount++ == -1) {
//                        goto error;
//                    }
                }

                mUidHelper->removeId( "SynCETodo", kUid );
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


    bool TodoHandler::writeSyncee( KSync::TodoSyncee *mCalendarSyncee )
    {
        bool ret;

        if ( mCalendarSyncee->isValid() ) {
            KCal::Todo::List todoAdded;
            KCal::Todo::List todoRemoved;
            KCal::Todo::List todoModified;

            setMaximumSteps(mCalendarSyncee->added().count() + mCalendarSyncee->removed().count() + mCalendarSyncee->modified().count());
            resetSteps();
            getTodos( todoAdded, mCalendarSyncee->added() );
            // This is a bad hack - but ksync provides deleted calendar-entries only as todos
            // So lets look if a removed "todo" is actually a removed todo ...
            getTodosAsFakedTodos( todoRemoved, mCalendarSyncee->removed() );
            getTodos( todoModified, mCalendarSyncee->modified() );

            setStatus( "Writing added Todos" );
            if (ret = addTodos( todoAdded )) {
                setStatus( "Erasing deleted Todos" );
                if (ret = removeTodos( todoRemoved )) {
                    setStatus( "Writing changed Todos" );
                    ret = updateTodos( todoModified );
                }
            }
        }

        return ret;
    }
}
