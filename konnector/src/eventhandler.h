//
// C++ Interface: eventhandler
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef POCKETPCCOMMUNICATIONEVENTHANDLER_H
#define POCKETPCCOMMUNICATIONEVENTHANDLER_H


#include "PimHandler.h"
#include "RecordType.h"
#include <libkcal/calendar.h>
#include <qregexp.h>

namespace pocketPCCommunication {

/**
@author Christian Fremgen; Volker Christian
*/
class EventHandler : public PimHandler
{
public:
    /** Just a simple constructor.
      * @param p_pdaName as the name says :)
      */
    EventHandler(const QString& p_pdaName);

    EventHandler(KSharedPtr<Rra> p_rra);

    virtual ~EventHandler();


    /** Get events from the device.
      * @param p_calendar store events in this calendar
      * @param p_recType specify in which events your are interested (@see RecordType)
      */
    bool getAllEvents (KCal::Calendar& p_calendar, RecordType p_recType);


    /** Put events to the device.
      * @param p_calendar events are this calendar
      */
    bool putEvents (KCal::Calendar& p_calendar);



    bool getIdStatus (QMap<QString, RecordType>& p_statusMap);

    void addEvents    (KCal::Event::List& p_events);
    void updateEvents (KCal::Event::List& p_evenst);
    void removeEvents (KCal::Event::List& p_events);


    bool getEvents    (KCal::Event::List& p_events, const QStringList& p_ids);

protected:
    bool getTypeId ();


private:

    /** This private method retrieves all the requested information to fill the calendar with events.
      * @param p_calendar events are stored in here
      * @param p_ids this struct holds all the ids (changed, unchanged, deleted)
      * @param p_recType specify in which information you are interested
      */
    void getEventEntry (KCal::Calendar& p_calendar, const struct Rra::ids& p_ids, RecordType p_recType);

    void deleteEventEntry (const uint32_t& p_objectId);

    QString makeVIncidence (KCal::Incidence* p_incidence);

    static uint32_t    s_typeIdEvent;   /**< This static member stores the typeId belonging to "Appointment" */


    QRegExp m_incidenceRegexp;

    QString sCurrentTimeZone;
};

}

#endif
