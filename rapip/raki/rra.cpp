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
#include <rra/contact.h>
}
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
    }

    if(useCount == 0) {
        kdDebug(2120) << i18n("RRA-Disconnect") << endl;

        rra_syncmgr_disconnect(rra);
        Ce::rapiUninit();
    }
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


static bool callback(RRA_SyncMgrTypeEvent event, uint32_t /*type*/, uint32_t count,
        uint32_t *ids, void *cookie)
{
    QValueList<uint32_t> *eventIds;
    Rra::ids *_ids = (Rra::ids *) cookie;

    switch(event) {
    case SYNCMGR_TYPE_EVENT_UNCHANGED:
        eventIds = &_ids->unchangedIds;
        kdDebug(2120) << i18n("-------- unchanged --------") << endl;
        break;
    case SYNCMGR_TYPE_EVENT_CHANGED:
        eventIds = &_ids->changedIds;
        kdDebug(2120) << i18n("--------- changed ---------") << endl;
        break;
    case SYNCMGR_TYPE_EVENT_DELETED:
        eventIds = &_ids->deletedIds;
        kdDebug(2120) << i18n("--------- deleted ---------") << endl;
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
    bool gotEvent;

    _ids.changedIds.clear();
    _ids.unchangedIds.clear();
    _ids.deletedIds.clear();

    if (connect()) {
        rra_syncmgr_subscribe(rra, type_id, callback, &_ids);
        if (rra_syncmgr_start_events(rra)) {
            while(rra_syncmgr_event_wait(rra, 3, &gotEvent) && gotEvent) {
                rra_syncmgr_handle_event(rra);
            }
        } else {
            rraOk = false;
        }
        rra_syncmgr_unsubscribe(rra, type_id);
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
                                   (object_id != 0) ? 0x40 : 2, buffer,
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
        const char *vevent = vEvent.ascii();
        if (!rra_appointment_from_vevent(vevent, NULL, &buffer,
                &buffer_size, 0, NULL)) {
            rraOk = false;
        } else if (!rra_syncmgr_put_single_object(rra, type_id, object_id,
                (object_id != 0) ? 0x40 : 2, buffer,
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
