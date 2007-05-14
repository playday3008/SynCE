# -*- coding: utf-8 -*-
############################################################################
#    Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>       #
#                                                                          #
#    This program is free software; you can redistribute it and#or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################

import dbus
import dbus.glib
import gobject
import thread
import threading
import time
import sys
import logging
from opensync import *
import array

SYNC_ITEM_CALENDAR  = 0
SYNC_ITEM_CONTACTS  = 1
SYNC_ITEM_EMAIL     = 2
SYNC_ITEM_FAVORITES = 3
SYNC_ITEM_FILES     = 4
SYNC_ITEM_MEDIA     = 5
SYNC_ITEM_NOTES     = 6
SYNC_ITEM_TASKS     = 7

SUPPORTED_ITEM_TYPES = {
    SYNC_ITEM_CONTACTS : ("contact", "xml-contact-doc"),
    SYNC_ITEM_CALENDAR : ("event",   "xml-event-doc"),
    SYNC_ITEM_TASKS    : ("todo",    "xml-todo-doc"),
    SYNC_ITEM_NOTES    : ("note",    "xml-note-doc"),
}

OBJ_TYPE_TO_ITEM_TYPE = {
    "contact" : SYNC_ITEM_CONTACTS,
    "event"   : SYNC_ITEM_CALENDAR,
    "todo"    : SYNC_ITEM_TASKS,
    "note"    : SYNC_ITEM_NOTES,
}

class SyncClass:
    def __init__(self, member):
        self.logger = logging.getLogger("SynCE")

        self.__member = member
        self.engine = None
	self.isPrefilled=False

        gobject.threads_init()

        self.mainloop_exit_event = threading.Event()
        thread.start_new_thread(self._mainloop_thread_func, ())

    def _mainloop_thread_func(self):
        self.mainloop = gobject.MainLoop()
        self.mainloop.run()
        self.mainloop_exit_event.set()

    def connect(self, ctx):
        self.logger.debug("connect() called")
        gobject.idle_add(self._do_connect_idle_cb, ctx)

    def _synchronized_cb(self):
        self.logger.info("device synchronization complete")
        self.engine_synced = True

    def _prefillcomplete_cb(self):
        self.logger.info("prefill complete")
        self.isPrefilled = True

    def _do_prefill(self, items):
        self.logger.info("initiating prefill")
	self.isPrefilled = False
        rc = self.engine.PrefillRemote(items)
        if rc == 1:
            while self.isPrefilled==False:
                time.sleep(1)
        return rc

    def _do_sync(self):
        self.logger.info("requesting device synchronization")
        self.engine_synced = False
        self.engine.Synchronize()
	self.logger.info("waiting for engine to complete sync")
        while not self.engine_synced:
            time.sleep(1)

    def _do_connect_idle_cb(self, ctx):
        try:
            proxy_obj = dbus.SessionBus().get_object("org.synce.SyncEngine", "/org/synce/SyncEngine")
            self.engine = dbus.Interface(proxy_obj, "org.synce.SyncEngine")
            self.engine.connect_to_signal("Synchronized", lambda: gobject.idle_add(self._synchronized_cb))
	    self.engine.connect_to_signal("PrefillComplete", lambda: gobject.idle_add(self._prefillcomplete_cb))

            ctx.report_success()
        except Exception, e:
            self.logger.error("connect() failed: %s", e)
            ctx.report_error()

    def get_changeinfo(self, ctx):
        self.logger.debug("get_changeinfo() called")

	isSlow = 0
	prefill = []
        if self.__member.get_slow_sync("contact"):
	    self.logger.debug("slow sync requested for Contacts")
	    prefill.append("Contacts")
	if self.__member.get_slow_sync("event"):
	    self.logger.debug("slow sync requested for Calendar")
	    prefill.append("Calendar")
	if self.__member.get_slow_sync("todo"):
	    self.logger.debug("slow sync requested for Tasks")
	    prefill.append("Tasks")

	time.sleep(1)
        self._do_sync()

        if len(prefill) > 0:
            if self._do_prefill(prefill) == 0:
                self.logger.debug("prefill failed")

        self.logger.debug("requesting remote changes")
       	changesets = self.engine.GetRemoteChanges(self.engine.GetSynchronizedItemTypes())
        self.logger.debug("got %d changesets", len(changesets))

        for item_type, changes in changesets.items():
            if changes:
                self.logger.debug("processing changes for %d items of item type %d", len(changesets.items()), item_type)
            else:
                self.logger.debug("no changes for item type %d", item_type)

            acks = { item_type : [] }

            for change in changes:
                guid, chg_type, data = change

                change = OSyncChange()
		change.uid = array.array('B',guid).tostring() 
                change.changetype = chg_type

                if item_type in SUPPORTED_ITEM_TYPES:
                    change.objtype, change.format = SUPPORTED_ITEM_TYPES[item_type]

                    if chg_type != CHANGE_DELETED:
                        bytes = '<?xml version="1.0" encoding="utf-8"?>\n'
                        bytes += array.array('B',data).tostring()

                        change.set_data(bytes)
                else:
                    raise Exception("Unhandled item type %d" % item_type)

                change.report(ctx)

                acks[item_type].append(array.array('B',guid).tostring())

            if len(acks[item_type]) > 0:
                self.logger.debug("acknowledging remote changes for item type %d", item_type)
                self.engine.AcknowledgeRemoteChanges(acks)

        ctx.report_success()

    def commit_change(self, ctx, chg):
        self.logger.debug("commit_change() called for item %s with change type %d", chg.uid, chg.changetype)

        if chg.objtype in OBJ_TYPE_TO_ITEM_TYPE:
		
            item_type = OBJ_TYPE_TO_ITEM_TYPE[chg.objtype]

            data = ""
            if chg.changetype != CHANGE_DELETED:
                data = chg.data

            self.engine.AddLocalChanges( { item_type : ((chg.uid,chg.changetype,data),),} )

        else:
            raise Exception("SynCE: object type %s not yet handled" % chg.objtype)

        ctx.report_success()

    def sync_done(self, ctx):
        self.logger.debug("sync_done() called")

        self._do_sync()

	self.engine.FlushItemDB()
        ctx.report_success()

    def disconnect(self, ctx):
        self.logger.debug("disconnect() called")

        self.engine = None

        ctx.report_success()

    def finalize(self):
        self.logger.debug("finalize() called")

        gobject.idle_add(self.mainloop.quit)
        self.mainloop_exit_event.wait()

def initialize(member):
    logging.basicConfig(level=logging.DEBUG, stream=sys.stdout)
    return SyncClass(member)

def get_info(info):
    info.name = "synce-plugin"
    info.longname = "Plugin to synchronize with Windows CE device"
    info.description = "by Ole Andre Vadla Ravnaas"

    info.version = 2

    info.accept_objtype("contact")
    info.accept_objformat("contact", "xml-contact-doc")

    info.accept_objtype("event")
    info.accept_objformat("event", "xml-event-doc")

    info.accept_objtype("todo")
    info.accept_objformat("todo", "xml-todo-doc")

    info.accept_objtype("note")
    info.accept_objformat("note", "xml-note-doc")
