/*
    This file is part of KitchenSync.

    Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>
    Copyright (c) 2004 Holger Hans Peter Freyther <freyther@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "eventsyncee.h"
#include "calendarmerger.h"

#include <libkcal/filestorage.h>
#include <libkcal/calformat.h>
#include <libkdepim/calendardiffalgo.h>

#include <kdebug.h>


using namespace KSync;
using namespace KCal;

EventSyncEntry::EventSyncEntry( Incidence *incidence, Syncee *parent )
  : SyncEntry( parent ), mIncidence( incidence )
{
  setType( QString::fromLatin1( "EventSyncEntry" ) );
}

EventSyncEntry::EventSyncEntry( Syncee* parent )
  : SyncEntry( parent )
{
  /* that is hard, use todo or calendar as the default? */
  mIncidence = new KCal::Event;
  setType( QString::fromLatin1( "EventSyncEntry" ) );
}


EventSyncEntry::EventSyncEntry( const EventSyncEntry& ent )
  : SyncEntry( ent ), mIncidence( ent.mIncidence->clone() )
{}

QString EventSyncEntry::name()
{
  return mIncidence->summary();
}

QString EventSyncEntry::id()
{
  return mIncidence->uid();
}

void EventSyncEntry::setId(const QString& id)
{
  mIncidence->setUid( id );
}

QString EventSyncEntry::timestamp()
{
  // FIXME: last modified isn't sufficient to tell if an event has changed.
  return mIncidence->lastModified().toString();
}

KCal::Incidence *EventSyncEntry::incidence()const {
    return mIncidence;
}

bool EventSyncEntry::equals( SyncEntry *entry )
{
  EventSyncEntry *eventEntry = dynamic_cast<EventSyncEntry *>(entry);
  if (!eventEntry) {
    kdDebug() << "EventSyncee::addEntry(): Wrong type." << endl;
    return false;
  }

  kdDebug() << "UID: " << mIncidence->uid() << " <-> "
            << eventEntry->incidence()->uid() << endl;
  kdDebug() << "LAM: " << mIncidence->lastModified().toTime_t() << " <-> "
            << eventEntry->incidence()->lastModified().toTime_t() << endl;

  if ( mIncidence->uid() != eventEntry->incidence()->uid() ) {
    kdDebug() << "UID unequal" << endl;
    return false;
  }
  if ( mIncidence->lastModified() != eventEntry->incidence()->lastModified() ) {
    kdDebug() << "LAM unequal" << endl;
    return false;
  }

  if ( *mIncidence == *( eventEntry->incidence() ) ) return true;

  return false;
}

EventSyncEntry *EventSyncEntry::clone()
{
  return new EventSyncEntry( *this );
}

KPIM::DiffAlgo* EventSyncEntry::diffAlgo( SyncEntry *syncEntry, SyncEntry *targetEntry )
{
  EventSyncEntry *eventSyncEntry = dynamic_cast<EventSyncEntry*>( syncEntry );
  EventSyncEntry *eventTargetEntry = dynamic_cast<EventSyncEntry*>( targetEntry );

  if ( !eventSyncEntry || !eventTargetEntry )
    return 0;

  return new KPIM::CalendarDiffAlgo( eventSyncEntry->incidence(), eventTargetEntry->incidence() );
}


/// Syncee starts here
EventSyncee::EventSyncee( Calendar *calendar, CalendarMerger* merger )
    : Syncee( merger )
{
  setType( QString::fromLatin1( "EventSyncee" ) );
  mCalendar = calendar;
}

EventSyncee::~EventSyncee()
{
  clearEntries();
}

void EventSyncee::reset()
{
  clearEntries();
}

void EventSyncee::clearEntries()
{
  QMap<Incidence *, EventSyncEntry *>::Iterator it;
  for( it = mEntries.begin(); it != mEntries.end(); ++it ) {
    delete it.data();
  }
  mEntries.clear();
}

EventSyncEntry *EventSyncee::firstEntry()
{
  mEvents = mCalendar->events();
  mCurrentEvent = mEvents.begin();
  if( mCurrentEvent == mEvents.end() ) {
    return 0;
  }

  return createEntry( *mCurrentEvent );
}

EventSyncEntry *EventSyncee::nextEntry()
{
  ++mCurrentEvent;
  if ( mCurrentEvent == mEvents.end() ) {
    return 0;
  }
  return createEntry( *mCurrentEvent );
}

void EventSyncee::addEntry( SyncEntry *entry )
{
  EventSyncEntry *eventEntry = dynamic_cast<EventSyncEntry *>(entry);
  if (eventEntry) {
    Event *sourceEvent = dynamic_cast<Event *>(eventEntry->incidence());
    Event *event = dynamic_cast<Event *>(sourceEvent);
    mCalendar->addEvent(event);
    /* do not lose the syncStatus and insert the Entry directly */
    eventEntry->setSyncee( this );
    mEntries.insert(eventEntry->incidence(), eventEntry);
  }
}

void EventSyncee::removeEntry( SyncEntry *entry )
{
  EventSyncEntry *eventEntry = dynamic_cast<EventSyncEntry *>( entry );
  if ( eventEntry ) {
    Event *ev = dynamic_cast<Event *>( eventEntry->incidence() );
    mCalendar->deleteEvent( ev );
    eventEntry->setSyncee( 0l );
    mEntries.remove( eventEntry->incidence() );
  }
}

EventSyncEntry *EventSyncee::createEntry( Incidence *incidence )
{
  if ( incidence ) {
    QMap<Incidence *,EventSyncEntry *>::ConstIterator it;
    it = mEntries.find( incidence );
    if ( it != mEntries.end() ) return it.data();

    EventSyncEntry *entry = new EventSyncEntry( incidence, this );
    mEntries.insert( incidence, entry );
    return entry;
  } else {
    return 0;
  }
}

bool EventSyncee::writeBackup( const QString &filename )
{
  KCal::FileStorage storage( mCalendar, filename );

  bool ok = true;
  ok = ok && storage.open();
  ok = ok && storage.save();
  ok = ok && storage.close();

  return ok;
}

bool EventSyncee::restoreBackup( const QString &filename )
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

QString EventSyncee::generateNewId() const {
    return KCal::CalFormat::createUniqueId();
}
