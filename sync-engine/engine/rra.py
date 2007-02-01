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

import gobject
import struct

from xml.dom import minidom
from xml import xpath

import SocketServer
import threading
import logging
import time
import select

from util import *
from errors import *
from constants import *

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

class ServerStopped(Exception):
    pass

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
        self.logger = logging.getLogger("engine.rra.GetMetaDataCmd")
        self.mask = get_mask

    def handle_response(self, response):
        magic_value, success, has_body = struct.unpack("<III", response[0:12])
        buf = response[12:]

        if magic_value != self.MAGIC_VALUE:
            raise ProtocolError("Magic value is %#010x, expected %#010x." % (magic_value, self.MAGIC_VALUE))

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
                ssp_id, count, total_size, file_time = struct.unpack("<IIIQ", rec[364:384])

                parsed.append((ssp_id, name1, count, total_size, file_time))

            if parsed:
                result["ObjectTypes"] = parsed

            # UnknownDataType1
            buf, records = self._fetch_next(buf, 1, 20, True)
            i = 0
            for rec in records:
                self.logger.info("handle_response: UnknownDataType1 record #%d, HexDump: \n%s:", i, hexdump(rec))
                i += 1

            # UnknownDataType2
            buf, records = self._fetch_next(buf, 2, 8, False)
            i = 0
            for rec in records:
                self.logger.info("handle_response: UnknownDataType2 record #%d, HexDump: \n%s:", i, hexdump(rec))
                i += 1

            # Volumes
            buf, records = self._fetch_next(buf, 4, 8, True)
            i = 0
            for rec in records:
                self.logger.info("handle_response: Volume record #%d, HexDump: \n%s:", i, hexdump(rec))
                i += 1

            if len(buf) > 0:
                self.logger.info("handle_response: Unhandled trailing data: HexDump: \n%s:", hexdump(rec))

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
                raise ProtocolError("Response to bit is %d, expected %d" % (response_to_bit, mask))

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


class RRAHandler(SocketServer.BaseRequestHandler):

    def setup(self):
        if len(self.server.clients) >= 2:
            self.server.logger.error("Only 2 RRA clients allowed")
            raise Exception("Only 2 RRA clients allowed")

        if len(self.server.clients) == 0:
            self.sub_proto = SUB_PROTO_CONTROL
        else:
            self.sub_proto = SUB_PROTO_DATA

        self.server.logger.debug("setup: client %s:%d connected" % self.client_address)
        self.server.clients.append(self)

        self.stopped = False

        self.recv_cache = ""

    def handle(self):
        while True:
            ready = select.select([self.request], [], [], 1)
            if self.stopped:
                return
            if ready[0]:
                try:
                    self.recv_cache += self.request.recv(1024)
                except Exception, e:
                    self.server.logger.info("handle: exception while receiving from peer %s", e)
                    return

                while len(self.recv_cache) > 0:
                    consumed = self.handleIncoming(self.recv_cache)
                    if consumed > 0:
                        self.recv_cache = self.recv_cache[consumed:]
                    else:
                        break

    def finish(self):
        self.server.logger.debug("finish: client %s:%d disconnecting" % self.client_address)
        self.server.clients.remove(self)
        self.request.close()

    def stop(self):
        self.stopped = True

    def handleIncoming(self, data):
        if self.sub_proto != SUB_PROTO_CONTROL:
            self.server.logger.info("handleIncoming: Ignoring incoming data because sub_proto=%d", self.sub_proto)
            return len(data)

        if len(data) < 4:
            return 0

        type, length = struct.unpack("<HH", data[0:4])
        full_length = 4 + length
        if len(data) < full_length:
            return 0

        if type in RRA_COMMANDS:
            cmd_name = RRA_COMMANDS[type]
            self.server.logger.debug("handleIncoming: Received command: %s", cmd_name)
            name = "_handle_%s_command" % cmd_name.lower()
            if hasattr(self, name):
                handler = getattr(self, name)
                handler(data)
            else:
                self.server.logger.warning("handleIncoming: Unhandled command: %s (looking for method %s)", cmd_name, name)
        else:
            self.server.logger.warning("handleIncoming: Received unknown command: %#04x", type)

        return full_length

    def _handle_response_command(self, buf):
        reply_to, result, data_size, has_data = struct.unpack("<IIII", buf[4:20])

        self.server.logger.debug("_handle_response_command: Dispatching response: ReplyToCommand = %#04x, Result: %d, ResponseDataSize: %d, HasResponseData: %d",
            reply_to, result, data_size, has_data)

        if not reply_to in self.pending_responses:
            self.server.logger.debug("_handle_response_command: Ignoring reply to command %#04x to which no response is currently expected", reply_to)
            return

        cmd = self.pending_responses[reply_to]
        del self.pending_responses[reply_to]

        if result == 0:
            cmd.handle_response(buf[20:])
        else:
            Exception("command failed with result %#010x" % result)

    def send_command(self, cmd):
        if cmd.response_expected:
            if cmd.type in self.pending_responses:
                self.server.logger.warning("send_command: already expecting a response to %#04x", cmd.type)
                return
            self.pending_responses[cmd.type] = cmd
        self.request.sendall(cmd.data)

class RRAServer(SocketServer.ThreadingTCPServer):

    allow_reuse_address = True

    def __init__(self, server_address, RequestHandlerClass, thread, engine):
        SocketServer.ThreadingTCPServer.__init__(self, server_address, RequestHandlerClass)
        self.logger = logging.getLogger("engine.rra.RRAServer")

        self.thread = thread
        self.engine = engine

        self.stopped = False

        self.clients = []

    def get_request(self):
        while True:
            ready = select.select([self.socket], [], [], 1)
            if self.stopped:
                raise ServerStopped()
            if ready[0]:
                return SocketServer.ThreadingTCPServer.get_request(self)
            else:
                pass

    def serve_forever(self):
        try:
            SocketServer.ThreadingTCPServer.serve_forever(self)
        except ServerStopped:
            pass
        for client in self.clients:
            client.stop()
        self.server_close()

    def stop_server(self):
        self.stopped = True

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

        self.logger.info("_send_command: Command = %s", cmd)

        return self.clients[0].send_command(cmd)



class RRAThread(gobject.GObject, threading.Thread):
    def __init__(self, engine):
        self.__gobject_init__()
        threading.Thread.__init__(self)
        self.logger = logging.getLogger("engine.rra.RRAThread")

        self.engine = engine
        self.server = RRAServer(("", RRA_PORT), RRAHandler, self, engine)

    def stop(self):
        self.logger.debug("stop: stopping RRA server")
        self.server.stop_server()

    def run(self):
        self.logger.debug("run: listening for RRA connections")
        self.server.serve_forever()
