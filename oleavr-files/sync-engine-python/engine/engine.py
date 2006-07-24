#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
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
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

import gobject
import dbus
import dbus.service
import dbus.glib

from twisted.internet import glib2reactor
glib2reactor.install()
from twisted.internet import reactor
from twisted.web2 import server, channel

from pyrapi2 import *
from partnership import *
from remsync import RemSyncServer
from rra import RRAServer
from airsync import *
from util import *
from interfaces import *
from errors import *
import socket

import os

from xml.dom import minidom

BUS_NAME = "org.synce.SyncEngine"
OBJECT_PATH = "/org/synce/SyncEngine"

class SyncEngine(dbus.service.Object):
    """
    A D-Bus service which provides an interface for synchronizing
    with Windows CE-based devices.
    """

    def __init__(self, bus_name, object_path, rss):
        dbus.service.Object.__init__(self, bus_name, object_path)

        self.rss = rss

        self.session = RAPISession(SYNCE_LOG_LEVEL_DEFAULT)
        self.cur_partnership = None

        res = ASResource()
        res.connect("sync-begin", self._airsync_sync_begin_cb)
        res.connect("sync-end", self._airsync_sync_end_cb)
        res.connect("object-added", self._airsync_object_added_cb)
        res.connect("object-modified", self._airsync_object_modified_cb)
        res.connect("object-deleted", self._airsync_object_deleted_cb)
        self.asr = res

        self._get_partnerships()
        self._init_transports()

        self._rra_started = False
        self._syncing = False

    def _get_partnerships(self):
        self.partnerships = {}
        self.slots = [ None, None ]

        reg_entries = {}
        sync_entries = []

        # Inspect registry entries
        hklm = self.session.HKEY_LOCAL_MACHINE
        partners_key = hklm.create_sub_key(
                r"Software\Microsoft\Windows CE Services\Partners")
        for pos in xrange(1, 3):
            key = partners_key.create_sub_key("P%d" % pos)
            if key.disposition == REG_OPENED_EXISTING_KEY:
                try:
                    id = key.query_value("PId")
                    hostname = key.query_value("PName")

                    if id != 0 and len(hostname) > 0:
                        reg_entries[hostname] = (pos, id)
                except RAPIError:
                    pass
            key.close()
        partners_key.close()

        # Look up the synchronization data on each
        for ctic in config_query_get(self.session, "Sync", "Sources").children.values():
            sub_ctic = config_query_get(self.session, "Sync.Sources", ctic.type, recursive=True)

            guid = sub_ctic.type
            hostname = sub_ctic["Server"]
            description = sub_ctic["Name"]

            if hostname in reg_entries:
                pos, id = reg_entries[hostname]
                del reg_entries[hostname]

                pship = Partnership(pos, id, guid, hostname, description)
                if pship.is_our():
                    self.cur_partnership = pship
                    self._partnership_changed()

                self.partnerships[id] = pship
                self.slots[pos-1] = pship

                engine = sub_ctic.children["Engines"].children[GUID_WM5_ACTIVESYNC_ENGINE]
                for provider in engine.children["Providers"].children.values():
                    if int(provider["Enabled"]) != 0:
                        id = None

                        if provider.type in SYNC_ITEM_ID_FROM_GUID:
                            id = SYNC_ITEM_ID_FROM_GUID[provider.type]
                        else:
                            if provider["Name"] == "Media":
                                id = SYNC_ITEM_MEDIA

                        if id == None:
                            raise ValueError("Unknown GUID \"%s\" for provider with name \"%s\"" \
                                    % (provider.type, provider["Name"]))

                        pship.sync_items.append(id)
            else:
                sync_entries.append((guid, hostname, description))

        for entry in reg_entries.values():
            print "deleting dangling registry entry:", entry
            hklm.delete_sub_key(
                r"Software\Microsoft\Windows CE Services\Partners\P%d" % entry[0])

        for entry in sync_entries:
            print "deleting dangling sync source:", entry
            config_query_remove(self.session, "Sync.Sources", entry[0])

        for pship in self.slots:
            print pship

    def _init_transports(self):
        entries = config_query_get(self.session, None, "CM_ProxyEntries").children.values()
        transport = None
        for entry in entries:
            if entry.type.startswith("HTTP-"):
                transport = entry
                break

        if transport is None:
            raise ProtocolError("HTTP transport not found")

        transport = config_query_get(self.session, "CM_ProxyEntries", transport.type)

        dtpt = Characteristic("CurrentDTPTNetwork")
        dtpt["DestId"] = transport["SrcId"]
        config_query_set(self.session, "CM_NetEntries", dtpt)
        print "CurrentDTPTNetwork set"

        site = server.Site(self.asr)
        factory = channel.HTTPFactory(site)
        reactor.listenTCP(26675, factory)

        self.rra = RRAServer()
        self.rra.connect("ready", self._rra_ready_cb)

    def _partnership_changed(self):
        pship = self.cur_partnership

        partners = self.session.HKEY_LOCAL_MACHINE.create_sub_key(
                r"Software\Microsoft\Windows CE Services\Partners")
        partners.set_value("PCur", 1, REG_DWORD)

        self.asr.set_partnership(pship)

    @dbus.service.method(SYNC_ENGINE_INTERFACE, in_signature='', out_signature='a{us}')
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

    @dbus.service.method(SYNC_ENGINE_INTERFACE, in_signature='', out_signature='a(ussau)')
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

        ret = []
        for p in self.partnerships.values():
            ret.append((p.id, p.name, p.hostname, p.sync_items))
        return ret

    @dbus.service.method(SYNC_ENGINE_INTERFACE, in_signature='sau', out_signature='u')
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

        if len(name) > 20:
            raise InvalidArgument("name too long (20 chars max)")

        for item in sync_items:
            if not item in SYNC_ITEMS:
                raise InvalidArgument("sync item identifier %d is invalid" % item)

        if len(self.partnerships) == 2:
            raise NoFreeSlots("all slots are currently full")

        slot = 1
        for partnership in self.slots:
            if partnership == None:
                break
            slot += 1

        pship = Partnership(slot, generate_id(), generate_guid(),
                            socket.gethostname(), name)

        #
        # Create the synchronization config data source
        #
        source = Characteristic(pship.guid)
        source["Name"] = pship.name
        source["Server"] = pship.hostname

        #
        # StoreType
        #  2 = ActiveSync desktop
        #  3 = Exchange server
        #
        source["StoreType"] = "2"

        engines = Characteristic("Engines")
        source.add_child(engines)

        engine = Characteristic(GUID_WM5_ACTIVESYNC_ENGINE)
        engines.add_child(engine)

        settings = Characteristic("Settings")
        settings["User"] = "DEFAULT"
        settings["Domain"] = "DEFAULT"
        settings["Password"] = "DEFAULT"
        settings["SavePassword"] = "1"
        settings["UseSSL"] = "0"
        settings["ConflictResolution"] = "1"
        settings["URI"] = "Microsoft-Server-ActiveSync"
        engine.add_child(settings)

        providers = Characteristic("Providers")
        engine.add_child(providers)

        for item_id, item_rec in SYNC_ITEMS.items():
            item_str, item_readonly = item_rec
            item_guid = SYNC_ITEM_ID_TO_GUID[item_id]
            item_enabled = (item_id in sync_items)

            provider = Characteristic(item_guid)
            provider["Enabled"] = str(int(item_enabled))
            provider["ReadOnly"] = str(int(item_readonly))
            provider["Name"] = item_str
            providers.add_child(provider)

        config_query_set(self.session, "Sync.Sources", source)

        #
        # Update the registry
        #
        hklm = self.session.HKEY_LOCAL_MACHINE

        partners_key = hklm.create_sub_key(
                r"Software\Microsoft\Windows CE Services\Partners")
        partners_key.set_value("PCur", pship.slot)

        key = partners_key.create_sub_key("P%d" % slot)
        key.set_value("PId", pship.id)
        key.set_value("DataSourceID", pship.guid)
        key.set_value("PName", pship.hostname)
        key.close()

        partners_key.close()

        #
        # Store it
        #
        self.partnerships[pship.id] = pship
        self.slots[pship.slot-1] = pship

        pship.create_sync_state()
        self.cur_partnership = pship
        self._partnership_changed()

        return pship.id

    @dbus.service.method(SYNC_ENGINE_INTERFACE, in_signature='u', out_signature='')
    def DeletePartnership(self, id):
        """
        Delete a partnership on the device.

        Parameters:
        id - integer identifier of the partnership to be deleted

        Possible Errors:
        Disconnected, InvalidArgument
        """

        if not id in self.partnerships:
            raise InvalidArgument("invalid id %d" % id)

        pship = self.partnerships[id]

        config_query_remove(self.session, "Sync.Sources", pship.guid)

        if pship.is_our():
            pship.delete_sync_state()
            self.cur_partnership = None

        del self.partnerships[id]
        self.slots[pship.slot-1] = None

    @dbus.service.method(SYNC_ENGINE_INTERFACE, in_signature='', out_signature='')
    def StartSync(self):
        pship = self.cur_partnership
        if pship is None:
            raise NotAvailable("No partnership found")

        if self._syncing:
            print "StartSync: Doing nothing because we're already syncing"
            return

        print "StartSync: Starting synchronization"

        doc = minidom.Document()
        doc_node = doc.createElement("sync")
        doc_node.setAttribute("xmlns", "http://schemas.microsoft.com/as/2004/core")
        doc_node.setAttribute("type", "Interactive")
        doc.appendChild(doc_node)

        node = doc.createElement("partner")
        node.setAttribute("id", pship.guid)
        doc_node.appendChild(node)

        self.session.sync_start(doc_node.toxml())
        self._syncing = True

        if not self._rra_started:
            self.session.start_replication()
            self._rra_started = True

    @dbus.service.method(SYNC_ENGINE_INTERFACE, in_signature='', out_signature='a{sv}')
    def GetRemoteChanges(self):
        return self.cur_partnership.state.get_remote_changes()

    @dbus.service.method(SYNC_ENGINE_INTERFACE, in_signature='as', out_signature='')
    def AcknowledgeRemoteChanges(self, sids):
        for sid in sids:
            self.cur_partnership.state.ack_remote_change(sid)
        self.cur_partnership.save_sync_state()

    def _rra_ready_cb(self, rra):
        print "_rra_ready_cb"
        d = rra.get_object_types()
        d.addCallback(self._get_object_types_cb)

    def _get_object_types_cb(self, result):
        print "_get_object_types_cb:", result
        ids = [ot[0] for ot in result["ObjectTypes"]]
        d = self.rra.set_boring_ssp_ids(ids)
        d.addCallback(self._set_boring_ssp_ids_cb)

    def _set_boring_ssp_ids_cb(self, result):
        print "_set_boring_ssp_ids_cb"
        d = self.rra.get_volumes()
        d.addCallback(self._get_volumes_cb)

    def _get_volumes_cb(self, result):
        print "_get_volumes_cb:", result
        d = self.rra.get_unknown_1_and_2()
        d.addCallback(self._get_unknown_1_and_2_cb)

    def _get_unknown_1_and_2_cb(self, result):
        print "_get_unknown_1_and_2_cb:", result

        self.session.sync_resume()

    def _airsync_sync_begin_cb(self, res):
        print "_airsync_sync_begin_cb"
        self._syncing = True

    def _airsync_sync_end_cb(self, res):
        print "_airsync_sync_end_cb"

        print "calling CeSyncPause()"
        self.session.sync_pause()

        print "calling StartOfSync, SetProgressRange and SetProgressValue"
        self.rss.set_start_of_sync()
        self.rss.set_progress_range(1, 1000)
        self.rss.set_progress_value(1)

        print "calling set_unknown_02(1, 4)"
        d = self.rra.set_unknown_02(1, 4)
        d.addCallback(self._set_unknown_02_1_4_cb)

    def _set_unknown_02_1_4_cb(self, result):
        print "_set_unknown_02_1_4_cb"
        print "calling EndOfSync"
        self.rss.set_end_of_sync()

        print "calling set_unknown_02(2, 0)"
        self.rra.set_unknown_02(2, 0)

        print "calling get_unknown_1_and_2()"
        d = self.rra.get_unknown_1_and_2()
        d.addCallback(self._get_unknown_1_and_2_rra_cb)

    def _get_unknown_1_and_2_rra_cb(self, result):
        print "_get_unknown_1_and_2_rra_cb:", result

        print "calling SetStatus(\"$UPTODATE$\")"
        self.rss.set_status("$UPTODATE$")

        self.cur_partnership.state.shift_changesets()
        self.cur_partnership.save_sync_state()
        self.Synchronized()

        self._syncing = False

        print "calling CeSyncResume()"
        self.session.sync_resume()

    def _airsync_object_added_cb(self, res, sid, item_type, data):
        print "Queuing AirSync remote object add: sid=%s item_type=%d" % (sid, item_type)
        self.cur_partnership.state.add_remote_change(sid, CHANGE_ADDED,
                                                     item_type, data)

    def _airsync_object_modified_cb(self, res, sid, item_type, data):
        print "Queuing AirSync remote object modify: sid=%s item_type=%d" % (sid, item_type)
        self.cur_partnership.state.add_remote_change(sid, CHANGE_MODIFIED,
                                                     item_type, data)

    def _airsync_object_deleted_cb(self, res, sid, item_type):
        print "Queuing AirSync remote object delete: sid=%s item_type=%d" % (sid, item_type)
        self.cur_partnership.state.add_remote_change(sid, CHANGE_DELETED,
                                                     item_type)

    @dbus.service.signal(SYNC_ENGINE_INTERFACE, signature="")
    def Synchronized(self):
        print "Emitting Synchronized"


if __name__ == "__main__":
    rss = RemSyncServer()

    # hack hack hack
    os.setgid(1000)
    os.setuid(1000)

    session_bus = dbus.SessionBus()
    bus_name = dbus.service.BusName(BUS_NAME, bus=session_bus)
    obj = SyncEngine(bus_name, OBJECT_PATH, rss)

    reactor.run()
