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
#include "wince_ids.h"

#include <kabc/addressee.h>
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
    rra = rra_new();
    _ids.object_ids = NULL;
    _ids.object_types = NULL;
    _ids.deleted_ids = NULL;
    objectTypes.setAutoDelete(true);
    useCount = 0;
}


Rra::~Rra()
{
    disconnect();
    rra_free(rra);
}


bool Rra::connect()
{
    rraOk = true;

    if (useCount == 0) {
        if (!Ce::rapiInit(pdaName)) {
            rraOk = false;
        } else {
            rraOk = rra_connect(rra);
            if (!rraOk) {
                rra_disconnect(rra);
                Ce::rapiUninit();
            } else {
                kdDebug(2120) << "RRA-Connect" << endl;
            }
        }
    }

    if (rraOk) {
        useCount++;
    }

    return rraOk;
}


void Rra::disconnect()
{
    useCount--;

    if(useCount == 0) {
        kdDebug(2120) << "RRA-Disconnect" << endl;
        rra_disconnect(rra);
        Ce::rapiUninit();
    }
}


bool Rra::ok()
{
    return rraOk;
}


bool Rra::getTypes(QPtrDict<ObjectType> *objectTypes)
{
    size_t object_type_count = 0;
    ObjectType *objectType;

    rraOk = true;

    objectTypes->clear();

    if (connect()) {
        _ids.object_types = NULL;
        if (rra_get_object_types(rra, &_ids.object_types, &object_type_count)) {
            for (size_t i = 0; i < object_type_count; i++) {
                objectType = new ObjectType();
                *objectType = _ids.object_types[i];
                objectTypes->insert((void *) objectType->id, objectType);
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


bool Rra::getIds(uint32_t type_id, struct Rra::ids *ids)
{
    size_t deleted_count = 0;
    unsigned id = 0;

    rraOk = true;

    rra_free_object_ids(_ids.object_ids);
    rra_free_deleted_object_ids(_ids.deleted_ids);

    _ids.changedIds.clear();
    _ids.unchangedIds.clear();
    _ids.deletedIds.clear();

    if (connect()) {
        if (rra_get_object_ids(rra, type_id, &_ids.object_ids)) {
            for (size_t i = 0; i < _ids.object_ids->unchanged; i++) {
                _ids.unchangedIds.append(&_ids.object_ids->ids[id++]);
            }
            for (size_t i = 0; i < _ids.object_ids->changed; i++) {
                _ids.changedIds.append(&_ids.object_ids->ids[id++]);
            }
            if (rra_get_deleted_object_ids(rra, type_id, _ids.object_ids,
                                           &_ids.deleted_ids, &deleted_count)) {
                kdDebug(2120) << "Deleted objects count: " << deleted_count <<
                        endl;
                for (id = 0; id < deleted_count; id++) {
                    _ids.deletedIds.append(&_ids.deleted_ids[id]);
                }
            } else {
                rraOk = false;
            }
        } else {
            rraOk = false;
        }
        disconnect();
    } else {
        rraOk = false;
    }

    *ids = _ids;

    return rraOk;
}


bool Rra::getPartner(uint32_t index, struct Rra::Partner *partner)
{
    char *name = NULL;

    rraOk = true;

    if (!rra_partner_get_id(rra, index, &partner->id)) {
        partner->id = 0;
        rraOk = false;
    }

    if (!rra_partner_get_name(rra, index, &name)) {
        partner->name = "";
        rraOk = false;
    } else {
        partner->name = name;
    }
    
    partner->index = index;

    return rraOk;
}


bool Rra::getCurrentPartner(struct Rra::Partner *partner)
{
    DWORD currentIndex;

    rraOk = true;

    if (rra_partner_get_current(rra, &currentIndex)) {
        if (!getPartner(currentIndex, partner)) {
            rraOk = false;
        }
    } else {
        partner->id = 0;
        partner->name = "";
        partner->index = 0;
        rraOk = false;
    }

    return rraOk;
}


bool Rra::partnerCreate(uint32_t *index)
{
    rraOk = true;

    if (rra_partner_create(rra, index)) {
        kdDebug(2120) <<
                "Partnership creation succeeded. Using partnership index " <<
                *index << endl;
    } else {
        kdDebug(2120) << "Partnership creation failed." << endl;
        *index = 0;
        rraOk = false;
    }

    return rraOk;
}


bool Rra::partnerReplace(int index)
{
    rraOk = true;

    if (index == 1 || index == 2) {
        if (rra_partner_replace(rra, index)) {
            kdDebug(2120) << "Partnership replacement succeeded." << endl;
        } else {
            kdDebug(2120) << "Partnership replacement failed." << endl;
            rraOk = false;
        }
    } else {
        kdDebug(2120) <<
                "Invalid or missing index of partnership to replace." <<
                endl;
        rraOk = false;
    }

    return rraOk;
}


bool Rra::setPartner(struct Rra::Partner& partner)
{
    if (!rra_partner_set_id(rra, partner.index, partner.id)) {
        return false;
    }

    if (!rra_partner_set_name(rra, partner.index, partner.name.ascii())) {
        return false;
    }

    return true;
}


bool Rra::setCurrentPartner(uint32_t index)
{
    if (!rra_partner_set_current(rra, index)) {
        disconnect();
        return false;
    }

    return true;
}


QString Rra::getVCard(uint32_t type_id, uint32_t object_id)
{
    uint8_t* data = NULL;
    size_t data_size = 0;
    char *vcard = NULL;

    rraOk = true;
    QString vCard = "";

    if (connect()) {
        if (!rra_object_get(rra, type_id, object_id, &data,
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
        } else if (!rra_object_put(rra, type_id, object_id,
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
        if (!rra_object_get(rra, type_id, object_id, &data,
                &data_size)) {
            rraOk = false;
        } else if (!rra_appointment_to_vevent(object_id, data, data_size,
                &vevent, 0, /*p_tzi*/ NULL)) {
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
                &buffer_size, 0)) {
            rraOk = false;
        } else if (!rra_object_put(rra, type_id, object_id,
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
        if (!rra_object_delete(rra, type_id, object_id)) {
            rraOk = false;
        }
        disconnect();
    }
}


contact_ids_t contact_ids[] = {
    {    0x4003,  0x0040, setBirthday},
    {    0x4002,  0x001f, setSecretary},
    {    0x4004,  0x001f, setSecretaryPhone},
    {    0x3a1e,  0x001f, setCarPhone},
    {    0x4006,  0x001f, setChildren},
    {    0x4083,  0x001f, setEmail},
    {    0x4093,  0x001f, setEmail2},
    {    0x40a3,  0x001f, setEmail3},
    {    0x3a25,  0x001f, setPrivateFax},
    {    0x3a09,  0x001f, setPrivatePhone},
    {    0x3a2f,  0x001f, setPrivatePhone2},
    {    0x3a1c,  0x001f, setMobilPhone},
    {    0x4009,  0x001f, setPager},
    {    0x3a1d,  0x001f, setRadioPhone},
    {    0x400a,  0x001f, setPartner},
    {    0x4008,  0x001f, setWebsite},
    {    0x3a24,  0x001f, setOfficeFax},
    {    0x3a08,  0x001f, setOfficePhone},
    {    0x4007,  0x001f, setOfficePhone2},
    {    0x4013,  0x001f, setFormatedName},
    {    0x3a16,  0x001f, setCompany},
    {    0x3a18,  0x001f, setDepartment},
    {    0x3a06,  0x001f, setFirstName},
    {    0x3a11,  0x001f, setLastName},
    {    0x4024,  0x001f, setFirstName2},
    {    0x4023,  0x001f, setSalutation},
    {    0x3a19,  0x001f, setOffice},
    {    0x3a05,  0x001f, setTitle},
    {    0x3a17,  0x001f, setPosition},
    {    0x4005,  0x001f, setCategory},
    {    0x0017,  0x0041, setNotes},
    {    0x4040,  0x001f, setHomeStreet},
    {    0x4041,  0x001f, setHomeCity},
    {    0x4042,  0x001f, setHomeRegion},
    {    0x4043,  0x001f, setHomeZipCode},
    {    0x4044,  0x001f, setHomeCountry},
    {    0x4045,  0x001f, setOfficeStreet},
    {    0x4046,  0x001f, setOfficeCity},
    {    0x4047,  0x001f, setOfficeRegion},
    {    0x4048,  0x001f, setOfficeZipCode},
    {    0x4049,  0x001f, setOfficeCountry},
    {    0x404a,  0x001f, setAdditionalStreet},
    {    0x404b,  0x001f, setAdditionalCity},
    {    0x404c,  0x001f, setAdditionalRegion},
    {    0x404d,  0x001f, setAdditionalZipCode},
    {    0x404e,  0x001f, setAdditionalCountry},
    {    0xfffd,  0x0013, NULL},
    {    0xfffe,  0x0013, NULL},
    {0, 0, NULL}
};

event_ids_t event_ids[] = {
#ifndef WITH_ICAL
    { 0, 0, NULL, NULL}
#else
    { 0x0001, 0x0000, unknown0001, NULL }, /* something about repeated appointemnts */
    { 0x0002, 0x0003, unknown0002, NULL }, /* Unknown */
    { 0x0004, 0x0002, sensitivity, NULL }, /* sensitivity */
#define SENSITIVITY_PUBLIC  0
#define SENSITIVITY_PRIVATE 1

    { 0x000f, 0x0002, bussy_status, NULL }, /* bussy status */
#define BUSY_STATUS_FREE           0
#define BUSY_STATUS_TENTATIVE      1
#define BUSY_STATUS_BUSY           2
#define BUSY_STATUS_OUT_OF_OFFICE  3

    { 0x0016, 0x0000, categories, NULL }, /* Categories */
    { 0x0017, 0x0041, notes, NULL }, /* Notes */
    { 0x0037, 0x001f, subject, NULL }, /* Subject */
    { 0x0042, 0x001f, unknown0042, NULL }, /* Unknown */
    { 0x0067, 0x0041, unknown0067, NULL }, /* Unknown */
    { 0x4005, 0x001f, category, NULL }, /* Cetegory */
    { 0x4015, 0x0003, unknown4015, NULL }, /* Unknown */
    { 0x4171, 0x0003, unknown4171, NULL }, /* Unknown */
    { 0x4208, 0x001f, location, NULL }, /* Location */
    { 0x420d, 0x0040, appointment_start, NULL }, /* Appointment start */
    { 0x4213, 0x0003, appointment_duration, NULL }, /* Appointment duration */
    { 0x4215, 0x0003, appointment_type, NULL }, /* Appointment Type */
#define APPOINTMENT_TYPE_ALL_DAY     1
#define APPOINTMENT_TYPE_NORMAL      2

    { 0x4223, 0x0002, occurance, NULL }, /* Occurance */
#define OCCURANCE_ONCE      0
#define OCCURANCE_REPEATED  1

    { 0x4501, 0x0003, reminder_minutes_before_start, NULL }, /* Reminder_minutes_before_start*/
    { 0x4503, 0x0002, reminder_enabled, NULL }, /* Reminder_enabled */
    { 0x4509, 0x001f, reminder_sound_file, NULL }, /* Reminder_sound_file */
    { 0x450a, 0x0003, reminder_options, NULL }, /* Reminder options */
#define REMINDER_LED 1
#define REMINDER_VIBRATE 2
#define REMINDER_DIALOG 4
#define REMINDER_SOUND 8
#define REMINDER_REPEAT 16

    { 0xfffd, 0x0013, unknownfffd, NULL }, /* Unknown */
    { 0xfffe, 0x0013, unknownfffe, NULL },
    { 0, 0, NULL, NULL }
#endif //WITH_ICAL
};

#ifdef __cplusplus
extern "C"
{
#endif

bool dbstream_to_propvals(
                const uint8_t* stream,
                uint32_t count,
                synce::CEPROPVAL* propval);

bool dbstream_from_propvals(
                synce::CEPROPVAL* propval,
                uint32_t count,
                uint8_t** result,
                size_t* result_size);

#define dbstream_free_propvals(p)  if(p) free(p)
#define dbstream_free_stream(p)    if(p) free(p)

#ifdef __cplusplus
}
#endif

KABC::Addressee Rra::getAddressee(uint32_t type_id, uint32_t object_id)
{
    uint8_t* data = NULL;
    size_t data_size = 0;
    uint32_t field_count = 0;
    synce::CEPROPVAL* propvals = NULL;
    MyAddress myAddress;
    myAddress.homeAddress.setType(KABC::Address::Home | KABC::Address::Pref);
    myAddress.workAddress.setType(KABC::Address::Work);
    myAddress.otherAddress.setType(KABC::Address::Intl);

    rraOk = true;

    if (connect()) {
        if (!rra_object_get(rra, type_id, object_id, &data,
                            &data_size)) {
            rraOk = false;
        } else {
            field_count = letoh32(*(uint32_t *)(data + 0));
            propvals = (synce::CEPROPVAL *)malloc(sizeof(
                    synce::CEPROPVAL) * field_count);
            if (dbstream_to_propvals(data + 8, field_count, propvals)) {
                for (unsigned int i = 0; i < field_count; i++) {
                    for (int j = 0; contact_ids[j].id; j++) {
                        if (contact_ids[j].id == (propvals[i].propid >> 16)) {
                            if (contact_ids[j].function != NULL) {
                                contact_ids[j].function(myAddress, &propvals[i],
                                        NULL, false);
                            }
                        }
                    }
                }
                myAddress.addressee.insertAddress(myAddress.homeAddress);
                myAddress.addressee.insertAddress(myAddress.workAddress);
                myAddress.addressee.insertAddress(myAddress.otherAddress);
            } else {
                rraOk = false;
            }
            if (propvals) {
                free(propvals);
            }
        }
        if (data) {
            free(data);
        }
        disconnect();
    }
    myAddress.addressee.setUid("RRA-ID-" + (QString("00000000") +
            QString::number(object_id, 16)).right(8));
    return myAddress.addressee;
}


bool Rra::putAddressee(const KABC::Addressee& addressee, uint32_t type_id,
        uint32_t ceUid, uint32_t *newCeUid)
{
    synce::CEPROPVAL propvals[MAX_FIELD_COUNT];
    QString stores[MAX_FIELD_COUNT];
    int i = 0;
    uint8_t* data = NULL;
    size_t data_size = 0;

    MyAddress myAddress;
    myAddress.addressee = addressee;
    myAddress.homeAddress = addressee.address(KABC::Address::Home |
            KABC::Address::Pref);
    myAddress.workAddress = addressee.address(KABC::Address::Work);
    myAddress.otherAddress = addressee.address(KABC::Address::Intl);


    if (connect()) {
        for (int j = 0; contact_ids[j].id; j++) {
            if (contact_ids[j].function != NULL) {
                if (contact_ids[j].function(myAddress, &propvals[i],
                        &stores[i], true)) {
                    propvals[i].propid |= contact_ids[j].id << 16;
                    i++;
                }
            }
        }
        if (dbstream_from_propvals(propvals, i, &data, &data_size)) {
            if (!rra_object_put(rra, type_id, ceUid,
                                (ceUid != 0) ? 0x40 : 2, data,
                                data_size, newCeUid)) {
                return false;
            }
        } else {
            return false;
        }

        if (data) {
            free(data);
        }
        disconnect();
    }

    return true;
}


ICAL::icalcomponent *Rra::getEvent(uint32_t type_id, uint32_t object_id)
{
    ICAL::icalcomponent *event = NULL;
    uint8_t* data = NULL;
    size_t data_size = 0;
    uint32_t field_count = 0;
    synce::CEPROPVAL* propvals = NULL;

    rraOk = true;

    if (connect()) {
        if (!rra_object_get(rra, type_id, object_id, &data,
                            &data_size)) {
            rraOk = false;
        } else {
            field_count = letoh32(*(uint32_t *)(data + 0));
            propvals = (synce::CEPROPVAL *)malloc(sizeof(
                    synce::CEPROPVAL) * field_count);
            if (dbstream_to_propvals(data + 8, field_count, propvals)) {
                event = ICAL::icalcomponent_new(ICAL::ICAL_VEVENT_COMPONENT);
                for (unsigned int i = 0; i < field_count; i++) {
                    for (int j = 0; event_ids[j].id; j++) {
                        if (event_ids[j].id == (propvals[i].propid >> 16)) {
                            if (event_ids[j].function != NULL) {
                                event_ids[j].prop = event_ids[j].function(
                                        event, &propvals[i], NULL, false);
                            }
                        }
                    }
                }
            } else {
                rraOk = false;
            }
            if (propvals) {
                free(propvals);
            }
        }
        if (data) {
            free(data);
        }
        disconnect();
    }

    return event;
}


bool Rra::putEvent(const ICAL::icalcomponent */*event*/, uint32_t /*type_id*/,
        uint32_t /*ceUid*/, uint32_t */*newCeUid*/)
{
    return true;
}
