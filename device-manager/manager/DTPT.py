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
        #open("/tmp/request.bin", "w").write(buf)
        qs = QuerySet()
        qs.initialize(buf)

    def _handle_connection_session(self, s):
        print "ConnectionSession"

class QuerySet:
    def __init__(self):
        pass

    def initialize(self, data):
        self._data = data
        self._offset = 0
        
        raw_flat = self.read_field(60)
        print "ServiceInstanceName: '%s'" % self.read_string()
        
    def read_dword(self):
        dw = struct.unpack("<I", self._data[self._offset:self._offset + 4])[0]
        self._offset += 4
        return dw
    
    def read_n_bytes(self, n):
        buf = self._data[self._offset:self._offset + n]
        if len(buf) % 4 != 0:
            align = 4 - (len(buf) % 4)
        else:
            align = 0
        self._offset += n + align
        return buf
    
    def read_field(self, expected_size=-1):
        size = self.read_dword()
        if size == 0:
            return None
        data = self.read_n_bytes(size)
        if expected_size != -1 and len(data) != expected_size:
            raise ValueError("expected_size=%d, size=%d" % (expected_size, len(data)))
        return data

    def read_string(self):
        s = self.read_field()
        if s == None:
            return s
        #open("/tmp/woot.bin", "wb").write(s)
        return s.decode("utf_16_le").rstrip("\0")

if __name__ == "__main__":
    srv = DTPTServer()
    srv.run()