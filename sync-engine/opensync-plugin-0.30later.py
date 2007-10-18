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
# +-+- NOT READY FOR PRODUCTION USE -+-+
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
import dbus.glib
import gobject
import thread
import threading
import time
import sys
import logging
import opensync
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
# that in the old plugin (seems to relieve a segfault on dbus-python > 0.80 
# - though I can't think why) and events rather than loops are used to wait and
# ensure better sequencing of sync-engine communication
#
###############################################################################


class EngineIntermediary:

	def __init__(self):
        	
		self.logger         = logging.getLogger("SynCE")
        	self.engine         = None
		self.engine_synced  = False
        	self.isPrefilled    = False
		self.refcnt_connect = 0
		self.refcnt_sync    = 0
	
		gobject.threads_init()
		self.EventLoopExitEvent  = threading.Event()
		self.ConnectEvent        = threading.Event()
		self.EventLoopStartEvent = threading.Event()
	
		self.logger.info("Intermediary init complete")

	def _EventLoopEntry(self):
		
		self.logger.info("Event loop starting")
        	self.EventLoop = gobject.MainLoop()
		self.EventLoopStartEvent.set()
        	self.EventLoop.run()
        	self.EventLoopExitEvent.set()

    	def _CBSynchronized(self):
        	self.logger.info("device synchronization complete")
        	self.engine_synced = True

    	def _CBPrefillComplete(self):
        	self.logger.info("prefill complete")
        	self.isPrefilled = True

	def _TriggerDeviceSync(self):
        	self.logger.info("requesting device synchronization")
        	self.engine_synced = False
		self.engine.Synchronize()
        	self.logger.info("waiting for engine to complete sync")
        	while not self.engine_synced:
			time.sleep(1)

	def _CBConnectOnIdle(self):
		try:
			self.logger.info("Connecting")
			proxy_obj = dbus.SessionBus().get_object("org.synce.SyncEngine", "/org/synce/SyncEngine")
			self.engine = dbus.Interface(proxy_obj, dbus_interface="org.synce.SyncEngine")
			self.engine.connect_to_signal("Synchronized", self._CBSynchronized)
			self.engine.connect_to_signal("PrefillComplete", lambda: gobject.idle_add(self._CBPrefillComplete))
			self.logger.info("Connected")
			self.ConnectEvent.set()
			
		except Exception, e:
			self.logger.error("Connection failed. Unable to connect to running SyncEngine")
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
        				#gobject.idle_add(self._CBConnectOnIdle)
					self._CBConnectOnIdle()
					self.logger.debug("connection queued")
					self.ConnectEvent.wait()
        				thread.start_new_thread(self._EventLoopEntry,())
					self.EventLoopStartEvent.wait()
					self.logger.debug("connection complete")
				except Exception:
					self.logger.error("Connection failed.")
					raise
		self.refcnt_connect += 1
		self.logger.error("connect exit")
	
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
				self.logger.info("triggering second device synchronization")
				self._TriggerDeviceSync()
			else:
				self.logger.info("other items to be synced: postponing final sync (%d)" % self.refcnt_sync)
		else:
			self.logger.error("device sync refcount out of sequence")

    	def TriggerPrefill(self, items):
        	self.logger.info("initiating prefill")
        	self.isPrefilled = False
        	rc = self.engine.PrefillRemote(items)
        	if rc == 1:
        		while self.isPrefilled==False:
				time.sleep(1)
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
		opensync.ObjTypeSinkCallbacks.__init__(self, self.objtype)
		self.sink.add_objformat(self.format)

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

	def get_changes(self, info, ctx):
		engine = intermediaries[0]
		
		engine.logger.info("Getting changes for item %s" % self.sink.get_name())
		
		prefill = []
		if self.sink.get_slowsync():
			engine.logger.info("slow sync requested for item %s" % self.sink.get_name())
			prefill.append(TYPETONAMES[self.sink.get_name()])
			
		engine.AcquireChanges()	
		
	        if len(prefill) > 0:
			if engine.TriggerPrefill(prefill) == 0:
				engine.logger.error("prefill failed")

	        engine.logger.debug("requesting remote changes for %s " %self.sink.get_name())
		t = OBJ_TYPE_TO_ITEM_TYPE[self.sink.get_name()]
        	changesets = engine.engine.GetRemoteChanges([t])
		time.sleep(3)
        	engine.logger.debug("got %d changesets", len(changesets))

		acks = {}	
			
		if len(changesets) > 0:
			
			fmtenv=info.get_format_env()
			fmt = fmtenv.find_objformat(self.format)
			
			item_type = OBJ_TYPE_TO_ITEM_TYPE[self.sink.get_name()]
			changes = changesets[item_type]
		
        		engine.logger.debug("processing changes for item type %d" % item_type)

                	acks[item_type] = []

                	for change in changes:
                    		guid, chg_type, data = change

                    		change = opensync.Change()
                    		change.set_uid(array.array('B',guid).tostring())
                    		change.set_changetype(chg_type)

                       		if chg_type != opensync.CHANGE_TYPE_DELETED:
                       			bytes = '<?xml version="1.0" encoding="utf-8"?>\n'
                       			bytes += array.array('B',data).tostring()
					osdata = opensync.Data(bytes,fmt)
					osdata.set_objtype(self.sink.get_name())
                       			change.set_data(osdata)
				
				engine.logger.debug("reporting change")
				ctx.report_change(change)
				engine.logger.debug("change reported")
				
                    		acks[item_type].append(array.array('B',guid).tostring())
				
            	else:
                		engine.logger.debug("no changes for item type %d" % item_type)

		if acks!={}:
            		engine.logger.debug("acknowledging remote changes for item type %d", item_type)
            		engine.engine.AcknowledgeRemoteChanges(acks)
	    		engine.logger.debug("acknowledgement complete")
			
		engine.logger.debug("exitting get_changeinfo")
	
	#
	# commit
	#
	# Again, bit like the commit in the old plugin, but operates on a per-item basis
	#
	
	def commit(self, info, ctx, chg):
		engine = intermediaries[0]
		
		engine.logger.debug("commit called for item %s with change type %d", chg.uid, chg.changetype)

        	if chg.objtype in OBJ_TYPE_TO_ITEM_TYPE:
                
            		item_type = OBJ_TYPE_TO_ITEM_TYPE[chg.objtype]

            		data = ""
            		if chg.changetype != opensync.CHANGE_TYPE_DELETED:
                		data = chg.get_data().get_data()

            		engine.engine.AddLocalChanges( { item_type : ((chg.get_uid(),chg.get_changetype(),data),),} )

        	else:
            		raise Exception("SynCE: object type %s not yet handled" % chg.objtype)

	#
	# committed_all
	#
	# Called by OpenSync when all changes are committed. We use this via the intermediary
	# to trigger the second device sync once all items have been exchanged.
	#
	
	def committed_all(self, info, ctx):
		engine = intermediaries[0]
		engine.logger.info("All items committed")
		engine.TransmitChanges()
	
#	def read(self, info, ctx, chg):
#		print "read called!"
#		print "OpenSync wants me to read the data for UID", chg.uid

#	def write(self, info, ctx, chg):
#		print "write called!"
#		print "Opensync wants me to write data for UID", chg.uid
	
	#
	# disconnect
	#
	# We use the intermediary to ensure disconnection once _all_ items
	# have been synced (sunk?) :-)
	
	def disconnect(self, info, ctx):
		engine = intermediaries[0]
		engine.Disconnect()

	#
	# sync_done
	#
	# Called when all syncing is done, but before we disconnect. We may put
	# the TransmitChanges call here if necessary.

	def sync_done(self, info, ctx):
		print "sync_done called!"


# 
# initialize
#
# Called to initialize the plugin. Create and maintain the global instance
# of the intermediary, then tell OpenSync about the objtypes we support.

def initialize(info):
	logging.basicConfig(level=logging.DEBUG, stream=sys.stdout)
	intermediaries.append(EngineIntermediary())
	info.add_objtype(ItemSink(SYNC_ITEM_CONTACTS).sink)
	info.add_objtype(ItemSink(SYNC_ITEM_CALENDAR).sink)
	info.add_objtype(ItemSink(SYNC_ITEM_TASKS).sink)

#
# discover
#
# New in 0.30+. Used to enable OpenSync to actually find out what we can and
# can't sync. We interrogate the device here so we actually match what the device
# partnership is capable of.

def discover(info):
	intermediaries[0].Connect()
	SyncTypes = intermediaries[0].engine.GetSynchronizedItemTypes()
	for sink in info.objtypes:
		for wmtype in SyncTypes:
			if OBJ_TYPE_TO_ITEM_TYPE[sink.get_name()] == wmtype:
				sink.available = True
	info.version = opensync.Version()
	info.version.plugin = "synce-plugin"
	intermediaries[0].Disconnect()

#
# get_sync_info
#
# Provide information about the plugin to Opensync

def get_sync_info(plugin):
	plugin.name = "synce-plugin"
	plugin.longname = "Plugin to synchronize with Windows Mobile 5/6 devices"
	plugin.description = "Developed by Dr J A Gow 2007 for Opensync version >= 0.30"
	plugin.config_type = opensync.PLUGIN_NO_CONFIGURATION
