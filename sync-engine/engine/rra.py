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
import struct
from protocol import BaseProtocol
from util import hexdump, decode_wstr
from twisted.internet import reactor, defer
from twisted.internet.protocol import Factory
from errors import *

RRA_PORT = 5678

COMMAND_TYPE_GET_METADATA = 0x6F
COMMAND_TYPE_SET_METADATA = 0x70

GET_OBJECT_TYPES_MASK = 0x000007c1
GET_VOLUMES_MASK      = 0x00000010
GET_UNK_1AND2_MASK    = 0x00000006

OID_UNKNOWN_02    = 2
OID_BORING_SSPIDS = 3

SUB_PROTO_UNKNOWN = 0
SUB_PROTO_CONTROL = 1
SUB_PROTO_DATA    = 2

RRA_COMMANDS = {
    0x65: "Ack",
    0x66: "DeleteObject",
    0x67: "GetObject",
    0x69: "Notify",
    #0x69: "ChangeLog",
    0x6c: "Response",
    0x6e: "Nack",
    0x6f: "GetMetaData",
    0x70: "SetMetaData",
}

class Command:
    def __init__(self, type, arg, response_expected, payload=None):
        self.type = type

        payload_len = 0
        if payload is not None:
            payload_len = len(payload)

        self.data = struct.pack("<HHI", type, 4 + payload_len, arg)
        if payload is not None:
            self.data += payload

        self.response_expected = response_expected

    def __str__(self):
        return hexdump(self.data)

    def handle_response(self, response):
        return None


class GetMetaDataCmd(Command):
    MAGIC_VALUE = 0xf0000001

    def __init__(self, get_mask):
        Command.__init__(self, COMMAND_TYPE_GET_METADATA, get_mask, True)
        self.mask = get_mask

    def handle_response(self, response):
        magic_value, success, has_body = struct.unpack("<III", response[0:12])
        buf = response[12:]

        if magic_value != self.MAGIC_VALUE:
            raise ProtocolError("Magic value is %#010x, expected %#010x." % \
                (magic_value, self.MAGIC_VALUE))

        if success != 1:
            raise ProtocolError("Success isn't TRUE.")

        result = {}

        if has_body != 0:
            # ObjectTypes
            buf, records = self._fetch_next(buf, 0, 384, True)
            parsed = []
            for rec in records:
                flags = struct.unpack("<I", rec[0:4])
                name1 = decode_wstr(rec[4:204])
                name2 = decode_wstr(rec[204:284])
                name3 = decode_wstr(rec[284:364])
                ssp_id, count, total_size, file_time = \
                    struct.unpack("<IIIQ", rec[364:384])

                parsed.append((ssp_id, name1, count, total_size, file_time))

            if parsed:
                result["ObjectTypes"] = parsed

            # UnknownDataType1
            buf, records = self._fetch_next(buf, 1, 20, True)
            i = 0
            for rec in records:
                print "UnknownDataType1 record #%d:" % i
                print hexdump(rec)
                print
                i += 1

            # UnknownDataType2
            buf, records = self._fetch_next(buf, 2, 8, False)
            i = 0
            for rec in records:
                print "UnknownDataType2 record #%d:" % i
                print hexdump(rec)
                print
                i += 1

            # Volumes
            buf, records = self._fetch_next(buf, 4, 8, True)
            i = 0
            for rec in records:
                print "Volume record #%d:" % i
                print hexdump(rec)
                print
                i += 1

            if len(buf) > 0:
                print "Unhandled trailing data:"
                print hexdump(buf)

            # UnknownDataType4
            #buf, records = self._fetch_next(buf, 4, 8, False)
            #i = 0
            #for rec in records:
            #    print "Volume record #%d:" % i
            #    print hexdump(rec)
            #    print
            #    i += 1

        return result

    def _fetch_next(self, buf, bit, record_size, variable):
        records = []
        off = 0
        mask = pow(2, bit)

        if (self.mask & mask) != 0:
            response_to_bit = struct.unpack("<I", buf[off:off+4])[0]
            if response_to_bit != mask:
                raise ProtocolError("Response to bit is %d, expected %d" \
                        % (response_to_bit, mask))

            off += 4

            if variable:
                num_records = struct.unpack("<I", buf[off:off+4])[0]
                off += 4
            else:
                num_records = 1

            for i in xrange(0, num_records):
                records.append(buf[off:off+record_size])
                off += record_size

        return (buf[off:], records)


class SetMetaDataCmd(Command):
    MAGIC_VALUE = 0xf0000001

    def __init__(self, set_oid, data):
        body = struct.pack("<II", self.MAGIC_VALUE, set_oid)
        body += data
        Command.__init__(self, COMMAND_TYPE_SET_METADATA, len(body), True, body)


class RRA(BaseProtocol):
    def __init__(self):
        self.__gobject_init__()
        self.sub_proto = SUB_PROTO_UNKNOWN

    def set_sub_protocol(self, sub_proto):
        self.sub_proto = sub_proto

    def connectionMade(self):
        BaseProtocol.connectionMade(self)
        self.pending_responses = {}

    def handleIncoming(self, data):
        if self.sub_proto != SUB_PROTO_CONTROL:
            print "Ignoring incoming data because sub_proto=%d" % self.sub_proto
            return len(data)

        if len(data) < 4:
            return 0

        type, length = struct.unpack("<HH", data[0:4])
        full_length = 4 + length
        if len(data) < full_length:
            return 0

        if type in RRA_COMMANDS:
            cmd_name = RRA_COMMANDS[type]
            print "Received command: %s" % cmd_name
            name = "_handle_%s_command" % cmd_name.lower()
            if hasattr(self, name):
                handler = getattr(self, name)
                handler(data)
            else:
                print "Unhandled command: %s (looking for %s)" % (cmd_name, name)
        else:
            print "Received unknown command: %#04x" % type

        return full_length

    def _handle_response_command(self, buf):
        reply_to, result, data_size, has_data = struct.unpack("<IIII", buf[4:20])

        print "Dispatching response:"
        print "  ReplyToCommand: %#04x" % reply_to
        print "  Result: %d" % result
        print "  ResponseDataSize: %d" % data_size
        print "  HasResponseData: %d" % has_data
        print

        if not reply_to in self.pending_responses:
            print "Ignoring reply to command %#04x to which no response is currently expected" % reply_to
            print
            return

        deferred = self.pending_responses[reply_to]
        del self.pending_responses[reply_to]

        if result == 0:
            deferred.callback(buf[20:])
        else:
            deferred.errback(Exception("command failed with result %#010x" % result))

    def send_command(self, cmd):
        deferred = defer.Deferred()

        if cmd.response_expected:
            if cmd.type in self.pending_responses:
                print "already expecting a response to %#04x" % cmd.type
                return

            deferred.addCallback(cmd.handle_response)

            self.pending_responses[cmd.type] = deferred

        self.send_data(cmd.data)

        if not cmd.response_expected:
            deferred.callback(None)

        return deferred


class RRAServer(gobject.GObject, Factory):
    __gsignals__ = {
            "ready": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                      ()),
    }

    def __init__(self):
        self.__gobject_init__()

        self.protocol = RRA
        self.clients = []

        reactor.listenTCP(RRA_PORT, self)

    def buildProtocol(self, addr):
        p = RRA()

        p.connect("connected", self._client_connected_cb)
        p.connect("disconnected", self._client_disconnected_cb)
        p.factory = self

        return p

    def _client_connected_cb(self, client):
        print "_client_connected_cb"

        if len(self.clients) == 2:
            print "A third client connected -- shouldn't happen"
            return

        self.clients.append(client)

        count = len(self.clients)
        if count == 1:
            client.set_sub_protocol(SUB_PROTO_CONTROL)
        elif count == 2:
            client.set_sub_protocol(SUB_PROTO_DATA)
            self.emit("ready")

    def _client_disconnected_cb(self, client):
        print "_client_disconnected_cb"
        self.clients.remove(client)

    def get_object_types(self):
        cmd = GetMetaDataCmd(GET_OBJECT_TYPES_MASK)
        return self._send_command(cmd)

    def set_boring_ssp_ids(self, ids):
        data = struct.pack("<IIIII", 0, 0, 0, 0, len(ids))
        for id in ids:
            data += struct.pack("<I", id)
        cmd = SetMetaDataCmd(OID_BORING_SSPIDS, data)
        return self._send_command(cmd)

    def get_volumes(self):
        cmd = GetMetaDataCmd(GET_VOLUMES_MASK)
        return self._send_command(cmd)

    def get_unknown_1_and_2(self):
        cmd = GetMetaDataCmd(GET_UNK_1AND2_MASK)
        return self._send_command(cmd)

    def set_unknown_02(self, x, y):
        data = ""

        # 50 unknown DWORDs (or 200 bytes), all zero
        for i in xrange(50):
            data += struct.pack("<I", 0)

        # Then two DWORDs with the values x and y, respectively
        data += struct.pack("<I", x)
        data += struct.pack("<I", y)

        # And 4 records of 6 bytes each, all zero
        for i in xrange(4):
            data += struct.pack("<I", 0)
            data += struct.pack("<H", 0)

        cmd = SetMetaDataCmd(OID_UNKNOWN_02, data)
        return self._send_command(cmd)

    def is_ready(self):
        return len(self.clients) == 2

    def _send_command(self, cmd):
        if not self.is_ready():
            raise Exception("not ready")

        print "Sending command:"
        print cmd

        return self.clients[0].send_command(cmd)

