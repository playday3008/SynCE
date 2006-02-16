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

#include <kabc/addressee.h>
#include <klocale.h>
#include <kdebug.h>

#include <stdlib.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

#define MAX_FIELD_COUNT 60

Rra::Rra(QString pdaName)
{
    rraOk = true;
    this->pdaName = pdaName;
    rra = rra_syncmgr_new();
    useCount = 0;
}


Rra::~Rra()
{
    disconnect();
    rra_syncmgr_destroy(rra);
}


bool Rra::connect()
{
    rraOk = true;

    if (useCount == 0) {
        if (!Ce::rapiInit(pdaName)) {
            rraOk = false;
        } else if (rra_syncmgr_connect(rra)) {
            kdDebug(2120) << i18n("RRA-Connect") << endl;
        } else {
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
        if(useCount == 0) {
            kdDebug(2120) << i18n("RRA-Disconnect") << endl;

            rra_syncmgr_disconnect(rra);
            Ce::rapiUninit();
        }
    }
}


bool Rra::ok()
{
    return rraOk;
}


bool Rra::getTypes(QValueList<uint32_t> *objectTypes)
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
                objectTypes->append(object_types[i].id);
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


RRA_SyncMgrType* Rra::getTypeForId(const uint32_t type_id)
{
    RRA_SyncMgrType* id;
    id = rra_syncmgr_type_from_id (rra, type_id);

    return id;
}


static bool callback(RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count,
        uint32_t *ids, void *cookie)
{
    QValueList<uint32_t> *eventIds;
    Rra::ids *_ids = (Rra::ids *) cookie;

    switch(event) {
    case SYNCMGR_TYPE_EVENT_UNCHANGED:
        eventIds = &_ids->unchangedIds;
        kdDebug(2120) << "    " << count << " IDs unchanged of type " << type << endl;
        break;
    case SYNCMGR_TYPE_EVENT_CHANGED:
        eventIds = &_ids->changedIds;
        kdDebug(2120) << "    " << count << " IDs changed of type" << type << endl;
        break;
    case SYNCMGR_TYPE_EVENT_DELETED:
        eventIds = &_ids->deletedIds;
        kdDebug(2120) << "    " << count << " IDs deleted of type" << type << endl;
        break;
    default:
        eventIds = NULL;
        break;
    }

    if (eventIds != NULL) {
        for (uint32_t i = 0; i < count; i++) {
            eventIds->append(ids[i]);
            rra_uint32vector_add(_ids->uidVector, ids[i]);
        }
    }

    return true;
}


bool Rra::checkForAllIdsRead()
{
    size_t totalIds = 0;
    size_t readIds = 0;

    for (QMap<uint32_t, Rra::ids *>::iterator it = idMap.begin(); it != idMap.end(); ++it) {
        totalIds += rra_syncmgr_type_from_id(rra, it.key())->count;
        readIds += it.data()->unchangedIds.count() + it.data()->changedIds.count() + it.data()->deletedIds.count();
    }
    return readIds == totalIds;
}


void Rra::subscribeForType(uint32_t typeId)
{
    Rra::ids *ids = new Rra::ids;
    ids->changedIds.clear();
    ids->unchangedIds.clear();
    ids->deletedIds.clear();
    idMap.insert(typeId, ids);
    rra_syncmgr_subscribe(rra, typeId, callback, ids);
}


void Rra::unsubscribeType(uint32_t typeId)
{
    QMap<uint32_t, Rra::ids *>::iterator it = idMap.find(typeId);

    if (it != idMap.end()) {
        delete(it.data());
    }

    idMap.remove(typeId);

    rra_syncmgr_unsubscribe(rra, typeId);
}


void Rra::getIdsForType( uint32_t mTypeId, Rra::ids *ids )
{
    QMap<uint32_t, Rra::ids *>::iterator it = idMap.find(mTypeId);

    if (it != idMap.end()) {
        Rra::ids *_ids = it.data();

        if (_ids) {
            *ids = *(_ids);
        }
    }
}


bool Rra::getIds()
{
    rraOk = true;

    bool allIdsRead = false;


    bool gotEvent = false;

    if (rra_syncmgr_start_events(rra)) {

        allIdsRead = checkForAllIdsRead();

        for (QMap<uint32_t, Rra::ids *>::iterator it = idMap.begin(); it != idMap.end(); ++it) {
            it.data()->uidVector = rra_uint32vector_new();
        }

        if (!allIdsRead ) {
            kdDebug(2120) << "Waiting for ids to come in" << endl;
            while(rra_syncmgr_event_wait(rra, 3, &gotEvent) && gotEvent && !allIdsRead) {
                rra_syncmgr_handle_event(rra);
                allIdsRead = checkForAllIdsRead();
            }
        } else {
            rra_syncmgr_handle_event(rra);
        }

        for (QMap<uint32_t, Rra::ids *>::iterator it = idMap.begin(); it != idMap.end(); ++it) {
            RRA_Uint32Vector* uidVector = rra_uint32vector_new();


            rra_syncmgr_get_deleted_object_ids(rra, it.key(), it.data()->uidVector, uidVector);
            for (size_t i = 0; i < uidVector->used; i++) {
                it.data()->deletedIds.append(uidVector->items[i]);
            }

            kdDebug(2120) << "    " << it.data()->deletedIds.size() << " IDs deleted of type" << it.key() << endl;

            rra_uint32vector_destroy(uidVector, true);
            rra_uint32vector_destroy(it.data()->uidVector, true);
        }

    } else {
        rraOk = false;
    }

    return rraOk;
}


bool Rra::removeDeletedObjects(uint32_t mTypeId, RRA_Uint32Vector* deleted_ids)
{
    return rra_syncmgr_purge_deleted_object_ids(rra, mTypeId, deleted_ids);
}


bool Rra::registerAddedObjects(uint32_t mTypeId, RRA_Uint32Vector* added_ids)
{
    return rra_syncmgr_register_added_object_ids(rra, mTypeId, added_ids);
}


bool Rra::markIdUnchanged(uint32_t type_id, uint32_t object_id)
{
    return rra_syncmgr_mark_object_unchanged(rra, type_id, object_id);
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

    if (!rraOk) {
        new_object_id = 0;
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

    if (connect()) {
        vEvent = vEvent.stripWhiteSpace();
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

    if (!rraOk) {
        new_object_id = 0;
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

    if (!rraOk) {
        new_object_id = 0;
    }

    return new_object_id;
}


bool Rra::deleteObject(uint32_t type_id, uint32_t object_id)
{
    rraOk = true;

    if (connect()) {
        if (!rra_syncmgr_delete_object(rra, type_id, object_id)) {
            rraOk = false;
        }
        disconnect();
    }

    return rraOk;
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


/*!
    \fn pocketPCCommunication::Rra::getTimezone(RRA_TimeZone *tzi)
 */
bool Rra::getTimezone(RRA_Timezone *tzi)
{
    return rra_timezone_get(tzi);
}

