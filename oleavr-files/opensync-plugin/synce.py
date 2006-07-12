import dbus
import dbus.glib
import gobject
import thread
import threading
from opensync import *

class SyncClass:
    def __init__(self, member):
        self.__member = member
        self.engine = None
        self.hack = []
        gobject.threads_init()
        thread.start_new_thread(self._mainloop_thread_func, ())

    def _mainloop_thread_func(self):
        self.mainloop = gobject.MainLoop()
        self.mainloop.run()

    def connect(self, ctx):
        print "SynCE::connect called"
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
            vcard = self._contact_to_vcard(sid, contact)
            bytes = vcard.encode("utf-8")
            # this is just a temporary hack around a bug in the opensync bindings
            # (OSyncChange.set_data() should either copy the string or
            #  hold a reference to it)
            self.hack.append(bytes)
            change.set_data(bytes, len(bytes) + 1, TRUE)
            change.changetype = CHANGE_ADDED
            change.report(self.ctx)

    def _contact_to_vcard(self, sid, contact):
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
            s += "FN:%s\n" % fn

        # N
        n = ""
        if "LastName" in contact:
            n += contact["LastName"]
        n += ";"
        if "FirstName" in contact:
            n += contact["FirstName"]
        n += ";"
        if "MiddleName" in contact:
            n += contact["MiddleName"]
            del pending["MiddleName"]
        n += ";"
        # honorific prefixes: FIXME
        n += ";"
        # honorific suffixes: FIXME

        if len(n) > 4:
            s += "N:%s\n" % n

        # TITLE
        if "JobTitle" in contact:
            s += "TITLE:%s\n" % contact["JobTitle"]
            del pending["JobTitle"]

        # URL
        if "WebPage" in contact:
            s += "URL:%s\n" % contact["WebPage"]
            del pending["WebPage"]

        # NICKNAME
        if "NickName" in contact:
            s += "NICKNAME:%s\n" % contact["NickName"]
            del pending["NickName"]

        # BDAY
        if "Birthday" in contact:
            s += "BDAY:%s\n" % contact["Birthday"].split("T")[0]
            del pending["Birthday"]

        # PHOTO
        if "Picture" in contact:
            s += "PHOTO;ENCODING=b;TYPE=JPEG:%s\n" % contact["Picture"]
            del pending["Picture"]

        # EMAIL;TYPE=HOME
        if "Email1Address" in contact:
            s += "EMAIL;TYPE=HOME;X-EVOLUTION-UI-SLOT=1:%s\n" % contact["Email1Address"]
            del pending["Email1Address"]

        # ADR;TYPE=HOME
        adr = ""
        # po box: FIXME
        adr += ";"
        # extended address: FIXME
        adr += ";"
        # street address
        if "HomeStreet" in contact:
            adr += contact["HomeStreet"]
            del pending["HomeStreet"]
        adr += ";"
        # locality (city)
        if "HomeCity" in contact:
            adr += contact["HomeCity"]
            del pending["HomeCity"]
        adr += ";"
        # region (state/province): FIXME
        adr += ";"
        # postal code
        if "HomePostalCode" in contact:
            adr += contact["HomePostalCode"]
            del pending["HomePostalCode"]
        adr += ";"
        # country name
        if "HomeCountry" in contact:
            adr += contact["HomeCountry"]
            del pending["HomeCountry"]

        if len(adr) > 6:
            s += "ADR;TYPE=HOME:%s\n" % adr

        # TEL;TYPE=HOME
        if "HomePhoneNumber" in contact:
            s += "TEL;TYPE=HOME;TYPE=VOICE;X-EVOLUTION-UI-SLOT=1:%s\n" % contact["HomePhoneNumber"]
            del pending["HomePhoneNumber"]

        # TEL;TYPE=VOICE
        if "Home2PhoneNumber" in contact:
            s += "TEL;TYPE=VOICE;X-EVOLUTION-UI-SLOT=2:%s\n" % contact["Home2PhoneNumber"]
            del pending["Home2PhoneNumber"]

        # TEL:TYPE=CELL
        if "MobilePhoneNumber" in contact:
            s += "TEL;TYPE=CELL;X-EVOLUTION-UI-SLOT=3:%s\n" % contact["MobilePhoneNumber"]
            del pending["MobilePhoneNumber"]

        # UID
        s += "UID:%s\n" % sid

        # X-EVOLUTION-FILE-AS
        if "FileAs" in contact:
            s += "X-EVOLUTION-FILE-AS:%s\n" % contact["FileAs"].replace(",", "\\,")
            del pending["FileAs"]

        s += "END:VCARD\n"

        if pending:
            print
            print "WARNING: Unhandled AirSync contact properties:"
            for k in pending.keys():
                print "  %s" % k
            print

        return s

    def _synchronized_cb(self):
        print "SynCE: Reporting success"
        self.completions_hack += 1
        if self.completions_hack == 2:
            self.ctx.report_success()

    def get_changeinfo(self, ctx):
        print "SynCE: get_changeinfo called"
        if self.__member.get_slow_sync("data"):
            print "SynCE: slow-sync requested"

        self.ctx = ctx
        gobject.idle_add(self._do_get_changeinfo_idle_cb)

    def _do_get_changeinfo_idle_cb(self):
        print "SynCE: Calling StartSync"
        self.completions_hack = 0
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
