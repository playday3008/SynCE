//
// C++ Interface: todohandler
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef POCKETPCCOMMUNICATIONTODOHANDLER_H
#define POCKETPCCOMMUNICATIONTODOHANDLER_H


#include "PimHandler.h"
#include "RecordType.h"
#include <kitchensync/calendarsyncee.h>
#include <kitchensync/idhelper.h>

namespace pocketPCCommunication {

/**
@author Christian Fremgen cfremgen@users.sourceforge.net, Volker Christian voc@users.sourceforge.net
*/
class TodoHandler : public PimHandler
{
public:
    TodoHandler (KSharedPtr<Rra> p_rra, QString mBaseDir, KSync::KonnectorUIDHelper *mUidHelper);

    bool init();

    virtual ~TodoHandler();

    int retrieveTodoListFromDevice(KCal::Todo::List &mTodoList, QValueList<uint32_t> &idList);
    int fakeTodoListFromDevice(KCal::Todo::List &mTodoList, QValueList<uint32_t> &idList);
    bool getIds();
    int getTodoListFromDevice(KCal::Todo::List &mTodoList, int mRecType);
    bool readSyncee(KSync::CalendarSyncee *mCalendarSyncee, bool firstSync);
    void getTodos (KCal::Todo::List& p_addressees, KSync::SyncEntry::PtrList p_ptrList );
    void getTodosAsFakedTodos(KCal::Todo::List& p_todos, KSync::SyncEntry::PtrList p_ptrList );
    bool writeSyncee(KSync::CalendarSyncee *mCalendarSyncee);
    void insertIntoCalendarSyncee(KSync::CalendarSyncee *mCalendarSyncee, KCal::Todo::List &list, int state);

    void addTodos    (KCal::Todo::List& p_todoList);
    void updateTodos (KCal::Todo::List& p_todoList);
    void removeTodos (KCal::Todo::List& p_todoList);

private:
    KSync::KonnectorUIDHelper *mUidHelper;
    QString mBaseDir;
};

}

#endif
