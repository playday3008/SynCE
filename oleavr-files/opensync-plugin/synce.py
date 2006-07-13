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

        gobject.threads_init()

        self.mainloop_exit_event = threading.Event()
        thread.start_new_thread(self._mainloop_thread_func, ())

    def _mainloop_thread_func(self):
        self.mainloop = gobject.MainLoop()
        self.mainloop.run()
        self.mainloop_exit_event.set()

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

        for sid, vcard in adds:
            change = OSyncChange()
            change.uid = sid.encode("utf-8")
            change.objtype = "contact"
            change.format = "vcard30"
            bytes = vcard.encode("utf-8")
            # this is just a temporary hack around a bug in the opensync bindings
            # (OSyncChange.set_data() should either copy the string or
            #  hold a reference to it)
            self.hack.append(bytes)
            change.set_data(bytes, len(bytes) + 1, TRUE)
            change.changetype = CHANGE_ADDED
            change.report(self.ctx)

    def _synchronized_cb(self):
        # FIXME: this shouldn't be emitted multiple times...
        if self.ctx != None:
            ctx = self.ctx
            self.ctx = None

            print "SynCE: Reporting success"
            ctx.report_success()

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
        gobject.idle_add(self.mainloop.quit)
        self.mainloop_exit_event.wait()

def initialize(member):
    return SyncClass(member)

def get_info(info):
    info.name = "synce-plugin"
    info.longname = "Plugin to synchronize with Windows CE device"
    info.description = "by Ole Andre Vadla Ravnaas"

    info.version = 2

    info.accept_objtype("contact")
    info.accept_objformat("contact", "vcard30")
