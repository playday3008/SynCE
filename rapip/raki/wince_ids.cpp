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

#include "wince_ids.h"

#include <synce.h>

#include <qdatetime.h>
#include <kabc/phonenumber.h>
#include <kurl.h>
#include <kdebug.h>
#include <time.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

static QString extractQString(synce::CEPROPVAL *propval)
{
    return QString::fromUcs2(propval->val.lpwstr);
}


static QDateTime extractQDateTime(synce::CEPROPVAL *propval)
{
    QDateTime date;
    
    time_t time = synce::filetime_to_unix_time(&propval->val.filetime);
    date.setTime_t(time);
    return date;
}


static bool insertQString(synce::CEPROPVAL *propval, QString& string)
{
    if (!string.isEmpty()) {
        propval->val.lpwstr = (WCHAR *) string.ucs2();
        propval->propid = CEVT_LPWSTR;
        kdDebug(2120) << string.ascii() << endl;
        return true;
    }
    
    return false;
}

/*
static short extractShort(synce::CEPROPVAL *propval)
{
    return propval->val.iVal;
}


static int extractInt(synce::CEPROPVAL *propval)
{
    return propval->val.iVal;
}
*/

/*
static float extractFloat(synce::CEPROPVAL *propval)
{
}

static bool extractBool(synce::CEPROPVAL *propval)
{
}
*/


bool setBirthday(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.setBirthday(extractQDateTime(propval));
    } else {
        *store = myAddress.addressee.birthday().toString();
    }
    
    return false;
}


bool setSecretary(MyAddress& /*myAddress.addressee*/, synce::CEPROPVAL */*propval*/, QString */*store*/, bool /*read*/)
{
    return false;
}


bool setSecretaryPhone(MyAddress& /*myAddress.addressee*/, synce::CEPROPVAL */*propval*/, QString */*store*/, bool /*read*/)
{   
    return false;
}


bool setCarPhone(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    if (!read) {
        myAddress.addressee.insertPhoneNumber(KABC::PhoneNumber(extractQString(propval), KABC::PhoneNumber::Car));
    } else {
        *store = myAddress.addressee.phoneNumber(KABC::PhoneNumber::Car).number();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setChildren(MyAddress& /*myAddress.addressee*/, synce::CEPROPVAL */*propval*/, QString */*store*/, bool /*read*/)
{
    return false;
}


bool setEmail(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{  
    if (!read) {
        myAddress.addressee.insertEmail(extractQString(propval), true);
    } else {
        *store = myAddress.addressee.preferredEmail();
        return insertQString(propval, *store);
    }
        
    return true;
}


bool setEmail2(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    
    if (!read) {
        myAddress.addressee.insertEmail(extractQString(propval), false);
    } else {
        QStringList emails = myAddress.addressee.emails();
        if (emails.begin() != emails.end()) {
            for (QStringList::Iterator it = emails.begin(); it != emails.end(); ++it ) {
                if ((*it) != myAddress.addressee.preferredEmail()) {
                    *store = *it;
                    return insertQString(propval, *store);
                }
            }
        }
    }
    
    return false;
}


bool setEmail3(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    bool inserted = false;
    
    if (!read) {
        myAddress.addressee.insertEmail(extractQString(propval), false);
    } else {
        QStringList emails = myAddress.addressee.emails();
        if (emails.begin() != emails.end()) {
            for (QStringList::Iterator it = emails.begin(); it != emails.end(); ++it ) {
                if ((*it) != myAddress.addressee.preferredEmail()) {
                    *store = *it;
                    insertQString(propval, *store);
                    inserted = true;
                }
            }
        }
    }
    
    return false;
}


bool setPrivateFax(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.insertPhoneNumber(KABC::PhoneNumber(extractQString(propval), KABC::PhoneNumber::Home | KABC::PhoneNumber::Fax));
    } else {
        *store = myAddress.addressee.phoneNumber(KABC::PhoneNumber::Home | KABC::PhoneNumber::Fax).number();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setPrivatePhone(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.insertPhoneNumber(KABC::PhoneNumber(extractQString(propval), KABC::PhoneNumber::Home));
    } else {
        *store = myAddress.addressee.phoneNumber(KABC::PhoneNumber::Home).number();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setPrivatePhone2(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.insertPhoneNumber(KABC::PhoneNumber(extractQString(propval), KABC::PhoneNumber::Voice));
    } else {
        *store = myAddress.addressee.phoneNumber(KABC::PhoneNumber::Voice).number();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setMobilPhone(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.insertPhoneNumber(KABC::PhoneNumber(extractQString(propval), KABC::PhoneNumber::Cell));
    } else {
        *store = myAddress.addressee.phoneNumber(KABC::PhoneNumber::Cell).number();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setPager(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.insertPhoneNumber(KABC::PhoneNumber(extractQString(propval), KABC::PhoneNumber::Pager));
    } else {
        *store = myAddress.addressee.phoneNumber(KABC::PhoneNumber::Pager).number();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setRadioPhone(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.insertPhoneNumber(KABC::PhoneNumber(extractQString(propval), KABC::PhoneNumber::Pcs));
    } else {
        *store = myAddress.addressee.phoneNumber(KABC::PhoneNumber::Pcs).number();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setPartner(MyAddress& /*myAddress.addressee*/, synce::CEPROPVAL */*propval*/, QString */*store*/, bool /*read*/)
{
    return false;
}


bool setWebsite(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.setUrl(KURL(extractQString(propval)));
    } else {
        *store = myAddress.addressee.url().url();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setOfficeFax(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.insertPhoneNumber(KABC::PhoneNumber(extractQString(propval), KABC::PhoneNumber::Fax | KABC::PhoneNumber::Work));
    } else {
        *store = myAddress.addressee.phoneNumber(KABC::PhoneNumber::Fax | KABC::PhoneNumber::Work).number();
        return insertQString(propval, *store);    
    }
    
    return true;
}


bool setOfficePhone(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.insertPhoneNumber(KABC::PhoneNumber(extractQString(propval), KABC::PhoneNumber::Work));
    } else {
        *store = myAddress.addressee.phoneNumber(KABC::PhoneNumber::Work).number();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setOfficePhone2(MyAddress& /*myAddress.addressee*/, synce::CEPROPVAL */*propval*/, QString */*store*/, bool /*read*/)
{   
    return false;
}


bool setFormatedName(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.setFormattedName(extractQString(propval));
    } else {
        *store = myAddress.addressee.formattedName();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setCompany(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.setOrganization(extractQString(propval));
    } else {
        *store = myAddress.addressee.organization();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setDepartment(MyAddress& /*myAddress.addressee*/, synce::CEPROPVAL */*propval*/, QString */*store*/, bool /*read*/)
{
    return false;
}


bool setFirstName(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.setGivenName(extractQString(propval));
    } else {
        *store = myAddress.addressee.givenName();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setLastName(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.setFamilyName(extractQString(propval));
    } else {
        *store = myAddress.addressee.familyName();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setFirstName2(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.setAdditionalName(extractQString(propval));
    } else {
        *store = myAddress.addressee.additionalName();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setSalutation(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.setPrefix(extractQString(propval));
    } else {
        *store = myAddress.addressee.prefix();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setOffice(MyAddress& /*myAddress.addressee*/, synce::CEPROPVAL */*propval*/, QString */*store*/, bool /*read*/)
{   
    return false;
}


bool setTitle(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.addressee.setTitle(extractQString(propval));
    } else {
        *store = myAddress.addressee.title();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setPosition(MyAddress& /*myAddress.addressee*/, synce::CEPROPVAL */*propval*/, QString */*store*/, bool /*read*/)
{
    return false;
}


bool setCategory(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    if (!read) {
        myAddress.addressee.insertCategory(extractQString(propval));
    } else {
        QStringList categories = myAddress.addressee.categories();
        if (categories.begin() != categories.end()) {
            *store = *(categories.begin());
            return insertQString(propval, *store);
        } else {
            return false;
        }
    }
    
    return true;
}


bool setNotes(MyAddress& /*myAddress*/, synce::CEPROPVAL */*propval*/, QString */*store*/, bool /*read*/)
{
/*
    if (!read)
        myAddress.addressee.setNote(extractQString(propval));
*/
    
    return false;
}


bool setHomeStreet(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    if (!read) {
        myAddress.homeAddress.setStreet(extractQString(propval));
    } else {
        *store = myAddress.homeAddress.street();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setHomeCity(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    if (!read) {
        myAddress.homeAddress.setLocality(extractQString(propval));
    } else {
        *store = myAddress.homeAddress.locality();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setHomeRegion(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    if (!read) {
        myAddress.homeAddress.setRegion(extractQString(propval));
    } else {
        *store = myAddress.homeAddress.region();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setHomeZipCode(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    if (!read) {
        myAddress.homeAddress.setPostalCode(extractQString(propval));
    } else {
        *store = myAddress.homeAddress.postalCode();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setHomeCountry(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    if (!read) {
        myAddress.homeAddress.setCountry(extractQString(propval));
    } else {
        *store = myAddress.homeAddress.country();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setOfficeStreet(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.workAddress.setStreet(extractQString(propval));
    } else {
        *store = myAddress.workAddress.street();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setOfficeCity(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    if (!read) {
        myAddress.workAddress.setLocality(extractQString(propval));
    } else {
        *store = myAddress.workAddress.locality();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setOfficeRegion(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    if (!read) {
        myAddress.workAddress.setRegion(extractQString(propval));
    } else {
        *store = myAddress.workAddress.region();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setOfficeZipCode(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    if (!read) {
        myAddress.workAddress.setPostalCode(extractQString(propval));
    } else {
        *store = myAddress.workAddress.postalCode();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setOfficeCountry(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    if (!read) {
        myAddress.workAddress.setCountry(extractQString(propval));
    } else {
        *store = myAddress.workAddress.country();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setAdditionalStreet(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.otherAddress.setStreet(extractQString(propval));
    } else {
        *store = myAddress.otherAddress.street();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setAdditionalCity(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.otherAddress.setLocality(extractQString(propval));
    } else {
        *store = myAddress.otherAddress.locality();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setAdditionalRegion(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.otherAddress.setRegion(extractQString(propval));
    } else {
        *store = myAddress.otherAddress.region();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setAdditionalZipCode(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{   
    if (!read) {
        myAddress.otherAddress.setPostalCode(extractQString(propval));
    } else {
        *store = myAddress.otherAddress.postalCode();
        return insertQString(propval, *store);
    }
    
    return true;
}


bool setAdditionalCountry(MyAddress& myAddress, synce::CEPROPVAL *propval, QString *store, bool read)
{
    if (!read) {
        myAddress.otherAddress.setCountry(extractQString(propval));
    } else {
        *store = myAddress.otherAddress.country();
        return insertQString(propval, *store);
    }
    
    return true;
}
