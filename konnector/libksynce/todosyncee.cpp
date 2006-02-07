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

#include "todosyncee.h"
#include "calendarmerger.h"

#include <libkcal/filestorage.h>
#include <libkcal/calformat.h>
#include <libkdepim/calendardiffalgo.h>

#include <kdebug.h>


using namespace KSync;
using namespace KCal;

TodoSyncEntry::TodoSyncEntry( Incidence *incidence, Syncee *parent )
  : SyncEntry( parent ), mIncidence( incidence )
{
  setType( QString::fromLatin1( "TodoSyncEntry" ) );
}

TodoSyncEntry::TodoSyncEntry( Syncee* parent )
  : SyncEntry( parent )
{
  /* that is hard, use todo or calendar as the default? */
  mIncidence = new KCal::Todo;
  setType( QString::fromLatin1( "TodoSyncEntry" ) );
}


TodoSyncEntry::TodoSyncEntry( const TodoSyncEntry& ent )
  : SyncEntry( ent ), mIncidence( ent.mIncidence->clone() )
{}

QString TodoSyncEntry::name()
{
  return mIncidence->summary();
}

QString TodoSyncEntry::id()
{
  return mIncidence->uid();
}

void TodoSyncEntry::setId(const QString& id)
{
  mIncidence->setUid( id );
}

QString TodoSyncEntry::timestamp()
{
  // FIXME: last modified isn't sufficient to tell if an event has changed.
  return mIncidence->lastModified().toString();
}

KCal::Incidence *TodoSyncEntry::incidence()const {
    return mIncidence;
}

bool TodoSyncEntry::equals( SyncEntry *entry )
{
  TodoSyncEntry *todoEntry = dynamic_cast<TodoSyncEntry *>(entry);
  if (!todoEntry) {
    kdDebug() << "TodoSyncee::addEntry(): Wrong type." << endl;
    return false;
  }

  kdDebug() << "UID: " << mIncidence->uid() << " <-> "
            << todoEntry->incidence()->uid() << endl;
  kdDebug() << "LAM: " << mIncidence->lastModified().toTime_t() << " <-> "
            << todoEntry->incidence()->lastModified().toTime_t() << endl;

  if ( mIncidence->uid() != todoEntry->incidence()->uid() ) {
    kdDebug() << "UID unequal" << endl;
    return false;
  }
  if ( mIncidence->lastModified() != todoEntry->incidence()->lastModified() ) {
    kdDebug() << "LAM unequal" << endl;
    return false;
  }

  if ( *mIncidence == *( todoEntry->incidence() ) ) return true;

  return false;
}

TodoSyncEntry *TodoSyncEntry::clone()
{
  return new TodoSyncEntry( *this );
}

KPIM::DiffAlgo* TodoSyncEntry::diffAlgo( SyncEntry *syncEntry, SyncEntry *targetEntry )
{
  TodoSyncEntry *todoSyncEntry = dynamic_cast<TodoSyncEntry*>( syncEntry );
  TodoSyncEntry *todoTargetEntry = dynamic_cast<TodoSyncEntry*>( targetEntry );

  if ( !todoSyncEntry || !todoTargetEntry )
    return 0;

  return new KPIM::CalendarDiffAlgo( todoSyncEntry->incidence(), todoTargetEntry->incidence() );
}


/// Syncee starts here
TodoSyncee::TodoSyncee( Calendar *calendar, CalendarMerger* merger )
    : Syncee( merger )
{
  setType( QString::fromLatin1( "TodoSyncee" ) );
  mCalendar = calendar;
}

TodoSyncee::~TodoSyncee()
{
  clearEntries();
}

void TodoSyncee::reset()
{
  clearEntries();
}

void TodoSyncee::clearEntries()
{
  QMap<Incidence *, TodoSyncEntry *>::Iterator it;
  for( it = mEntries.begin(); it != mEntries.end(); ++it ) {
    delete it.data();
  }
  mEntries.clear();
}

TodoSyncEntry *TodoSyncee::firstEntry()
{
  mTodos = mCalendar->todos();
  mCurrentTodo = mTodos.begin();
  if( mCurrentTodo == mTodos.end() ) {
    return 0;
  }

  return createEntry( *mCurrentTodo );
}

TodoSyncEntry *TodoSyncee::nextEntry()
{
  ++mCurrentTodo;
  if ( mCurrentTodo == mTodos.end() ) {
    return 0;
  }
  return createEntry( *mCurrentTodo );
}

void TodoSyncee::addEntry( SyncEntry *entry )
{
  TodoSyncEntry *todoEntry = dynamic_cast<TodoSyncEntry *>(entry);
  if (todoEntry) {
    Todo *sourceTodo = dynamic_cast<Todo *>(todoEntry->incidence());
    Todo *todo = dynamic_cast<Todo *>(sourceTodo);
    mCalendar->addTodo(todo);
    /* do not lose the syncStatus and insert the Entry directly */
    todoEntry->setSyncee( this );
    mEntries.insert(todoEntry->incidence(), todoEntry);
  }
}

void TodoSyncee::removeEntry( SyncEntry *entry )
{
  TodoSyncEntry *todoEntry = dynamic_cast<TodoSyncEntry *>( entry );
  if ( todoEntry ) {
    Todo *todo = dynamic_cast<Todo *>( todoEntry->incidence() );
    mCalendar->deleteTodo( todo );
    todoEntry->setSyncee( 0l );
    mEntries.remove( todoEntry->incidence() );
  }
}

TodoSyncEntry *TodoSyncee::createEntry( Incidence *incidence )
{
  if ( incidence ) {
    QMap<Incidence *,TodoSyncEntry *>::ConstIterator it;
    it = mEntries.find( incidence );
    if ( it != mEntries.end() ) return it.data();

    TodoSyncEntry *entry = new TodoSyncEntry( incidence, this );
    mEntries.insert( incidence, entry );
    return entry;
  } else {
    return 0;
  }
}

bool TodoSyncee::writeBackup( const QString &filename )
{
  KCal::FileStorage storage( mCalendar, filename );

  bool ok = true;
  ok = ok && storage.open();
  ok = ok && storage.save();
  ok = ok && storage.close();

  return ok;
}

bool TodoSyncee::restoreBackup( const QString &filename )
{
  mCalendar->close();

  KCal::FileStorage storage( mCalendar, filename );

  bool ok = true;
  ok = ok && storage.open();
  ok = ok && storage.load();
  ok = ok && storage.close();

  clearEntries();

  return ok;
}

QString TodoSyncee::generateNewId() const {
    return KCal::CalFormat::createUniqueId();
}
