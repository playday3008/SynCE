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

#ifndef WINCE_IDS_H
#define WINCE_IDS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rapi.h>

#include <kabc/addressee.h>
#include <qstring.h>

/**
@author Volker Christian,,,
*/

class MyAddress {
public:    
    KABC::Addressee addressee;
    KABC::Address homeAddress;
    KABC::Address workAddress;
    KABC::Address otherAddress;
};


bool setBirthday(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setSecretary(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setSecretaryPhone(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setCarPhone(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setChildren(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setEmail(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setEmail2(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setEmail3(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setPrivateFax(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setPrivatePhone(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setPrivatePhone2(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setMobilPhone(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setPager(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setRadioPhone(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setPartner(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setWebsite(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setOfficeFax(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setOfficePhone(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setOfficePhone2(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setFormatedName(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setCompany(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setDepartment(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setFirstName(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setLastName(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setFirstName2(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setSalutation(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setOffice(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setTitle(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setPosition(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setCategory(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setNotes(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setHomeStreet(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setHomeCity(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setHomeRegion(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setHomeZipCode(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setHomeCountry(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setOfficeStreet(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setOfficeCity(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setOfficeRegion(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setOfficeZipCode(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setOfficeCountry(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setAdditionalStreet(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setAdditionalCity(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setAdditionalRegion(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setAdditionalZipCode(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);
bool setAdditionalCountry(MyAddress&, synce::CEPROPVAL *propval, QString *store, bool read = false);


struct _contact_ids {
    unsigned int id;
    int type;
    char *typeName;
    char *valueName;
    bool (*function)(MyAddress&, synce::CEPROPVAL *propval,QString *store, bool read);
};

typedef struct _contact_ids contact_ids_t;
typedef contact_ids_t *contact_ids_p;

 
#endif
