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
from protocol import BaseProtocol
from util import hexdump, encode_wstr
from twisted.internet.protocol import Factory
from twisted.internet import reactor
import struct

REMSYNC_PORT = 999


class RemSync(BaseProtocol):
    def send_command(self, type, arg, payload=None):
        data = struct.pack("<HH", arg, type)
        if payload != None:
            data += payload

        print "Sending RemSync command:"
        print hexdump(data)

        self.send_data(data)


class RemSyncServer(gobject.GObject, Factory):
    __gsignals__ = {
            "ready": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                      ()),
    }

    def __init__(self):
        self.__gobject_init__()

        self.protocol = RemSync
        self.client = None

        reactor.listenTCP(REMSYNC_PORT, self)

    def buildProtocol(self, addr):
        p = RemSync()

        p.connect("connected", self._client_connected_cb)
        p.connect("disconnected", self._client_disconnected_cb)
        p.factory = self

        return p

    def _client_connected_cb(self, client):
        print "_client_connected_cb"

        if self.client != None:
            print "Another client connected -- shouldn't happen"
            return

        self.client = client
        self.emit("ready")

    def _client_disconnected_cb(self, client):
        print "_client_disconnected_cb"
        if self.client == client:
            self.client = None

    def set_status(self, message):
        message_w = encode_wstr(message)
        self._send_command(0x01, len(message_w), message_w)

    def set_start_of_sync(self):
        self._send_command(0x5a, 0)

    def set_end_of_sync(self):
        self._send_command(0x5b, 0)

    def set_progress_range(self, min, max):
        data = struct.pack("<HH", min, max)
        self._send_command(0x5c, 0, data)

    def set_progress_value(self, value):
        self._send_command(0x5d, value)

    def _send_command(self, *args):
        if not self.is_ready():
            raise Exception("not ready")

        self.client.send_command(*args)

    def is_ready(self):
        return self.client != None

