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

import cPickle as pickle
import os
import os.path
import logging
import socket

from pyrapi2 import *

from util import *
from errors import *
from constants import *
from rapiutil import *

class Partnerships:

    def __init__(self, engine):
        self.logger = logging.getLogger("engine.partnerships.Partnerships")
        self.engine = engine

        self.current = None
        self.slots = [ None, None ]

        self._read_device()

        for pship in self.slots:
            if pship != None and pship.has_state():
                self.set_current(pship)
                break

    def _read_device(self):

        reg_entries = {}
        sync_entries = []

        hklm = self.engine.rapi_session.HKEY_LOCAL_MACHINE

        # Inspect registry entries
        self.logger.debug("_read_device: reading partnerships from device registry")
        partners_key = hklm.create_sub_key(r"Software\Microsoft\Windows CE Services\Partners")
        for pos in xrange(1, 3):
            key = partners_key.create_sub_key("P%d" % pos)
            if key.disposition == REG_OPENED_EXISTING_KEY:
                try:
                    id = key.query_value("PId")
                    hostname = key.query_value("PName")

                    self.logger.debug("_read_device: read partnership ID = %s, Hostname = %s", id, hostname)

                    if id != 0 and len(hostname) > 0:
                       self.logger.debug("_read_device: Adding entry")
                       reg_entries[hostname] = (pos, id)
                except RAPIError, e:
                    self.logger.warn("_read_device: Error getting partnership key %d from device registry: %s", pos, e)
            key.close()
        partners_key.close()

        # Look up the synchronization data on each
        self.logger.debug("_read_device: querying synchronization source information from device")
        for ctic in config_query_get(self.engine.rapi_session, "Sync", "Sources").children.values():
            sub_ctic = config_query_get(self.engine.rapi_session, "Sync.Sources", ctic.type, recursive=True)

            guid = sub_ctic.type
            hostname = sub_ctic["Server"]
            description = sub_ctic["Name"]

            self.logger.debug("_read_device: read source GUID = %s, Hostname = %s, Description = %s", guid, hostname, description)

            if hostname in reg_entries:
                pos, id = reg_entries[hostname]
                del reg_entries[hostname]

                self.logger.debug("_read_device: source matches partnerhip from registry.  Initializing partnership")

                pship = Partnership(pos, id, guid, hostname, description)
                self.slots[pos-1] = pship

                self.logger.debug("_read_device: querying partnerhip synchronization items (providers)")

                engine = sub_ctic.children["Engines"].children[GUID_WM5_ACTIVESYNC_ENGINE]
                for provider in engine.children["Providers"].children.values():

                    self.logger.debug("_read_device: found provider %s", provider["Name"])

                    if int(provider["Enabled"]) != 0:
                        id = None

                        self.logger.debug("_read_device: provider is enabled")

                        if provider.type in SYNC_ITEM_ID_FROM_GUID:
                            id = SYNC_ITEM_ID_FROM_GUID[provider.type]
                        else:
                            if provider["Name"] == "Media":
                                id = SYNC_ITEM_MEDIA

                        if id == None:
                            raise ValueError("Unknown GUID \"%s\" for provider with name \"%s\"" \
                                    % (provider.type, provider["Name"]))

                        self.logger.debug("_read_device: provider ID is %d", id)

                        pship.sync_items.append(id)
            else:
                self.logger.debug("_read_device: Found dangling sync source: GUID = %s, Hostname = %s, Description = %s",
                    guid, hostname, description)
                sync_entries.append((guid, hostname, description))

        for entry in reg_entries.values():
            self.logger.info("_read_device: Deleting dangling registry entry: %s", entry)
            hklm.delete_sub_key(r"Software\Microsoft\Windows CE Services\Partners\P%d" % entry[0])

        for entry in sync_entries:
            self.logger.info("_read_device: Deleting dangling sync source: %s", entry)
            config_query_remove(self.engine.rapi_session, "Sync.Sources", entry[0])

    def get_list(self):
        pships = []
        for pship in self.slots:
            if pship != None:
                pships.append(pship)
        return pships

    def get(self, id):
        for pship in self.slots:
            if pship != None and pship.id == id:
                return pship
        return None

    def get_current(self):
        return self.current

    def set_current(self, pship):
        partners = self.engine.rapi_session.HKEY_LOCAL_MACHINE.create_sub_key(r"Software\Microsoft\Windows CE Services\Partners")

        if self.current == pship:
            return

        if pship != None and not pship.has_state():
            raise ValueError("Requested partnership has no existing state")

        if pship != None and not (pship in self.slots):
            raise ValueError("Requested partnership %s not found in slots" % pship)

        # Load or create the sync state and update the registry entry
        if pship == None:
            self.logger.debug("set_current: setting current partnership to 0")
            partners.set_value("PCur", 0, REG_DWORD)
        else:
            idx = self.slots.index(pship)
            self.logger.debug("set_current: partnership found in slot %d", idx + 1)
            partners.set_value("PCur", idx + 1, REG_DWORD)

        self.current = pship

    def add(self, name, sync_items):
        if len(name) > 20:
            raise InvalidArgument("name too long (20 chars max)")

        for item in sync_items:
            if not item in SYNC_ITEMS:
                raise InvalidArgument("sync item identifier %d is invalid" % item)

        if not (None in self.slots):
            raise NoFreeSlots("all slots are currently full")

        slot = self.slots.index(None) + 1

        self.logger.debug("add: creating new partnership in slot %d", slot)
        pship = Partnership(slot, generate_id(), generate_guid(), socket.gethostname(), name)

        for item in sync_items:
            self.logger.debug("add: adding synchronization item %s", item)
            pship.sync_items.append(item)

        # Create the synchronization config data source
        source = Characteristic(pship.guid)
        source["Name"] = pship.name
        source["Server"] = pship.hostname

        # StoreType
        #  2 = ActiveSync desktop
        #  3 = Exchange server
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

        self.logger.debug("add: setting synchronization data source \n%s", source)

        config_query_set(self.engine.rapi_session, "Sync.Sources", source)

        # Update the registry
        self.logger.debug("add: updating device registry")
        hklm = self.engine.rapi_session.HKEY_LOCAL_MACHINE

        partners_key = hklm.create_sub_key(r"Software\Microsoft\Windows CE Services\Partners")
        partners_key.set_value("PCur", pship.slot)

        key = partners_key.create_sub_key("P%d" % slot)
        key.set_value("PId", pship.id)
        key.set_value("DataSourceID", pship.guid)
        key.set_value("PName", pship.hostname)
        key.close()

        partners_key.close()

        # Store it
        self.logger.debug("add: storing partnership in slot %d", pship.slot)
        self.slots[pship.slot-1] = pship

        pship.create_state(pship.sync_items)

        if self.current is None:
            self.logger.debug("add: no current partnership, setting new partnership as current")
            self.set_current(pship)

        return pship

    def delete(self, pship):
        if not pship in self.slots:
            raise InvalidArgument("Invalid partnership %s" % pship)

        if self.current == pship:
            self.logger.debug("delete: partnership was set as current. removing state files")
            self.set_current(None)

        self.logger.debug("delete: removing partnership %s from device", pship)
        config_query_remove(self.engine.rapi_session, "Sync.Sources", pship.guid)

        self.logger.debug("delete: cleaning up partnership")
        pship.delete_state()
        self.slots[pship.slot-1] = None


class Partnership:
    def __init__(self, slot, id, guid, hostname, name):
        self.logger = logging.getLogger("engine.partnerships.Partnership")

        self.slot = slot
        self.id = id
        self.guid = guid
        self.hostname = hostname
        self.name = name

        self.sync_items = []
        self.state = None

        # FIXME: make these static
        self.config_dir = os.path.join(os.path.expanduser("~"), ".synce")
        self.state_path = os.path.join(self.config_dir, "sync_state_" + str(self.id))

        self.load_state()

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

    def has_state(self):
        return self.state != None

    def create_state(self, sync_items):
        self.state = SyncState(sync_items)
        self.save_state()

    def delete_state(self):
        self.logger.info("delete_state: removing %s", self.state_path)
        try:
            os.remove(self.state_path)
        except:
            pass

    def load_state(self):
        f = None
        try:
            f = open(self.state_path, "rb")
        except:
            return

        try:
            self.state = pickle.load(f)
        except Exception, e:
            self.logger.warning("load_sync_state: Failed to load state: %s", e)
            return

        self.logger.info("load_state: loaded state with %d items", len(self.state.items))

    def save_state(self):
        try:
            os.makedirs(self.config_dir)
        except OSError, e:
            if e.errno != 17:
                self.logger.error("save_state: Failed to create directory %s: %s", self.config_dir, e)
                return

        try:
            f = open(self.state_path, "wb")
            pickle.dump(self.state, f, pickle.HIGHEST_PROTOCOL)
        except Exception, e:
            self.logger.warning("save_state: Failed to save sync state: %s", e)

class SyncState:
    def __init__(self, sync_items):

        folder_items = (
            (0, "Inbox", 2),
            (0, "Drafts", 3),
            (0, "Deleted Items", 4),
            (0, "Sent Items", 5),
            (0, "Outbox", 6),
            (0, "Tasks", 7),
            (0, "Calendar", 8),
            (0, "Contacts", 9),
            (0, "Notes", 10),
            (0, "Journal", 11),
            (0, "Junk E-mail", 12),
        )

        self.folders = {}
        for item in folder_items:
            id = generate_guid()
            self.folders[id] = item

        self.items = {}
        for item in sync_items:
            self.items[item] = SyncItem(item)

        self.luid_to_guid = {}
        self.guid_to_luid = {}

    def __str__(self):
        s = "Items:\n"
        for key, value in self.items.items():
            s += "  ItemID = %s, Item = %s" % (key, value)

        s += "LUID -> GUID Mappings:\n"
        for key, value in self.luid_to_guid.items():
            s += "  LUID = %s, GUID = %s\n" % (key, value)

        s += "GUID -> LUID Mappings:\n"
        for key, value in self.guid_to_luid.items():
            s += "  GUID = %s, LUID = %s\n" % (key, value)

        return s

    def register_luid(self):
        luid = None
        guid = None
        while True:
            try:
                luid = generate_guid()
                guid = generate_opensync_guid()
                self._create_uid_mapping(luid, guid)
                break
            except ValueError:
                pass
        return luid, guid

    def register_guid(self, guid):
        luid = None
        while True:
            try:
                luid = generate_guid()
                self._create_uid_mapping(luid, guid)
                break
            except ValueError:
                pass
        return luid, guid

    def _create_uid_mapping(self, luid, guid):
        if self.luid_to_guid.has_key(luid):
            raise ValueError("luid already registered")
        if self.guid_to_luid.has_key(guid):
            raise ValueError("guid already registered")

        self.luid_to_guid[luid] = guid
        self.guid_to_luid[guid] = luid

    def get_luid_from_guid(self, guid):
        return self.guid_to_luid[guid]

    def get_guid_from_luid(self, luid):
        return self.luid_to_guid[luid]

class SyncItem:
    def __init__(self, type):
        self.type = type
        self.local_changes = {}
        self.remote_changes = {}

    def __str__(self):
        s = "Local Changes:\n"
        i = 0
        for key, value in self.local_changes.items():
            change_type, data = value
            s += "  %d: GUID = %s, Type = %s, Data = %s\n" % (i, key, change_type, data)
            i += 1

        s += "Remote Changes:\n"
        i = 0
        for key, value in self.remote_changes.items():
            change_type, data = value
            s += "  %d: GUID = %s, Type = %s, Data = %s\n" % (i, key, change_type, data)
            i += 1

        return s

    def add_local_change(self, guid, change_type, data=""):
        self.local_changes[guid] = (change_type, data)

    def add_remote_change(self, guid, change_type, data=""):
        if self.remote_changes.has_key(guid):
            # There was already a remote change for this item, so we need to
            # reconcile it here.  This is an exhaustive list of each of the
            # possible state transitions, along with the actions we take:
            #   ADDED    -> DELETED  : delete change record
            #   ADDED    -> ADDED    : impossible
            #   ADDED    -> MODIFIED : overwrite change record
            #   DELETED  -> DELETED  : impossible
            #   DELETED  -> ADDED    : impossible
            #   DELETED  -> MODIFIED : impossible
            #   MODIFIED -> DELETED  : delete change record
            #   MODIFIED -> ADDED    : impossible
            #   MODIFIED -> MODIFIED : overwrite change record
            cur_change_type, cur_data = self.remote_changes[guid]
            if cur_change_type in (CHANGE_ADDED, CHANGE_MODIFIED) and change_type == CHANGE_DELETED:
                del self.remote_changes[guid]
                return
            elif cur_change_type in (CHANGE_ADDED, CHANGE_MODIFIED) and change_type == CHANGE_MODIFIED:
                self.remote_changes[guid] = (change_type, data)
                return
            else:
                raise NotAvailable("Unhandled change state transition: %d -> %d" % (cur_change_type, change_type))

        # The change didn't exist already, so we add it.
        self.remote_changes[guid] = (change_type, data)

    def get_local_change_count(self):
        return len(self.local_changes)

    def get_remote_change_count(self):
        return len(self.remote_changes)

    def extract_local_changes(self, max):
        changeset = self.local_changes
        if len(changeset) <= max:
            self.local_changes = {}
            return changeset
        else:
            # FIXME: optimize this
            items = changeset.items()[:max]
            changeset = {}
            for key, value in items:
                del self.local_changes[key]
                changeset[key] = value
            return changeset

    def get_remote_changes(self):
        return self.remote_changes

    def ack_remote_change(self, guid):
        del self.remote_changes[guid]
