//
// C++ Implementation: eventhandler
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "eventhandler.h"

#include <kdebug.h>
//#include <qregexp.h>

//#include <libkcal/vcalformat.h>
#include <libkcal/icalformat.h>
#include <libkcal/event.h>
#include <libkcal/calendarnull.h>
#include <libkcal/calendarlocal.h>
#include <qfile.h>

namespace pocketPCCommunication {


uint32_t EventHandler::s_typeIdEvent = 0;

EventHandler::EventHandler(const QString& p_pdaName)
    : PimHandler(p_pdaName)
{
    m_incidenceRegexp = QRegExp ("BEGIN.*VERSION:2.0");

    s_typeIdEvent = 0;

    QFile f("/etc/timezone");
    if(f.open(IO_ReadOnly))
    {
        QTextStream ts(&f);
        ts >> sCurrentTimeZone;
    }

    f.close();
}


EventHandler::EventHandler(KSharedPtr<Rra> p_rra)
    : PimHandler(p_rra)
{
    QFile f("/etc/timezone");
    if(f.open(IO_ReadOnly))
    {
        QTextStream ts(&f);
        ts >> sCurrentTimeZone;
    }

    f.close();
}


EventHandler::~EventHandler()
{
}


bool EventHandler::getAllEvents (KCal::Calendar& p_calendar, RecordType p_recType)
{
    kdDebug(2120) << "[EventHandler]: getCalendarEvents" << endl;

    if (!getTypeId())
    {
        return false;
    }

    p_calendar.setTimeZoneId(sCurrentTimeZone);

    struct Rra::ids ids;

    if (!m_rra->getIds (s_typeIdEvent, &ids))
    {
        return false;
    }

    // and now.. get the object ids...
    kdDebug(2120) << "[EventHandler]: got event ids.. fetching information" << endl;

    switch (p_recType)
    {
        case ALL:
            getEventEntry (p_calendar, ids, CHANGED);
            getEventEntry (p_calendar, ids, DELETED);
            getEventEntry (p_calendar, ids, UNCHANGED);
            break;
        case CHANGED:
            getEventEntry (p_calendar, ids, CHANGED);
            break;
        case DELETED:
            getEventEntry (p_calendar, ids, DELETED);
            break;
        case UNCHANGED:
            getEventEntry (p_calendar, ids, UNCHANGED);
            break;
    }

    // iterate over calendar and add X-POCKETPCCOMM-REMOTE_ID_*

    return true;
}


void EventHandler::getEventEntry (KCal::Calendar& p_calendar, const struct Rra::ids& p_ids, RecordType p_recType)
{
    QValueList<uint32_t>::const_iterator it;
    QValueList<uint32_t>::const_iterator end;

    QString vCal;

    switch (p_recType)
    {
        case CHANGED:
            it = p_ids.changedIds.begin();
            end = p_ids.changedIds.end();
            break;
        case DELETED:
            it = p_ids.deletedIds.begin();
            end = p_ids.deletedIds.end();
            break;
        case UNCHANGED:
            it = p_ids.unchangedIds.begin();
            end = p_ids.unchangedIds.end();
            break;
        case ALL: // not reasonable to have ALL here! added to avoid warning of gcc
            break;
    }

    KCal::ICalFormat calFormat; // NEEDED FOR EVENTS!!
//    KCal::CalendarLocal cal;
    calFormat.setTimeZone(sCurrentTimeZone, false);

    QString vCalBegin = "BEGIN:VCALENDAR\nPRODID:-//K Desktop Environment//NONSGML KOrganizer 3.2.1//EN\nVERSION:2.0\n";
    QString vCalEnd = "END:VCALENDAR\n";

    /*
    uint32_t id;
    m_rra->getMatchMaker()->get_partner_id(1, &id);
    QString partnerId = QString::number (id, 16);
    */
    QString partnerId;
    partnerId = getPartnerId();

    for (;it != end; ++it)
    {
        //remoteId = "X-" + m_appName + "-" + m_keyName + "_" + partnerId + ":" + *it + "\n";

        vCal = vCalBegin + m_rra->getVEvent(s_typeIdEvent, *it) + vCalEnd;
        kdDebug(2120) << "[EventHandler2]::getEventEntry calendar: " << vCal << endl;

        if (!calFormat.fromString (&p_calendar, vCal))
            kdDebug(2120) << "[EventHandler]: getting of event failed!" << endl;
        else
        {
            /*
            const QString key = m_keyName + "-" + partnerId;
            const QString value = "RRA-ID-" + QString::number (*it, 16).rightJustify (8, '0');
            */
            //KCal::Event* ev = p_calendar.event(value);

            //ev->setNonKDECustomProperty (("X-"+m_appName+"-"+key).latin1(), value);

            //ev->updated();
        }
    }
}


bool EventHandler::putEvents (KCal::Calendar& p_calendar)
{
    //Rra rra(m_pdaName);
    //rra.connect();

    KCal::Event::List eList = p_calendar.rawEvents();
    KCal::Event::List::iterator it = eList.begin();

    QString vEvent;
    uint32_t objectId = 0;
    uint32_t newObjectId;
    bool ok;

    if (eList.begin() == eList.end()) {
        return true;
    }

    if (!getTypeId())
    {
        return false;
    }
    /*
    uint32_t id;
    m_rra->getMatchMaker()->get_partner_id(1, &id);
    QString partnerId = QString::number (id, 16);
    */
    QString partnerId;
    partnerId = getPartnerId();

    clearIdPairs();

    for (; it != eList.end(); ++it)
    {
        kdDebug(2120) << "[EventHandler]: putCalendarEvents: event.summary(): " << (*it)->summary() << endl;
        // convert this to a vEvent!
        vEvent = makeVIncidence (*it);
        //kdDebug(2120) << "[EventHandler]: putCalendarEvents: calString: " << vEvent << endl;

        // ok.. write them to the device!
        QString curId;
        curId = (*it)->uid();
        objectId = 0;
        if ((*it)->uid().startsWith ("RRA-ID-"))
        {
            //doPush = (*it).changed();
            objectId = (*it)->uid().remove("RRA-ID-").toUInt(&ok, 16);
            if (!ok)
                kdDebug(2120) << "could not convert UID to uint32_t" << endl;
            else
                kdDebug(2120) << "RRA-ID: " << objectId << endl;
        }


        newObjectId = m_rra->putVEvent(vEvent, s_typeIdEvent, objectId);
        if (newObjectId)
        {
            // the object was newly generated on the device!!
            // so.. what shall we do with this id?
            // ---> save it in the local addressBook!
            //(*it)->setUid ("RRA-ID-" + QString::number(newObjectId, 16));

            //(*it)->setNonKDECustomProperty (("X-"+m_appName+"-"+m_keyName + "-" + partnerId).latin1(), "RRA-ID-" + QString::number(newObjectId, 16).rightJustify(8, '0'));

            if ("RRA-ID-" + QString::number(newObjectId, 16).rightJustify(8, '0') != curId)
                addIdPair("RRA-ID-" + QString::number(newObjectId, 16).rightJustify(8, '0'), curId);
        }

    }

    return true;
}


bool EventHandler::getTypeId ()
{
    if (!s_typeIdEvent)
        s_typeIdEvent = m_rra->getTypeForName(RRA_SYNCMGR_TYPE_APPOINTMENT);

    if (!s_typeIdEvent)
        return false;
    else
        return true;
}


bool EventHandler::getIdStatus (QMap<QString, RecordType>& p_statusMap)
{
    if (!getTypeId())
    {
        return false;
    }

    return  getIdStatusPro (p_statusMap, s_typeIdEvent);
}


void EventHandler::deleteEventEntry (const uint32_t& p_objectId)
{
    kdDebug() << "EventHandler::deleteCalendarEntry" << endl;

    if (!getTypeId())
        return;

    deleteSingleEntry (s_typeIdEvent, p_objectId);
}


void EventHandler::addEvents (KCal::Event::List& p_events)
{
    if (p_events.begin() == p_events.end())
        return;

    if (!getTypeId())
    {
        return;
    }

    KCal::Event::List::Iterator it = p_events.begin();
    QString vEvent;
    uint32_t newObjectId;
    QString curId;

    for (; it != p_events.end(); ++it)
    {
        //if (!isARraId ((*it)->uid())) // this must be a new entry!!!
        {
            curId = (*it)->uid();
            vEvent = makeVIncidence (*it);
            //kdDebug() << "EventHandler vincidence: " << endl;
            //kdDebug() << vEvent << endl;
            uint32_t remId = 0;
            if (isARraId(curId))
                remId = getOriginalId(curId);

            newObjectId = m_rra->putVEvent (vEvent, s_typeIdEvent, remId); //getOriginalId((*it)->uid()));

            if (newObjectId) // must be!!!
            {
                //(*it)->setNonKDECustomProperty (("X-"+m_appName+"-"+m_keyName + "-" + partnerId).latin1(), "RRA-ID-" + QString::number(newObjectId, 16).rightJustify(8, '0'));
                if ("RRA-ID-" + QString::number(newObjectId, 16).rightJustify(8, '0') != curId)
                    addIdPair("RRA-ID-" + QString::number(newObjectId, 16).rightJustify(8, '0'), curId);
            }
        }
    }
}


void EventHandler::updateEvents (KCal::Event::List& p_events)
{
    if (p_events.begin() == p_events.end())
        return;

    if (!getTypeId())
    {
        return;
    }

    KCal::Event::List::Iterator it = p_events.begin();
    QString vEvent;
    QString curId;

    for (; it != p_events.end(); ++it)
    {
        if (isARraId ((*it)->uid())) // must exist on device!!!
        {
            curId = (*it)->uid();
            vEvent = makeVIncidence (*it);
            m_rra->putVEvent (vEvent, s_typeIdEvent, getOriginalId((*it)->uid()));
        }
    }
}


void EventHandler::removeEvents (KCal::Event::List& p_events)
{
    if (p_events.begin() == p_events.end())
        return;

    if (!getTypeId())
    {
        return;
    }

    KCal::Event::List::Iterator it = p_events.begin();

    for (; it != p_events.end(); ++it)
    {
        if (isARraId((*it)->uid()))
            deleteEventEntry (getOriginalId((*it)->uid()));
    }
}


bool EventHandler::getEvents (KCal::Event::List& p_events, const QStringList& p_ids)
{
    if (p_ids.begin() == p_ids.end())
        return true;

    if (!getTypeId())
    {
        return false;
    }


    QStringList::const_iterator it = p_ids.begin();
    QString vEvent;
    KCal::Incidence* event;
    KCal::ICalFormat conv;
    conv.setTimeZone(sCurrentTimeZone, false);

    QString vCalBegin = "BEGIN:VCALENDAR\nPRODID:-//K Desktop Environment//NONSGML KOrganizer 3.2.1//EN\nVERSION:2.0\n";
    QString vCalEnd = "END:VCALENDAR\n";

    for (; it != p_ids.end(); ++it)
    {
        QString id = *it;
        if (isARraId (id)) // this is not an RRA-ID!!
        {
            //vEvent = vCalBegin + m_rra->getVEvent(s_typeIdEvent, getOriginalId((id))) + vCalEnd;;
            vEvent = m_rra->getVEvent(s_typeIdEvent, getOriginalId((id)));
            if (!vEvent.isEmpty())
            {
                vEvent = vCalBegin + vEvent + vCalEnd;
                //kdDebug() << "EventHandler::getEvents:" << endl;
                //kdDebug() << vEvent << endl;
                event = conv.fromString (vEvent);
                p_events.push_back (dynamic_cast<KCal::Event*>(event));
            }
        }
    }

    return true;
}


QString EventHandler::makeVIncidence(KCal::Incidence* p_incidence)
{
    KCal::ICalFormat calFormat;

    calFormat.setTimeZone(sCurrentTimeZone, false);

    QString vIncidence;

    vIncidence = calFormat.toICalString(p_incidence);

    //kdDebug() << "EventHandler::makeVIncidence: converted string" << endl;
    //kdDebug() << vIncidence << endl;
    // remove first three lines and the last line of the vEvent!
    vIncidence = vIncidence.remove (QRegExp ("BEGIN.*VERSION:2.0"));
    //vIncidence = vIncidence.remove (m_incidenceRegexp);
    vIncidence = vIncidence.remove ("END:VCALENDAR");
    vIncidence = vIncidence.stripWhiteSpace();
    vIncidence = vIncidence + "\n";

    int pos;
    QString vAlarm = "END:VALARM";
    if ((pos = vIncidence.find(vAlarm)) != -1) // remove the empty line after EDN:VALARM
    {
        QChar newLine = vIncidence.at (pos + vAlarm.length());
        if (vIncidence.at(pos+vAlarm.length()+1) == newLine)
            vIncidence = vIncidence.remove(pos+vAlarm.length()+1, 1);
    }

    return vIncidence;
}

}
