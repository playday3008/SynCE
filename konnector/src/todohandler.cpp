//
// C++ Implementation: todohandler
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "todohandler.h"

#include <kdebug.h>
//#include <qregexp.h>

//#include <libkcal/vcalformat.h>
#include <libkcal/icalformat.h>
#include <libkcal/event.h>
#include <libkcal/calendarnull.h>
#include <libkcal/calendarlocal.h>
#include <qfile.h>

namespace pocketPCCommunication {

uint32_t TodoHandler::s_typeIdTodo = 0;

TodoHandler::TodoHandler(const QString& p_pdaName)
    : PimHandler(p_pdaName)
{
    m_incidenceRegexp = QRegExp ("BEGIN.*VERSION:2.0");

    s_typeIdTodo = 0;

    QFile f("/etc/timezone");
    if(f.open(IO_ReadOnly))
    {
        QTextStream ts(&f);
        ts >> sCurrentTimeZone;
    }

    f.close();
}


TodoHandler::TodoHandler(KSharedPtr<Rra> p_rra)
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


TodoHandler::~TodoHandler()
{
}


bool TodoHandler::getAllTodos (KCal::Calendar& p_calendar, RecordType p_recType)
{
    kdDebug(2120) << "[TodoHandler]: getCalendarTodos" << endl;

    if (!getTypeId())
    {
        return false;
    }

    struct Rra::ids ids;

    if (!m_rra->getIds (s_typeIdTodo, &ids))
    {
        return false;
    }

    // and now.. get the object ids...
    kdDebug(2120) << "[TodoHandler]: got todo ids.. fetching information" << endl;

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

    return true;
}


void TodoHandler::getTodoEntry (KCal::Calendar& p_calendar, const struct Rra::ids& p_ids, RecordType p_recType)
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
        //kdDebug(2120) << "[TodoHandler]: calendar: " << vCal << endl;

        if (!calFormat.fromString (&p_calendar, vCal))
            kdDebug(2120) << "[TodoHandler]: getting of todo failed!" << endl;
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


bool TodoHandler::putTodos (KCal::Calendar& p_calendar)
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
        kdDebug(2120) << "[TodoHandler]: putCalendarTasks: todo.summary(): " << (*it)->summary() << endl;
        // convert this to a vTodo!
        //vTodo = calFormat.toICalString (*it);
        vTodo = makeVIncidence (*it);

        //kdDebug(2120) << "[TodoHandler]: putCalendarTasks: calString: " << vTodo << endl;

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

        //kdDebug(2120) << "[TodoHandler]: putCalendarTodos: calString after removal: " << endl << vTodo << endl;


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

    return true;
}


bool TodoHandler::getTypeId ()
{
    if (!s_typeIdTodo)
        s_typeIdTodo = m_rra->getTypeForName(RRA_SYNCMGR_TYPE_TASK);

    if (!s_typeIdTodo)
        return false;
    else
        return true;
}


bool TodoHandler::getIdStatus (QMap<QString, RecordType>& p_statusMap)
{

    if (!getTypeId())
    {
        return false;
    }

    return getIdStatusPro(p_statusMap, s_typeIdTodo);
}


void TodoHandler::deleteTodoEntry(const uint32_t& p_objectId)
{
    kdDebug() << "TodoHandler::deleteTodoEntry" << endl;

    if (!getTypeId())
        return;

    deleteSingleEntry (s_typeIdTodo, p_objectId);
}


void TodoHandler::addTodos (KCal::Todo::List& p_todos)
{
    if (p_todos.begin() == p_todos.end())
        return;

    if (!getTypeId())
    {
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
            //kdDebug() << "TodoHandler vincidence: " << endl;
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
}


void TodoHandler::updateTodos (KCal::Todo::List& p_todos)
{
    if (p_todos.begin() == p_todos.end())
        return;

    if (!getTypeId())
    {
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
}


void TodoHandler::removeTodos (KCal::Todo::List& p_todos)
{
    if (p_todos.begin() == p_todos.end())
        return;

    if (!getTypeId()) {
        return;
    }

    KCal::Todo::List::Iterator it = p_todos.begin();

    for (; it != p_todos.end(); ++it)
    {
        if (isARraId((*it)->uid()))
            deleteTodoEntry(getOriginalId((*it)->uid()));
    }
}


bool TodoHandler::getTodos (KCal::Todo::List& p_todos, const QStringList& p_ids)
{
    if (p_ids.begin() == p_ids.end())
        return true;

    if (!getTypeId())
    {
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


QString TodoHandler::makeVIncidence(KCal::Incidence* p_incidence)
{
    KCal::ICalFormat calFormat;

    calFormat.setTimeZone(sCurrentTimeZone, false);

    QString vIncidence;

    vIncidence = calFormat.toICalString(p_incidence);

    //kdDebug() << "TodoHandler::makeVIncidence: converted string" << endl;
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
