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
#include <libkcal/calendar.h>
#include <qregexp.h>

namespace pocketPCCommunication {

/**
@author Christian Fremgen; Volker Christian
*/
class TodoHandler : public PimHandler
{
public:
    /** Just a simple constructor.
      * @param p_pdaName as the name says :)
      */
    TodoHandler(const QString& p_pdaName);

    TodoHandler(KSharedPtr<Rra> p_rra);

    virtual ~TodoHandler();


    /** Get events from the device.
      * @param p_calendar store events in this calendar
      * @param p_recType specify in which events your are interested (@see RecordType)
      */
    bool getAllTodos (KCal::Calendar& p_calendar, RecordType p_recType);


    /** Put events to the device.
      * @param p_calendar events are this calendar
      */
    bool putTodos (KCal::Calendar& p_calendar);



    bool getIdStatus (QMap<QString, RecordType>& p_statusMap);

    void addTodos    (KCal::Todo::List& p_todos);
    void updateTodos (KCal::Todo::List& p_todos);
    void removeTodos (KCal::Todo::List& p_todos);


    bool getTodos    (KCal::Todo::List& p_todos, const QStringList& p_ids);

protected:
    bool getTypeId ();


private:

    /** This private method retrieves all the requested information to fill the calendar with events.
      * @param p_calendar events are stored in here
      * @param p_ids this struct holds all the ids (changed, unchanged, deleted)
      * @param p_recType specify in which information you are interested
      */
    void getTodoEntry (KCal::Calendar& p_calendar, const struct Rra::ids& p_ids, RecordType p_recType);

    void deleteTodoEntry (const uint32_t& p_objectId);

    QString makeVIncidence (KCal::Incidence* p_incidence);

    static uint32_t    s_typeIdTodo;   /**< This static member stores the typeId belonging to "Appointment" */


    QRegExp m_incidenceRegexp;

    QString sCurrentTimeZone;
};

}

#endif
