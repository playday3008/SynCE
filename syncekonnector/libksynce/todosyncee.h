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

#ifndef TODOSYNCEE_H
#define TODOSYNCEE_H

#include <libkcal/calendar.h>

#include "syncee.h"

namespace KSync {
class CalendarMerger;
class KDE_EXPORT TodoSyncEntry : public SyncEntry
{
  public:
    typedef QPtrList<TodoSyncEntry> PtrList;

    TodoSyncEntry( Syncee* parent );
    TodoSyncEntry( KCal::Incidence *, Syncee *parent );
    TodoSyncEntry( const TodoSyncEntry& );

    QString name();
    QString id();
    QString timestamp();

    bool equals( SyncEntry *entry );

    TodoSyncEntry *clone();

    KCal::Incidence *incidence()const;
    void setId(const QString &id);

    KPIM::DiffAlgo* diffAlgo( SyncEntry*, SyncEntry* );

  private:
    KCal::Incidence *mIncidence;
};

/**
  This class provides an implementation of the @see KSyncee interface for KSync.
  It provides syncing of iCalendar files.
*/
class KDE_EXPORT TodoSyncee : public Syncee
{
  public:
    TodoSyncee( KCal::Calendar*, CalendarMerger* merger = 0);
    ~TodoSyncee();

    void reset();

    TodoSyncEntry *firstEntry();
    TodoSyncEntry *nextEntry();

    void addEntry( SyncEntry * );
    void removeEntry( SyncEntry * );


    KCal::Calendar *calendar() const { return mCalendar; }

    bool writeBackup( const QString & );
    bool restoreBackup( const QString & );

    QString generateNewId() const;

  private:
    TodoSyncEntry *createEntry( KCal::Incidence * );

    void clearEntries();

    KCal::Calendar *mCalendar;
    KCal::Todo::List mTodos;
    KCal::Todo::List::ConstIterator mCurrentTodo;

    QMap<KCal::Incidence *,TodoSyncEntry *> mEntries;
};

}

#endif
