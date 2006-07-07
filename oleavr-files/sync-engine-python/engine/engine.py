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
from pyrapi2 import *
from util import *
from constants import *
from interfaces import *
from errors import *
import socket

from twisted.internet import glib2reactor
glib2reactor.install()
from twisted.internet import reactor, defer
from twisted.internet.protocol import Protocol, Factory
from twisted.web2 import server, http, resource, channel, stream
import os

from xml.dom import minidom
import pywbxml

BUS_NAME = "org.synce.SyncEngine"
OBJECT_PATH = "/org/synce/SyncEngine"

class SyncEngine(dbus.service.Object):
    """
    A D-Bus service which provides an interface for synchronizing
    with Windows CE-based devices.
    """

    def __init__(self, bus_name, object_path):
        dbus.service.Object.__init__(self, bus_name, object_path)

        self.session = RAPISession(SYNCE_LOG_LEVEL_DEFAULT)
        self._get_partnerships()

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
                self.partnerships[id] = pship
                self.slots[pos-1] = pship

                engine = sub_ctic.children["Engines"].children[GUID_WM5_ENGINE]
                for provider in engine.children["Providers"].children.values():
                    if int(provider["Enabled"]) != 0:
                        pship.sync_items.append(SYNC_ITEM_ID_FROM_GUID[provider.type])
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

        print "adding:", pship

        #
        # Create the synchronization config data source
        #
        source = Characteristic(pship.guid)
        source["Name"] = pship.name
        source["Server"] = pship.hostname
        source["StoreType"] = "2"

        engines = Characteristic("Engines")
        source.add_child(engines)

        engine = Characteristic(GUID_WM5_ENGINE)
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
        self.slots[pship.slot] = pship

        self._get_partnerships()

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

        config_query_remove(self.session, "Sync.Sources", self.partnerships[id].guid)
        self._get_partnerships()

    @dbus.service.method(SYNC_ENGINE_INTERFACE, in_signature='', out_signature='')
    def TestSync(self):
        partner = self.slots[0]

        print
        print "Testing synchronization with partner", partner

        entries = config_query_get(self.session, None, "CM_ProxyEntries").children.values()
        transport = None
        for entry in entries:
            if entry.type.startswith("HTTP-"):
                transport = entry
                break

        if transport is None:
            raise ProtocolError("HTTP transport not found")

        transport = config_query_get(self.session, "CM_ProxyEntries", transport.type)
        print "Using the following transport:"
        print transport

        dtpt = Characteristic("CurrentDTPTNetwork")
        dtpt["DestId"] = transport["SrcId"]
        config_query_set(self.session, "CM_NetEntries", dtpt)
        print "CurrentDTPTNetwork set"

        print "Starting synchronization"
        doc = minidom.Document()
        doc_node = doc.createElement("sync")
        doc_node.setAttribute("xmlns", "http://schemas.microsoft.com/as/2004/core")
        doc_node.setAttribute("type", "Interactive")
        doc.appendChild(doc_node)

        node = doc.createElement("partner")
        node.setAttribute("id", partner.guid)
        doc_node.appendChild(node)

        print "Calling sync_start() with params:"
        print doc_node.toprettyxml()

        self.session.sync_start(doc_node.toxml())
        print "Succeeded"

        # set the current partnership on the device
        print "Setting PCur...",
        partners = self.session.HKEY_LOCAL_MACHINE.create_sub_key(
                r"Software\Microsoft\Windows CE Services\Partners")
        partners.set_value("PCur", 1, REG_DWORD)
        print "success"

        print "Calling start_replication...",
        self.session.start_replication()
        print "success"

        print "Calling sync_resume...",
        self.session.sync_resume()
        print "success"


class Partnership:
    def __init__(self, slot, id, guid, hostname, name):
        self.slot = slot
        self.id = id
        self.guid = guid
        self.hostname = hostname
        self.name = name

        self.sync_items = []

    def __str__(self):
        str = ""
        for id in self.sync_items:
            if str:
                str += ", "
            else:
                str = "[ "
            str += SYNC_ITEMS[id][0]
        str += " ]"

        return "P%d: id=%#x, guid=\"%s\", hostname=\"%s\", name=\"%s\", sync_items=%s" % \
            (self.slot, self.id, self.guid, self.hostname, self.name, str)


class BaseProtocol(Protocol):
    def connectionMade(self):
        print "%s.connectionMade: client connected" % \
            (self.__class__.__name__)

    def connectionLost(self, reason):
        print "%s.connectionLost: connection lost because: %s" % \
            (self.__class__.__name__, reason)

    def dataReceived(self, data):
        print "%s.dataReceived: got %d bytes of data" % \
            (self.__class__.__name__, len(data))


class RRAC(BaseProtocol):
    pass


class Status(BaseProtocol):
    pass


class ASResource(resource.PostableResource):
    def locateChild(self, request, segments):
        #print "locateChild"
        #print "requests:", request
        #print "segments:", segments

        found = True

        if len(segments) < 1:
            found = False
        else:
            if segments[0] != "Microsoft-Server-ActiveSync":
                found = False

        print "found \"%s\": %s" % (request, found)

        print

        if found:
            return (self, ())
        else:
            return (None, ())

    def render(self, request):
        print "render: %s" % request.path
        print
        return self.create_response(404)

    def create_response(self, code):
        resp = http.Response(code)
        headers = resp.headers

        headers.addRawHeader("Pragma", "no-cache")
        # FIXME: "Date" is probably missing ...
        headers.addRawHeader("Server", "ActiveSync/4.1")
        headers.addRawHeader("MS-Server-ActiveSync", "4.1.4841.0")

        return resp

    def http_OPTIONS(self, request):
        if request.path != "/Microsoft-Server-ActiveSync":
            print "http_OPTIONS: Returning 404 for path %s" % request.path
            return http.Response(404)

        resp = self.create_response(200)
        headers = resp.headers
        headers.addRawHeader("Allow", "OPTIONS, POST")
        headers.addRawHeader("Public", "OPTIONS, POST")
        headers.addRawHeader("MS-ASProtocolVersions", "2.5")
        headers.addRawHeader("MS-ASProtocolCommands",
                "Sync,SendMail,SmartForward,SmartReply,GetAttachment,FolderSync,FolderCreate,FolderUpdate,MoveItems,GetItemEstimate,MeetingResponse")

        return resp

    def http_POST(self, request):
        if request.path == "/Microsoft-Server-ActiveSync":
            return self.parse_sync_request(request)
        elif request.path == "/Microsoft-Server-ActiveSync/SyncStat.dll":
            return self.parse_status_request(request)
        else:
            return self.create_response(404)

    def parse_sync_request(self, request):
        args = request.args

        print "parse_sync_request"
        print "  path=\"%s\"" % request.path
        print "  args=\"%s\"" % args
        print

        req_def = defer.Deferred()

        d = request.stream.read()

        cmd = args["Cmd"][0]
        print "parse_sync_request: Cmd=\"%s\"" % cmd
        if cmd == "Sync":
            d.addCallback(self.parse_sync_body)
        elif cmd == "FolderSync":
            d.addCallback(self.parse_foldersync_body)
        else:
            print "Unknown"
            req_def.callback(self.create_response(404))

        d.chainDeferred(req_def)

        return req_def

    def parse_sync_body(self, body):
        print "parse_sync_body:"

        xml_raw = pywbxml.wbxml2xml(body)
        doc = minidom.parseString(xml_raw)
        print doc.toprettyxml()

        return self.create_response(404)

    def parse_foldersync_body(self, body):
        print "parse_foldersync_body:"

        xml_raw = pywbxml.wbxml2xml(body)
        doc = minidom.parseString(xml_raw)
        print doc.toprettyxml()

        folder_node = node_find_child(doc, "FolderSync")
        key_node = node_find_child(folder_node, "SyncKey")
        key = node_get_value(key_node)
        if key != "0":
            raise ValueError("SyncKey specified is not 0")

        doc = minidom.Document()
        folder_node = doc.createElement("FolderSync")
        doc.appendChild(folder_node)

        node_append_child(folder_node, "Status", "1")
        node_append_child(folder_node, "SyncKey",
                "{00000000-0000-0000-0000-000000000000}1")

        changes_node = node_append_child(folder_node, "Changes")

        folders = (
            ("{E1D7A28F-2806-4B0D-8145-C8A3AEB16C7D}", 0, "Deleted Items", 4),
            ("{2C02FAE0-5776-4CB1-8BE4-BF324A3B9118}", 0, "Inbox", 2),
            ("{512856CA-3DC8-41EC-B550-51AA5BDC5C63}", 0, "Outbox", 6),
            ("{FD0A63A4-B027-4C56-AA00-BB34D74362F7}", 0, "Sent Items", 5),
            ("{09C98557-355B-4937-B208-91EAA659162E}", 0, "Calendar", 8),
            ("{07EAD926-D6A8-4B51-912F-42A3E0305E3A}", 0, "Contacts", 9),
            ("{5801C5D3-93E5-45EC-90FB-31AEBC6A56DB}", 0, "Journal", 11),
            ("{D957E4C0-B098-46E3-9490-033EB8FF3542}", 0, "Notes", 10),
            ("{61CF8DB7-774F-4C3F-BD12-F2C2CD810FFC}", 0, "Tasks", 7),
            ("{4FA50CB4-A991-4283-AC30-BDB66686116B}", 0, "Drafts", 3),
            ("{A1B4AE44-F61C-4735-8411-137AE2207BE1}", 0, "Junk E-mail", 12),
        )

        print folders

        node_append_child(changes_node, "Count", len(folders))

        for folder in folders:
            server_id, parent_id, display_name, type = folder

            add_node = node_append_child(changes_node, "Add")

            node_append_child(add_node, "ServerId", server_id)
            node_append_child(add_node, "ParentId", parent_id)
            node_append_child(add_node, "DisplayName", display_name)
            node_append_child(add_node, "Type", type)

        print "Going to send:"
        print doc.toprettyxml()

        """
        <FolderSync>
            <Status>
                1
            <Status/>
            <SyncKey>
                {00000000-0000-0000-0000-000000000000}1
            <SyncKey/>
            <Changes>
                <Count>
                    11
                <Count/>
                <Add>
                    <ServerId>
                        {E1D7A28F-2806-4B0D-8145-C8A3AEB16C7D}
                    <ServerId/>
                    <ParentId>
                        0
                    <ParentId/>
                    <DisplayName>
                        Deleted Items
                    <DisplayName/>
                    <Type>
                        4
                    <Type/>
                <Add/>
                <Add>
                    <ServerId>
                        {2C02FAE0-5776-4CB1-8BE4-BF324A3B9118}
                    <ServerId/>
                    <ParentId>
                        0
                    <ParentId/>
                    <DisplayName>
                        Inbox
                    <DisplayName/>
                    <Type>
                        2
                    <Type/>
                <Add/>
                <Add>
                    <ServerId>
                        {512856CA-3DC8-41EC-B550-51AA5BDC5C63}
                    <ServerId/>
                    <ParentId>
                        0
                    <ParentId/>
                    <DisplayName>
                        Outbox
                    <DisplayName/>
                    <Type>
                        6
                    <Type/>
                <Add/>
                <Add>
                    <ServerId>
                        {FD0A63A4-B027-4C56-AA00-BB34D74362F7}
                    <ServerId/>
                    <ParentId>
                        0
                    <ParentId/>
                    <DisplayName>
                        Sent Items
                    <DisplayName/>
                    <Type>
                        5
                    <Type/>
                <Add/>
                <Add>
                    <ServerId>
                        {09C98557-355B-4937-B208-91EAA659162E}
                    <ServerId/>
                    <ParentId>
                        0
                    <ParentId/>
                    <DisplayName>
                        Calendar
                    <DisplayName/>
                    <Type>
                        8
                    <Type/>
                <Add/>
                <Add>
                    <ServerId>
                        {07EAD926-D6A8-4B51-912F-42A3E0305E3A}
                    <ServerId/>
                    <ParentId>
                        0
                    <ParentId/>
                    <DisplayName>
                        Contacts
                    <DisplayName/>
                    <Type>
                        9
                    <Type/>
                <Add/>
                <Add>
                    <ServerId>
                        {5801C5D3-93E5-45EC-90FB-31AEBC6A56DB}
                    <ServerId/>
                    <ParentId>
                        0
                    <ParentId/>
                    <DisplayName>
                        Journal
                    <DisplayName/>
                    <Type>
                        11
                    <Type/>
                <Add/>
                <Add>
                    <ServerId>
                        {D957E4C0-B098-46E3-9490-033EB8FF3542}
                    <ServerId/>
                    <ParentId>
                        0
                    <ParentId/>
                    <DisplayName>
                        Notes
                    <DisplayName/>
                    <Type>
                        10
                    <Type/>
                <Add/>
                <Add>
                    <ServerId>
                        {61CF8DB7-774F-4C3F-BD12-F2C2CD810FFC}
                    <ServerId/>
                    <ParentId>
                        0
                    <ParentId/>
                    <DisplayName>
                        Tasks
                    <DisplayName/>
                    <Type>
                        7
                    <Type/>
                <Add/>
                <Add>
                    <ServerId>
                        {4FA50CB4-A991-4283-AC30-BDB66686116B}
                    <ServerId/>
                    <ParentId>
                        0
                    <ParentId/>
                    <DisplayName>
                        Drafts
                    <DisplayName/>
                    <Type>
                        3
                    <Type/>
                <Add/>
                <Add>
                    <ServerId>
                        {A1B4AE44-F61C-4735-8411-137AE2207BE1}
                    <ServerId/>
                    <ParentId>
                        0
                    <ParentId/>
                    <DisplayName>
                        Junk E-mail
                    <DisplayName/>
                    <Type>
                        12
                    <Type/>
                <Add/>
            <Changes/>
        <FolderSync/>
        """

        return self.create_response(404)

    def parse_status_request(self, request):
        d = request.stream.read()
        d.addCallback(self.parse_status_request_body)

    def parse_status_request_body(self, body):
        print "parse_status_request_body:"

        print body
        #doc = minidom.parseString(body)
        #print doc.toprettyxml()

        return self.create_response(200)


if __name__ == "__main__":
    factory = Factory()
    factory.protocol = Status
    reactor.listenTCP(999, factory)

    factory = Factory()
    factory.protocol = RRAC
    reactor.listenTCP(5678, factory)

    site = server.Site(ASResource())
    factory = channel.HTTPFactory(site)
    reactor.listenTCP(26675, factory)

    # hack hack hack
    os.setgid(1000)
    os.setuid(1000)

    session_bus = dbus.SessionBus()
    bus_name = dbus.service.BusName(BUS_NAME, bus=session_bus)
    obj = SyncEngine(bus_name, OBJECT_PATH)

    reactor.run()
