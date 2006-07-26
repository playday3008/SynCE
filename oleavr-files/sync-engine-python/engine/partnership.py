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

    def create_sync_state(self):
        self.state = SyncState()
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
            f = open(self.sync_state_path, "r")
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
        print "  Remote changesets:"
        i = 0
        for changeset in self.state.remote_changes:
            print "    [#%d] %d changes" % (i, len(changeset))
            i += 1

    def save_sync_state(self):
        try:
            os.makedirs(self.config_dir)
        except OSError, e:
            if e.errno != 17:
                print "Failed to create directory %s: %s" % (self.config_dir, e)
                return

        f = None
        try:
            f = open(self.sync_state_path, "w")
            pickle.dump(self.id, f)
            pickle.dump(self.state, f)
        except Exception, e:
            print "Failed to save sync state:", e


class SyncState:
    def __init__(self):
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

        self.local_changes = [ {}, ]
        self.remote_changes = [ {}, ]

    def add_local_change(self, sid, change_type, item_type, data=""):
        self.local_changes[-1][sid] = (change_type, item_type, data)

    def add_remote_change(self, sid, change_type, item_type, data=""):
        self.remote_changes[-1][sid] = (change_type, item_type, data)

    def shift_changesets(self):
        if len(self.local_changes[-1]) > 0:
            self.local_changes.append({})
        if len(self.remote_changes[-1]) > 0:
            self.remote_changes.append({})

    def get_remote_changes(self):
        changeset = self.remote_changes[0]
        return changeset

    def ack_remote_change(self, sid):
        changeset = self.remote_changes[0]
        del changeset[sid]
        if len(changeset) == 0:
            del self.remote_changes[0]

