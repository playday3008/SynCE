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

import logging
from mutex import mutex

from pyrapi2 import *

from partnerships import *
from constants import *
from engine import *
from airsync import AirsyncThread
from rra import RRAThread
from synchandler import SyncHandler

class SyncEngine(dbus.service.Object):
    """
    A D-Bus service which provides an interface for synchronizing
    with Windows CE-based devices.
    """

    def __init__(self):
        dbus.service.Object.__init__(self, dbus.service.BusName(DBUS_SYNCENGINE_BUSNAME, bus=dbus.SessionBus()), DBUS_SYNCENGINE_OBJPATH)
        self.logger = logging.getLogger("engine.syncengine.SyncEngine")

        self.partnerships = None

        self.synchandler = None
        self.syncing = mutex()

        self.rapi_session = None
        self.rra = None
        self.airsync = None
        self.sync_begin_handler_id = None

        self.device_manager = dbus.Interface(dbus.SystemBus().get_object(DBUS_ODCCM_BUSNAME, DBUS_ODCCM_OBJPATH), DBUS_ODCCM_IFACE)
        self.device_manager.connect_to_signal("DeviceConnected", self._device_connected_cb)
        self.device_manager.connect_to_signal("DeviceDisconnected", self._device_disconnected_cb)

        if len(self.device_manager.GetConnectedDevices()) > 0:
            self.logger.info("__init__: connected device found")
            self.sessions_start()

    def _device_connected_cb(self, obj_path):
        self.logger.info("_device_connected_cb: device connected at path %s", obj_path)
        self.sessions_start()

    def _device_disconnected_cb(self, obj_path):
        self.logger.info("_device_disconnected_cb: device disconnected from path %s", obj_path)
        self.sessions_stop()
        self.sessions_wait_for_stop()

    def _check_device_connected(self):
        if self.partnerships == None:
            raise Disconnected("No device connected")

    def _check_valid_partnership(self):
        self._check_device_connected()
        if self.partnerships.get_current() is None:
            raise NotAvailable("No current partnership")

    def sessions_start(self):
        self.rra = RRAThread(self)
        self.rra.start()

        self.airsync = AirsyncThread(self)
        self.sync_begin_handler_id = self.airsync.connect("sync-begin", self._sync_begin_cb)
        self.airsync.start()

        self.logger.debug("sessions_start: setting up RAPI session")
        self.rapi_session = RAPISession(SYNCE_LOG_LEVEL_DEFAULT)

        self.logger.debug("sessions_start: initializing partnerships")
        self.partnerships = Partnerships(self)

        self.logger.debug("sessions_start: calling RAPI start_replication")
        self.rapi_session.start_replication()

        self.logger.debug("sessions_start: calling RAPI sync_resume")
        self.rapi_session.sync_resume()

    def sessions_stop(self):
        self.logger.debug("sessions_stop: stopping sync handler thread")
        if self.synchandler != None:
            self.synchandler.stop()

        self.logger.debug("sessions_stop: stopping Airsync server")
        if self.airsync != None:
            self.airsync.stop()

        self.logger.debug("sessions_stop: stopping RRA server")
        if self.rra != None:
            self.rra.stop()

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

        self.logger.debug("sessions_wait_for_stop: waiting for RRA server thread")
        if self.rra != None:
            self.rra.join()
            self.rra = None

        self.logger.debug("sessions_wait_for_stop: closing RAPI session")
        self.rapi_session = None

        self.logger.debug("sessions_wait_for_stop: clearing partnerships")
        self.partnerships = None

    def notify_quit(self):
        self.sessions_stop()

    def wait_quit(self):
        self.sessions_wait_for_stop()

    def _sync_begin_cb(self, res):
        if not self.syncing.testandset():
            raise Exception("Received sync request when already syncing")

        self.logger.info("_sync_begin_cb: monitoring auto sync with partnership %s", self.partnerships.get_current())
        self.synchandler = SyncHandler(self, True)
        self.synchandler.start()

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='')
    def Synchronize(self):
        self._check_valid_partnership()

        if not self.syncing.testandset():
            self.logger.info("Synchronize: doing nothing because we're already syncing")
            return

        self.logger.info("Synchronize: starting manual sync with partnership %s", self.partnerships.get_current())
        self.synchandler = SyncHandler(self, False)
        self.synchandler.start()

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
        """
        self.partnerships.delete(self.partnerships.get(id))

    @dbus.service.signal(DBUS_SYNCENGINE_IFACE, signature="")
    def Synchronized(self):
        self.logger.info("Synchronized: Emitting Synchronized signal")

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='', out_signature='s')
    def GetStateSummary(self):
        self._check_valid_partnership()
        return str(self.partnerships.get_current().state)

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='a{ua(sus)}', out_signature='')
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
                self.logger.debug("AddLocalChanges: adding change GUID = %s, ChangeType = %d, Data = %s",
                    guid, chg_type, data)
                item.add_local_change(guid, chg_type, data)

        self.logger.debug("AddLocalChanges: added (or ignored) %d changesets", len(changesets))


    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='au', out_signature='a{ua(sus)}')
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

        return changes

    @dbus.service.method(DBUS_SYNCENGINE_IFACE, in_signature='a{uas}', out_signature='')
    def AcknowledgeRemoteChanges(self, changes):
        self._check_valid_partnership()
        items = self.partnerships.get_current().state.items

        for item_type, guids in changes.items():
            item = items[item_type]

            self.logger.debug("AcknowledgeRemoteChanges: acking changes for items of type %d", item_type)
            for guid in guids:
                self.logger.debug("AcknowledgeRemoteChanges: acking change for items GUID = %s", guid)
                item.ack_remote_change(guid)

        self.logger.debug("AcknowledgeRemoteChanges: saving synchronization state")
        self.partnerships.get_current().save_state()
