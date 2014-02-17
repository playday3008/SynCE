
import dbus
import logging
import rrasyncmanager
import os
import array
import prefill
import auth
import rapicontext
import pyrapi2
import dtptserver
import pshipmgr
import errors
import re
import gobject

from mutex import mutex
from constants import *
from airsync import AirsyncThread
from synchandler import SyncHandler
from config import Config

#
#
#
# Device
#
# This object represents a connected device
#

class Device(gobject.GObject):

	__gsignals__ = {
		"connected": (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),
		"disconnected": (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),
		"prefill-complete"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),
		"synchronized"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),
		"partnerships-changed"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),

		"status-sync-start": (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),
		"status-sync-end"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),
		"status-sync-start-partner": (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING,) ),
		"status-sync-end-partner"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING,) ),
		"status-sync-start-datatype": (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING, gobject.TYPE_STRING,) ),
		"status-sync-end-datatype"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING, gobject.TYPE_STRING,) ),
		"status-set-progress-value"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_UINT,) ),
		"status-set-max-progress-value"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_UINT,) ),
		"status-set-status-string"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING,) ),
		}


	#
	#
	# Initialization

	def __init__(self,config,objpath):

		self.__gobject_init__()

		# reference to the app's Config object
		self.config = config
		# odccm or udev object path to the device
		self.objpath = objpath

		self.logger = logging.getLogger("engine.syncengine.device")

		# if we are ready to communicate
		self.isConnected = False

		# device partnership information
		self.PshipManager = pshipmgr.PartnershipManager(self)

		self.synchandler = None

		self.syncing = mutex()

		# RAPI connection to the device
		self.rapi_session = None	

#		self.rra = None
		# RRA connection to the device
		self.RRASession = rrasyncmanager.RRASyncManager(self)

		# DTPT (desktop passthrough) for the device
		self.dtptsession = None

		self.airsync = None
		self.sync_begin_handler_id = None
		self.autosync_triggered = False

#		self.device = None
		self.deviceName = ""
		self.devicePath = ""
		self.iface_addr = ""
		self.partnerships = None

		if "/org/synce/odccm" in objpath:
			device_obj = dbus.SystemBus().get_object("org.synce.odccm",objpath)
			self.dev_iface = dbus.Interface(device_obj,"org.synce.odccm.Device")
			self.iface_addr = "0.0.0.0"
		else:
			device_obj = dbus.SystemBus().get_object("org.synce.dccm", objpath)
			self.dev_iface = dbus.Interface(device_obj,"org.synce.dccm.Device")
			self.iface_addr = self.dev_iface.GetIfaceAddress()

		self.dev_iface.connect_to_signal("PasswordFlagsChanged", self._CBDeviceAuthStateChanged)
		self.deviceName = self.dev_iface.GetName()
		self.name = self.dev_iface.GetName()
		self.logger.info(" device %s connected" % self.name)
		self.devicePath = objpath

		os_major, os_minor = self.dev_iface.GetOsVersion()
		if os_major < 5:
			self.pim_type = PIM_TYPE_RRA
		else:
			self.pim_type = PIM_TYPE_AIRSYNC

	#
	# _CBDeviceAuthStateChanged
	#
	# INTERNAL
	#
	# Callback triggered when a device authorization state is changed
	#

	def _CBDeviceAuthStateChanged(self,added,removed):
			
		self.logger.info("_CBDeviceAuthStateChanged: device authorization state changed: reauthorizing")
		if not self.isConnected:
			if self.ProcessAuth():
				self.OnConnect()


	#
	# ProcessAuth
	#
	# EXTERNAL
	#
	# Process authorization on either callback or initial connection

	def ProcessAuth(self):

		self.logger.info("ProcessAuth : processing authorization for device '%s'" % self.name) 
		rc=True
		if auth.IsAuthRequired(self.dev_iface):
		
			# if we suddenly need auth, first shut down all threads if they
			# are running

			if self.PshipManager.GetCurrentPartnership() != None:
				self.OnDisconnect()

			result = auth.Authorize(self.devicePath,self.dev_iface,self.config.config_Global)
			if result == 0:
				self.logger.info("Authorization pending")
				rc = False
			elif result == 1:
				self.logger.info("Authorization successful - reconnecting to device")
				self.isConnected = True
			elif result == 2:
				self.logger.info("Authorization pending - waiting for password on device")
				rc = False
			else:
				self.logger.info("Failed to authorize - disconnect and reconnect device to try again")
				rc = False
		else:
			self.logger.info("ProcessAuth: authorization not required for device '%s'" % self.name)
			self.isConnected = True

		return rc

	# 
	# OnConnect
	#
	# EXTERNAL
	#
	# Called when device is firmly established. Sets up the RAPI connection
	# and then starts the sync handler sessions
	#

	def OnConnect(self):
	
		# ensure current state is set to defaults

		self._ResetCurrentState()

		# and start the sessions

		self.logger.debug("OnConnect: setting up RAPI session")
		self.rapi_session = rapicontext.RapiContext(self.name, pyrapi2.SYNCE_LOG_LEVEL_DEFAULT)

		self.logger.debug("OnConnect: Attempting to bind partnerships")
		self.PshipManager.AttemptToBind()

		# don't start any sessions if we don't have a valid partnership.

		try:
			self.CheckAndGetValidPartnership()
			self.StartSessions()

		except Exception,e:
			self.logger.debug("OnConnect: No valid partnership bindings are available, please create one (%s)" % str(e))
			pass

		self.isConnected = True


	#
	# OnDisconnect
	#
	# EXTERNAL
	#
	# Called when the device disconnects from the bus. Ensures all sessions
	# are cleanly shut down.

	def OnDisconnect(self):

		self.StopSessions()
		self.WaitForStoppingSessions()

		self.logger.debug("OnDisconnect: closing RAPI session")
		self.rapi_session = None

		self.logger.debug("OnDisconnect: clearing partnerships")
		self.PshipManager.ClearDevicePartnerships()
	
		self.isConnected = False
		
	#
	# StartSessions
	#
	# EXTERNAL
	#
	# Performs the mechanics of actually starting the sync handler sessions
	#
	
	def StartSessions(self):

		# We know we have a valid partnership if we get here, so run the config
		# without looking for exceptions
	
		pship = self.PshipManager.GetCurrentPartnership()
	
		# check if DTPT is enabled for this partnership - if so, start it
		# ## assuming here that pre WM5 doesn't support DTPT, need to check this
		if self.pim_type != PIM_TYPE_RRA:
			mh = pship.QueryConfig("/syncpartner-config/DTPT/Enabled[position()=1]","0")
			gh = self.config.config_Global.cfg["EnableDTPT"]
	
			if mh == "1" and gh == 1:
				self.logger.debug("StartSessions: DTPT starting")
				self.dtptsession = dtptserver.DTPTServer(self.iface_addr)
				self.dtptsession.start()
			else:
				self.dtptsession = None

		if self.pim_type == PIM_TYPE_AIRSYNC:
			self.logger.debug("StartSessions: starting AirSync handler")
			self.airsync = AirsyncThread(self)
			self.sync_begin_handler_id = self.airsync.connect("sync-begin", self._CBStartDeviceTriggeredSync)
			self.airsync.start()

			self.logger.debug("StartSessions: calling RAPI start_replication")
			self.rapi_session.start_replication()

			# The device will never trigger an autosync, or attempt to sync, until
			# sync_resume is called.

			self.logger.debug("StartSessions: calling RAPI sync_resume")
			self.rapi_session.sync_resume()

		self.logger.debug("StartSessions: starting RRA session")
		self.RRASession.StartRRAEventHandler()

	#
	# StopSessions
	#
	# EXTERNAL
	#
	# Triggers all sync session threads and servers to stop. It does not
	# wait for a stop.

	def StopSessions(self):

		self.logger.debug("StopSessions: stopping RRA server")
		self.RRASession.StopRRAEventHandler()
		
		if self.dtptsession != None:
			self.logger.debug("StopSessions: stopping DTPT server")
			self.dtptsession.shutdown()
	
		if self.synchandler != None:
			self.logger.debug("StopSessions: stopping sync handler thread")
			self.synchandler.stop()

		if self.airsync != None:
			self.logger.debug("StopSessions: stopping Airsync server")
			self.airsync.stop()

	#
	# WaitForStoppingSessions
	#
	# EXTERNAL
	#
	# Once StopSessions has been called, this function can be called to wait
	# until all threads and servers have actually stopped
	#
		
	def WaitForStoppingSessions(self):

		if self.dtptsession != None:
        		self.logger.debug("WaitForStoppingSessions: waiting for DTPT server thread")
			self.dtptsession.join()
			self.dtptsession = None

		if self.synchandler != None:
			self.logger.debug("WaitForStoppingSessions: waiting for sync handler thread")
			self.synchandler.join()
			self.synchandler = None

		if self.airsync != None:
			self.logger.debug("WaitForStoppingSessions: waiting for Airsync server thread")
			self.airsync.join()
			self.sync_begin_handler_id = None
			self.airsync = None

		self.logger.debug("sessions_wait_for_stop: shutting down RRA server")
		self.RRASession.StopRRAEventHandler()

	# _CBStartDeviceTriggeredSync
	#
	# INTERNAL
	#
	# Called to trigger a device-triggered sync autosync, either a manual sync from the device,
	# or from the timer
	#

	def _CBStartDeviceTriggeredSync(self, res):

		pship=self.CheckAndGetValidPartnership()
	
		if not self.syncing.testandset():
			raise errors.SyncRunning
		
		self.logger.info("_CBStartDeviceTriggeredSync: monitoring auto sync with partnership %s", pship)
	
		if not pship.itemDBLoaded:
			pship.LoadItemDB()
		
		self.logger.info("_CBStartDeviceTriggeredSync: itemDB loaded")

		self.synchandler = SyncHandler(self, True)
		self.synchandler.start()
	
		if self.config.config_AutoSync.cfg["Disable"] == 0:
		
			cmd_list = self.config.config_AutoSync.cfg["AutoSyncCommand"]
				
			if len(cmd_list) > 0:
				self.logger.info("_CBStartDeviceTriggeredSync: command %s" % cmd_list[0])
				try:
					self.autosync_triggered = True	
					pid = os.spawnvp(os.P_NOWAIT,cmd_list[0],cmd_list)
					self.logger.info("_CBStartDeviceTriggeredSync: process spawned with PID %d" % pid)
				except:
					self.autosync_triggered = False
					self.logger.debug("_CBStartDeviceTriggeredSync : failed to spawn process : cmd=%s" % cmd_list[0])
		else:
			self.logger.debug("_CBStartDeviceTriggeredSync : device triggered sync disabled in config")

	#
	# CheckAndGetValidPartnership
	#
	# EXTERNAL
	#
	# Utility function to retrieve the current partnership. Will throw if 
	# the system is currently unbound.
	#

	def CheckAndGetValidPartnership(self):

		pship = self.PshipManager.GetCurrentPartnership()
		if pship is None:
			raise errors.NoBoundPartnership
		return pship

	#
	# _ResetCurrentState
	#
	# INTERNAL
	#
	# Called to reset any internal state objects to defaults, usually
	# called on disconnect. Just so we can put them all in one place

	def _ResetCurrentState(self):
		
		self.autosync_triggered=False
		self.syncing.unlock()


	##############################
	## EXPORTED DBUS API - exported by Engine for the active_device, ideally 
	## we would have this exported by each Device
	##

	#
	# GetDeviceBindingState
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Query the binding state of the device, and if it is connected
	# or not. If it is connected, whether it is bound. This function
	# is safe to call at any time and should not throw.
	#

	#@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='u')
	def GetDeviceBindingState(self):
		
		rc=BSTATE_DEVNOTCONNECTED

		if self.isConnected:
			rc=BSTATE_DEVNOTBOUND
			if self.PshipManager.GetCurrentPartnership() is not None:
				rc=BSTATE_DEVBOUND
	
		return rc

	#
	# Synchronize
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Trigger a synchronization run from the host
	#

	#@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='')
	def Synchronize(self):
 
		self.logger.info("Synchronize: manual sync triggered")

		pship = self.CheckAndGetValidPartnership()
	
		if not self.syncing.testandset():
			self.logger.info("Synchronize: doing nothing because we're already syncing")
			return

		if not self.autosync_triggered:

			self.logger.info("Synchronize: starting manual sync with partnership %s", pship)
				
			if not pship.itemDBLoaded:
				pship.LoadItemDB()
				self.logger.info("Synchronize: itemDB loaded")

			self.synchandler = SyncHandler(self, False)
			self.synchandler.start()
		else:
			self.syncing.unlock()
			self.logger.debug("Synchronize: previous sync triggered externally, no need to repeat")
			self.autosync_triggered = False
			self.Synchronized()

		self.logger.info("Synchronize: leaving method")

	#
	# PrefillRemote
	#
	# EXPORTED: DBUS SERVICED API
	#
	# PrefillRemote should be called after a sync run, and it alters the list of
	# remote changes that will be reported to the synchronizer plugin to include everything	
	# in the db.

	#@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='as', out_signature='u')
	def PrefillRemote(self,types):

		pship = self.CheckAndGetValidPartnership()
		self.logger.info("PrefillRemote: slow sync prefill triggered")
		
		# Ensure the itemDB is loaded

		if not pship.itemDBLoaded:
			pship.LoadItemDB()

		rc = 0

		if not self.config.config_Global.cfg["SlowSyncDisable"]:
			if self.syncing.testandset():
				pfThread = prefill.PrefillThread(self,types)
				pfThread.start()
				rc=1
			else:
				self.logger.debug("PrefillRemote: impossible while syncing")
		else:
			self.logger.info("PrefillRemote: slow sync prefill disabled in config")

		return rc

	#
	# GetPartnerships
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Get a list of partnerships on the device. Returns an array of structs containing:
	#     Partnership ID
	#     Partnership GUID
	#     Partnership name
	#     Hostname
	#     Device name
	#     an array of sync item ids

	#@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='a(ussssuau)')
	def GetPartnerships(self):

		ret = []
		for p in self.PshipManager.GetList():
			ret.append((p.info.id, p.info.guid, p.info.name, p.info.hostname, p.info.devicename, p.storetype, p.devicesyncitems))
		return ret

	#
	# CreatePartnership
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Pass it a name, and an array of items to sync. The function will return the
	# ID of the newly created partnership. If no bound partnership exists, the
	# new partnership will be made current and the sync sessions started.
	
	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='sau', out_signature='u')
	def CreatePartnership(self, name, sync_items):

		# set a flag if we have a current partnership (already bound)
	
		start = True
		if self.PshipManager.GetCurrentPartnership() != None:
			start = False
	
		id=self.PshipManager.CreateNewPartnership(name, sync_items).info.id

		if start:
			self.StartSessions()
		
		self.PartnershipsChanged()
		return id
	
	#
	# GetSynchronizedItemTypes
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Returns a list of synchronizeable item types offered by the current partnership
	
	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='au')
	def GetSynchronizedItemTypes(self):

		pship = self.CheckAndGetValidPartnership()
		return pship.devicesyncitems

	#
	# DeletePartnership
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Delete a partnership on the device, specified by ID and GUID. If we hit our
	# current partnership, stop all sessions before doing it.

	#@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='us', out_signature='')
	def DeletePartnership(self,id,guid):
	
		if not self.syncing.testandset():
			raise errors.SyncRunning
	
		# We need to do this in all cases
	
		try:
			cpship = self.CheckAndGetValidPartnership()
			if cpship.info.id == id:
				self.logger.debug("DeletePartnership: calling sync pause")
				self.rapi_session.sync_pause()
				self.StopSessions()
				self.WaitForStoppingSessions()
		except Exception,e:
			# ignore if we have no valid partnership
			pass

		self.PshipManager.DeleteDevicePartnership(id,guid)
	
		self.syncing.unlock()
		self.PartnershipsChanged()
	
	#
	# AddLocalChanges
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Called during a sync run by the host synchronization system to pass information
	# relating to changes on the host side.
		
	#@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='a{ua(ayuay)}', out_signature='')
	def AddLocalChanges(self, changesets):

		pship = self.CheckAndGetValidPartnership()

		if not pship.itemDBLoaded:
			pship.LoadItemDB()

		AvailableItemDBs = pship.deviceitemdbs

		for item_type, changes in changesets.items():

			if not AvailableItemDBs.has_key(item_type):
				self.logger.info("AddLocalChanges: skipping changes for item of type %d", item_type)
				continue

			itemDB = AvailableItemDBs[item_type]
			self.logger.debug("AddLocalChanges: adding changes for item of type %d", itemDB.type)

			for change in changes:

				itemID, chg_type, data = change
				itemID = array.array('B',itemID).tostring()
				data = array.array('B',data).tostring()
				self.logger.debug("AddLocalChanges: adding change GUID = %s, ChangeType = %d, Data = %s",
				itemID, chg_type, data)
				
				itemDB.AddLocalChanges([(itemID, chg_type, data)])

		self.logger.debug("AddLocalChanges: added (or ignored) %d changesets", len(changesets))
	
		# we have updated the IDB, so must save it.
	
		pship.SaveItemDB()

	#
	# GetRemoteChanges
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Used by the host synchronization software to retrieve changes on the remote
	#

	#@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='au', out_signature='a{ua(ayuay)}')
	def GetRemoteChanges(self, item_types):

		pship = self.CheckAndGetValidPartnership()

		AvailableItemDBs = pship.deviceitemdbs

		if not pship.itemDBLoaded:
       			pship.LoadItemDB()

		changes = {}
	
		self.logger.debug("GetRemoteChanges: pship %s",str(pship))


		for type in item_types:
			
			changes[type] = []

			self.logger.debug("GetRemoteChanges: getting changes for items of type %d", type)

			for itemID, change in AvailableItemDBs[type].GetRemoteChanges().items():
			
				chgtype, chgdata = change

				self.logger.debug("GetRemoteChanges: got change GUID = %s, ChangeType = %d, Data = %s",
						   itemID, chgtype, chgdata)

				changes[type].append((itemID, chgtype, chgdata))
		
		self.logger.debug("return")
		return changes

	#
	# AcknowledgeRemoteChanges
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Used by the host synchronization software to acknowledge the processing
	# of remote changes, in order that they can be removed from the remote's
	# change stack
	
	#@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='a{uaay}', out_signature='')
	def AcknowledgeRemoteChanges(self, changes):

		pship = self.CheckAndGetValidPartnership()

		AvailableItemDBs = pship.deviceitemdbs

		for item_type, itemIDs in changes.items():

			itemDB = AvailableItemDBs[item_type]

			self.logger.debug("AcknowledgeRemoteChanges: acking changes for items of type %d", item_type)

			for itemID in itemIDs:
				itemID = array.array('B',itemID).tostring()
				self.logger.debug("AcknowledgeRemoteChanges: acking change for items GUID = %s", itemID)
				itemDB.AcknowledgeRemoteChange(itemID)

		self.logger.debug("AcknowledgeRemoteChanges: saving synchronization state and itemDB")

		pship.SaveItemDB()

	#
	# FlushItemDB
	#
	# Called at the end of a sync session to save and flush the itemDB from memory.
	# There will be no ill-effects if this function is not called other than to retain the
	# item database in memory for all time, instead of just when syncing.
	#

	#@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='')
	def FlushItemDB(self):

		pship = self.CheckAndGetValidPartnership()

		if not self.syncing.testandset():
			self.logger.debug("FlushItemDB: impossible while syncing")
			return

		if self.config.config_Global.cfg["FlushIDB"]==1:
			self.logger.info("FlushItemDB: flushing current partnership DB")
			pship.FlushItemDB()

		self.syncing.unlock()


	# emit signals
	#
	def PrefillComplete(self):
		self.emit("prefill-complete")

	def Synchronized(self):
		self.emit("synchronized")

	def PartnershipsChanged(self):
		self.emit("partnerships-changed")

	def StatusSyncStart(self):
		self.emit("status-sync-start")

	def StatusSyncEnd(self):
		self.emit("status-sync-end")

	def StatusSyncStartPartner(self, partner):
		self.emit("status-sync-start-partner", partner)

	def StatusSyncEndPartner(self, partner):
		self.emit("status-sync-end-partner", partner)

	def StatusSyncStartDatatype(self, partner, datatype):
		self.emit("status-sync-start-datatype", partner, datatype)

	def StatusSyncEndDatatype(self, partner, datatype):
		self.emit("status-sync-end-datatype", partner, datatype)

	def StatusSetProgressValue(self, progress_current):
		self.emit("status-set-progress-value", progress_current)

	def StatusSetMaxProgressValue(self, progress_max):
		self.emit("status-set-max-progress-value", progress_max)

	def StatusSetStatusString(self, status):
		self.emit("status-set-status-string", status)

gobject.type_register(Device)
