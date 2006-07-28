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

from constants import *
from util import *

import cPickle as pickle
import os
import os.path

class Partnership:
    def __init__(self, slot, id, guid, hostname, name):
        self.slot = slot
        self.id = id
        self.guid = guid
        self.hostname = hostname
        self.name = name

        self.sync_items = []
        self.state = None

        # FIXME: make these static
        self.config_dir = os.path.join(os.path.expanduser("~"),
                                       ".synce")
        self.sync_state_path = os.path.join(self.config_dir,
                                            "sync_state")

        self.load_sync_state()

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

    def is_our(self):
        return (self.state != None)

    def create_sync_state(self, sync_items):
        self.state = SyncState(sync_items)
        self.save_sync_state()

    def delete_sync_state(self):
        path = self.sync_state_path
        print "Removing %s" % path
        try:
            os.remove(path)
        except:
            pass

    def load_sync_state(self):
        f = None
        try:
            f = open(self.sync_state_path, "rb")
        except:
            return

        id = None
        try:
            id = pickle.load(f)
        except Exception, e:
            print "Failed to load sync_state [1]: %s" % e
            return

        if id != self.id:
            return

        try:
            self.state = pickle.load(f)
        except Exception, e:
            print "Failed to load sync state [2]: %s" % e
            return

        print "Loaded state:"
        print self.state.items
        #print "  Remote changesets:"
        #i = 0
        #for changeset in self.state.remote_changes:
        #    print "    [#%d] %d changes" % (i, len(changeset))
        #    i += 1

    def save_sync_state(self):
        try:
            os.makedirs(self.config_dir)
        except OSError, e:
            if e.errno != 17:
                print "Failed to create directory %s: %s" % (self.config_dir, e)
                return

        f = None
        try:
            f = open(self.sync_state_path, "wb")
            pickle.dump(self.id, f, pickle.HIGHEST_PROTOCOL)
            pickle.dump(self.state, f, pickle.HIGHEST_PROTOCOL)
        except Exception, e:
            print "Failed to save sync state:", e


class SyncState:
    def __init__(self, sync_items):
        items = (
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
        for item in items:
            id = generate_guid()
            self.folders[id] = item

        self.items = {}
        for item in sync_items:
            self.items[item] = SyncItem(item)

        self.luid_to_guid = {}
        self.guid_to_luid = {}

    def register_luid(self, luid):
        guid = generate_opensync_guid()
        self._create_uid_mapping(luid, guid)
        return guid

    def register_guid(self, guid, luid_gen_func):
        luid = luid_gen_func()
        self._create_uid_mapping(luid, guid)
        return luid

    def _create_uid_mapping(self, luid, guid):
        if self.luid_to_guid.has_key(luid):
            raise ValueError("luid already registered")
        if self.guid_to_luid.has_key(guid):
            raise ValueError("guid already registered")

        self.luid_to_guid[luid] = guid
        self.guid_to_luid[guid] = luid

        print "create_uid_mapping: registered %s <-> %s" % (guid, luid)

    def get_luid_from_guid(self, guid):
        return self.guid_to_luid[guid]

    def get_guid_from_luid(self, luid):
        return self.luid_to_guid[luid]

    def synchronized(self):
        for item in self.items.values():
            item.synchronized()


class SyncItem:
    def __init__(self, type):
        self.type = type
        self.local_changes = {}
        self.remote_changes = [ {}, ]

    def add_local_change(self, guid, change_type, data=""):
        self.local_changes[guid] = (change_type, data)

    def add_remote_change(self, guid, change_type, data=""):
        self.remote_changes[-1][guid] = (change_type, data)

    def synchronized(self):
        if len(self.remote_changes[-1]) > 0:
            self.remote_changes.append({})

    def get_change_counts(self):
        return (len(self.local_changes), len(self.remote_changes[0]))

    def grab_local_changes(self, max):
        changeset = self.local_changes
        if len(changeset) <= max:
            self.local_changes = {}
            return changeset
        else:
            # FIXME: optimize this
            changeset = {}
            items = changeset.items()[:max]
            for key, value in items:
                del self.local_changes[key]
                changeset[key] = value
            return changeset

    def get_remote_changes(self):
        return self.remote_changes[0]

    def ack_remote_change(self, guid):
        changeset = self.remote_changes[0]
        del changeset[guid]
        if len(changeset) == 0:
            del self.remote_changes[0]

