import dbus
import dbus.glib
import gobject
import thread
import threading
from opensync import *


def escape_str(s):
    ret = u""
    for c in s:
        if c == u"\r":
            continue
        elif c == u"\n":
            ret += u"\\n"
            continue
        elif c == u"\\":
            ret += u"\\\\"
            continue
        elif c in (u";", u","):
            ret += u"\\"
        ret += c
    return ret


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
        s += escape_str("TITLE:%s\n" % contact["JobTitle"])
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
    field = contact_address_to_vcard_field(contact,
            "HomeStreet", "HomeCity", "HomeState", "HomePostalCode",
            "HomeCountry", pending)
    if field != None:
        s += "ADR;TYPE=HOME:%s\n" % field

    # ADR;TYPE=WORK
    field = contact_address_to_vcard_field(contact,
            "BusinessStreet", "BusinessCity", "BusinessState",
            "BusinessPostalCode", "BusinessCountry", pending)
    if field != None:
        s += "ADR;TYPE=WORK:%s\n" % field

    # ADR;TYPE=OTHER
    field = contact_address_to_vcard_field(contact,
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


def contact_address_to_vcard_field(contact,
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


class SyncClass:
    def __init__(self, member):
        self.__member = member
        self.engine = None
        gobject.threads_init()
        thread.start_new_thread(self._mainloop_thread_func, ())

    def _mainloop_thread_func(self):
        self.mainloop = gobject.MainLoop()
        self.mainloop.run()

    def connect(self, ctx):
        print "SynCE::connect called"
        self.hack = []
        gobject.idle_add(self._do_connect_idle_cb, ctx)

    def _do_connect_idle_cb(self, ctx):
        try:
            bus = dbus.SessionBus()
            proxy_obj = bus.get_object("org.synce.SyncEngine", "/org/synce/SyncEngine")
            self.engine = dbus.Interface(proxy_obj, "org.synce.SyncEngine")
            self.engine.connect_to_signal("ContactsAdded", lambda *args: gobject.idle_add(self._contacts_added_cb, *args))
            self.engine.connect_to_signal("Synchronized", lambda: gobject.idle_add(self._synchronized_cb))

            ctx.report_success()
        except Exception, e:
            print "SynCE::connect: failed: %s" % e
            ctx.report_error()

    def _contacts_added_cb(self, adds):
        print "SynCE: Contacts added called with %d contacts" % len(adds)

        for sid, contact in adds:
            change = OSyncChange()
            change.uid = sid.encode("utf-8")
            change.objtype = "contact"
            change.format = "vcard30"
            vcard = contact_to_vcard(sid, contact)
            bytes = vcard.encode("utf-8")
            # this is just a temporary hack around a bug in the opensync bindings
            # (OSyncChange.set_data() should either copy the string or
            #  hold a reference to it)
            self.hack.append(bytes)
            change.set_data(bytes, len(bytes) + 1, TRUE)
            change.changetype = CHANGE_ADDED
            change.report(self.ctx)

    def _synchronized_cb(self):
        print "SynCE: Reporting success"
        self.ctx.report_success()

    def get_changeinfo(self, ctx):
        print "SynCE: get_changeinfo called"
        if self.__member.get_slow_sync("data"):
            print "SynCE: slow-sync requested"

        self.ctx = ctx
        gobject.idle_add(self._do_get_changeinfo_idle_cb)

    def _do_get_changeinfo_idle_cb(self):
        print "SynCE: Calling StartSync"
        self.engine.StartSync()

    def commit_change(self, ctx, chg):
        print "SynCE: commit_change called"
        print "Opensync wants me to write data with size " + str(chg.datasize)
        print chg.format
        print "Data: \"%s\"" % chg.data
        ctx.report_success()

    def disconnect(self, ctx):
        print "SynCE: disconnect called"

        self.engine = None
        self.get_changeinfo_ctx = None

        ctx.report_success()

        self.hack = []

    def sync_done(self, ctx):
        print "SynCE: sync_done called"
        ctx.report_success()

    def finalize(self):
        print "SynCE: finalize called"
        self.mainloop.quit()

def initialize(member):
    return SyncClass(member)

def get_info(info):
    info.name = "synce-plugin"
    info.longname = "Plugin to synchronize with Windows CE device"
    info.description = "by Ole Andre Vadla Ravnaas"

    info.version = 2

    info.accept_objtype("contact")
    info.accept_objformat("contact", "vcard30")
