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

import socket
import thread
import struct

DTPT_PORT = 5721

class DTPTServer:
    def __init__(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind(("0.0.0.0", DTPT_PORT))
        s.listen(5)
        self._sock = s
    
    def run(self):
        self._running = True
        while self._running:
            (s, address) = self._sock.accept()
            thread.start_new_thread(self._handle_client, (s,))
    
    def _handle_client(self, s):
        print "handling client"
        buf = s.recv(2, socket.MSG_PEEK | socket.MSG_WAITALL)
        print "%02x %02x" % (ord(buf[0]), ord(buf[1]))
        if ord(buf[0]) != 1:
            print "invalid protocol"
        if ord(buf[1]) == 9:
            self._handle_nsp_session(s)
        elif ord(buf[1]) == 1:
            self._handle_connection_session(s)
        else:
            print "unknown session type"
        return

    def _handle_nsp_session(self, s):
        print "NSPSession"
        while self._running:
            buf = s.recv(20, socket.MSG_WAITALL)
            if len(buf) != 20:
                print "_handle_nsp_session: read %d bytes, expected %d" % (len(buf), 20)
            ver, type, qv, dv1, dv2 = struct.unpack("<BBxxQII", buf)
            if ver != 1:
                raise ValueError("ver=%d, expected 1" % ver)
            
            if type == 9:
                self._handle_nsp_lookup_begin(s, qv, dv1, dv2)
            else:
                print "unhandled type=%d" % type

    def _handle_nsp_lookup_begin(self, s, q_value, control_flags, payload_size):
        print "_handle_nsp_lookup_begin: flags=0x%08x, payloadSize=%d" % (control_flags, payload_size)
        buf = s.recv(payload_size, socket.MSG_WAITALL)
        if len(buf) != payload_size:
            print "_handle_nsp_lookup_begin: read %d bytes, expected %d" % (len(buf), payload_size)
            return
        open("/tmp/request.bin", "w").write(buf)
        qs = QuerySet()
        qs.initialize(buf)

    def _handle_connection_session(self, s):
        print "ConnectionSession"


class RPCStream:
    def __init__(self, data=""):
        self.initialize(data, 0)

    def initialize(self, data):
        self._data = data
        self._offset = 0
        
    def get_data(self):
        return self._data
    
    def get_offset(self):
        return self._offset
    
    def get_length(self):
        return len(self._data)

    def read_dword(self):
        dw = struct.unpack("<I", self._data[self._offset:self._offset + 4])[0]
        self._offset += 4
        return dw
    
    def write_dword(self, dw):
        self._data += struct.pack("<I", dw)
    
    def read_n_bytes(self, n):
        buf = self._data[self._offset:self._offset + n]
        if len(buf) % 4 != 0:
            align = 4 - (len(buf) % 4)
        else:
            align = 0
        self._offset += n + align
        return buf
    
    def write_bytes(self, data):
        self._data += data
        if len(data) % 4 != 0:
            align = 4 - (len(data) % 4)
            for i in xrange(align):
                self._data += "\x00"

    def read_field(self, expected_size=-1):
        size = self.read_dword()
        if size == 0:
            return None
        data = self.read_n_bytes(size)
        if expected_size != -1 and len(data) != expected_size:
            raise ValueError("expected_size=%d, size=%d" % (expected_size, len(data)))
        return data
    
    def write_field(self, data):
        self.write_dword(len(data))
        self.write_bytes(data)

    def read_string(self):
        s = self.read_field()
        if s == None:
            return s
        #open("/tmp/woot.bin", "wb").write(s)
        return s.decode("utf_16_le").rstrip("\0")

    def write_string(self, s):
        if s != None:
            self.write_field(s.encode("utf_16_le") + "\x00\x00")
        else:
            self.write_dword(0)

    def read_guid(self):
        data = self.read_field(16)
        if data == None:
            return data

        v1, v2, v3 = struct.unpack("<LHH", data[0:8])        
        return "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}" % \
            (v1, v2, v3, ord(data[8]), ord(data[9]), ord(data[10]),
             ord(data[11]), ord(data[12]), ord(data[13]), ord(data[14]),
             ord(data[15]))

    def write_guid(self, guid):
        if guid != None:
            data = struct.pack("<LHHBBBBBBBB",
                               int(guid[1:9], 16),
                               int(guid[10:14], 16),
                               int(guid[15:19], 16),
                               int(guid[20:22], 16),
                               int(guid[22:24], 16),
                               int(guid[24:26], 16),
                               int(guid[26:28], 16),
                               int(guid[28:30], 16),
                               int(guid[30:32], 16),
                               int(guid[32:34], 16),
                               int(guid[34:36], 16))
            self.write_field(data)
        else:
            self.write_dword(0)


SVCID_INET_HOSTADDRBYNAME = "{0002A803-0000-0000-C000-000000000046}"

class QuerySet:
    def __init__(self):
        self.service_instance_name = None
        self.service_class_id = None
        self.version = None
        self.comment = None
        self.namespace = None
        self.ns_provider_id = None
        self.context = None
        self.num_protocols = 0
        self.query_string = None
        self.num_cs_addrs = 0
        self.output_flags = 0
        self.blob_size = 0

    def unserialize(self, data):
        s = RPCStream(data)

        raw_flat = s.read_field(60)
        size = struct.unpack("<I", raw_flat[0:4])[0]
        assert size == 60

        self.service_instance_name = s.read_string()
        self.service_class_id = s.read_guid()
        self.version = struct.unpack("<L", raw_flat[12:16])[0]
        self.comment = s.read_string()
        self.namespace = struct.unpack("<L", raw_flat[20:24])[0]
        self.ns_provider_id = s.read_guid()
        self.context = s.read_string()
        self.num_protocols = s.read_dword()
        if self.num_protocols > 0:
            print "FIXME1"
        self.query_string = s.read_string()
        self.num_cs_addrs = s.read_dword()
        if self.num_cs_addrs > 0:
            print "FIXME2"
        self.output_flags = struct.unpack("<L", raw_flat[52:56])[0]
        self.blob_size = s.read_dword()
        
        assert s.get_offset() == s.get_length()

    def serialize(self):
        s = RPCStream()

        dummy_ptr = 0xDEADBEEF

        if self.service_instance_name != None: svc_name_ptr = dummy_ptr
        else: svc_name_ptr = 0

        if self.service_class_id != None: svc_cls_id_ptr = dummy_ptr
        else: svc_cls_id_ptr = 0
        
        if self.comment != None: comment_ptr = dummy_ptr
        else: comment_ptr = 0

        if self.ns_provider_id != None: ns_provider_id_ptr = dummy_ptr
        else: ns_provider_id_ptr = 0

        if self.context != None: context_ptr = dummy_ptr
        else: context_ptr = 0

        if self.num_protocols > 0: protocols_ptr = dummy_ptr
        else: protocols_ptr = 0

        if self.query_string != None: query_string_ptr = dummy_ptr
        else: query_string_ptr = 0

        if self.num_cs_addrs > 0: cs_addrs_ptr = dummy_ptr
        else: cs_addrs_ptr = 0

        if self.blob_size > 0: blob_ptr = dummy_ptr
        else: blob_ptr = 0

        buf = struct.pack("<LLLLLLLLLLLLLLL",
                          60,
                          svc_name_ptr,
                          svc_cls_id_ptr,
                          self.version,
                          comment_ptr,
                          self.namespace,
                          ns_provider_id_ptr,
                          context_ptr,
                          self.num_protocols,
                          protocols_ptr,
                          query_string_ptr,
                          self.num_cs_addrs,
                          cs_addrs_ptr,
                          self.output_flags,
                          blob_ptr)
        s.write_bytes(buf)

        s.write_string(self.service_instance_name)
        s.write_guid(self.service_class_id)
        s.write_string(self.comment)
        s.write_guid(self.ns_provider_id)
        s.write_string(self.context)
        s.write_dword(0) # FIXME: WSAQUERYSET.dwNumberOfProtocols
        s.write_string(self.query_string)
        s.write_dword(0) # FIXME: WSAQUERYSET.dwNumberOfCsAddrs
        s.write_dword(0) # FIXME: WSAQUERYSET.lpBlob

        return s.get_data()


if __name__ == "__main__":
    srv = DTPTServer()
    srv.run()