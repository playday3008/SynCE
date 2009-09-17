# -*- coding: utf-8 -*-
###############################################################################
# DTPTSERVER.py
#
# Adapted from Ole's original stand-alone DTPT server, this version is started
# by sync-engine if both the global config and the partnership config allow
# it.
#
# Original DTPT.py (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>
#
# Dr J A Gow 28/11/2007
#
###############################################################################

import socket
import thread
import threading
import struct
import random
import select
import os
import logging
import util

DTPT_PORT = 5721
DUMMY_PTR = 0xDEADBEEF
SVCID_INET_HOSTADDRBYNAME = "{0002A803-0000-0000-C000-000000000046}"

WSAEFAULT = 10014
WSAHOST_NOT_FOUND = 11001

logger = logging.getLogger("engine.syncengine.dtptserver")


#
# DTPTServer
#
# The DTPTServer is now a threaded server that we can actually stop cleanly
# when the sync-engine is stopped. We track all spun off server threads and
# ensure that all is stopped before exitting.
#

class DTPTServer(threading.Thread):
	
	def __init__(self, bindaddr):
		
		threading.Thread.__init__(self)
		
        	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        	s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        	s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        	s.bind((bindaddr, DTPT_PORT))
        	s.listen(5)
        	self._sock = s
		self.threads = {}
		self.threadlock = threading.Lock()
		self.pr,self.pw = os.pipe()
		self.evtShutdown = threading.Event()

	def AddServer(self,tid,th):
		self.threadlock.acquire()
		self.evtShutdown.clear()
		rc = len(self.threads)
		self.threads[tid]=th
		self.threadlock.release()
		return rc
		
	def RemoveServer(self,tid):
		logger.info("removing thread")
		self.threadlock.acquire()
		del self.threads[tid]
		if len(self.threads)==0:
			self.evtShutdown.set()
		self.threadlock.release()

	def _CloseAllThreads(self):
		self.threadlock.acquire()
		for tid,thd in self.threads.items():
			os.write(thd.pw,"stop")
			thd.s.shutdown(0)
			thd.s.close()
		self.threadlock.release()

	def run(self):
        	self._running = True
		ip = [self._sock, self.pr]
		self.evtShutdown.set()
        	while self._running:
			
			irdy,ordy,xptrdy = select.select(ip,[],[])
			
			for sck in irdy:
				
				if sck == self._sock:
				
					try:
						(s, address) = self._sock.accept()
						s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

						tid = util.generate_id()
						thd=DTPTThread(s,self,tid)
						ix = self.AddServer(tid,thd)
						thd.start()
						
					except:
						self._running=0
						return
					
				elif sck == self.pr:
					
					dump=os.read(self.pr,4)
					logger.info("DTPT server shutdown request actioned")
					self._running = 0
				
				else:
					continue

		self._sock.close()
		logger.info("shutting down DTPT threads")
		self._CloseAllThreads()
		logger.info("waiting for final shutdown")
		self.evtShutdown.wait()
		os.close(self.pr)
		os.close(self.pw)

	def shutdown(self):
		try:
			os.write(self.pw,"stop")
			logger.info("DTPT server shutdown request posted")
		except Exception,e:
			logger.info("error shutting down DTPT server: %s" % e)
			pass

#
# DTPTThread
#
# Represents individual server threads which can be one of two types.
# This is the bottom thread layer of the threaded architecture.
#

class DTPTThread(threading.Thread):
	
	def __init__(self, socket,handler,tid):
		threading.Thread.__init__(self)
		self.s = socket
		self.handler=handler
		self.tid = tid
		self.pr,self.pw = os.pipe()
			
			
	def run(self):
			
		ip = [self.s, self.pr]

		irdy,ordy,xptrdy = select.select(ip,[],[])
		
		for fd in irdy:
			
			if fd==self.s:

				buf = self.s.recv(2, socket.MSG_PEEK | socket.MSG_WAITALL)
				if len(buf) > 0:
			
					if ord(buf[0]) != 1:
						logger.debug("invalid DTPT protocol")
					else:	
						session = None
						if ord(buf[1]) == 9:
							 NSPSession(self.s,self.pr).run()
						elif ord(buf[1]) == 1:
							ConnectionSession(self.s,self.pr).run()
						else:
							logger.debug("unknown session type %d" % ord(buf[1]))

        	self.s.close()
		self.handler.RemoveServer(self.tid)



class NSPSession:

	def __init__(self, s, pr):
		
        	self.socket = s
		self.pr = pr
        	self.initialize()
		self.ip = [self.socket, self.pr]
        
	def initialize(self):
		
        	self.handle = None
        	self.request = None
        	self.response = None

	def run(self):
		
		logger.info("starting NSP session")

        	while True:

			irdy,ordy,xptrdy = select.select(self.ip,[],[])

			for sc in irdy:
				
				if sc == self.socket:

					buf = self.socket.recv(20, socket.MSG_WAITALL)
        				if len(buf) != 20:
						if len(buf) != 0:
							logger.error("NSPSession: read %d bytes, expected %d - exitting" % (len(buf), 20))
						return

					ver, type, qv, dv1, dv2 = struct.unpack("<BBxxQII", buf)
					if ver != 1:
                				raise ValueError("ver=%d, expected 1" % ver)

					logger.info("dispatching %d %d %d %d %d" % (ver,type,qv,dv1,dv2))

					if type == 9:
						self._handle_lookup_begin(qv, dv1, dv2)
					elif type == 11:
						self._handle_lookup_next(qv, dv1, dv2)
					elif type == 13:
						self.initialize()
					else:
						logger.error("NSPSession: unhandled type=%d - exitting" % type)
				else:
					dump = os.read(self.pr,4)
					logger.info("NSPSession: connection terminated")
					return


	def _handle_lookup_begin(self, q_value, control_flags, payload_size):
 
		irdy,ordy,xptrdy = select.select(self.ip,[],[])
		
		for x in irdy:
			
			if x==self.socket:
		
				buf = self.socket.recv(payload_size, socket.MSG_WAITALL)
				if len(buf) != payload_size:
					logger.error("_handle_nsp_lookup_begin: read %d bytes, expected %d" % (len(buf), payload_size))
					return
        
				if self.handle != None:
					logger.error("_handle_nsp_lookup_begin: called twice")
					self.socket.close()
					return

				qs = QuerySet()
				qs.unserialize(buf)
        
				if qs.service_class_id != SVCID_INET_HOSTADDRBYNAME:
					logger.error("NSPSession - unsupported ServiceClassId %s" % qs.service_class_id)
            
					# FIXME: return a proper error here
					self._send_reply(10, 0, 0xDEADBEEF, 0)
					return

				self.handle = random.randint(2**10, 2**31)
				self.request = qs

				self._send_reply(10, self.handle, 0, 0)
			else:
				return


	def _handle_lookup_next(self, handle, dvalue1, buffer_size):

		if self.response == None:
			logger.debug("Resolving %s" % self.request.service_instance_name)
			qs = QuerySet()
			qs.service_instance_name = self.request.service_instance_name
			qs.namespace = 12
			
			try:
				results = socket.getaddrinfo(qs.service_instance_name, None)
			except:
				self._send_reply(12, 0, WSAHOST_NOT_FOUND, 0)
				return
            
			qs.blob = None
			
			for result in results:
				family, socket_type, proto, canonname, sockaddr = result

				ai = CSAddrInfo()
				
				if (family == socket.AF_INET):
					ai.local_addr = (family, "0.0.0.0", 0)
				elif (family == socket.AF_INET6):
					# We don't handle that yet, but don't bail out, keep
					# looking for a v4 address
					continue
				else:
					raise NotImplementedError("Unhandled family %d" % family)
				
				ai.remote_addr = (family, sockaddr[0], sockaddr[1])
				ai.socket_type = socket_type
				ai.protocol = proto
				
				if qs.blob == None:
					qs.blob = Hostent(family)
					qs.blob.h_name = qs.service_instance_name

					# All addresses in Hostent must be of the same family.
					# Every address should appear only once.

					if family == qs.blob.h_addrtype and (not sockaddr[0] in qs.blob.h_addr_list):
						qs.blob.h_addr_list.append(sockaddr[0])
					qs.cs_addrs.append(ai)

			self.response = qs.serialize()

		if len(self.response) <= buffer_size:
			self._send_reply(12, 0, 0, len(self.response))
			self.socket.sendall(self.response)
		else:
			self._send_reply(12, 0, WSAEFAULT, len(self.response))


	def _send_reply(self, msg_type, qvalue, dvalue1, dvalue2):
		
		buf = struct.pack("<BBxxQII", 1, msg_type, qvalue, dvalue1, dvalue2)
		self.socket.sendall(buf)



class ConnectionSession:
	
	def __init__(self, s, pr):
		self.socket = s
		self.pr = pr
		self.ext_socket = None
		self.ip = [self.socket, self.pr]

	def run(self):
		
		logger.info("starting connection session")
		
		self._initialize_connection()
		self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 0)

		while True:
			rfds, wfds, xfds = select.select([self.socket, self.ext_socket, self.pr], [], [])
			for fd in rfds:
				if fd == self.socket:
					other_fd = self.ext_socket
				elif fd == self.ext_socket:
					other_fd = self.socket
				else:
					logger.info("ConnectionSession: quitting")
					dump=os.read(self.pr,4)
					return
					
				try:
					buf = fd.recv(2048)
					if len(buf) == 0:
						logger.info("ConnectionSession - zero bytes received - exitting")
						return
					other_fd.sendall(buf)
				except Exception,e:
					logger.info("ConnectionSession - exception in socket processing :%s" % e)
					return
						


	def _initialize_connection(self):

		irdy,ordy,xptrdy = select.select(self.ip,[],[])
		
		for s in irdy:
			
			if s==self.socket:
				buf = self.socket.recv(36, socket.MSG_WAITALL)
				if len(buf) != 36:
					if len(buf) != 0:
						logger.error("ConnectionSession: read %d bytes, expected %d" % (len(buf), 36))
					return
			else:
				logger.info("ConnectionSession: quitting initialize connection")
				dump=os.read(self.pr,4)
				return

		family = struct.unpack("<L", buf[2:6])[0]
		if family != socket.AF_INET:
			raise NotImplementedError("Unhandled family %d" % family)

		port = struct.unpack(">H", buf[10:12])[0]
		addr = socket.inet_ntoa(buf[12:16])
		logger.debug("Connecting to %s:%s" % (addr, port))

		ext_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		errno = ext_socket.connect_ex((addr, port))
		local_addr, local_port = ext_socket.getsockname()

		if errno == 0:
			msg_type = 0x5A
		else:
			msg_type = 0x5B
		
		head = struct.pack("<BBLxxxx", 1, msg_type, family)
		body = struct.pack(">H", local_port)
		body += socket.inet_aton(local_addr)
		body += struct.pack("xxxxxxxxxxxxxxxx")
		tail = struct.pack("<L", errno) # FIXME: translate errno to WSAError

		reply = head + body + tail
		
		self.socket.sendall(reply)
		self.ext_socket = ext_socket

class RPCStream:

	def __init__(self, data=""):
		self.initialize(data)

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
					    int(guid[1:9], 16), int(guid[10:14], 16),
					    int(guid[15:19], 16), int(guid[20:22], 16),
					    int(guid[22:24], 16), int(guid[25:27], 16),
					    int(guid[27:29], 16), int(guid[29:31], 16),
					    int(guid[31:33], 16), int(guid[33:35], 16),
					    int(guid[35:37], 16))
			self.write_field(data)
		else:
			self.write_dword(0)


class QuerySet:

	def __init__(self):
		
		self.service_instance_name = None
		self.service_class_id = None
		self.version = 0
		self.comment = None
		self.namespace = None
		self.ns_provider_id = None
		self.context = None
		self.num_protocols = 0
		self.query_string = None
		self.cs_addrs = []
		self.output_flags = 0
		self.blob = None
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
			self.len_protocols = s.read_dword()
			for p in range(0,self.num_protocols):
				iAddressFamily = s.read_dword()
				iProtocol = s.read_dword()
			# Forget it.
			self.num_protocols = 0
		self.query_string = s.read_string()
		self.cs_addrs = []
		
		count = s.read_dword()
		if count > 0:
			data = s.read_field(count * 24)
			offset = 0
			for i in xrange(len(data) / 24):
				local_data = s.read_field()
				remote_data = s.read_field()

				ai = CSAddrInfo()
				ai.unserialize(data[offset:offset + 24], local_data, remote_data)
				self.cs_addrs.append(ai)
				offset += 24

		self.output_flags = struct.unpack("<L", raw_flat[52:56])[0]
		self.blob_size = s.read_dword()

		assert s.get_offset() == s.get_length()


	def serialize(self):

		s = RPCStream()

		if self.service_instance_name != None:
			svc_name_ptr = DUMMY_PTR
		else:
			svc_name_ptr = 0

		if self.service_class_id != None:
			svc_cls_id_ptr = DUMMY_PTR
		else:
			svc_cls_id_ptr = 0
        
		if self.comment != None:
			comment_ptr = DUMMY_PTR
		else:
			comment_ptr = 0

		if self.ns_provider_id != None:
			ns_provider_id_ptr = DUMMY_PTR
		else:
			ns_provider_id_ptr = 0

		if self.context != None:
			context_ptr = DUMMY_PTR
		else:
			context_ptr = 0

		if self.num_protocols > 0:
			protocols_ptr = DUMMY_PTR
		else:
			protocols_ptr = 0

		if self.query_string != None:
			query_string_ptr = DUMMY_PTR
		else:
			query_string_ptr = 0

		if len(self.cs_addrs) > 0:
			cs_addrs_ptr = DUMMY_PTR
		else:
			cs_addrs_ptr = 0

		if self.blob != None:
			blob_ptr = DUMMY_PTR
		else:
			blob_ptr = 0

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
				  len(self.cs_addrs),
				  cs_addrs_ptr,
				  self.output_flags,
				  blob_ptr)

		s.write_field(buf)

		s.write_string(self.service_instance_name)
		s.write_guid(self.service_class_id)
		s.write_string(self.comment)
		s.write_guid(self.ns_provider_id)
		s.write_string(self.context)
		s.write_dword(0) # FIXME: WSAQUERYSET.dwNumberOfProtocols
		s.write_string(self.query_string)
        
		s.write_dword(len(self.cs_addrs))
        
		buf = ""
		addrs = []
		
		for addr in self.cs_addrs:
			data, local_data, remote_data = addr.serialize()
			buf += data
			addrs.append(local_data)
			addrs.append(remote_data)
		s.write_field(buf)
        
		for addr in addrs:
			s.write_field(addr)
        
		if self.blob != None:
			blob_data = self.blob.serialize()
			s.write_dword(8)
			s.write_dword(len(blob_data))
			s.write_dword(DUMMY_PTR)
			s.write_field(blob_data)
		else:
			s.write_dword(0)

		return s.get_data()

	def __str__(self):
		addrs = ""
		for addr in self.cs_addrs:
			addrs += "\n    %s" % str(addr)
        
		body_templ = """<QuerySet>
					<ServiceInstanceName>%s</ServiceInstanceName>
					<ServiceClassId>%s</ServiceClassId>
					<Version>0x%08x</Version>
					<Comment>%s</Comment>
					<NameSpace>%s</NameSpace>
					<NSProviderId>%s</NSProviderId>
					<Context>%s</Context>
					<Protocols/>
					<QueryString>%s</QueryString>
					<CsAddrs>%s</CsAddrs>
					<OutputFlags>0x%08x</OutputFlags>
					<Blob/>
				</QuerySet>"""

		return body_templ % (self.service_instance_name, self.service_class_id,
				     self.version, self.comment, self.namespace,
				     self.ns_provider_id, self.context, self.query_string,
				     addrs, self.output_flags)

class Hostent:

	def __init__(self, family=socket.AF_INET):
		
		self.h_name = "";
		self.h_aliases = []
		self.h_addrtype = family
        
		if self.h_addrtype == socket.AF_INET:
			self.h_length = 4
		elif self.h_addrtype == socket.AF_INET6:
			self.h_length = 16
		else:
			raise NotImplementedError("Unhandled family %d" % family)
		
		self.h_addr_list = []

	def serialize(self):
	    
		# Base index: after the structure itself.
		pos = 16

		# Encode h_aliases.
		h_aliases_index = pos
		h_aliases_length = len(self.h_aliases) + 1
		h_aliases_bin_list = ""
		h_aliases_bin_data = ""

		pos += 4 * h_aliases_length
		for alias in self.h_aliases:
			h_aliases_bin_list += struct.pack("<L", pos)
			alias_bin = alias.encode("ascii") + "\x00"
			pos += len(alias_bin)
			h_aliases_bin_data += alias_bin
		h_aliases_bin_list += struct.pack("<L", 0)

		# Encode h_addr_list.
		
		h_addr_list_index = pos
		h_addr_list_length = len(self.h_addr_list) + 1
		h_addr_list_bin_list = ""
		h_addr_list_bin_data = ""
		pos += 4 * h_addr_list_length
        
		for addr in self.h_addr_list:
			h_addr_list_bin_list += struct.pack("<L", pos)
			addr_bin = socket.inet_pton(self.h_addrtype, addr)
			pos += len(addr_bin)
			h_addr_list_bin_data += addr_bin
		h_addr_list_bin_list += struct.pack("<L", 0)

		# Encode h_name.

		h_name_index = pos
		h_name_bin = self.h_name.encode("ascii") + "\x00"
		pos += len(h_name_bin)
        
		# Put everything together.

		s  = struct.pack("<L", h_name_index)
		s += struct.pack("<L", h_aliases_index)
		s += struct.pack("<H", self.h_addrtype)
		s += struct.pack("<H", self.h_length)
		s += struct.pack("<L", h_addr_list_index)
		s += h_aliases_bin_list
		s += h_aliases_bin_data
		s += h_addr_list_bin_list
		s += h_addr_list_bin_data
		s += h_name_bin

		return s

class CSAddrInfo:

	def __init__(self):
		self.local_addr = None
		self.remote_addr = None
		self.socket_type = -1
		self.protocol = -1

	def unserialize(self, data, local_data, remote_data):
		self.socket_type, self.protocol = struct.unpack("<LL", data[16:24])
		self.local_addr = self._unserialize_sockaddr(local_data)
		self.remote_addr = self._unserialize_sockaddr(remote_data)
        
	def serialize(self):

		local_data = self._serialize_sockaddr(self.local_addr)
		remote_data = self._serialize_sockaddr(self.remote_addr)

		if self.local_addr != None:
			local_addr_ptr = DUMMY_PTR
		else:
			local_addr_ptr = 0

		if self.remote_addr != None:
			remote_addr_ptr = DUMMY_PTR
		else:
			remote_addr_ptr = 0
        
		return (struct.pack("<LLLLLL", local_addr_ptr, len(local_data),
				    remote_addr_ptr, len(remote_data),
				    self.socket_type, self.protocol),
				    local_data, remote_data)

	def _unserialize_sockaddr(self, data):
		
		family = struct.unpack("<H", data[0:2])[0]
		if family == socket.AF_INET:
			port = struct.unpack(">H", data[2:4])[0]
			return (family, socket.inet_ntoa(data[4:8]), port)
		else:
			raise NotImplementedError("Unhandled family %d" % family)
    
	def _serialize_sockaddr(self, addr):
		
		if addr == None:
			return None
        
		family, ascii_addr, port = addr
        
		s = struct.pack("<H", family)
		s += struct.pack(">H", port)
		if family == socket.AF_INET:
			s += socket.inet_aton(ascii_addr)
			s += "\x00\x00\x00\x00\x00\x00\x00\x00"
		elif family == socket.AF_INET6:
			s += socket.inet_pton(family, ascii_addr)
			s += "\x00\x00\x00\x00\x00\x00\x00\x00"
		else:
			raise NotImplementedError("Unhandled family %d" % family)

		return s

	def __str__(self):
		return "<CSAddrInfo><LocalAddress>%s</LocalAddress><RemoteAddress>%s</RemoteAddress><SocketType>%d</SocketType><Protocol>%d</Protocol></CSAddrInfo>" % \
			(self.local_addr, self.remote_addr, self.socket_type, self.protocol)


