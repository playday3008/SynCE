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
from device import Device
from airsync import AirsyncThread
from synchandler import SyncHandler
from config import Config

#
#
#
# SyncEngine
#
# This object provides the API to sync-engine. It is accessible via d-bus
# but it should be noted that the dbus interface will disappear when the device is
# disconnected.
#

class SyncEngine(dbus.service.Object):

	#
	#
	# Initialization

	def __init__(self,configObj,mainloop):
		
		dbus.service.Object.__init__(self, dbus.service.BusName(DBUS_SYNCENGINE_BUSNAME, bus=dbus.SessionBus()), DBUS_SYNCENGINE_OBJPATH)
		
		self.mainloop = mainloop
		
		self.logger = logging.getLogger("engine.kernel")
		self.config = configObj

		self.isOdccmRunning = False
		self.isUdevRunning = False

		self.dbus_manager = dbus.Interface(dbus.SystemBus().get_object(DBUS_DBUS_BUSNAME, DBUS_DBUS_OBJPATH), DBUS_DBUS_IFACE)
		self.dbus_manager.connect_to_signal("NameOwnerChanged", self._CBDCCMStatusChanged)

		self.devices = []
		self.active_device = None

		self.connected_handler_id = None
		self.disconnected_handler_id = None
		self.prefill_complete_handler_id = None
		self.synchronized_handler_id = None
		self.partnerships_changed_handler_id = None

		self.status_sync_start_handler_id = None
		self.status_sync_end_handler_id = None
		self.status_sync_start_partner_handler_id = None
		self.status_sync_end_partner_handler_id = None
		self.status_sync_start_datatype_handler_id = None
		self.status_sync_end_datatype_handler_id = None
		self.status_set_progress_value_handler_id = None
		self.status_set_max_progress_value_handler_id = None
		self.status_set_status_string_handler_id = None

		self._ODCCMConnect()
		self._UdevConnect()



	#
	#
	# shutdown
	# 
	# We are to shut down, clean up
	#

	def shutdown(self):
		self._ODCCMDisconnect()
		self._UdevDisconnect()
		self.dbus_manager = None


	#
	# _ODCCMConnect
	#
	# INTERNAL
	#
	# Attempt to connect to a running odccm. If a running odccm is not available, we can wait for
	# it to become available.
	#

	def _ODCCMConnect(self):

		try:
			self.device_manager = dbus.Interface(dbus.SystemBus().get_object(DBUS_ODCCM_BUSNAME, DBUS_ODCCM_OBJPATH), DBUS_ODCCM_IFACE)
			self.device_manager.connect_to_signal("DeviceConnected", self._CBDeviceConnected)
			self.device_manager.connect_to_signal("DeviceDisconnected", self._CBDeviceDisconnected)
			self.isOdccmRunning = True

			obj_paths = self.device_manager.GetConnectedDevices()
			for obj_path in obj_paths:
				self.logger.info("__init__: connected device found: %s" % obj_path)
				self._CBDeviceConnected(obj_path)
		except:
			self.isOdccmRunning = False

	#
	# _ODCCMDisconnect
	#
	# INTERNAL
	#
	# Disconnect from odccm, either because odccm has disappeared or we are shutting down. 
	#

	def _ODCCMDisconnect(self):

		self.device_manager = None
		self.isOdccmRunning = False

		for device in self.devices:
			if "/org/synce/odccm" in device.devicePath:
				self.logger.info("_ODCCMDisconnect: disconnecting device: %s" % device.devicePath)
				self._CBDeviceDisconnected(device.devicePath)

	#
	# _UdevConnect
	#
	# INTERNAL
	#
	# Attempt to connect to a running Udev dccm. If a running dccm is not available, we can wait for
	# it to become available.
	#

	def _UdevConnect(self):

		try:
			self.udev_manager = dbus.Interface(dbus.SystemBus().get_object(DBUS_UDEV_BUSNAME, DBUS_UDEV_MANAGER_OBJPATH), DBUS_UDEV_MANAGER_IFACE)
			self.udev_manager.connect_to_signal("DeviceConnected", self._CBDeviceConnected)
			self.udev_manager.connect_to_signal("DeviceDisconnected", self._CBDeviceDisconnected)
			self.isUdevRunning = True

			obj_paths = self.udev_manager.GetConnectedDevices()
			for obj_path in obj_paths:
				self.logger.info("_UdevConnect: connected device found: %s", obj_path)
				self._CBDeviceConnected(obj_path)
		except Exception, e:
			self.logger.info("_UdevConnect: failed to connect to dccm: %s", e)
			self.isUdevRunning = False

	#
	# _UdevDisconnect
	#
	# INTERNAL
	#
	# Disconnect from udev, either because dccm has disappeared or we are shutting down. 
	#

	def _UdevDisconnect(self):

		self.udev_manager = None
		self.isUdevRunning = False

		for device in self.devices:
			self.logger.info("_UdevDisconnect: found device: %s" % device.devicePath)
			if "/org/synce/dccm" in device.devicePath:
				self.logger.info("_UdevDisconnect: disconnecting device: %s" % device.devicePath)
				self._CBDeviceDisconnected(device.devicePath)

	#
	# _CBDCCMStatusChanged
	#
	# INTERNAL
	#
	# Called upon a change of status in DCCM. This will happen if ODCCM or udev DCCM goes on/offline,
	# and also if a device is connected.
	#

	def _CBDCCMStatusChanged(self, obj_path, old_owner, new_owner):


		if obj_path == "org.synce.odccm":

			# If this parameter is empty, the odccm just came online 

			if old_owner == "":
				self.isOdccmRunning = True
				self.logger.info("_CBDCCMStatusChanged: odccm came online")
				self._ODCCMConnect()

			# If this parameter is empty, the odccm just went offline

			if new_owner == "":

				self.isOdccmRunning = False
				self.logger.info("_CBDCCMStatusChanged: odccm went offline")
				self._ODCCMDisconnect()


		if obj_path == "org.synce.dccm":

			# If this parameter is empty, the udev dccm just came online 

			if old_owner == "":
				self.isUdevRunning = True
				self.logger.info("_CBDCCMStatusChanged: udev dccm came online")
				self._UdevConnect()

			# If this parameter is empty, the odccm just went offline

			if new_owner == "":

				self.isUdevRunning = False
				self.logger.info("_CBDCCMStatusChanged: udev dccm went offline")
				self._UdevDisconnect()


	#
	# DeviceConnectSignals
	#
	# INTERNAL
	#
	# connect to the signals on a device object
	#

	def DeviceConnectSignals(self, device):
                self.prefill_complete_handler_id = device.connect("prefill-complete", self.PrefillCompleteCB)
                self.synchronized_handler_id = device.connect("synchronized", self.SynchronizedCB)
                self.partnerships_changed_handler_id = device.connect("partnerships-changed", self.PartnershipsChangedCB)
                self.status_sync_start_handler_id = device.connect("status-sync-start", self.StatusSyncStartCB)
                self.status_sync_end_handler_id = device.connect("status-sync-end", self.StatusSyncEndCB)
                self.status_sync_start_partner_handler_id = device.connect("status-sync-start-partner", self.StatusSyncStartPartnerCB)
                self.status_sync_end_partner_handler_id = device.connect("status-sync-end-partner", self.StatusSyncEndPartnerCB)
                self.status_sync_start_datatype_handler_id = device.connect("status-sync-start-datatype", self.StatusSyncStartDatatypeCB)
                self.status_sync_end_datatype_handler_id = device.connect("status-sync-end-datatype", self.StatusSyncEndDatatypeCB)
                self.status_set_progress_value_handler_id = device.connect("status-set-progress-value", self.StatusSetProgressValueCB)
                self.status_set_max_progress_value_handler_id = device.connect("status-set-max-progress-value", self.StatusSetMaxProgressValueCB)
                self.status_set_status_string_handler_id = device.connect("status-set-status-string", self.StatusSetStatusStringCB)

	#
	# DeviceConnectSignals
	#
	# INTERNAL
	#
	# connect to the signals on a device object
	#

	def DeviceDisconnectSignals(self, device):
                device.disconnect(self.prefill_complete_handler_id)
                device.disconnect(self.synchronized_handler_id)
                device.disconnect(self.partnerships_changed_handler_id)
                device.disconnect(self.status_sync_start_handler_id)
                device.disconnect(self.status_sync_end_handler_id)
                device.disconnect(self.status_sync_start_partner_handler_id)
                device.disconnect(self.status_sync_end_partner_handler_id)
                device.disconnect(self.status_sync_start_datatype_handler_id)
                device.disconnect(self.status_sync_end_datatype_handler_id)
                device.disconnect(self.status_set_progress_value_handler_id)
                device.disconnect(self.status_set_max_progress_value_handler_id)
                device.disconnect(self.status_set_status_string_handler_id)


	#
	# _CBDeviceConnected
	#
	# INTERNAL
	#
	# Callback triggered when a device is connected.
	#

	def _CBDeviceConnected(self, obj_path):
	
		self.logger.info("_CBDeviceConnected: device connected at path %s", obj_path)

		# update config from file
		self.config.UpdateConfig()

		new_device = Device(self.config, obj_path)
		self.devices.append(new_device)
		if len(self.devices) == 1:
			self.active_device = new_device
                        self.DeviceConnectSignals(self.active_device)

		if new_device.ProcessAuth():
			new_device.OnConnect()


	#
	# _CBDeviceDisconnected
	#
	# INTERNAL
	#
	# Callback triggered when a device is disconnected
	#

	def _CBDeviceDisconnected(self, obj_path):

		self.logger.info("_CBDeviceDisconnected: device disconnected from path %s", obj_path)
		for device in self.devices:
			if device.devicePath == obj_path:
				device.OnDisconnect()
				self.devices.remove(device)

				if device == self.active_device:
                                        self.DeviceDisconnectSignals(self.active_device)

					if len(self.devices) > 0:
						self.active_device = self.devices[0]
                                                self.DeviceConnectSignals(self.active_device)

					else:
						self.active_device = None
						# now, if the config tells us to run once, we need to 
						# trigger an exit here.
						if self.config.runonce == True:
							self.logger.debug("_CBDeviceDisconnecte: RunOnce specified on command line, requesting sync-engine shutdown")
							gobject.idle_add(self.mainloop.quit)

				return

		self.logger.info("_CBDeviceDisconnected: ignoring non-live device detach")

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
		return self.active_device.CheckAndGetValidPartnership()

	#
	# _CheckDeviceConnected
	#
	# INTERNAL
	#
	# Function to check if a device is connected.
	#

	def _CheckDeviceConnected(self):
		if self.active_device == None:
			raise errors.Disconnected
		if self.active_device.isConnected == False:
			raise errors.Disconnected


	##############################
	## EXPORTED DBUS API
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

	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='u')
	def GetDeviceBindingState(self):
		
		rc=BSTATE_DEVNOTCONNECTED

		if self.active_device != None:
			if self.active_device.isConnected:
				rc=BSTATE_DEVNOTBOUND
				if self.active_device.PshipManager.GetCurrentPartnership() is not None:
					rc=BSTATE_DEVBOUND
	
		return rc
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
	
		if not self.active_device.syncing.testandset():
			self.logger.info("Synchronize: doing nothing because we're already syncing")
			return

		if not self.active_device.autosync_triggered:

			self.logger.info("Synchronize: starting manual sync with partnership %s", pship)
				
			if not pship.itemDBLoaded:
				pship.LoadItemDB()
				self.logger.info("Synchronize: itemDB loaded")

			self.active_device.synchandler = SyncHandler(self.active_device, False)
			self.active_device.synchandler.start()
		else:
			self.active_device.syncing.unlock()
			self.logger.debug("Synchronize: previous sync triggered externally, no need to repeat")
			self.active_device.autosync_triggered = False
			self.active_device.Synchronized()

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
			if self.active_device.syncing.testandset():
				pfThread = prefill.PrefillThread(self.active_device,types)
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

	@dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='a(ussssuau)')
	def GetPartnerships(self):

		self._CheckDeviceConnected()

		ret = []
		for p in self.active_device.PshipManager.GetList():
			ret.append((p.info.id, p.info.guid, p.info.name, p.info.hostname, p.info.devicename, p.storetype, p.devicesyncitems))
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
		bindings = self.active_device.PshipManager.GetHostBindings()
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
			
		n=libxml2.parseDoc(self.active_device.PshipManager.QueryBindingConfiguration(id,guid))
			
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

		self.active_device.PshipManager.SetBindingConfiguration(id,guid,config)

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
		if self.active_device.PshipManager.GetCurrentPartnership() != None:
			start = False
	
		id=self.active_device.PshipManager.CreateNewPartnership(name, sync_items).info.id

		if start:
			self.active_device.StartSessions()
		
		self.active_device.PartnershipsChanged()
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
	
		if not self.active_device.syncing.testandset():
			raise errors.SyncRunning
	
		# We need to do this in all cases
	
		try:
			cpship = self._CheckAndGetValidPartnership()
			if cpship.info.id == id:
				self.logger.debug("DeletePartnership: calling sync pause")
				self.active_device.rapi_session.sync_pause()
				self.active_device.StopSessions()
				self.active_device.WaitForStoppingSessions()
		except Exception,e:
			# ignore if we have no valid partnership
			pass

		self.active_device.PshipManager.DeleteDevicePartnership(id,guid)
	
		self.active_device.syncing.unlock()
		self.active_device.PartnershipsChanged()
	
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

		if not self.active_device.syncing.testandset():
			self.logger.debug("FlushItemDB: impossible while syncing")
			return

		if self.config.config_Global.cfg["FlushIDB"]==1:
			self.logger.info("FlushItemDB: flushing current partnership DB")
			pship.FlushItemDB()

		self.active_device.syncing.unlock()

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


	##############################
	## callbacks from device objects to emit dbus signals
	##

	def PrefillCompleteCB(self, device, data=None):
		self.PrefillComplete()

	def SynchronizedCB(self, device, data=None):
		self.Synchronized()

	def PartnershipsChangedCB(self, device, data=None):
		self.PartnershipsChanged()

	def StatusSetMaxProgressValueCB(self, device, maxProgressValue, data=None):
		self.StatusSetMaxProgressValue(maxProgressValue)

	def StatusSetProgressValueCB(self, device, progressValue, data=None):
		self.StatusSetProgressValue(progressValue)

	def StatusSetStatusStringCB(self, device, statusString, data=None):
		self.StatusSetStatusString(statusString)

	def StatusSyncStartCB(self, device, data=None):
		self.StatusSyncStart()

	def StatusSyncEndCB(self, device, data=None):
		self.StatusSyncEnd()

	def StatusSyncStartPartnerCB(self, device, partner, data=None):
		self.StatusSyncStartPartner(partner)

	def StatusSyncEndPartnerCB(self, device, partner, data=None):
		self.StatusSyncEndPartner(partner)

	def StatusSyncStartDatatypeCB(self, device, partner, datatype, data=None):
		self.StatusSyncStartDatatype(partner,datatype)

	def StatusSyncEndDatatypeCB(self, device, partner, datatype, data=None):
		self.StatusSyncEndDatatype(partner,datatype)
