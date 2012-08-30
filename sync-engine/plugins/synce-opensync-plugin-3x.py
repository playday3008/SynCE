# -*- coding: utf-8 -*-
############################################################################
#
# sync-engine OpenSync plugin for versions of OpenSync >= 0.30
#
# (c) Dr J A Gow 2007
#
# Based upon original plugin but adapted to fit in with the later 0.30
# framework
#
# Prerequisites:
#
#   dbus and dbus-python > 0.80
#   OpenSync >= 0.30
#
# +-+- NOT READY FOR PRODUCTION USE -+-+
#
# 07/01/08 - JAG - The plugin itself seems to work very well. All the
#                  remaining problems appear to be related to format 
#                  conversion. It is for this reason alone that I have
#                  left the 'not ready' banner above. Once the format
#                  conversion code is ready, this will be removed.
#
# 23/10/07 - JAG - Fixed those bloody segfaults and random crashes. Turned 
#                  out to be a problem with thread safety in dbus-python
#                  requiring a simple call to 
#                  dbus.mainloop.glib.threads_init() in addition to the call
#                  to gobject.threads_init() This requirement (only 
#                  necessary in multithreaded dbus main loop environments)
#                  is highly non-obvious and absent from the dbus-python
#                  docs. This would also explain problems with dbus-python-0.8x
#                  in the earlier plugin so this fix has been backported.
#                  Also fixed the asserts on non-slow syncs. This was
#                  simply down to data corruption due to the repeated 
#                  segfaults and crashing.
#
# 22/10/07 - JAG - Some further code cleanups
#                  TODO: Must check and update XML formats
#
# 20/10/07 - JAG - Successfully slow-synced events and tasks with 
#                  file-sync. However, have not yet checked the complete 
#                  integrity of the actual content (believe XML format
#                  used by OpenSync has changed).
#
# 19/10/07 - JAG - Some code cleanups. Still no joy tracing those segfaults
#
# 18/10/07 - JAG - Got a good two-way sync with the file-sync plugin, but
#                  do see an assert in OpenSync on non-slow syncs:
#                  opensync/engine/opensync_obj_engine.c:212:E:
#                  osync_mapping_engine_new: Assertion 
#                  "mapping_entry" failed
#                  Not tested evo2 plugin because it is apparently broken
#                  and not ready for use yet.
#                  Occasional random segfaults on a 64 bit system that are
#                  difficult to trace - weird. Have not tested 32 bit yet
#
# 16/10/07 - JAG - Initial implementation. (using OpenSync SVN)
#
############################################################################

import dbus
from dbus.mainloop.glib import DBusGMainLoop
from dbus.mainloop.glib import threads_init
import gobject
import thread
import threading
import time
import sys
import logging
opensync = __import__('opensync1')
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
    SYNC_ITEM_CONTACTS : ("contact", "xmlformat-contact-doc"),
    SYNC_ITEM_CALENDAR : ("event",   "xmlformat-event-doc"),
    SYNC_ITEM_TASKS    : ("todo",    "xmlformat-todo-doc"),
    SYNC_ITEM_NOTES    : ("note",    "xmlformat-note-doc"),
}

OBJ_TYPE_TO_ITEM_TYPE = {
    "contact" : SYNC_ITEM_CONTACTS,
    "event"   : SYNC_ITEM_CALENDAR,
    "todo"    : SYNC_ITEM_TASKS,
    "note"    : SYNC_ITEM_NOTES,
}

TYPETONAMES = { "contact" : "Contacts",
                "event"   : "Calendar",
		"todo"    : "Tasks"
}

#
# Place the intermediary instance in an array. Python can't seem to retain
# global scalars, whereas it seems to manage to retain global compound types.
# Probably is a better way of doing this....

intermediaries = []


###############################################################################
# class EngineIntermediary
#
# This provides a reference-counted wrapper to ensure that we only connect
# once, disconnect once, and trigger a device sync twice per sync session. 
# The initialization of the d-bus communication chain has been reordered over
# that in the old plugin and events rather than loops are used to wait and
# ensure better sequencing of sync-engine communication
#
###############################################################################

class EngineIntermediary:

	def __init__(self):
        	
		self.logger         = logging.getLogger("SynCE.EngineIntermediary")
        	self.engine         = None
		self.refcnt_connect = 0
		self.refcnt_sync    = 0
		self.ConnectEvent        = threading.Event()
		self.EventLoopExitEvent  = threading.Event()
		self.EventLoopStartEvent = threading.Event()
		self.SyncEvent           = threading.Event()
		self.PrefillEvent        = threading.Event()
		
		DBusGMainLoop(set_as_default=True)
		gobject.threads_init()
		dbus.mainloop.glib.threads_init()
	
		self.logger.debug("init complete")

	def _EventLoopEntry(self):
		
		self.logger.debug("Event loop starting")
        	self.EventLoop = gobject.MainLoop()
		self.EventLoopStartEvent.set()
        	self.EventLoop.run()
        	self.EventLoopExitEvent.set()

    	def _CBSynchronized(self):
        	self.logger.info("device synchronization complete")
        	self.SyncEvent.set()

    	def _CBPrefillComplete(self):
        	self.logger.info("prefill complete")
        	self.PrefillEvent.set()

	def _TriggerDeviceSync(self):
        	self.logger.info("requesting device synchronization")
		self.SyncEvent.clear()
		self.engine.Synchronize()
        	self.logger.info("waiting for engine to complete sync")
		self.SyncEvent.wait()

	def _CBConnectOnIdle(self):

		try:
			self.logger.info("Connecting")
			proxy_obj = dbus.SessionBus().get_object("org.synce.SyncEngine", "/org/synce/SyncEngine")
			self.engine = dbus.Interface(proxy_obj, dbus_interface="org.synce.SyncEngine")
			self.engine.connect_to_signal("Synchronized", lambda: gobject.idle_add(self._CBSynchronized))
			self.engine.connect_to_signal("PrefillComplete", lambda: gobject.idle_add(self._CBPrefillComplete))
			self.logger.info("Connected")
			self.ConnectEvent.set()
			
		except Exception, e:
			self.logger.error("Connection failed. Unable to connect to running SyncEngine: %s", e)
			self.ConnectEvent.set()
			raise

    	def Connect(self):
		
		# The intermediary ensures we only connect once for each call to Connect, allowing
		# us to split off the objtypes we are dealing with.
		
		self.logger.info("Attempting to connect to running sync-engine")
		if self.refcnt_connect == 0:
			if self.engine == None:
				try:
					self.logger.debug("triggering connection")
					self._CBConnectOnIdle()
					self.logger.debug("connection queued")
					self.ConnectEvent.wait()
        				thread.start_new_thread(self._EventLoopEntry,())
					self.EventLoopStartEvent.wait()
					self.logger.info("connection complete")
				except Exception:
					self.logger.error("Connection failed.")
					raise
		self.refcnt_connect += 1
		self.logger.debug("connect exit")
	
	def Disconnect(self):
		if self.refcnt_connect > 0:
			self.refcnt_connect -= 1
			if self.refcnt_connect == 0:
				self.engine == None
			        gobject.idle_add(self.EventLoop.quit)
        			self.EventLoopExitEvent.wait()
		else:
			self.logger.error("engine connection refcount out of sequence")
	
	def AcquireChanges(self):
		if self.refcnt_sync == 0:
			self.logger.info("requesting first device synchronization")
        		self._TriggerDeviceSync()
		else:
			self.logger.info("device synchronization already triggered (%d)" % self.refcnt_sync)
		self.refcnt_sync += 1
		
	def TransmitChanges(self):
		if self.refcnt_sync > 0:
			self.refcnt_sync -= 1
			if self.refcnt_sync == 0:
				self.logger.info("all changes committed: triggering second device synchronization")
				self._TriggerDeviceSync()
			else:
				self.logger.info("other items to be synced: postponing final sync (%d)" % self.refcnt_sync)
		else:
			self.logger.error("device sync refcount out of sequence")

    	def TriggerPrefill(self, items):
        	self.logger.info("initiating prefill")
        	self.PrefillEvent.clear()
        	rc = self.engine.PrefillRemote(items)
        	if rc == 1:
        		self.PrefillEvent.wait()
        	return rc
	
###############################################################################
# ItemSink
#
# Common code used to sync each item. One instance of this class is created
# for each item engaged in the sync. Effectively this splits up the sync 
# process by item. 
#
###############################################################################
	
class ItemSink(opensync.ObjTypeSinkCallbacks):
	
	def __init__(self, item):
		
		self.objtype,self.format = SUPPORTED_ITEM_TYPES[item]
		self.logger = logging.getLogger("SynCE.ItemSink("+self.objtype+")")
		opensync.ObjTypeSinkCallbacks.__init__(self, self.objtype)
		self.sink.add_objformat(opensync.ObjFormatSink(self.format))

	#
	# connect
	#
	# OpenSync will call this function when connecting with this item.
	# Note it will call it once per item, hence the use of the intermediary
	#

	def connect(self, info, ctx):
		
		engine = intermediaries[0]
		engine.Connect()

	#
	# get_changes
	#
	# Bit like the get_changes in the old plugin, but operates on
	# a per-item basis. Note the new hierarchy of classes representing
	# items
	#

	def get_changes(self, info, ctx, slow_sync):
		
		intermediary = intermediaries[0]
		
		self.logger.info("Getting changes for item %s" % self.sink.get_name())
		
		prefill = []
		if slow_sync:
			self.logger.info("slow sync requested for item %s" % self.sink.get_name())
			prefill.append(TYPETONAMES[self.sink.get_name()])
			
		intermediary.AcquireChanges()	
		
	        if len(prefill) > 0:
			if intermediary.TriggerPrefill(prefill) == 0:
				self.logger.error("prefill failed")

	        self.logger.debug("requesting remote changes for %s objects" %self.sink.get_name())
		t = OBJ_TYPE_TO_ITEM_TYPE[self.sink.get_name()]
        	changesets = intermediary.engine.GetRemoteChanges([t])
        	self.logger.debug("got %d changesets", len(changesets))

		acks = {}	
			
		if len(changesets) > 0:
			
			fmtenv=info.get_format_env()
			fmt = fmtenv.find_objformat(self.format)
			
			item_type = OBJ_TYPE_TO_ITEM_TYPE[self.sink.get_name()]
			changes = changesets[item_type]
		
        		self.logger.debug("processing changes for item type %d" % item_type)

                	acks[item_type] = []

                	for change in changes:
                    		guid, chg_type, data = change

                    		change = opensync.Change()
                    		change.set_uid(array.array('B',guid).tostring())
                    		change.set_changetype(chg_type)

				bytes=None
                       		if chg_type != opensync.CHANGE_TYPE_DELETED:
        	               		bytes = array.array('B',data).tostring()
				
				osdata = opensync.Data(bytes,fmt)
				osdata.set_objtype(self.sink.get_name())
                       		change.set_data(osdata)
				
				self.logger.debug("reporting change")
				ctx.report_change(change)
				self.logger.debug("change reported")
				
                    		acks[item_type].append(array.array('B',guid).tostring())
            	else:
                	self.logger.debug("no changes for item type %d" % item_type)

		if acks!={}:
            		self.logger.debug("acknowledging remote changes for item type %d", item_type)
            		intermediary.engine.AcknowledgeRemoteChanges(acks)
	    		self.logger.debug("acknowledgement complete")
			
		self.logger.debug("exiting get_changeinfo")
	
	#
	# commit
	#
	# Again, bit like the commit in the old plugin, but operates on a per-item basis
	#
	
	def commit(self, info, ctx, chg):
		intermediary = intermediaries[0]
		
		self.logger.debug("commit called for item %s with change type %d", chg.uid, chg.changetype)

        	if chg.objtype in OBJ_TYPE_TO_ITEM_TYPE:
                
            		item_type = OBJ_TYPE_TO_ITEM_TYPE[chg.objtype]

            		data = ""
            		if chg.changetype != opensync.CHANGE_TYPE_DELETED:
                		data = chg.get_data().get_data()

            		intermediary.engine.AddLocalChanges( { item_type : ((chg.get_uid(),chg.get_changetype(),data),),} )

        	else:
            		raise Exception("SynCE: object type %s not yet handled" % chg.objtype)

	#
	# committed_all
	#
	# Called by OpenSync when all changes are committed. We use this via the intermediary
	# to trigger the second device sync once all items have been exchanged.
	#
	
	def committed_all(self, info, ctx):
		intermediary = intermediaries[0]
		self.logger.info("All SynCE items committed")
		intermediary.TransmitChanges()
		
	#
	# disconnect
	#
	# We use the intermediary to ensure disconnection once _all_ items
	# have been synced (sunk?) :-)
	
	def disconnect(self, info, ctx):
		intermediary = intermediaries[0]
		intermediary.Disconnect()

	#
	# sync_done
	#
	# Called when all syncing is done, but before we disconnect. We may put
	# the TransmitChanges call here if necessary.

	def sync_done(self, info, ctx):
		intermediary = intermediaries[0]
		self.logger.info("sync_done called!")

# 
# initialize
#
# Called to initialize the plugin. Create and maintain the global instance
# of the intermediary, then tell OpenSync about the objtypes we support.

def initialize(info):
	logging.basicConfig(level=logging.DEBUG, stream=sys.stdout)
	intermediaries.append(EngineIntermediary())

	# save the first sink to return from initialise, this is
	# then passed as userdata in the python module for the
	# "Main" sink to do it's work
	# *** is this the correct way to do this ? ***
	sink = ItemSink(SYNC_ITEM_CONTACTS)
	info.add_objtype(sink.sink)

	info.add_objtype(ItemSink(SYNC_ITEM_CALENDAR).sink)
	info.add_objtype(ItemSink(SYNC_ITEM_TASKS).sink)

	return

#
# discover
#
# New in 0.30+. Used to enable OpenSync to actually find out what we can and
# can't sync. We interrogate the device here so we actually match what the device
# partnership is capable of.

def discover(info, data):
	intermediary = intermediaries[0]
	intermediary.Connect()
	SyncTypes = intermediary.engine.GetSynchronizedItemTypes()

	#create a blank config
	config = opensync.PluginConfig()

	for sink in info.objtypes:
		for wmtype in SyncTypes:
			if OBJ_TYPE_TO_ITEM_TYPE[sink.get_name()] == wmtype:
				sink.available = True

				# create a new resource and add it to the config
				res = opensync.PluginResource()
				item_type = OBJ_TYPE_TO_ITEM_TYPE[sink.get_name()]
				res.add_objformat_sink(opensync.ObjFormatSink(SUPPORTED_ITEM_TYPES[item_type][1]))
				res.objtype = sink.name
				res.enabled = True
				config.add_resource(res)

	info.config = config

	info.version = opensync.Version()
	info.version.plugin = "synce-sync"
	intermediary.Disconnect()

