/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
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

#include "rra.h"
#include "rapiwrapper.h"

extern "C" {
#include <rra/appointment.h>
#include <rra/task.h>
#include <rra/contact.h>
}
#include <synce_log.h>

#include <kdebug.h>

#include <stdlib.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif


namespace pocketPCCommunication {

Rra::Rra(QString pdaName)
    //: KShared(*this)
{
    kdDebug() << "Rra::pdaName: " << pdaName << endl;
    rraOk = true;
    this->pdaName = pdaName;
    rra = 0;
    rra = rra_syncmgr_new();
    matchmaker = 0;

    useCount = 0;
}


Rra::Rra()
    //: KShared(*this)
{
    rraOk = true;
    this->pdaName = ""; //pdaName;
    rra = 0;
    rra = rra_syncmgr_new();

    matchmaker = 0;

    useCount = 0;
}


Rra::~Rra()
{
    // should useCount be set to 0 here???
    //useCount = 0; // make a real disconnect!!! nobody needs this anymore!
    //disconnect();
    finalDisconnect();
    rra_syncmgr_destroy(rra);
}


bool Rra::connect()
{
    rraOk = true;

    /*
    if (!rra)
    {
        return false;
    }
    */

    if (useCount == 0) {
        //if (!rra)
        //    rra = rra_syncmgr_new();
        if (!Ce::rapiInit(pdaName)) {
            rraOk = false;
        } else if (rra_syncmgr_connect(rra)) {
            matchmaker = rra_matchmaker_new();
            m_matchMaker = new MatchMaker (matchmaker);
            kdDebug(2120) << "RRA-Connect" << endl;
            if (getTimezone(&tzi)) {
                kdDebug(2120) << "rra_timezone_get ok" << endl;
            } else {
                kdDebug(2120) << "rra_timezone_get fault" << endl;
                rraOk = false;
                Ce::rapiUninit();
            }
        }
        else {
            rraOk = false;
            Ce::rapiUninit();
        }
    }

    if (rraOk) {
        useCount++;
    }

    return rraOk;
}


void Rra::disconnect()
{
    if (useCount > 0) {
        useCount--;
    }

    if(useCount == 0 && rra_syncmgr_is_connected(rra)) {
        kdDebug(2120) << "RRA-Disconnect" << endl;

        rra_matchmaker_destroy(matchmaker);
        matchmaker = 0;
        rra_syncmgr_disconnect(rra);
        Ce::rapiUninit();
    }
}


void Rra::finalDisconnect()
{
    useCount = 0;
    disconnect();
}


bool Rra::ok()
{
    return rraOk;
}


bool Rra::getTypes(QMap<int, RRA_SyncMgrType *> *objectTypes)
{
    RRA_SyncMgrType *object_types = NULL;
    size_t object_type_count = 0;

    rraOk = true;

    objectTypes->clear();

    if (connect()) {
        object_type_count = rra_syncmgr_get_type_count(rra);
        object_types = rra_syncmgr_get_types(rra);
        if (object_types) {
            for (size_t i = 0; i < object_type_count; i++) {
                objectTypes->insert(object_types[i].id, &object_types[i]);
            }
        } else {
            rraOk = false;
        }
        disconnect();
    } else {
        rraOk = false;
    }

    return rraOk;
}


uint32_t Rra::getTypeForName (const QString& p_typeName)
{
    RRA_SyncMgrType* id;
    id = rra_syncmgr_type_from_name (rra, p_typeName.latin1());
    if (id)
        return id->id;
    else
        return 0;
}


static bool callback(RRA_SyncMgrTypeEvent event, uint32_t /*type*/, uint32_t count,
        uint32_t *ids, void *cookie)
{
    QValueList<uint32_t> *eventIds;
    Rra::ids *_ids = (Rra::ids *) cookie;

    switch(event) {
    case SYNCMGR_TYPE_EVENT_UNCHANGED:
        eventIds = &_ids->unchangedIds;
        kdDebug(2120) << "----------------------- unchanged --------" << endl;
        break;
    case SYNCMGR_TYPE_EVENT_CHANGED:
        eventIds = &_ids->changedIds;
        kdDebug(2120) << "------------------------ changed ---------" << endl;
        break;
    case SYNCMGR_TYPE_EVENT_DELETED:
        eventIds = &_ids->deletedIds;
        kdDebug(2120) << "------------------------ deleted ---------" << endl;
        break;
    default:
        eventIds = NULL;
        break;
    }

    if (eventIds != NULL) {
        for (uint32_t i = 0; i < count; i++) {
            eventIds->append(ids[i]);
        }
    }

    return true;
}


bool Rra::getIds(uint32_t type_id, struct Rra::ids *ids)
{
    rraOk = true;

    _ids.changedIds.clear();
    _ids.unchangedIds.clear();
    _ids.deletedIds.clear();

    bool gotEvent = false;
    if (connect()) {
        rra_syncmgr_subscribe(rra, type_id, callback, &_ids);
        if (rra_syncmgr_start_events(rra)) {
            while(rra_syncmgr_event_wait(rra, 3, &gotEvent) && gotEvent) { // changed for new syncmgr.h
                rra_syncmgr_handle_event(rra);
            }
        } else {
            rraOk = false;
        }
        kdDebug(2120) << "UNSUBSCRIBING TYPE: " << type_id << endl;
        rra_syncmgr_unsubscribe(rra, type_id);
        kdDebug(2120) << type_id << "UNSUBSCRIBED" << endl;
        disconnect();
    } else {
        rraOk = false;
    }

    *ids = _ids;

    return rraOk;
}


QString Rra::getVCard(uint32_t type_id, uint32_t object_id)
{
    uint8_t* data = NULL;
    size_t data_size = 0;
    char *vcard = NULL;

    rraOk = true;
    QString vCard = "";

    if (connect()) {
        if (!rra_syncmgr_get_single_object(rra, type_id, object_id, &data,
                            &data_size)) {
            rraOk = false;
        } else if (!rra_contact_to_vcard(object_id, data, data_size, &vcard,
                                         RRA_CONTACT_VERSION_3_0)) {
            rraOk = false;
        }
        disconnect();
    }

    if (rraOk) {
        vCard = vcard;
    }

    if (data) {
        free(data);
    }

    if (vcard) {
        free(vcard);
    }

    return vCard;
}


uint32_t Rra::putVCard(QString& vCard, uint32_t type_id, uint32_t object_id)
{
    uint32_t new_object_id = 0;
    uint8_t *buffer = NULL;
    size_t buffer_size = 0;
    rraOk = true;

    if (connect()) {
        const char *vCardc = vCard.ascii();
        if (!rra_contact_from_vcard(vCardc, NULL, &buffer, &buffer_size,
                                    ((object_id != 0) ? RRA_CONTACT_UPDATE :
                                    RRA_CONTACT_NEW) | RRA_CONTACT_ISO8859_1 |
                                    RRA_CONTACT_VERSION_3_0)) {
            rraOk = false;
        } else if (!rra_syncmgr_put_single_object(rra, type_id, object_id,
                                   (object_id != 0) ? RRA_SYNCMGR_UPDATE_OBJECT : RRA_SYNCMGR_NEW_OBJECT, buffer,
                                   buffer_size, &new_object_id)) {
            rraOk = false;
        }

        if (buffer) {
            free(buffer);
        }
        disconnect();
    }

    return new_object_id;
}


QString Rra::getVEvent(uint32_t type_id, uint32_t object_id)
{
    uint8_t* data = NULL;
    size_t data_size = 0;
    char *vevent = NULL;

    rraOk = true;
    QString vEvent = "";

    if (connect()) {
        if (!rra_syncmgr_get_single_object(rra, type_id, object_id, &data,
                &data_size)) {
            rraOk = false;
        } else if (!rra_appointment_to_vevent(object_id, data, data_size,
                &vevent, 0, NULL)) {
            rraOk = false;
        }

        disconnect();
    }

    if (rraOk) {
        vEvent = vevent;
        kdDebug(2120) << "getVEvent --------------" << endl;
        kdDebug(2120) << vEvent << endl;
        kdDebug(2120) << "------------------------" << endl;
    }

    if (data) {
        free(data);
    }

    if (vevent) {
        free(vevent);
    }

    return vEvent;
}


uint32_t Rra::putVEvent(QString& vEvent, uint32_t type_id, uint32_t object_id)
{
    uint32_t new_object_id = 0;
    uint8_t *buffer = NULL;
    size_t buffer_size = 0;
    rraOk = true;

        kdDebug(2120) << "putVEvent --------------" << endl;
        kdDebug(2120) << vEvent << endl;
        kdDebug(2120) << "------------------------" << endl;
    if (connect()) {
        const char *vevent = vEvent.ascii();
        if (!rra_appointment_from_vevent(vevent, NULL, &buffer,
                &buffer_size, ((object_id != 0) ? RRA_APPOINTMENT_UPDATE :
                RRA_APPOINTMENT_NEW) | RRA_APPOINTMENT_ISO8859_1, NULL)) {
            rraOk = false;
        } else if (!rra_syncmgr_put_single_object(rra, type_id, object_id,
                (object_id != 0) ? RRA_SYNCMGR_UPDATE_OBJECT : RRA_SYNCMGR_NEW_OBJECT, buffer,
                buffer_size, &new_object_id)) {
            rraOk = false;
        }

        if (buffer) {
            free(buffer);
        }

        disconnect();
    }

    return new_object_id;
}


QString Rra::getVToDo(uint32_t type_id, uint32_t object_id)
{
    uint8_t* data = NULL;
    size_t data_size = 0;
    char *vtask = NULL;

    rraOk = true;
    QString vTask = "";

    if (connect()) {
        if (!rra_syncmgr_get_single_object(rra, type_id, object_id, &data,
                &data_size)) {
            rraOk = false;
        } else if (!rra_task_to_vtodo(object_id, data, data_size,
                &vtask, 0, &tzi)) {
            rraOk = false;
        }

        disconnect();
    }

    if (rraOk) {
        vTask = vtask;
    }

    if (data) {
        free(data);
    }

    if (vtask) {
        free(vtask);
    }

    return vTask ;
}


uint32_t Rra::putVToDo(QString& vToDo, uint32_t type_id, uint32_t object_id)
{
    uint32_t new_object_id = 0;
    uint8_t *buffer = NULL;
    size_t buffer_size = 0;
    rraOk = true;

    if (connect()) {
        const char *vtodo = vToDo.ascii();
        if (!rra_task_from_vtodo(vtodo, NULL, &buffer,
                &buffer_size, ((object_id != 0) ? RRA_TASK_UPDATE :
                RRA_TASK_NEW) | RRA_TASK_ISO8859_1, NULL)) {
            rraOk = false;
        } else if (!rra_syncmgr_put_single_object(rra, type_id, object_id,
                (object_id != 0) ? RRA_SYNCMGR_UPDATE_OBJECT : RRA_SYNCMGR_NEW_OBJECT, buffer,
                buffer_size, &new_object_id)) {
            rraOk = false;
        }

        if (buffer) {
            free(buffer);
        }

        disconnect();
    }

    return new_object_id;
}


void Rra::deleteObject(uint32_t type_id, uint32_t object_id)
{
    rraOk = true;

    if (connect()) {
        if (!rra_syncmgr_delete_object(rra, type_id, object_id)) {
            rraOk = false;
        }
        disconnect();
    }
}


MatchMaker* Rra::getMatchMaker()
{
    return m_matchMaker;
}


QString Rra::getPdaName() const
{
    return pdaName;
}


bool Rra::isConnected() const
{
    return (useCount>0);
}


void Rra::setLogLevel (int p_level)
{
    synce_log_set_level(p_level);
}
} // namespace


/*!
    \fn pocketPCCommunication::Rra::getTimezone(RRA_TimeZone *tzi)
 */
bool pocketPCCommunication::Rra::getTimezone(RRA_Timezone *tzi)
{
    return rra_timezone_get(tzi);
}
