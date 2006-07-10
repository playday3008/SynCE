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

from util import *
import cPickle as pickle
import os.path

class Partnership:
    def __init__(self, slot, id, guid, hostname, name):
        self.slot = slot
        self.id = id
        self.guid = guid
        self.hostname = hostname
        self.name = name

        self.sync_items = []
        self.sync_state = None

        # FIXME: make these static
        self.config_dir = os.path.join(os.path.expanduser("~"),
                                       ".synce")
        self.sync_state_path = os.path.join(self.config_dir,
                                            "sync_state")

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
        return (self.sync_state != None)

    def create_sync_state(self):
        self.sync_state = SyncState()
        self.save_sync_state()

    def delete_sync_state(self):
        path = self._get_state_path()
        print "Removing %s" % path
        os.path.remove(path)

    def load_sync_state(self):
        f = None
        try:
            f = open(self.sync_state_path, "r")
        except:
            return

        id = None
        try:
            id = pickle.load(f)
        except:
            return

        if id != self.id:
            return

        try:
            self.sync_state = pickle.load(f)
        except:
            print "Failed to load sync state"

    def save_sync_state(self):
        try:
            os.path.makedirs(self.config_dir)
        except:
            print "Failed to create configdir path \"%s\"" % self.config_dir
            return

        f = None
        try:
            f = open(self.sync_state_path, "w")
            pickle.dump(self.id, f)
            pickle.dump(self.sync_state, f)
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

