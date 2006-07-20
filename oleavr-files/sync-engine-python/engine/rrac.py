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
from util import hexdump, encode_wstr, decode_wstr
from twisted.internet import reactor, defer
from twisted.internet.protocol import Protocol, Factory
from errors import *


RRAC_PORT = 5678
STATUS_PORT = 999

COMMAND_TYPE_GET_METADATA = 0x6F
COMMAND_TYPE_SET_METADATA = 0x70

GET_OBJECT_TYPES_MASK = 0x000007c1
GET_VOLUMES_MASK      = 0x00000010
GET_UNK_1AND2_MASK    = 0x00000006

OID_BORING_SSPIDS = 3

SUB_PROTO_UNKNOWN = 0
SUB_PROTO_CONTROL = 1
SUB_PROTO_DATA    = 2
SUB_PROTO_STATUS  = 3

RRAC_COMMANDS = {
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


class BaseProtocol(Protocol):
    def connectionMade(self):
        print "%s.connectionMade: client connected on %s" % \
            (self.__class__.__name__, self.transport)

    def connectionLost(self, reason):
        print "%s.connectionLost: connection lost because: %s" % \
            (self.__class__.__name__, reason)

    def dataReceived(self, data):
        print "%s.dataReceived: got %d bytes of data" % \
            (self.__class__.__name__, len(data))
        print hexdump(data)
        print


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


class RRAC(gobject.GObject, BaseProtocol):
    __gsignals__ = {
            "connected": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                          ()),
            "disconnected": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                             ()),
            "data-exchanged": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                               ()),
    }

    def __init__(self):
        self.__gobject_init__()
        self.activity = False
        self.sub_proto = SUB_PROTO_UNKNOWN

    def set_sub_protocol(self, sub_proto):
        print "%s: sub_proto set to %s" % (self, sub_proto)
        self.sub_proto = sub_proto

    def _activity(self):
        if not self.activity:
            self.activity = True
            self.emit("data-exchanged")

    def connectionMade(self):
        BaseProtocol.connectionMade(self)
        self.emit("connected")
        self.recv_cache = ""
        self.pending_responses = {}

    def connectionLost(self, reason):
        BaseProtocol.connectionLost(self, reason)
        self.emit("disconnected")

    def dataReceived(self, data):
        BaseProtocol.dataReceived(self, data)
        self._activity()

        if self.sub_proto != SUB_PROTO_CONTROL:
            print "Ignoring incoming data because sub_proto=%d" % self.sub_proto
            return

        self.recv_cache += data

        buf = self.recv_cache
        if len(buf) < 4:
            return

        type, length = struct.unpack("<HH", buf[0:4])
        full_length = 4 + length
        if len(buf) < full_length:
            return

        self.recv_cache = buf[full_length:]
        buf = buf[:full_length]

        if not type in RRAC_COMMANDS:
            print "Received unknown command: %#04x" % type
            return

        cmd_name = RRAC_COMMANDS[type]
        print "Received command: %s" % cmd_name
        name = "_handle_%s_command" % cmd_name.lower()
        if hasattr(self, name):
            handler = getattr(self, name)
            handler(buf)
        else:
            print "Unhandled command: %s (looking for %s)" % (cmd_name, name)

    def _handle_response_command(self, buf):
        reply_to, result, data_size, has_data = struct.unpack("<IIII", buf[4:20])

        print "Dispatching response:"
        print "  ReplyToCommand: %#04x" % reply_to
        print "  Result: %d" % result
        print "  ResponseDataSize: %d" % data_size
        print "  HasResponseData: %d" % has_data

        if not reply_to in self.pending_responses:
            print "Ignoring reply to command %#04x to which no response is currently expected" % reply_to
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

    def send_data(self, data):
        self._activity()
        self.transport.write(data)


class RRACServer(gobject.GObject, Factory):
    __gsignals__ = {
            "ready": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                      ()),
    }

    def __init__(self):
        self.__gobject_init__()

        self.protocol = RRAC

        self.ready = False

        self.rrac_channels = []
        self.control_chan = None
        self.data_chan = None
        self.status_chan = None

        reactor.listenTCP(STATUS_PORT, self)
        reactor.listenTCP(RRAC_PORT, self)

    def buildProtocol(self, addr):
        p = RRAC()

        if addr.port == STATUS_PORT:
            p.set_sub_protocol(SUB_PROTO_STATUS)

        p.connect("connected", self._client_connected_cb)
        p.connect("disconnected", self._client_disconnected_cb)
        p.connect("data-exchanged", self._client_data_exchanged_cb)
        p.factory = self

        return p

    def _client_connected_cb(self, client):
        print "_client_connected_cb"
        host = client.transport.getHost()
        if host.port != RRAC_PORT:
            self.status_chan = client
            return

        self.rrac_channels.append(client)

        if len(self.rrac_channels) == 2:
            self.ready = True
            self.emit("ready")

    def _client_disconnected_cb(self, client):
        print "_client_disconnected_cb"
        if client in self.rrac_channels:
            self.rrac_channels.remove(client)
        self.ready = False

    def _client_data_exchanged_cb(self, client):
        print "_client_data_exchanged_cb"

        host = client.transport.getHost()
        if host.port != RRAC_PORT:
            return

        if self.control_chan == None:
            self.control_chan = client
            client.set_sub_protocol(SUB_PROTO_CONTROL)

        if self.data_chan == None:
            for chan in self.rrac_channels:
                if chan != self.control_chan:
                    self.data_chan = client
                    chan.set_sub_protocol(SUB_PROTO_DATA)

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

    def set_status_message(self, message):
        message_w = encode_wstr(message)

        self._send_status_notification(0x01, len(message_w), message_w)

    def set_status_5a(self, arg):
        self._send_status_notification(0x5a, arg)

    def set_status_5b(self, arg):
        self._send_status_notification(0x5b, arg)

    def set_status_5c(self, arg, x, y):
        payload = struct.pack("<HH", x, y)
        self._send_status_notification(0x5c, arg, payload)

    def set_status_5d(self, arg):
        self._send_status_notification(0x5d, arg)

    def _send_status_notification(self, type, arg, payload=None):
        if self.status_chan == None:
            raise Exception("status client not connected")

        data = struct.pack("<HH", arg, type)
        if payload != None:
            data += payload

        print "Sending status message:"
        print hexdump(data)

        self.status_chan.send_data(data)

    def _send_command(self, cmd):
        if not self.ready:
            raise Exception("not ready")

        if self.control_chan == None:
            self.control_chan = self.rrac_channels[0]
            self.control_chan.set_sub_protocol(SUB_PROTO_CONTROL)

        if self.data_chan == None:
            for chan in self.rrac_channels:
                if chan != self.control_chan:
                    self.data_chan = chan
                    chan.set_sub_protocol(SUB_PROTO_DATA)

        print "Sending command:"
        print cmd

        return self.control_chan.send_command(cmd)

