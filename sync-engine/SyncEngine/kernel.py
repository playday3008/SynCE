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

from mutex import mutex
from constants import *
from SyncEngine import *
from airsync import AirsyncThread
from synchandler import SyncHandler
from config import Config

class SyncEngine(dbus.service.Object):
    """
    A D-Bus service which provides an interface for synchronizing
    with Windows CE-based devices.
    """

    def __init__(self):
	    
        dbus.service.Object.__init__(self, dbus.service.BusName(DBUS_SYNCENGINE_BUSNAME, bus=dbus.SessionBus()), DBUS_SYNCENGINE_OBJPATH)
	self.logger = logging.getLogger("engine.syncengine.kernel")
	self.config = Config()

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
        self.odccm_manager.connect_to_signal("NameOwnerChanged", self._odccm_status_changed_cb)
	
	self.device = None
	self.deviceName = ""
	self.devicePath = ""

	try:
            self.device_manager = dbus.Interface(dbus.SystemBus().get_object(DBUS_ODCCM_BUSNAME, DBUS_ODCCM_OBJPATH), DBUS_ODCCM_IFACE)
            self.device_manager.connect_to_signal("DeviceConnected", self._device_connected_cb)
            self.device_manager.connect_to_signal("DeviceDisconnected", self._device_disconnected_cb)

	    self.isOdccmRunning = True
	    

	    obj_paths = self.device_manager.GetConnectedDevices()
            if len(obj_paths) > 0:
                self.logger.info("__init__: connected device found")
                self._device_connected_cb(obj_paths[0])
	except:
	    self.isOdccmRunning = False


    def _odccm_status_changed_cb(self, obj_path, param2, param3):
        if obj_path == "org.synce.odccm":
            #If this parameter is empty, the odccm just came online 
            if param2 == "":
                self.isOdccmRunning = True
                self.logger.info("_odccm_status_changed_cb: odccm came online")

            #If this parameter is empty, the odccm just went offline
            if param3 == "":
                self.isOdccmRunning = False
                self.logger.info("_odccm_status_changed_cb: odccm went offline")
            
            
            if self.isOdccmRunning:
		self.device = None
		self.deviceName = ""
		self.devicePath = ""

		try:
		    self.device_manager = dbus.Interface(dbus.SystemBus().get_object(DBUS_ODCCM_BUSNAME, DBUS_ODCCM_OBJPATH), DBUS_ODCCM_IFACE)
		    self.device_manager.connect_to_signal("DeviceConnected", self._device_connected_cb)
		    self.device_manager.connect_to_signal("DeviceDisconnected", self._device_disconnected_cb)

		    self.isOdccmRunning = True

		    obj_paths = self.device_manager.GetConnectedDevices()
		    if len(obj_paths) > 0:
			self.logger.info("_odccm_status_changed_cb: connected device found")
			self._device_connected_cb(obj_paths[0])
		except:
		    self.isOdccmRunning = False



    def _device_connected_cb(self, obj_path):
	 
	self.logger.info("_device_connected_cb: device connected at path %s", obj_path)

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
			self.logger.info("_device_connected_cb: device already connected")
		else:
			self.logger.info("_device_connected_cb: other device already connected - ignoring new device")

    def _device_disconnected_cb(self, obj_path):
        self.logger.info("_device_disconnected_cb: device disconnected from path %s", obj_path)
	if self.devicePath == obj_path:
		self.device=None
		self.deviceName = ""
		self.OnDisconnect()
	else:
		self.logger.info("_device_disconnected_cb: ignoring non-live device detach")

    def _check_device_connected(self):
        if self.isConnected == None:
            raise Disconnected("No device connected")

    def _CBDeviceAuthStateChanged(self,added,removed):
        self.logger.info("device authorization state changed: reauthorizing")
	self._ProcessAuth()

    def _CheckAndGetValidPartnership(self):
        self._check_device_connected()
	pship = self.PshipManager.GetCurrentPartnership()
        if pship is None:
            raise Exception("No current bound partnership, need to create one")
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
    # _reset_current_state
    #
    # INTERNAL
    #
    # Called to reset any internal state objects to defaults, usually
    # called on disconnect. Just so we can put them all in one place

    def _reset_current_state(self):
        autosync_triggered=False

    # Additional functions to separate device and sync handlers 
    
    def OnConnect(self):
	
	# ensure current state is set to defaults
	
        self._reset_current_state()
	self.isConnected = True

	# and start the sessions

        self.logger.debug("OnConnect: setting up RAPI session")
        self.rapi_session = rapicontext.RapiContext(pyrapi2.SYNCE_LOG_LEVEL_DEFAULT)

        self.logger.debug("OnConnect: Attempting to bind partnerships")
        self.PshipManager.AttemptToBind()
	
	# don't start any sessions if we don't have a valid partnership.
	
	try:
            self._CheckAndGetValidPartnership()
            self.sessions_start()
	except Exception,e:
            self.logger.debug("OnConnect: No valid partnership bindings are available, please create one (%s)" % str(e))
	    pass



    def OnDisconnect(self):
	    
	self.sessions_stop()
	self.sessions_wait_for_stop()

        self.logger.debug("sessions_wait_for_stop: closing RAPI session")
        self.rapi_session = None

        self.logger.debug("sessions_wait_for_stop: clearing partnerships")
        self.PshipManager.ClearDevicePartnerships()
	
	self.isConnected = False

    def sessions_start(self):

	# We have a valid partnership here, so run the config
	
	pship = self.PshipManager.GetCurrentPartnership()
	
	mh = pship.QueryConfig("/syncpartner-config/DTPT/Enabled[position()=1]","0")
	gh = self.config.config_Global.cfg["EnableDTPT"]
	self.logger.debug("sessions_start: config for partnership returns %s",mh)
	if mh == "1" and gh == 1:
		self.dtptsession = dtptserver.DTPTServer("0.0.0.0")
		self.dtptsession.start()
	else:
		self.dtptsession = None

	self.logger.debug("sessions_start: starting AirSync handler")
	self.airsync = AirsyncThread(self)
        self.sync_begin_handler_id = self.airsync.connect("sync-begin", self._sync_begin_cb)
        self.airsync.start()

	self.logger.debug("sessions_start: calling RAPI start_replication")
        self.rapi_session.start_replication()

        self.logger.debug("sessions_start: calling RAPI sync_resume")
        self.rapi_session.sync_resume()

	self.logger.debug("sessions_start: starting RRA session")
	self.RRASession.StartRRAEventHandler()

    def sessions_stop(self):

        self.logger.debug("sessions_stop: stopping RRA server")
	self.RRASession.StopRRAEventHandler()
		
	if self.dtptsession != None:
            self.logger.debug("sessions_stop: stopping DTPT server")
            self.dtptsession.shutdown()
	
        if self.synchandler != None:
	    self.logger.debug("sessions_stop: stopping sync handler thread")
            self.synchandler.stop()

        if self.airsync != None:
            self.logger.debug("sessions_stop: stopping Airsync server")
            self.airsync.stop()

		
    def sessions_wait_for_stop(self):

	if self.dtptsession != None:
        	self.logger.debug("sessions_wait_for_stop: waiting for DTPT server thread")
		self.dtptsession.join()
		self.dtptsession = None

        self.logger.debug("sessions_wait_for_stop: waiting for sync handler thread")
        if self.synchandler != None:
            self.synchandler.join()
            self.synchandler = None

        self.logger.debug("sessions_wait_for_stop: waiting for Airsync server thread")
        if self.airsync != None:
            self.airsync.join()
            self.sync_begin_handler_id = None
            self.airsync = None

        self.logger.debug("sessions_wait_for_stop: shutting down RRA server")
	self.RRASession.StopRRAEventHandler()
        
    def notify_quit(self):
        self.sessions_stop()

    def wait_quit(self):
        self.sessions_wait_for_stop()

    # _sync_begin_cb
    #
    # Called to trigger an autosync, either a manual sync from the device,
    # or from the timer
    #

    def _sync_begin_cb(self, res):
	
	pship=self._CheckAndGetValidPartnership()
	
	if not self.syncing.testandset():
            raise Exception("Received sync request when already syncing")

	self.logger.info("autosync: triggered")
	self.logger.info("_sync_begin_cb: monitoring auto sync with partnership %s", pship)
	
	if not pship.itemDBLoaded:
        	pship.LoadItemDB()
		
	self.logger.info("autosync: itemDB loaded")

	self.synchandler = SyncHandler(self, True)
       	self.synchandler.start()
	
	if self.config.config_AutoSync.cfg["Disable"] == 0:
		
		cmd_list = self.config.config_AutoSync.cfg["AutoSyncCommand"]
				
		if len(cmd_list) > 0:
			self.logger.info("autosync: command %s" % cmd_list[0])
			try:
				self.autosync_triggered = True	
				pid = os.spawnvp(os.P_NOWAIT,cmd_list[0],cmd_list)
				self.logger.info("autosync: process spawned with PID %d" % pid)
			except:
				self.autosync_triggered = False
				self.logger.debug("autosync : failed to spawn process : cmd=%s" % cmd_list[0])
    #
    # Synchronize
    #
    # Trigger a manual synchronization

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
    # EXPORTED
    #
    # PrefillRemote should be called after a sync run, and it alters the list of
    # remote changes that will be reported to the synchronizer plugin to include everything
    # in the db.
	
    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='as', out_signature='u')
    def PrefillRemote(self,types):

        pship = self._CheckAndGetValidPartnership()
	self.logger.info("prefill: slow sync prefill triggered")

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
                self.logger.debug("prefill: impossible while syncing")
        else:
            self.logger.info("slow sync prefill disabled in config")

        return rc


    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='a{us}')
    def GetItemTypes(self):
        """
        Get a list of synchronizable item types.

        Returns:
        a dictionary mapping item identifiers to names.
        """

        types = {}
        for id, val in SYNC_ITEMS.items():
            name, readonly = val
            types[id] = name
        return types

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='a(ussssau)')
    def GetPartnerships(self):
        """
        Get a list of partnerships on the device.

        Returns:
        an array of structs containing:
          an integer identifier
          a string with the partnership name
          a string with the hostname associated with it
          an array of sync item ids

        Possible Errors:
        Disconnected
        """
        self._check_device_connected()

        ret = []
        for p in self.PshipManager.GetList():
            ret.append((p.info.id, p.info.guid, p.info.name, p.info.hostname, p.info.devicename, p.devicesyncitems))
        return ret


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
	
    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='us',out_signature='s')
    def QueryBindingConfig(self,id,guid):
	    #
	    # 'prettify' the stored XML somewhat...
	    s = self.PshipManager.QueryBindingConfiguration(id,guid)
	    n=libxml2.parseDoc(s)
	    return n.serialize("utf-8",1)

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='uss', out_signature='')
    def SetBindingConfig(self,id,guid,config):
	    self.PshipManager.SetBindingConfiguration(id,guid,config)

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='sau', out_signature='u')
    def CreatePartnership(self, name, sync_items):
        """
        Create a new partnership on the device.

        Parameters:
        name - a user-specified name of the partnership, for example "Linux desktop"
        sync_items - an array of sync item ids

        Returns:
        an integer identifying the newly created partnership

        Possible Errors:
        Disconnected, InvalidArgument, NoFreeSlots
        """
        self._check_device_connected()
	
	# set a flag if we have a current partnership (already bound)
	
	start = True
	if self.PshipManager.GetCurrentPartnership() != None:
		start = False
	
        id=self.PshipManager.CreateNewPartnership(name, sync_items).info.id
	
	if start:
		self.sessions_start()
		
	return id
	

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='au')
    def GetSynchronizedItemTypes(self):
        pship = self._CheckAndGetValidPartnership()
        return pship.devicesyncitems

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='us', out_signature='')
    def DeletePartnership(self,id,guid):
        """
        Delete a partnership on the device.

        Parameters:
        id - integer identifier of the partnership to be deleted

        Possible Errors:
        Disconnected, InvalidArgument
	
	Now what happens if we delete our current partnership? We still have
	sessions running. We should terminate all running sessions if we delete
	our current partnership
	
        """
	
        if not self.syncing.testandset():
            raise Exception("Need to wait for sync to finish")
	
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
	

    @dbus.service.signal(DBUS_SYNCENGINE_IFACE, signature='')
    def PrefillComplete(self):
        self.logger.info("Prefill complete: Emitting PrefillComplete signal")

    @dbus.service.signal(DBUS_SYNCENGINE_IFACE, signature='')
    def Synchronized(self):
        self.logger.info("Synchronized: Emitting Synchronized signal")

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
