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
from twisted.internet.protocol import Protocol
from util import hexdump

class BaseProtocol(gobject.GObject, Protocol):
    __gsignals__ = {
            "connected": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                          ()),
            "disconnected": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                             ()),
    }

    def __init__(self):
        self.__gobject_init__()

    def connectionMade(self):
        print "%s.connectionMade: client connected on %s" % \
            (self.__class__.__name__, self.transport)
        self.recv_cache = ""
        self.emit("connected")

    def connectionLost(self, reason):
        print "%s.connectionLost: connection lost because: %s" % \
            (self.__class__.__name__, reason)
        self.emit("disconnected")

    def dataReceived(self, data):
        print "%s.dataReceived: got %d bytes of data" % \
            (self.__class__.__name__, len(data))
        print hexdump(data)
        print

        self.recv_cache += data

        while len(self.recv_cache) > 0:
            consumed = self.handleIncoming(self.recv_cache)
            if consumed > 0:
                self.recv_cache = self.recv_cache[consumed:]
            else:
                break

    def handleIncoming(self, data):
        return len(data)

    def send_data(self, data):
        self.transport.write(data)
