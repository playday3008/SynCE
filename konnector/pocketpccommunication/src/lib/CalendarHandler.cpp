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
#include "CalendarHandler.h"

#include <kdebug.h>
//#include <qregexp.h>

//#include <libkcal/vcalformat.h>
#include <libkcal/icalformat.h>
#include <libkcal/event.h>
#include <libkcal/calendarnull.h>
#include <libkcal/calendarlocal.h>

namespace pocketPCCommunication {

uint32_t CalendarHandler::s_typeIdEvent = 0;
uint32_t CalendarHandler::s_typeIdTodo = 0;

CalendarHandler::CalendarHandler(const QString& p_pdaName)
    : PimHandler(p_pdaName)
{
    m_incidenceRegexp = QRegExp ("BEGIN.*VERSION:2.0");
}


CalendarHandler::CalendarHandler(KSharedPtr<Rra> p_rra)
    : PimHandler(p_rra)
{
}


CalendarHandler::~CalendarHandler()
{
}


bool CalendarHandler::getCalendarEvents (KCal::Calendar& p_calendar, RecordType p_recType)
{
    kdDebug(2120) << "[CalendarHandler]: getCalendarEvents" << endl;
    
    //Rra rra(m_pdaName);
    m_rra->connect();

    if (!getTypeId())
    {
        m_rra->disconnect();
        return false;
    }
        
    struct Rra::ids ids;
    
    if (!m_rra->getIds (s_typeIdEvent, &ids))    
    {
        m_rra->disconnect();
        return false;
    }
    
    // and now.. get the object ids...
    kdDebug(2120) << "[CalendarHandler]: got event ids.. fetching information" << endl;
         
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
    
    m_rra->disconnect();
    
    // iterate over calendar and add X-POCKETPCCOMM-REMOTE_ID_* 
    
    return true;
}


bool CalendarHandler::getCalendarTodos (KCal::Calendar& p_calendar, RecordType p_recType)
{
    kdDebug(2120) << "[CalendarHandler]: getCalendarTodos" << endl;
    
    //Rra rra(m_pdaName);
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
        return false;
    }
    
    struct Rra::ids ids;
    
    if (!m_rra->getIds (s_typeIdTodo, &ids))    
    {
        m_rra->disconnect();
        return false;
    }
    
    // and now.. get the object ids...
    kdDebug(2120) << "[CalendarHandler]: got todo ids.. fetching information" << endl;
         
    switch (p_recType)
    {
        case ALL:
            getTodoEntry (p_calendar, ids, CHANGED);
            getTodoEntry (p_calendar, ids, DELETED);
            getTodoEntry (p_calendar, ids, UNCHANGED);
            break;        
        case CHANGED:
            getTodoEntry (p_calendar, ids, CHANGED);
            break;
        case DELETED:
            getTodoEntry (p_calendar, ids, DELETED);
            break;
        case UNCHANGED:
            getTodoEntry (p_calendar, ids, UNCHANGED);
            break;
    }
    m_rra->disconnect();
    
    return true;
}


void CalendarHandler::getEventEntry (KCal::Calendar& p_calendar, const struct Rra::ids& p_ids, RecordType p_recType)
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
    KCal::CalendarLocal cal;
    
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
        kdDebug(2120) << "[CalendarHandler2]::getEventEntry calendar: " << vCal << endl;

        if (!calFormat.fromString (&p_calendar, vCal))
            kdDebug(2120) << "[CalendarHandler]: getting of event failed!" << endl;  
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


void CalendarHandler::getTodoEntry (KCal::Calendar& p_calendar, const struct Rra::ids& p_ids, RecordType p_recType)
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
    
    KCal::ICalFormat calFormat; // NEEDED FOR TODOS!!!
    KCal::CalendarLocal cal;
    
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
        vCal = vCalBegin + m_rra->getVToDo(s_typeIdTodo, *it) + vCalEnd;
        //kdDebug(2120) << "[CalendarHandler]: calendar: " << vCal << endl;

        if (!calFormat.fromString (&p_calendar, vCal))
            kdDebug(2120) << "[CalendarHandler]: getting of todo failed!" << endl;  
        else
        {
            /*
            const QString key = m_keyName + "-" + partnerId;
            const QString value = "RRA-ID-" + QString::number (*it, 16).rightJustify (8, '0');                   
            */
            //KCal::Todo* todo = p_calendar.todo(value);
            //todo->setNonKDECustomProperty (("X-"+m_appName+"-"+key).latin1(), value);            
            
            //ev->updated();
        }

    }    
}


bool CalendarHandler::putCalendarEvents (KCal::Calendar& p_calendar)
{
    //Rra rra(m_pdaName);
    //rra.connect();        
    
    KCal::Event::List eList = p_calendar.rawEvents();
    KCal::Event::List::iterator it = eList.begin();
    
    QString vEvent;
    uint32_t objectId = 0;
    uint32_t newObjectId;
    bool ok;
    
    if (eList.begin() == eList.end())
        return true;
    
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
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
        kdDebug(2120) << "[CalendarHandler]: putCalendarEvents: event.summary(): " << (*it)->summary() << endl;
        // convert this to a vEvent!
        vEvent = makeVIncidence (*it); 
        //kdDebug(2120) << "[CalendarHandler]: putCalendarEvents: calString: " << vEvent << endl;
        
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
    m_rra->disconnect();
    return true;
}


bool CalendarHandler::putCalendarTodos (KCal::Calendar& p_calendar)
{
    //Rra rra(m_pdaName);
    //rra.connect();
        
    KCal::Todo::List eList = p_calendar.rawTodos();
    KCal::Todo::List::iterator it = eList.begin();
    
    QString vTodo;
    uint32_t objectId = 0;
    uint32_t newObjectId;
    bool ok;
    
    if (eList.begin() == eList.end())
        return true;
    
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
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
        kdDebug(2120) << "[CalendarHandler]: putCalendarTasks: todo.summary(): " << (*it)->summary() << endl;
        // convert this to a vTodo!
        //vTodo = calFormat.toICalString (*it);
        vTodo = makeVIncidence (*it);
        
        //kdDebug(2120) << "[CalendarHandler]: putCalendarTasks: calString: " << vTodo << endl;
        
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
        newObjectId = 0;
        
        //kdDebug(2120) << "[CalendarHandler]: putCalendarTodos: calString after removal: " << endl << vTodo << endl;
        
        
        newObjectId = m_rra->putVToDo(vTodo, s_typeIdTodo, objectId);
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
    m_rra->disconnect();
    
    return true;
}

void CalendarHandler::deleteCalendar ()
{
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
        return;
    }
    
    struct pocketPCCommunication::Rra::ids eventIds;
    if (!m_rra->getIds (s_typeIdEvent, &eventIds))
    {
        m_rra->disconnect();
        return;
    }
    
    deleteEntries(eventIds, s_typeIdEvent, CHANGED);
    deleteEntries(eventIds, s_typeIdEvent, UNCHANGED);
    deleteEntries(eventIds, s_typeIdEvent, DELETED);
    
    m_rra->finalDisconnect();
    m_rra->connect();
    
    
    struct pocketPCCommunication::Rra::ids todoIds;
    if (!m_rra->getIds (s_typeIdTodo, &todoIds))
    {
        m_rra->disconnect();
        return;
    }
    
    deleteEntries(todoIds, s_typeIdTodo, CHANGED);
    deleteEntries(todoIds, s_typeIdTodo, UNCHANGED);
    deleteEntries(todoIds, s_typeIdTodo, DELETED);

    m_rra->disconnect();
}


void CalendarHandler::deleteCalendarEntries (const uint32_t& p_typeId, RecordType p_recType)
{
    //pocketPCCommunication::Rra rra(m_pdaName);
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
        return;
    }
    
    struct pocketPCCommunication::Rra::ids ids;
    if (!m_rra->getIds (p_typeId, &ids))
    {
        m_rra->disconnect();
        return;
    }
    
    QValueList<uint32_t>::const_iterator it;
    QValueList<uint32_t>::const_iterator end;
    
    switch (p_recType)
    {
        case CHANGED:
            it = ids.changedIds.begin();
            end = ids.changedIds.end();            
            break;
        case DELETED:
            it = ids.deletedIds.begin();
            end = ids.deletedIds.end();
            break;
        case UNCHANGED:            
            it = ids.unchangedIds.begin();
            end = ids.unchangedIds.end();
            break;
        case ALL: // not reasonable to have ALL here! added to avoid warning of gcc
            break;
    }
    
    for (; it != end; ++it)
        m_rra->deleteObject (p_typeId, *it);
    
    m_rra->disconnect();
}


bool CalendarHandler::getTypeId ()
{
    if (!s_typeIdEvent)
        s_typeIdEvent = m_rra->getTypeForName(RRA_SYNCMGR_TYPE_APPOINTMENT);
        
    //s_typeIdEvent = 10003;
    
    if (!s_typeIdTodo)
        s_typeIdTodo = m_rra->getTypeForName(RRA_SYNCMGR_TYPE_TASK);
    
    if (!s_typeIdEvent || !s_typeIdTodo)
        return false;
    else
        return true;
}


bool CalendarHandler::getIdStatus (QMap<QString, RecordType>& p_statusMap)
{
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
        return false;
    }
    //m_rra->finalDisconnect();
    
    /*
    if (!getIdStatusPro (p_statusMap, s_typeIdEvent))
        return false;
    return getIdStatusPro (p_statusMap, s_typeIdTodo);
    */
    
    bool ok;
    ok = getIdStatusPro (p_statusMap, s_typeIdEvent);
    if (ok)
    {
        m_rra->finalDisconnect();
        m_rra->connect();
        ok = getIdStatusPro(p_statusMap, s_typeIdTodo);
    }
    m_rra->disconnect();
    
    return ok;
}


void CalendarHandler::deleteEventEntry (const uint32_t& p_objectId)
{
    kdDebug() << "CalendarHandler::deleteCalendarEntry" << endl;
    
    if (!getTypeId())
        return;
    
    deleteSingleEntry (s_typeIdEvent, p_objectId);
}


void CalendarHandler::deleteTodoEntry (const uint32_t& p_objectId)
{
    kdDebug() << "CalendarHandler::deleteTodoEntry" << endl;
    
    if (!getTypeId())
        return;
    
    deleteSingleEntry (s_typeIdTodo, p_objectId);
}


void CalendarHandler::addEvents (KCal::Event::List& p_events)
{
    if (p_events.begin() == p_events.end())
        return;
    
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
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
			//kdDebug() << "CalendarHandler vincidence: " << endl;
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
    
    m_rra->disconnect();
}


void CalendarHandler::addTodos (KCal::Todo::List& p_todos)
{
    if (p_todos.begin() == p_todos.end())
        return;
    
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
        return;
    }
    
    KCal::Todo::List::Iterator it = p_todos.begin();
    QString vEvent;
    uint32_t newObjectId;
    QString curId; 
    
    for (; it != p_todos.end(); ++it)
    {
        //if (!isARraId ((*it)->uid())) // this must be a new entry!!!
        {
            curId = (*it)->uid();
            vEvent = makeVIncidence (*it);            
			//kdDebug() << "CalendarHandler vincidence: " << endl;
			//kdDebug() << vEvent << endl;          
            uint32_t remId = 0;
            if (isARraId(curId))
                remId = getOriginalId(curId);
              
            newObjectId = m_rra->putVToDo (vEvent, s_typeIdTodo, remId); //getOriginalId((*it)->uid()));
            
            if (newObjectId) // must be!!!
            {
                //(*it)->setNonKDECustomProperty (("X-"+m_appName+"-"+m_keyName + "-" + partnerId).latin1(), "RRA-ID-" + QString::number(newObjectId, 16).rightJustify(8, '0'));
                if ("RRA-ID-" + QString::number(newObjectId, 16).rightJustify(8, '0') != curId)
                    addIdPair("RRA-ID-" + QString::number(newObjectId, 16).rightJustify(8, '0'), curId);
            }
        }
    }
    
    m_rra->disconnect();
}
 

void CalendarHandler::updateEvents (KCal::Event::List& p_events)
{
    if (p_events.begin() == p_events.end())
        return;
    
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
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
        
    m_rra->disconnect();    
}     


void CalendarHandler::updateTodos (KCal::Todo::List& p_todos)
{
    if (p_todos.begin() == p_todos.end())
        return;
    
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
        return;
    }
    
    KCal::Todo::List::Iterator it = p_todos.begin();
    QString vEvent;
    QString curId;
    
    for (; it != p_todos.end(); ++it)
    {
        if (isARraId ((*it)->uid())) // must exist on device!!!
        {
            curId = (*it)->uid();
            vEvent = makeVIncidence (*it);
            m_rra->putVToDo (vEvent, s_typeIdTodo, getOriginalId((*it)->uid()));
        }
    }
        
    m_rra->disconnect();    
}     


void CalendarHandler::removeEvents (KCal::Event::List& p_events)
{
    if (p_events.begin() == p_events.end())
        return;
    
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
        return;
    }
    
    KCal::Event::List::Iterator it = p_events.begin();
    
    for (; it != p_events.end(); ++it)
    {
        if (isARraId((*it)->uid()))
            deleteEventEntry (getOriginalId((*it)->uid()));
    }
    
    m_rra->disconnect();
}


void CalendarHandler::removeTodos (KCal::Todo::List& p_todos)
{
    if (p_todos.begin() == p_todos.end())
        return;
    
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
        return;
    }
    
    KCal::Todo::List::Iterator it = p_todos.begin();
    
    for (; it != p_todos.end(); ++it)
    {
        if (isARraId((*it)->uid()))
            deleteTodoEntry(getOriginalId((*it)->uid()));
    }
    
    m_rra->disconnect();
}


bool CalendarHandler::getEvents (KCal::Event::List& p_events, const QStringList& p_ids)
{
    if (p_ids.begin() == p_ids.end())
        return true;
    
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
        return false;
    }
    
    QStringList::const_iterator it = p_ids.begin();
    QString vEvent;
    KCal::Incidence* event;
    KCal::ICalFormat conv;
    
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
				//kdDebug() << "CalendarHandler::getEvents:" << endl;
				//kdDebug() << vEvent << endl;
                event = conv.fromString (vEvent);                        
                p_events.push_back (dynamic_cast<KCal::Event*>(event));        
            }
        }
    }
    
    return true;
}


bool CalendarHandler::getTodos (KCal::Todo::List& p_todos, const QStringList& p_ids)
{
    if (p_ids.begin() == p_ids.end())
        return true;
    
    m_rra->connect();
    
    if (!getTypeId())
    {
        m_rra->disconnect();
        return false;
    }
    
    QStringList::const_iterator it = p_ids.begin();
    QString vTodo;
    KCal::Incidence* todo;
    KCal::ICalFormat conv;
    
    QString vCalBegin = "BEGIN:VCALENDAR\nPRODID:-//K Desktop Environment//NONSGML KOrganizer 3.2.1//EN\nVERSION:2.0\n";
    QString vCalEnd = "END:VCALENDAR\n";
    
    for (; it != p_ids.end(); ++it)
    {
        QString id = *it;
        if (isARraId (id)) // this is not an RRA-ID!!
        {
            //vTodo = vCalBegin + m_rra->getVEvent(s_typeIdTodo, getOriginalId((id))) + vCalEnd;
			vTodo = m_rra->getVToDo(s_typeIdTodo, getOriginalId((id)));
            if (!vTodo.isEmpty())
            {
				vTodo = vCalBegin + vTodo + vCalEnd;
                todo = conv.fromString (vTodo);                        
                p_todos.push_back (dynamic_cast<KCal::Todo*>(todo));        
            }
        }
    }
    
    return true;
}
        

QString CalendarHandler::makeVIncidence(KCal::Incidence* p_incidence)
{
    KCal::ICalFormat calFormat;

    QString vIncidence;
    
    vIncidence = calFormat.toICalString(p_incidence);
            
	//kdDebug() << "CalendarHandler::makeVIncidence: converted string" << endl;
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
    
};
