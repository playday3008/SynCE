# -*- coding: utf-8 -*-
############################################################################
# KERNEL.py
#
# Main object for sync-engine containing the core API. All sync-engine
# entries arrive here somewhere. One instance of this object exists for
# the life of the session
# 
# Adapted by Dr J A Gow 12/2007 from the original sync-engine
#
# Original sync-engine copyright (C) 2006  Ole André Vadla Ravnås
# <oleavr@gmail.com>
#
# This program is free software; you can redistribute it and#or modify
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
# along with this program; if not, write to the
# Free Software Foundation, Inc.
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
############################################################################

import dbus.service
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
import libxml2
import gobject
import errors

from mutex import mutex
from constants import *
from SyncEngine import *
from airsync import AirsyncThread
from synchandler import SyncHandler
from config import Config

#
#
#
# SyncEngine
#
# This object provides the API to sync-engine. It is accessible via d-bus
# but it should be noted that it will disappear when the device is
# disconnected.
#

class SyncEngine(dbus.service.Object):

	#
	#
	# Initialization

	def __init__(self,configObj,mainloop):
		
		dbus.service.Object.__init__(self, dbus.service.BusName(DBUS_SYNCENGINE_BUSNAME, bus=dbus.SessionBus()), DBUS_SYNCENGINE_OBJPATH)
		
		self.mainloop = mainloop
		
		self.logger = logging.getLogger("engine.syncengine.kernel")
		self.config = configObj

		self.PshipManager = pshipmgr.PartnershipManager(self)
	
		self.isConnected = False
	
		self.isOdccmRunning = False
		
		self.synchandler = None

		self.syncing = mutex()

		self.rapi_session = None	
		self.rra = None
		self.RRASession = rrasyncmanager.RRASyncManager(self)
		
		self.dtptsession = None
		self.airsync = None
		self.sync_begin_handler_id = None
		self.autosync_triggered = False


		self.odccm_manager = dbus.Interface(dbus.SystemBus().get_object(DBUS_DBUS_BUSNAME, DBUS_DBUS_OBJPATH), DBUS_DBUS_IFACE)
		self.odccm_manager.connect_to_signal("NameOwnerChanged", self._CBODCCMStatusChanged)
	
		self.device = None
		self.deviceName = ""
		self.devicePath = ""

		# Attempt to connect to a running odccm. If a running odccm is not available, we can wait for
		# it to become available.

		try:
			self.device_manager = dbus.Interface(dbus.SystemBus().get_object(DBUS_ODCCM_BUSNAME, DBUS_ODCCM_OBJPATH), DBUS_ODCCM_IFACE)
			self.device_manager.connect_to_signal("DeviceConnected", self._CBDeviceConnected)
			self.device_manager.connect_to_signal("DeviceDisconnected", self._CBDeviceDisconnected)
			self.isOdccmRunning = True

			obj_paths = self.device_manager.GetConnectedDevices()
			if len(obj_paths) > 0:
				self.logger.info("__init__: connected device found")
				self._CBDeviceConnected(obj_paths[0])
		except:
			self.isOdccmRunning = False

	#
	# _CBODCCMStatusChanged
	#
	# INTERNAL
	#
	# Called upon a change of status in ODCCM. This will happen if ODCCM goes on/offline,
	# and also if a device is connected.
	#

	def _CBODCCMStatusChanged(self, obj_path, param2, param3):


		if obj_path == "org.synce.odccm":

			# If this parameter is empty, the odccm just came online 

			if param2 == "":
				self.isOdccmRunning = True
				self.logger.info("_CBODCCMStatusChanged: odccm came online")

			# If this parameter is empty, the odccm just went offline

			if param3 == "":

				self.isOdccmRunning = False
				self.logger.info("_CBODCCMStatusChanged: odccm went offline")


			if self.isOdccmRunning:
				
				self.device = None
				self.deviceName = ""
				self.devicePath = ""

				try:
					self.device_manager = dbus.Interface(dbus.SystemBus().get_object(DBUS_ODCCM_BUSNAME, DBUS_ODCCM_OBJPATH), DBUS_ODCCM_IFACE)
					self.device_manager.connect_to_signal("DeviceConnected", self._CBDeviceConnected)
					self.device_manager.connect_to_signal("DeviceDisconnected", self._CBDeviceDisconnected)
				
					self.isOdccmRunning = True

					obj_paths = self.device_manager.GetConnectedDevices()
					if len(obj_paths) > 0:
						self.logger.info("_CBODCCMStatusChanged: connected device found")
						self._CBDeviceConnected(obj_paths[0])
						
				except:
					self.isOdccmRunning = False

	#
	# _CBDeviceConnected
	#
	# INTERNAL
	#
	# Callback triggered when a device is connected.
	#

	def _CBDeviceConnected(self, obj_path):
	 
		self.logger.info("_CBDeviceConnected: device connected at path %s", obj_path)

		if self.isConnected == False:
		
			# update config from file
	
			self.config.UpdateConfig()

			deviceObject = dbus.SystemBus().get_object("org.synce.odccm",obj_path)
			self.device = dbus.Interface(deviceObject,"org.synce.odccm.Device")
       			self.device.connect_to_signal("PasswordFlagsChanged", self._CBDeviceAuthStateChanged)
			self.deviceName = self.device.GetName()
			self.logger.info(" device %s connected" % self.deviceName)
			self.devicePath = obj_path
       			if self._ProcessAuth():
				self.OnConnect()
		else:
			if obj_path == self.devicePath:
				self.logger.info("_CBDeviceConnected: device already connected")
			else:
				self.logger.info("_CBDeviceConnected: other device already connected - ignoring new device")

	#
	# _CBDeviceDisconnected
	#
	# INTERNAL
	#
	# Callback triggered when a device is disconnected
	#

	def _CBDeviceDisconnected(self, obj_path):

		self.logger.info("_CBDeviceDisconnected: device disconnected from path %s", obj_path)
		if self.devicePath == obj_path:
			self.device=None
			self.deviceName = ""
			self.OnDisconnect()
		else:
			self.logger.info("_CBDeviceDisconnected: ignoring non-live device detach")

	#
	# _CheckDeviceConnected
	#
	# INTERNAL
	#
	# Function to check if a device is connected.
	#

	def _CheckDeviceConnected(self):
			
		if not self.isConnected:
			raise errors.Disconnected

	#
	# _CBDeviceAuthStateChanged
	#
	# INTERNAL
	#
	# Callback triggered when a device authorization state is changed
	#

	def _CBDeviceAuthStateChanged(self,added,removed):
			
		self.logger.info("_CBDeviceAuthStateChanged: device authorization state changed: reauthorizing")
		self._ProcessAuth()

	#
	# _CheckAndGetValidPartnership
	#
	# INTERNAL
	#
	# Utility function to retrieve the current partnership. Will throw if 
	# the system is currently unbound.
	#

	def _CheckAndGetValidPartnership(self):

		self._CheckDeviceConnected()
		pship = self.PshipManager.GetCurrentPartnership()
		if pship is None:
			raise errors.NoBoundPartnership
		return pship

	#
	# _ProcessAuth
	#
	# INTERNAL
	#
	# Process authorization on either callback or initial connection

	def _ProcessAuth(self):
	
		self.logger.info("ProcessAuth : processing authorization for device '%s'" % self.deviceName) 
		rc=True
		if auth.IsAuthRequired(self.device):
		
			# if we suddenly need auth, first shut down all threads if they
			# are running
		
			if self.partnerships != None:
				self.OnDisconnect()
			
			if auth.Authorize(self.devicePath,self.device,self.config.config_Global):
				self.logger.info("Authorization successful - reconnecting to device")
			else:
				self.logger.info("Failed to authorize - disconnect and reconnect device to try again")
				rc = False
		else:
			self.logger.info("ProcessAuth: authorization not required for device '%s'" % self.deviceName)

		return rc
		
	#
	# _ResetCurrentState
	#
	# INTERNAL
	#
	# Called to reset any internal state objects to defaults, usually
	# called on disconnect. Just so we can put them all in one place

	def _ResetCurrentState(self):
			
		autosync_triggered=False

	# 
	# OnConnect
	#
	# Called when device is firmly established. Sets up the RAPI connection
	# and then starts the sync handler sessions
	#

	def OnConnect(self):
	
		# ensure current state is set to defaults
	
		self._ResetCurrentState()
		self.isConnected = True

		# and start the sessions

		self.logger.debug("OnConnect: setting up RAPI session")
		self.rapi_session = rapicontext.RapiContext(pyrapi2.SYNCE_LOG_LEVEL_DEFAULT)

		self.logger.debug("OnConnect: Attempting to bind partnerships")
		self.PshipManager.AttemptToBind()
	
		# don't start any sessions if we don't have a valid partnership.
	
		try:
			self._CheckAndGetValidPartnership()
			self.StartSessions()
				
		except Exception,e:
			self.logger.debug("OnConnect: No valid partnership bindings are available, please create one (%s)" % str(e))
			pass

	#
	# OnDisconnect
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
		
		# now, if the config tells us to run once, we need to 
		# trigger an exit here.
		
		if self.config.runonce == True:
			self.logger.debug("OnDisconnect: RunOnce specified on command line, requesting sync-engine shutdown")
			gobject.idle_add(self.mainloop.quit)

	#
	# StartSessions
	#
	# Performs the mechanics of actually starting the sync handler sessions
	#
	
	def StartSessions(self):

		# We know we have a valid partnership if we get here, so run the config
		# without looking for exceptions
	
		pship = self.PshipManager.GetCurrentPartnership()
	
		# check if DTPT is enabled for this partnership - if so, start it
	
		mh = pship.QueryConfig("/syncpartner-config/DTPT/Enabled[position()=1]","0")
		gh = self.config.config_Global.cfg["EnableDTPT"]
	
		#### we MUST change this to bind to the device address only!
	
		if mh == "1" and gh == 1:
			self.dtptsession = dtptserver.DTPTServer("0.0.0.0")
			self.dtptsession.start()
		else:
			self.dtptsession = None

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
	# Called to trigger a device-triggered sync autosync, either a manual sync from the device,
	# or from the timer
	#

	def _CBStartDeviceTriggeredSync(self, res):

		pship=self._CheckAndGetValidPartnership()
	
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


	##############################
	## EXPORTED DBUS API
	##

	#
	# Synchronize
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Trigger a synchronization run from the host
	#

	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='')
	def Synchronize(self):
 
		self.logger.info("Synchronize: manual sync triggered")

		pship = self._CheckAndGetValidPartnership()
	
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

	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='as', out_signature='u')
	def PrefillRemote(self,types):

		pship = self._CheckAndGetValidPartnership()
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
	# GetItemTypes
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Get a list of synchronizable item types. Returns a dictionary mapping item
	# identifiers to names.

	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='a{us}')
	def GetItemTypes(self):

		types = {}
		for id, val in SYNC_ITEMS.items():
			name, readonly = val
			types[id] = name
		return types
	
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

	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='a(ussssau)')
	def GetPartnerships(self):

		self._CheckDeviceConnected()

		ret = []
		for p in self.PshipManager.GetList():
			ret.append((p.info.id, p.info.guid, p.info.name, p.info.hostname, p.info.devicename, p.devicesyncitems))
		return ret

	#
	# GetPartnershipBindings
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Get a list of partnership bindings on the host. Retuen an array of structs containing:
	# 
	#     Binding ID
	#     Binding GUID
	#     Binding name
	#     Binding hostname
	#     Binding devicename
	#     List of sync items enabled for this device


	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='a(ussssau)')
	def GetPartnershipBindings(self):
		
		#
		# We don't need a connected device to do this
		#
	
		bindstructs = []
		bindings = self.PshipManager.GetHostBindings()
		for binding in bindings:
			bindstructs.append((binding.id, binding.guid, binding.name, binding.hostname, binding.devicename, binding.lastSyncItems))
		return bindstructs
	
	#
	# QueryBindingConfig
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Return an XML string containing the configuration for the selected binding. Bindings
	# are indexed by ID and GUID
	#

	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='us',out_signature='s')
	def QueryBindingConfig(self,id,guid):
			
		n=libxml2.parseDoc(self.PshipManager.QueryBindingConfiguration(id,guid))
			
		# 'prettify' the stored XML somewhat...
		return n.serialize("utf-8",1)

	#
	# SetBindingConfig
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Pass the function an XML string containing the binding configuration, and the
	# ID and GUID of the partnership you wish to configure.
	#
	
	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='uss', out_signature='')
	def SetBindingConfig(self,id,guid,config):

		self.PshipManager.SetBindingConfiguration(id,guid,config)

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

		self._CheckDeviceConnected()	
	
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

		pship = self._CheckAndGetValidPartnership()
		return pship.devicesyncitems

	#
	# DeletePartnership
	#
	# EXPORTED: DBUS SERVICED API
	#
	# Delete a partnership on the device, specified by ID and GUID. If we hit our
	# current partnership, stop all sessions before doing it.

	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='us', out_signature='')
	def DeletePartnership(self,id,guid):
	
		if not self.syncing.testandset():
			raise errors.SyncRunning
	
		# We need to do this in all cases
	
		try:
			cpship = self._CheckAndGetValidPartnership()
			if cpship.info.id == id:
				self.logger.debug("DeletePartnership: calling sync pause")
				self.rapi_session.sync_pause()
				self.sessions_stop()
				self.sessions_wait_for_stop()
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
		
	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='a{ua(ayuay)}', out_signature='')
	def AddLocalChanges(self, changesets):

		pship = self._CheckAndGetValidPartnership()

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

	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='au', out_signature='a{ua(ayuay)}')
	def GetRemoteChanges(self, item_types):

		pship = self._CheckAndGetValidPartnership()

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
	
	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='a{uaay}', out_signature='')
	def AcknowledgeRemoteChanges(self, changes):

		pship = self._CheckAndGetValidPartnership()

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

	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='')
	def FlushItemDB(self):

		pship = self._CheckAndGetValidPartnership()

		if not self.syncing.testandset():
			self.logger.debug("FlushItemDB: impossible while syncing")
			return

		if self.config.config_Global.cfg["FlushIDB"]==1:
			self.logger.info("FlushItemDB: flushing current partnership DB")
			pship.FlushItemDB()

		self.syncing.unlock()

	##############################
	## OUTGOING DBUS SIGNALS
	##

	#
	# PrefillComplete
	#
	# EXPORTED: DBUS SERVICE SIGNAL
	#
	# Message will be sent out on d-bus when prefill is complete. Processing prefill
	# asynchronously can prevent apparent long hangs in user tools

	@dbus.service.signal(DBUS_SYNCENGINE_IFACE, signature='')
	def PrefillComplete(self):
		self.logger.info("Prefill complete: Emitting PrefillComplete signal")

	#
	# Synchronized
	#
	# EXPORTED: DBUS SERVICE SIGNAL
	#
	# Message will be sent out on d-bus when remote synchronization with the itemDB is 
	# complete. As with prefill, running asynchronously prevents a user tool from appearing
	# to hang and also reduces the likelihood of d-bus timeouts
	
	@dbus.service.signal(DBUS_SYNCENGINE_IFACE, signature='')
	def Synchronized(self):
		self.logger.info("Synchronized: Emitting Synchronized signal")


	#
	# PartnershipsChanged
	#
	# EXPORTED: DBUS SERVICE SIGNAL
	#
	# Message will be sent out on d-bus when a partnership is deleted / created. This will
	# make sure all clients are updated with these changes
	
	@dbus.service.signal(DBUS_SYNCENGINE_IFACE, signature='')
	def PartnershipsChanged(self):
		self.logger.info("Synchronized: Emitting PartnershipsChanged signal")



	@dbus.service.signal('org.synce.SyncEngine.Status', signature='u')
	def StatusSetMaxProgressValue(self, maxProgressValue):
		self.logger.info("Status: Emitting StatusSetMaxProgressValue signal")

	
	@dbus.service.signal('org.synce.SyncEngine.Status', signature='u')
	def StatusSetProgressValue(self, progressValue):
		self.logger.info("Status: Emitting StatusSetProgressValue signal")


	@dbus.service.signal('org.synce.SyncEngine.Status', signature='s')
	def StatusSetStatusString(self, statusString):
		self.logger.info("Status: Emitting StatusSetStatusString signal")



	@dbus.service.signal('org.synce.SyncEngine.Status', signature='')
	def StatusSyncStart(self):
		self.logger.info("Status: Emitting StatusSyncStart signal")
	
	@dbus.service.signal('org.synce.SyncEngine.Status', signature='')
	def StatusSyncEnd(self):
		self.logger.info("Status: Emitting StatusSyncEnd signal")
	
	
	@dbus.service.signal('org.synce.SyncEngine.Status', signature='s')
	def StatusSyncStartPartner(self,partner):
		self.logger.info("Status: Emitting StatusSyncStartPartner signal")
	
	@dbus.service.signal('org.synce.SyncEngine.Status', signature='s')
	def StatusSyncEndPartner(self,partner):
		self.logger.info("Status: Emitting StatusSyncEndPartner signal")
	
	
	@dbus.service.signal('org.synce.SyncEngine.Status', signature='ss')
	def StatusSyncStartDatatype(self,partner,datatype):
		self.logger.info("Status: Emitting StatusSyncStartDatatype signal")
	
	@dbus.service.signal('org.synce.SyncEngine.Status', signature='ss')
	def StatusSyncEndDatatype(self,partner,datatype):
		self.logger.info("Status: Emitting StatusSyncEndDatatype signal")
