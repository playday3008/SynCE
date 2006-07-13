from util import escape_str

def contact_to_vcard(sid, contact):
    """
    Fields supported so far:
        [x] Anniversary
        [x] AssistantName
        [x] AssistnamePhoneNumber
        [x] Birthday
        [ ] Body
        [ ] BodySize
        [ ] BodyTruncated
        [x] Business2PhoneNumber
        [x] BusinessCity
        [x] BusinessCountry
        [x] BusinessPostalCode
        [x] BusinessState
        [x] BusinessStreet
        [x] BusinessFaxNumber
        [x] BusinessPhoneNumber
        [x] CarPhoneNumber
        [ ] Categories
        [ ] Category
        [ ] Children
        [ ] Child
        [x] CompanyName
        [x] Department
        [x] Email1Address
        [x] Email2Address
        [x] Email3Address
        [x] FileAs
        [x] FirstName
        [x] Home2PhoneNumber
        [x] HomeCity
        [x] HomeCountry
        [x] HomePostalCode
        [x] HomeState
        [x] HomeStreet
        [x] HomeFaxNumber
        [x] HomePhoneNumber
        [x] JobTitle
        [x] LastName
        [x] MiddleName
        [x] MobilePhoneNumber
        [x] OfficeLocation
        [x] OtherCity
        [x] OtherCountry
        [x] OtherPostalCode
        [x] OtherState
        [x] OtherStreet
        [x] PagerNumber
        [x] RadioPhoneNumber
        [x] Spouse
        [x] Suffix
        [x] Title
        [x] WebPage
        [ ] YomiCompanyName
        [ ] YomiFirstName
        [ ] YomiLastName
        [ ] Rtf
        [x] Picture
        [ ] CustomerId
        [ ] GovernmentId
        [x] IMAddress
        [x] IMAddress2
        [x] IMAddress3
        [x] ManagerName
        [x] CompanyMainPhone
        [ ] AccountName
        [x] NickName
        [ ] MMS
    """

    s = u"BEGIN:VCARD\nVERSION:3.0\n"

    pending = {}
    for k in contact.keys():
        pending[k] = None

    # FN
    fn = ""
    if "FirstName" in contact:
        fn += contact["FirstName"]
        del pending["FirstName"]
    if "LastName" in contact:
        if fn:
            fn += " "
        fn += contact["LastName"]
        del pending["LastName"]

    if fn:
        s += "FN:%s\n" % escape_str(fn)

    # N
    n = ""
    if "LastName" in contact:
        n += escape_str(contact["LastName"])
    n += ";"
    if "FirstName" in contact:
        n += escape_str(contact["FirstName"])
    n += ";"
    if "MiddleName" in contact:
        n += escape_str(contact["MiddleName"])
        del pending["MiddleName"]
    n += ";"
    # honorific prefixes
    if "Title" in contact:
        n += escape_str(contact["Title"])
        del pending["Title"]
    n += ";"
    # honorific suffixes
    if "Suffix" in contact:
        n += escape_str(contact["Suffix"])
        del pending["Suffix"]

    if len(n) > 4:
        s += "N:%s\n" % n

    # TITLE
    if "JobTitle" in contact:
        s += "TITLE:%s\n" % escape_str(contact["JobTitle"])
        del pending["JobTitle"]

    # ORG
    org = ""
    if "CompanyName" in contact:
        org += escape_str(contact["CompanyName"])
        del pending["CompanyName"]
    org += ";"
    if "OfficeLocation" in contact:
        org += escape_str(contact["OfficeLocation"])
        del pending["OfficeLocation"]
    org += ";"
    if "Department" in contact:
        org += escape_str(contact["Department"])
        del pending["Department"]
    if len(org) > 2:
        s += "ORG:%s\n" % org

    # URL
    if "WebPage" in contact:
        s += "URL:%s\n" % escape_str(contact["WebPage"])
        del pending["WebPage"]

    # NICKNAME
    if "NickName" in contact:
        s += "NICKNAME:%s\n" % escape_str(contact["NickName"])
        del pending["NickName"]

    # BDAY
    if "Birthday" in contact:
        s += "BDAY:%s\n" % contact["Birthday"].split("T")[0]
        del pending["Birthday"]

    # X-EVOLUTION-ANNIVERSARY
    if "Anniversary" in contact:
        s += "X-EVOLUTION-ANNIVERSARY:%s\n" % contact["Anniversary"].split("T")[0]
        del pending["Anniversary"]

    # X-EVOLUTION-SPOUSE
    if "Spouse" in contact:
        s += "X-EVOLUTION-SPOUSE:%s\n" % escape_str(contact["Spouse"])
        del pending["Spouse"]

    # X-EVOLUTION-MANAGER
    if "ManagerName" in contact:
        s += "X-EVOLUTION-MANAGER:%s\n" % escape_str(contact["ManagerName"])
        del pending["ManagerName"]

    # X-EVOLUTION-ASSISTANT
    if "AssistantName" in contact:
        s += "X-EVOLUTION-ASSISTANT:%s\n" % escape_str(contact["AssistantName"])
        del pending["AssistantName"]

    # PHOTO
    if "Picture" in contact:
        s += "PHOTO;ENCODING=b;TYPE=JPEG:%s\n" % contact["Picture"]
        del pending["Picture"]

    # EMAIL;TYPE=HOME
    if "Email1Address" in contact:
        s += "EMAIL;TYPE=HOME:%s\n" % \
              escape_str(contact["Email1Address"])
        del pending["Email1Address"]

    if "Email2Address" in contact:
        s += "EMAIL;TYPE=HOME:%s\n" % \
              escape_str(contact["Email2Address"])
        del pending["Email2Address"]

    if "Email3Address" in contact:
        s += "EMAIL;TYPE=HOME:%s\n" % \
              escape_str(contact["Email3Address"])
        del pending["Email3Address"]

    # ADR;TYPE=HOME
    field = _contact_address_to_vcard_field(contact,
            "HomeStreet", "HomeCity", "HomeState", "HomePostalCode",
            "HomeCountry", pending)
    if field != None:
        s += "ADR;TYPE=HOME:%s\n" % field

    # ADR;TYPE=WORK
    field = _contact_address_to_vcard_field(contact,
            "BusinessStreet", "BusinessCity", "BusinessState",
            "BusinessPostalCode", "BusinessCountry", pending)
    if field != None:
        s += "ADR;TYPE=WORK:%s\n" % field

    # ADR;TYPE=OTHER
    field = _contact_address_to_vcard_field(contact,
            "OtherStreet", "OtherCity", "OtherState",
            "OtherPostalCode", "OtherCountry", pending)
    if field != None:
        s += "ADR;TYPE=OTHER:%s\n" % field

    # X-MSN
    if "IMAddress" in contact:
        s += "X-MSN;TYPE=HOME:%s\n" % escape_str(contact["IMAddress"])
        del pending["IMAddress"]

    if "IMAddress2" in contact:
        s += "X-MSN;TYPE=HOME:%s\n" % escape_str(contact["IMAddress2"])
        del pending["IMAddress2"]

    if "IMAddress3" in contact:
        s += "X-MSN;TYPE=HOME:%s\n" % escape_str(contact["IMAddress3"])
        del pending["IMAddress3"]

    # TEL;TYPE=HOME;TYPE=VOICE
    if "HomePhoneNumber" in contact:
        s += "TEL;TYPE=HOME;TYPE=VOICE:%s\n" % \
              escape_str(contact["HomePhoneNumber"])
        del pending["HomePhoneNumber"]

    if "Home2PhoneNumber" in contact:
        s += "TEL;TYPE=HOME;TYPE=VOICE:%s\n" % \
              escape_str(contact["Home2PhoneNumber"])
        del pending["Home2PhoneNumber"]

    # TEL;TYPE=HOME;TYPE=FAX
    if "HomeFaxNumber" in contact:
        s += "TEL;TYPE=HOME;TYPE=FAX:%s\n" % \
              escape_str(contact["HomeFaxNumber"])
        del pending["HomeFaxNumber"]

    # TEL;TYPE=CELL
    if "MobilePhoneNumber" in contact:
        s += "TEL;TYPE=CELL:%s\n" % \
              escape_str(contact["MobilePhoneNumber"])
        del pending["MobilePhoneNumber"]

    # TEL;TYPE=WORK;TYPE=VOICE
    if "BusinessPhoneNumber" in contact:
        s += "TEL;TYPE=WORK;TYPE=VOICE:%s\n" % \
              escape_str(contact["BusinessPhoneNumber"])
        del pending["BusinessPhoneNumber"]

    if "Business2PhoneNumber" in contact:
        s += "TEL;TYPE=WORK;TYPE=VOICE:%s\n" % \
              escape_str(contact["Business2PhoneNumber"])
        del pending["Business2PhoneNumber"]

    # TEL;TYPE=WORK;TYPE=FAX
    if "BusinessFaxNumber" in contact:
        s += "TEL;TYPE=WORK;TYPE=FAX:%s\n" % \
              escape_str(contact["BusinessFaxNumber"])
        del pending["BusinessFaxNumber"]

    # TEL;TYPE=CAR
    if "CarPhoneNumber" in contact:
        s += "TEL;TYPE=CAR:%s\n" % escape_str(contact["CarPhoneNumber"])
        del pending["CarPhoneNumber"]

    # TEL;TYPE=PAGER
    if "PagerNumber" in contact:
        s += "TEL;TYPE=PAGER:%s\n" % escape_str(contact["PagerNumber"])
        del pending["PagerNumber"]

    # TEL;TYPE="X-EVOLUTION-RADIO"
    if "RadioPhoneNumber" in contact:
        s += "TEL;TYPE=\"X-EVOLUTION-RADIO\":%s\n" % \
              escape_str(contact["RadioPhoneNumber"])
        del pending["RadioPhoneNumber"]

    # TEL;TYPE="X-EVOLUTION-COMPANY"
    if "CompanyMainPhone" in contact:
        s += "TEL;TYPE=\"X-EVOLUTION-COMPANY\":%s\n" % \
              escape_str(contact["CompanyMainPhone"])
        del pending["CompanyMainPhone"]

    # TEL;TYPE="X-EVOLUTION-ASSISTANT"
    if "AssistnamePhoneNumber" in contact:
        s += "TEL;TYPE=\"X-EVOLUTION-ASSISTANT\":%s\n" % \
              escape_str(contact["AssistnamePhoneNumber"])
        del pending["AssistnamePhoneNumber"]

    # UID
    s += "UID:%s\n" % sid

    # X-EVOLUTION-FILE-AS
    if "FileAs" in contact:
        s += "X-EVOLUTION-FILE-AS:%s\n" % escape_str(contact["FileAs"])
        del pending["FileAs"]

    s += "END:VCARD\n"

    if pending:
        print
        print "WARNING: Unhandled AirSync contact properties:"
        for k in pending.keys():
            print "  %s" % k
        print

    return s


def _contact_address_to_vcard_field(contact,
                                    street_field, city_field, state_field,
                                    po_code_field, country_field,
                                    pending):
    adr = ""
    # po box: FIXME
    adr += ";"
    # extended address: FIXME
    adr += ";"
    # street address
    if street_field in contact:
        adr += escape_str(contact[street_field])
        del pending[street_field]
    adr += ";"
    # locality (city)
    if city_field in contact:
        adr += escape_str(contact[city_field])
        del pending[city_field]
    adr += ";"
    # region (state/province)
    if state_field in contact:
        adr += escape_str(contact[state_field])
        del pending[state_field]
    adr += ";"
    # postal code
    if po_code_field in contact:
        adr += escape_str(contact[po_code_field])
        del pending[po_code_field]
    adr += ";"
    # country name
    if country_field in contact:
        adr += escape_str(contact[country_field])
        del pending[country_field]

    if len(adr) > 6:
        return adr
    else:
        return None

