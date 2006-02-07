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

#ifndef POCKETPCCOMMUNICATIONTODOHANDLER_H
#define POCKETPCCOMMUNICATIONTODOHANDLER_H


#include "pimhandler.h"
#include "recordtype.h"
#include "todosyncee.h"
#include <kitchensync/idhelper.h>
#include <libkdepim/progressmanager.h>


namespace KPIM {
    class ProgressItem;
}

namespace PocketPCCommunication {

/**
@author Christian Fremgen cfremgen@users.sourceforge.net, Volker Christian voc@users.sourceforge.net
*/
class TodoHandler : public PimHandler
{
public:
    TodoHandler (Rra *p_rra, KSync::KonnectorUIDHelper *mUidHelper );

    bool init();

    virtual ~TodoHandler();

    int retrieveTodoListFromDevice(KCal::Todo::List &mTodoList, QValueList<uint32_t> &idList);
    int fakeTodoListFromDevice(KCal::Todo::List &mTodoList, QValueList<uint32_t> &idList);
    bool getIds();
    int getTodoListFromDevice(KCal::Todo::List &mTodoList, int mRecType);
    bool readSyncee(KSync::TodoSyncee *mCalendarSyncee, bool firstSync);
    void getTodos (KCal::Todo::List& p_addressees, KSync::SyncEntry::PtrList p_ptrList );
    void getTodosAsFakedTodos(KCal::Todo::List& p_todos, KSync::SyncEntry::PtrList p_ptrList );
    bool writeSyncee(KSync::TodoSyncee *mCalendarSyncee);
    void insertIntoCalendarSyncee(KSync::TodoSyncee *mCalendarSyncee, KCal::Todo::List &list, int state);

    void addTodos    (KCal::Todo::List& p_todoList);
    void updateTodos (KCal::Todo::List& p_todoList);
    void removeTodos (KCal::Todo::List& p_todoList);

private:
    KSync::KonnectorUIDHelper *mUidHelper;
};

}

#endif
