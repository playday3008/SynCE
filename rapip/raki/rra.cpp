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
    rra_free(rra);
    Ce::rapiUninit();
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
                kdDebug(2120) << "Deleted objects count: " << deleted_count << endl;
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
        kdDebug(2120) << "Partnership creation succeeded. Using partnership index " << *index << endl;
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
        kdDebug(2120) << "Invalid or missing index of partnership to replace." << endl;
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


/*
QString Rra::getVCal(uint32_t type_id, uint32_t object_id)
{
    uint8_t* data = NULL;
    size_t data_size = 0;
    char *vcard = NULL;
    uint32_t field_count = 0;
    synce::CEPROPVAL* propvals = NULL;

    rraOk = true;
    QString vCard = "";

    if (connect()) {
        if (!rra_object_get(rra, type_id, object_id, &data,
                            &data_size)) {
            rraOk = false;
        } else {
            field_count = letoh32(*(uint32_t *)(data + 0));
            propvals = (synce::CEPROPVAL *)malloc(sizeof(synce::CEPROPVAL) * field_count);
            if (!dbstream_to_propvals(data + 8, field_count, propvals)) {
                rraOk = false;
            } else if (!appointment_to_vcal(APPOINTMENT_OID_UNKNOWN, propvals, field_count, &vcard)) {
                rraOk = false;
            }
        }
        disconnect();
    }

    if (rraOk) {
        vCard = vcard;
    }

    if (data) {
        free(data);
    }

    if (propvals) {
        free(propvals);
    }

    if (vcard) {
        free(vcard);
    }

    return vCard;
}
*/

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
                                {    0x4003,  0x0040, (char *) "FILETIME",  (char *) "Geburtstag", setBirthday},
                                {    0x4002,  0x001f, (char *) "LPWSTR",    (char *) "Sekretariat", setSecretary},
                                {    0x4004,  0x001f, (char *) "LPWSTR",    (char *) "TelefonSekretariat", setSecretaryPhone},
                                {    0x3a1e,  0x001f, (char *) "LPWSTR",    (char *) "TelefonAuto", setCarPhone},
                                {    0x4006,  0x001f, (char *) "LPWSTR",    (char *) "Kinder", setChildren},
                                {    0x4083,  0x001f, (char *) "LPWSTR",    (char *) "Email", setEmail},
                                {    0x4093,  0x001f, (char *) "LPWSTR",    (char *) "Email2", setEmail2},
                                {    0x40a3,  0x001f, (char *) "LPWSTR",    (char *) "Email3", setEmail3},
                                {    0x3a25,  0x001f, (char *) "LPWSTR",    (char *) "FaxPrivat", setPrivateFax},
                                {    0x3a09,  0x001f, (char *) "LPWSTR",    (char *) "TelefonPrivat", setPrivatePhone},
                                {    0x3a2f,  0x001f, (char *) "LPWSTR",    (char *) "Telefon2Privat", setPrivatePhone2},
                                {    0x3a1c,  0x001f, (char *) "LPWSTR",    (char *) "TelefonMobil", setMobilPhone},
                                {    0x4009,  0x001f, (char *) "LPWSTR",    (char *) "Pager", setPager},
                                {    0x3a1d,  0x001f, (char *) "LPWSTR",    (char *) "TelefonFunk", setRadioPhone},
                                {    0x400a,  0x001f, (char *) "LPWSTR",    (char *) "Partner", setPartner},
                                {    0x4008,  0x001f, (char *) "LPWSTR",    (char *) "Webseite", setWebsite},
                                {    0x3a24,  0x001f, (char *) "LPWSTR",    (char *) "FaxBuero", setOfficeFax},
                                {    0x3a08,  0x001f, (char *) "LPWSTR",    (char *) "TelefonBuero", setOfficePhone},
                                {    0x4007,  0x001f, (char *) "LPWSTR",    (char *) "Telefon2Buero", setOfficePhone2},
                                {    0x4013,  0x001f, (char *) "LPWSTR",    (char *) "Nachname, Vorname", setFormatedName},
                                {    0x3a16,  0x001f, (char *) "LPWSTR",    (char *) "Firma", setCompany},
                                {    0x3a18,  0x001f, (char *) "LPWSTR",    (char *) "Abteilung", setDepartment},
                                {    0x3a06,  0x001f, (char *) "LPWSTR",    (char *) "Vorname", setFirstName},
                                {    0x3a11,  0x001f, (char *) "LPWSTR",    (char *) "Nachname", setLastName},
                                {    0x4024,  0x001f, (char *) "LPWSTR",    (char *) "Vorname2", setFirstName2},
                                {    0x4023,  0x001f, (char *) "LPWSTR",    (char *) "Anrede", setSalutation},
                                {    0x3a19,  0x001f, (char *) "LPWSTR",    (char *) "Buero", setOffice},
                                {    0x3a05,  0x001f, (char *) "LPWSTR",    (char *) "Titel", setTitle},
                                {    0x3a17,  0x001f, (char *) "LPWSTR",    (char *) "Position", setPosition},
                                {    0x4005,  0x001f, (char *) "LPWSTR",    (char *) "GeschÃ¤ftlich", setCategory},
                                {    0x0017,  0x0041, (char *) "BLOB",      (char *) "Notizen", setNotes},
                                {    0x4040,  0x001f, (char *) "LPWSTR",    (char *) "StrassePrivat", setHomeStreet},
                                {    0x4041,  0x001f, (char *) "LPWSTR",    (char *) "OrtPrivat", setHomeCity},
                                {    0x4042,  0x001f, (char *) "LPWSTR",    (char *) "RegionPrivat", setHomeRegion},
                                {    0x4043,  0x001f, (char *) "LPWSTR",    (char *) "PlzPrivat", setHomeZipCode},
                                {    0x4044,  0x001f, (char *) "LPWSTR",    (char *) "StaatPrivat", setHomeCountry},
                                {    0x4045,  0x001f, (char *) "LPWSTR",    (char *) "StrasseBuero", setOfficeStreet},
                                {    0x4046,  0x001f, (char *) "LPWSTR",    (char *) "OrtBuero", setOfficeCity},
                                {    0x4047,  0x001f, (char *) "LPWSTR",    (char *) "RegionBuero", setOfficeRegion},
                                {    0x4048,  0x001f, (char *) "LPWSTR",    (char *) "PlzBuero", setOfficeZipCode},
                                {    0x4049,  0x001f, (char *) "LPWSTR",    (char *) "StaatBuero", setOfficeCountry},
                                {    0x404a,  0x001f, (char *) "LPWSTR",    (char *) "StrasseWeitere", setAdditionalStreet},
                                {    0x404b,  0x001f, (char *) "LPWSTR",    (char *) "OrtWeitere", setAdditionalCity},
                                {    0x404c,  0x001f, (char *) "LPWSTR",    (char *) "RegionWeitere", setAdditionalRegion},
                                {    0x404d,  0x001f, (char *) "LPWSTR",    (char *) "PlzWeitere", setAdditionalZipCode},
                                {    0x404e,  0x001f, (char *) "LPWSTR",    (char *) "StaatWeitere", setAdditionalCountry},
                                {    0xfffd,  0x0013, (char *) "UI4",       (char *) "NoName", NULL},
                                {    0xfffe,  0x0013, (char *) "UI4",       (char *) "NoName", NULL},
                                {0, 0, NULL, NULL, NULL}
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
            propvals = (synce::CEPROPVAL *)malloc(sizeof(synce::CEPROPVAL) * field_count);
            if (dbstream_to_propvals(data + 8, field_count, propvals)) {
                for (unsigned int i = 0; i < field_count; i++) {
                    for (int j = 0; contact_ids[j].id; j++) {
                        if (contact_ids[j].id == (propvals[i].propid >> 16)) {
                            if (contact_ids[j].function != NULL) {
                                contact_ids[j].function(myAddress, &propvals[i], NULL, false);
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
    myAddress.addressee.setUid("RRA-ID-" + (QString("00000000") + QString::number(object_id, 16)).right(8));
    return myAddress.addressee;
}


bool Rra::putAddressee(const KABC::Addressee& addressee, uint32_t type_id, uint32_t ceUid, uint32_t *newCeUid)
{
    synce::CEPROPVAL propvals[MAX_FIELD_COUNT];
    QString stores[MAX_FIELD_COUNT];
    int i = 0;
    uint8_t* data = NULL;
    size_t data_size = 0;

    MyAddress myAddress;
    myAddress.addressee = addressee;
    myAddress.homeAddress = addressee.address(KABC::Address::Home | KABC::Address::Pref);
    myAddress.workAddress = addressee.address(KABC::Address::Work);
    myAddress.otherAddress = addressee.address(KABC::Address::Intl);


    if (connect()) {
        for (int j = 0; contact_ids[j].id; j++) {
            if (contact_ids[j].function != NULL) {
                if (contact_ids[j].function(myAddress, &propvals[i], &stores[i], true)) {
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
