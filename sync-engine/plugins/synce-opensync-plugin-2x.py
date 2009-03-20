# -*- coding: utf-8 -*-
############################################################################
#    Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>         #
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
#
# 23/10/2007 - JAG - Updated to support dbus and dbus-python > 0.80 and to 
#                    ensure thread safety in these environments. This has 
#                    been back-ported from the OpenSync 0.30 plugin. We
#                    also clean up the wait systems by using events rather
#                    than endless loops. This fixes the random crashes and
#                    'broken pipe' messages with dbus-python 0,80
#                    Some code cleanups
#
############################################################################

import dbus
import dbus.service
import gobject
import thread
import threading
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
        global dbus
        self.logger = logging.getLogger("SynCE")

        self.__member = member
        self.engine = None
        self.isPrefilled=False

        gobject.threads_init()
        
	self.MainloopExitEvent    = threading.Event()
	self.SyncCompleteEvent    = threading.Event()
	self.PrefillCompleteEvent = threading.Event()
	
	# How we initialize the dbus comms and main loop
	# is somewhat version-dependent....
	
	DBusVersion = getattr(dbus,'version', (0,0,0))
	if DBusVersion < (0,80,0):
		import dbus.glib
		dbus.glib.init_threads()
	else:
		from dbus.mainloop.glib import DBusGMainLoop
		from dbus.mainloop.glib import threads_init
		dbus.mainloop.glib.threads_init()
		DBusGMainLoop(set_as_default=True)
		
        thread.start_new_thread(self._MainLoopEntry, ())

    def _MainLoopEntry(self):
        self.mainloop = gobject.MainLoop()
        self.mainloop.run()
        self.MainloopExitEvent.set()

    def _CBSyncComplete(self):
        self.logger.info("device synchronization complete")
        self.SyncCompleteEvent.set()

    def _CBPrefillComplete(self):
        self.logger.info("prefill complete")
        self.PrefillCompleteEvent.set()

    def _TriggerPrefill(self, items):
        self.logger.info("initiating prefill")
        self.PrefillCompleteEvent.clear()
	rc = self.engine.PrefillRemote(items)
        if rc == 1:
		self.PrefillCompleteEvent.wait()
	return rc

    def _TriggerSync(self):
        self.logger.info("initiating device synchronization")
        self.SyncCompleteEvent.clear()
        self.engine.Synchronize()
        self.logger.info("waiting for engine to complete sync")
	self.SyncCompleteEvent.wait()

    def _CBConnectOnIdle(self, ctx):
        try:
            proxy_obj = dbus.SessionBus().get_object("org.synce.SyncEngine", "/org/synce/SyncEngine")
            self.engine = dbus.Interface(proxy_obj, "org.synce.SyncEngine")
            self.engine.connect_to_signal("Synchronized", lambda: gobject.idle_add(self._CBSyncComplete))
            self.engine.connect_to_signal("PrefillComplete", lambda: gobject.idle_add(self._CBPrefillComplete))

            ctx.report_success()
        except Exception, e:
            self.logger.error("connect() failed: %s", e)
            ctx.report_error()

    def connect(self, ctx):
        self.logger.debug("Connect() called")
        gobject.idle_add(self._CBConnectOnIdle, ctx)

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

        self._TriggerSync()

        if len(prefill) > 0:
            if self._TriggerPrefill(prefill) == 0:
                self.logger.debug("failed to prefill - reverting to normal sync")

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
                        bytes = array.array('B',data).tostring()

                        change.set_data(bytes)
                else:
                    raise Exception("Unhandled item type %d" % item_type)

                change.report(ctx)

                acks[item_type].append(array.array('B',guid).tostring())

            if len(acks[item_type]) > 0:
                self.logger.debug("acknowledging remote changes for item type %d", item_type)
		try:
                	self.engine.AcknowledgeRemoteChanges(acks)
		except Exception,e:
			self.logger.debug("exception %s",e)

	self.logger.debug("reporting success")
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

        self._TriggerSync()

        self.engine.FlushItemDB()
        ctx.report_success()

    def disconnect(self, ctx):
        self.logger.debug("disconnect() called")

        self.engine = None

        ctx.report_success()

    def finalize(self):
        self.logger.debug("finalize() called")

        gobject.idle_add(self.mainloop.quit)
        self.MainloopExitEvent.wait()

def initialize(member):
    logging.basicConfig(level=logging.DEBUG, stream=sys.stdout)
    return SyncClass(member)

def get_info(info):
    info.name = "synce-opensync-plugin"
    info.longname = "Plugin to synchronize with Windows Mobile 5 and later devices"
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
