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

from mutex import mutex
from pyrapi2 import *
from partnerships import *
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
	self.logger = logging.getLogger("engine.syncengine.SyncEngine")
	self.config = Config()

        self.partnerships = None

        self.synchandler = None
        self.syncing = mutex()

        self.rapi_session = None
        self.rra = None
	self.RRASession = rrasyncmanager.RRASyncManager(self)
        self.airsync = None
        self.sync_begin_handler_id = None
	self.autosync_triggered = False

        self.device_manager = dbus.Interface(dbus.SystemBus().get_object(DBUS_ODCCM_BUSNAME, DBUS_ODCCM_OBJPATH), DBUS_ODCCM_IFACE)
        self.device_manager.connect_to_signal("DeviceConnected", self._device_connected_cb)
        self.device_manager.connect_to_signal("DeviceDisconnected", self._device_disconnected_cb)
	
	self.device = None
	self.deviceName = ""
	self.devicePath = ""

	obj_paths = self.device_manager.GetConnectedDevices()
        if len(obj_paths) > 0:
            self.logger.info("__init__: connected device found")
            self._device_connected_cb(obj_paths[0])

    def _device_connected_cb(self, obj_path):
	 
	self.logger.info("_device_connected_cb: device connected at path %s", obj_path)

	if self.partnerships == None:
		
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
        if self.partnerships == None:
            raise Disconnected("No device connected")

    def _CBDeviceAuthStateChanged(self,added,removed):
        self.logger.info("device authorization state changed: reauthorizing")
	self._ProcessAuth()

    def _check_valid_partnership(self):
        self._check_device_connected()
        if self.partnerships.get_current() is None:
            raise NotAvailable("No current partnership")
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

	# and start the sessions

        self.logger.debug("OnConnect: setting up RAPI session")
        self.rapi_session = RAPISession(SYNCE_LOG_LEVEL_DEFAULT)

        self.logger.debug("OnConnect: initializing partnerships")
        self.partnerships = Partnerships(self)

	try:
            self._check_valid_partnership()
	    self.sessions_start()
	except:
	    pass

    def OnDisconnect(self):
	    
	self.sessions_stop()
	self.sessions_wait_for_stop()

        self.logger.debug("sessions_wait_for_stop: closing RAPI session")
        self.rapi_session = None

        self.logger.debug("sessions_wait_for_stop: clearing partnerships")
        self.partnerships = None

    def sessions_start(self):

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
	
	self.logger.debug("sessions_stop: stopping sync handler thread")
        if self.synchandler != None:
            self.synchandler.stop()

        self.logger.debug("sessions_stop: stopping Airsync server")
        if self.airsync != None:
            self.airsync.stop()
		
    def sessions_wait_for_stop(self):
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
	
	self._check_valid_partnership()
	
	if not self.syncing.testandset():
            raise Exception("Received sync request when already syncing")

	self.logger.info("autosync: triggered")
	pship = self.partnerships.get_current()
	self.logger.info("_sync_begin_cb: monitoring auto sync with partnership %s", pship)
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

	self._check_valid_partnership()
	
        if not self.syncing.testandset():
            self.logger.info("Synchronize: doing nothing because we're already syncing")
            return

	if not self.autosync_triggered:
 
            pship = self.partnerships.get_current()
 
            self.logger.info("Synchronize: starting manual sync with partnership %s", pship)
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
        self._check_valid_partnership()
	self.logger.info("prefill: slow sync prefill triggered")

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

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='a(ussau)')
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
        for p in self.partnerships.get_list():
            ret.append((p.id, p.name, p.hostname, p.sync_items))
        return ret


    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='u', out_signature='u')
    def SetCurrentPartnership(self, id):
	"""
	Set the current partnership with the device. (keyed by ID)
	
	Input:
	    Integer ID of selected partnership

	"""
	mtx=0
        self._check_device_connected()
        if self.partnerships.get_current() is not None:
            if not self.syncing.testandset():
                self.logger.info("SetCurrentPartnership: can't change partnership in middle of a sync")
		return
	    else:
	        mtx = 1
	set = 1
	for p in self.partnerships.get_list():
	    if p.id == id:
		self.logger.info("found selected partnership")
		try:
		    self.partnerships.set_current(p)
		except:
		    self.logger.info("bad partnership")
		set=0
		break
	if set == 1:
	    self.logger.warning("SetCurrentPartnership: invalid ID provided - partnership not set")
	    
	if mtx == 1:
	    self.syncing.unlock()
	    
        return set
	

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
        return self.partnerships.add(name, sync_items).id

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='au')
    def GetSynchronizedItemTypes(self):
        self._check_valid_partnership()
        return self.partnerships.get_current().state.items.keys()

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='u', out_signature='')
    def DeletePartnership(self, id):
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
        self.partnerships.delete(self.partnerships.get(id))

    @dbus.service.signal(DBUS_SYNCENGINE_IFACE, signature='')
    def PrefillComplete(self):
        self.logger.info("Prefill complete: Emitting PrefillComplete signal")

    @dbus.service.signal(DBUS_SYNCENGINE_IFACE, signature='')
    def Synchronized(self):
        self.logger.info("Synchronized: Emitting Synchronized signal")

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='s')
    def GetStateSummary(self):
        self._check_valid_partnership()
        return str(self.partnerships.get_current().state)

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='a{ua(ayuay)}', out_signature='')
    def AddLocalChanges(self, changesets):
        self._check_valid_partnership()
        items = self.partnerships.get_current().state.items

        for item_type, changes in changesets.items():

            if not items.has_key(item_type):
                self.logger.info("AddLocalChanges: skipping changes for item of type %d", item_type)
                continue

            item = items[item_type]

            self.logger.debug("AddLocalChanges: adding changes for item of type %d", item.type)

            for change in changes:
                guid, chg_type, data = change
		guid = array.array('B',guid).tostring()
		data = array.array('B',data).tostring()
                self.logger.debug("AddLocalChanges: adding change GUID = %s, ChangeType = %d, Data = %s",
                    guid, chg_type, data)
                item.add_local_change(guid, chg_type, data)

        self.logger.debug("AddLocalChanges: added (or ignored) %d changesets", len(changesets))


    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='au', out_signature='a{ua(ayuay)}')
    def GetRemoteChanges(self, item_types):
        self._check_valid_partnership()
		
	items = self.partnerships.get_current().state.items

        changes = {}
        for type in item_types:
        	changes[type] = []

        	self.logger.debug("GetRemoteChanges: getting changes for items of type %d", type)

        	for guid, change in items[type].get_remote_changes().items():
        		chg_type, chg_data = change
        		self.logger.debug("GetRemoteChanges: got change GUID = %s, ChangeType = %d, Data = %s",
        			guid, chg_type, chg_data)
        		changes[type].append((guid, chg_type, chg_data))

	self.logger.debug("return")
        return changes
	
    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='a{uaay}', out_signature='')
    def AcknowledgeRemoteChanges(self, changes):
        self._check_valid_partnership()
        items = self.partnerships.get_current().state.items

        for item_type, guids in changes.items():
            item = items[item_type]

            self.logger.debug("AcknowledgeRemoteChanges: acking changes for items of type %d", item_type)
            for guid in guids:
		guid = array.array('B',guid).tostring()
                self.logger.debug("AcknowledgeRemoteChanges: acking change for items GUID = %s", guid)
                item.ack_remote_change(guid)

        self.logger.debug("AcknowledgeRemoteChanges: saving synchronization state and itemDB")
        self.partnerships.get_current().save_state()
	self.partnerships.get_current().SaveItemDB()
   
    #
    # FlushItemDB
    #
    # Called at the end of a sync session to flush the (already saved) itemDB from memory.
    # There will be no ill-effects if this function is not called other than to retain the
    # item database in memory for all time, instead of just when syncing.
    #

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='')
    def FlushItemDB(self):
        self._check_valid_partnership()
       
        if not self.syncing.testandset():
            self.logger.debug("FlushItemDB: impossible while syncing")
            return
       
        pship = self.partnerships.get_current()

        self.logger.info("FlushItemDB: flushing current partnership DB")
        pship.FlushItemDB()

        self.syncing.unlock()
