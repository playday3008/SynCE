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

#ifndef EVENTSYNCEE_H
#define EVENTSYNCEE_H

#include <libkcal/calendar.h>
#include <calendarsyncee.h>

#include <syncee.h>

namespace KSync {
class CalendarMerger;
class KDE_EXPORT EventSyncEntry : public SyncEntry
{
  public:
    typedef QPtrList<CalendarSyncEntry> PtrList;

    EventSyncEntry( Syncee* parent );
    EventSyncEntry( KCal::Incidence *, Syncee *parent );
    EventSyncEntry( const EventSyncEntry& );

    QString name();
    QString id();
    QString timestamp();

    bool equals( SyncEntry *entry );

    EventSyncEntry *clone();

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
class KDE_EXPORT EventSyncee : public Syncee
{
  public:
    EventSyncee( KCal::Calendar*, CalendarMerger* merger = 0);
    ~EventSyncee();

    void reset();

    EventSyncEntry *firstEntry();
    EventSyncEntry *nextEntry();

    void addEntry( SyncEntry * );
    void removeEntry( SyncEntry * );


    KCal::Calendar *calendar() const { return mCalendar; }

    bool writeBackup( const QString & );
    bool restoreBackup( const QString & );

    QString generateNewId() const;

  private:
    EventSyncEntry *createEntry( KCal::Incidence * );

    void clearEntries();

    KCal::Calendar *mCalendar;
    KCal::Event::List mEvents;
    KCal::Event::List::ConstIterator mCurrentEvent;

    QMap<KCal::Incidence *,EventSyncEntry *> mEntries;
};

}

#endif
