/***************************************************************************
 *   Copyright (C) 2004 by Christian Fremgen                               *
 *   cfremgen@users.sourceforge.net                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef POCKETPCCOMMUNICATIONCALENDARHANDLER_H
#define POCKETPCCOMMUNICATIONCALENDARHANDLER_H

#include "PimHandler.h"
#include "RecordType.h"
#include <libkcal/calendar.h>
#include <qregexp.h>

namespace pocketPCCommunication {

/**
This class handles a Calendar which can be read from and written to a Windows CE device.
Additionally it is possible to read/write only appointments(events) or tasks(todos).

@author Christian Fremgen cfremgen@users.sourceforge.net
*/

class CalendarHandler : public PimHandler
{
public:
    /** Just a simple constructor.
      * @param p_pdaName as the name says :)
      */
    CalendarHandler(const QString& p_pdaName);
    
    CalendarHandler(KSharedPtr<Rra> p_rra);
    
    virtual ~CalendarHandler();
    
    /** Get events from the device. 
      * @param p_calendar store events in this calendar
      * @param p_recType specify in which events your are interested (@see RecordType)
      */
    bool getCalendarEvents (KCal::Calendar& p_calendar, RecordType p_recType);
    
    /** Get todos from the device. 
      * @param p_calendar store todos in this calendar
      * @param p_recType specify in which todos your are interested (@see RecordType)
      */
    bool getCalendarTodos (KCal::Calendar& p_calendar, RecordType p_recType);
    
    /** Put events to the device. 
      * @param p_calendar events are this calendar
      */
    bool putCalendarEvents (KCal::Calendar& p_calendar);
    
    /** Put todos to the device. 
      * @param p_calendar events are this calendar
      */
    bool putCalendarTodos (KCal::Calendar& p_calendar);
    
    /** Delete entries on the device. Currently this is just a convenience function, because events without
      * a specified time are stored on 01011970 and cannot be deleted within the calendar on the device.
      */
    void deleteCalendarEntries (const uint32_t& p_typeId, RecordType p_recType);
    
    void deleteCalendar ();
    
    bool getIdStatus (QMap<QString, RecordType>& p_statusMap);
        
    void addEvents    (KCal::Event::List& p_events);
    void updateEvents (KCal::Event::List& p_evenst);
    void removeEvents (KCal::Event::List& p_events);
    void removeEvents (const QStringList& p_events);
    
    void addTodos     (KCal::Todo::List& p_todos);
    void updateTodos  (KCal::Todo::List& p_todos);
    void removeTodos  (KCal::Todo::List& p_todos);
    void removeTodos  (const QStringList& p_todos);
    
    
    bool getEvents    (KCal::Event::List& p_events, const QStringList& p_ids);
    bool getTodos     (KCal::Todo::List&  p_todos,  const QStringList& p_ids);

protected:
    bool getTypeId ();
    
private:
    
    /** This private method retrieves all the requested information to fill the calendar with events.
      * @param p_calendar events are stored in here
      * @param p_ids this struct holds all the ids (changed, unchanged, deleted)
      * @param p_recType specify in which information you are interested
      */
    void getEventEntry (KCal::Calendar& p_calendar, const struct Rra::ids& p_ids, RecordType p_recType);
    
    /** This private method retrieves all the requested information to fill the calendar with todos.
      * @param p_calendar todos are stored in here
      * @param p_ids this struct holds all the ids (changed, unchanged, deleted)
      * @param p_recType specify in which information you are interested
      */
    void getTodoEntry (KCal::Calendar& p_calendar, const struct Rra::ids& p_ids, RecordType p_recType);
    
    void deleteEventEntry (const uint32_t& p_objectId);
    void deleteTodoEntry     (const uint32_t& p_objectId);

    QString makeVIncidence (KCal::Incidence* p_incidence);
    
    static uint32_t    s_typeIdEvent;   /**< This static member stores the typeId belonging to "Appointment" */
    static uint32_t    s_typeIdTodo;    /**< This static member stores the typeId belonging to "Task" */
            
    QRegExp            m_incidenceRegexp;
};

};

#endif
