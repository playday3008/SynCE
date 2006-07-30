# -*- coding: utf-8 -*-
#
# Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

import dbus
import dbus.glib
import gobject
import thread
import threading
from opensync import *

#import time

SYNC_ITEM_CALENDAR  = 0
SYNC_ITEM_CONTACTS  = 1
SYNC_ITEM_EMAIL     = 2
SYNC_ITEM_FAVORITES = 3
SYNC_ITEM_FILES     = 4
SYNC_ITEM_MEDIA     = 5
SYNC_ITEM_NOTES     = 6
SYNC_ITEM_TASKS     = 7

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
            self.engine.connect_to_signal("Synchronized", lambda: gobject.idle_add(self._synchronized_cb))

            ctx.report_success()
        except Exception, e:
            print "SynCE::connect: failed: %s" % e
            ctx.report_error()

    def _synchronized_cb(self):
        if self.ctx != None:
            ctx = self.ctx
            self.ctx = None

            #print "Sleeping 10 seconds...",
            #time.sleep(10)
            #print "done"

            changesets = self.engine.GetRemoteChanges((SYNC_ITEM_CONTACTS,))
            for item_type, changes in changesets.items():
                guids = []

                if changes:
                    print "Processing changes for item_type %d" % item_type
                else:
                    print "No changes for item_type %d" % item_type

                for change in changes:
                    guid, chg_type, data = change

                    change = OSyncChange()
                    change.uid = guid.encode("utf-8")
                    change.changetype = chg_type

                    if item_type == SYNC_ITEM_CONTACTS:
                        change.objtype = "contact"
                        change.format = "xml-contact-string"

                        if chg_type != CHANGE_DELETED:
                            bytes = data.encode("utf-8")
                            # this is just a temporary hack around a bug in the opensync bindings
                            # (OSyncChange.set_data() should either copy the string or
                            #  hold a reference to it)
                            self.hack.append(bytes)
                            change.set_data(bytes, TRUE)
                    else:
                        raise Exception("Unhandled item_type %d" % item_type)

                    change.report(ctx)

                    guids.append(guid)

                if guids:
                    print "SynCE: Acknowledging remote changes"
                    acks = { item_type : guids }
                    self.engine.AcknowledgeRemoteChanges(acks)

            print "SynCE: Reporting success"
            ctx.report_success()

    def get_changeinfo(self, ctx):
        print "SynCE: get_changeinfo called"
        if self.__member.get_slow_sync("data"):
            print "SynCE: slow-sync requested"

        self.ctx = ctx
        ctx.report_success()
        #gobject.idle_add(self._do_get_changeinfo_idle_cb)

    def _do_get_changeinfo_idle_cb(self):
        print "SynCE: Calling StartSync"
        self.engine.StartSync()

    def commit_change(self, ctx, chg):
        print "SynCE: Calling AddLocalChanges() with changetype=%d" % chg.changetype

        if chg.changetype != CHANGE_DELETED:
            print "Format: '%s'" % chg.format
            print "Data: '%s'" % chg.data

        if chg.format != "xml-contact-string":
            raise Exception("SynCE: %s not yet handled" % chg.format)

        data = ""
        if chg.changetype != CHANGE_DELETED:
            data = chg.data.decode("utf-8")

            print "SynCE: Data='%s'" % data

        self.engine.AddLocalChanges(
                {
                    SYNC_ITEM_CONTACTS : ((chg.uid.decode("utf-8"),
                                           chg.changetype,
                                           data),),
                })

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
    info.accept_objformat("contact", "xml-contact-string")

    info.accept_objtype("event")
    info.accept_objformat("event", "xml-event-string")
