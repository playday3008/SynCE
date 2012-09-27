"""Python bindings to librra

RRA (Remote Replication Agent) is a protocol used to syncronise
data with Windows CE and Windows Mobile devices, and librra
is the open source implementation from the SynCE project.

For more information, go to www.synce.org"""

import sys
import pyrapi2

cdef extern from "stddef.h":
	ctypedef unsigned int  size_t
	ctypedef int  ssize_t

cdef extern from "time.h":
	ctypedef time_t

cdef extern from "Python.h":
	object PyString_FromStringAndSize(char *, int)
	int PyString_AsStringAndSize(object, char**, Py_ssize_t*)
	void * PyCObject_AsVoidPtr(object)

cdef extern from "stdlib.h":
	void *malloc(size_t size)
	void free(void *ptr)

cdef extern from "string.h":
	void * memcpy(void * dest, void * src, size_t n)

cdef extern from "stdint.h":
	ctypedef unsigned short int uint16_t
	ctypedef unsigned int uint32_t
	ctypedef unsigned char uint8_t
	
cdef extern from *:
	ctypedef uint8_t* const_uint8_ptr "const uint8_t*"

cdef extern from "synce.h":
	ctypedef uint16_t WCHAR
	ctypedef uint32_t bool
	ctypedef WCHAR *  LPCWSTR
	ctypedef uint32_t DWORD
	char *wstr_to_utf8(LPCWSTR unicode)
	LPCWSTR wstr_from_utf8(char * utf8)
	void wstr_free_string(void *str)

cdef extern from "synce_log.h":
	void synce_log_set_level(int level)

cdef extern from "rapi.h":
	ctypedef void RapiConnection

cdef extern from "../lib/syncmgr.h":
	ctypedef void  RRA_SyncMgr
	ctypedef struct RRA_SyncMgrType:
		uint32_t	id
		uint32_t	count
		uint32_t	total_size
		uint32_t	time_t
		char		name1[100]
		char		name2[80]

	ctypedef enum RRA_SyncMgrTypeEvent:
		SYNCMGR_TYPE_EVENT_UNCHANGED_h "SYNCMGR_TYPE_EVENT_UNCHANGED"
		SYNCMGR_TYPE_EVENT_CHANGED_h "SYNCMGR_TYPE_EVENT_CHANGED"
		SYNCMGR_TYPE_EVENT_DELETED_h "SYNCMGR_TYPE_EVENT_DELETED"

	ctypedef enum:
		RRA_SYNCMGR_NEW_OBJECT_h "RRA_SYNCMGR_NEW_OBJECT"
		RRA_SYNCMGR_UPDATE_OBJECT_h "RRA_SYNCMGR_UPDATE_OBJECT"

	ctypedef struct _RRA_Uint32Vector:
		pass

	ctypedef bool (*RRA_SyncMgrTypeCallback) (RRA_SyncMgrTypeEvent event, uint32_t type_id, uint32_t count, uint32_t* ids, void* cookie)

	ctypedef ssize_t (*RRA_SyncMgrReader) (uint32_t type_id, unsigned index, uint8_t* data, size_t data_size, void* cookie)

	ctypedef bool (*RRA_SyncMgrWriter) (uint32_t type_id, uint32_t object_id, const_uint8_ptr data, size_t data_size, void* cookie)
	
	RRA_SyncMgr * rra_syncmgr_new()

	void rra_syncmgr_destroy(RRA_SyncMgr * instance)

	bool rra_syncmgr_connect(RRA_SyncMgr * instance, RapiConnection *connection)

	void rra_syncmgr_disconnect(RRA_SyncMgr * instance)

	bool rra_syncmgr_is_connected(RRA_SyncMgr * instance)

	uint32_t rra_syncmgr_get_type_count(RRA_SyncMgr * instance)

	RRA_SyncMgrType * rra_syncmgr_get_types(RRA_SyncMgr * instance)

	void rra_syncmgr_subscribe(RRA_SyncMgr * instance, uint32_t type_id,\
                                   RRA_SyncMgrTypeCallback callback,\
                                   void *cookie)

	bool rra_syncmgr_start_events(RRA_SyncMgr * instance)

	bool rra_syncmgr_get_event_descriptor(RRA_SyncMgr * instance)

	bool rra_syncmgr_event_pending(RRA_SyncMgr * instance)

	bool rra_syncmgr_event_wait(RRA_SyncMgr * instance, int timeout, bool * gotEvent)

	bool rra_syncmgr_handle_event(RRA_SyncMgr * instance)

	bool rra_syncmgr_handle_all_pending_events(RRA_SyncMgr * instance)

	bool rra_syncmgr_get_single_object(RRA_SyncMgr * instance, uint32_t type_id,\
                                           uint32_t object_id, uint8_t ** data,\
                                           size_t * count)

	bool rra_syncmgr_get_multiple_objects(RRA_SyncMgr * instance, uint32_t type_id,\
                                              uint32_t c_cnt, uint32_t * c_oids,\
					      RRA_SyncMgrWriter writer,\
					      void *cookie)
	bool rra_syncmgr_put_multiple_objects(RRA_SyncMgr * instance, uint32_t type_id,\
					      uint32_t object_id_count, uint32_t * object_id_array,\
					      uint32_t * recv_object_id_array, uint32_t flags,
                                              RRA_SyncMgrReader reader,\
					      void *cookie)

	void rra_syncmgr_free_data_buffer(uint8_t* buffer)

	bool rra_syncmgr_purge_deleted_object_ids(RRA_SyncMgr * instance, uint32_t type, uint32_t * oids)

	bool rra_syncmgr_mark_object_unchanged(RRA_SyncMgr * instance, uint32_t type_id,\
                                               uint32_t obj_id)

	bool rra_syncmgr_delete_object(RRA_SyncMgr * instance,\
                                       uint32_t type_id,\
                                       uint32_t object_id)

cdef extern from "../lib/file.h":
	bool rra_file_unpack(uint8_t* data, size_t data_size, DWORD *ftype, char **path, uint8_t **file_content, size_t *file_size)
	bool rra_file_pack(DWORD ftype, char* path, uint8_t* file_content, size_t file_size, uint8_t **data, size_t *data_size)


#
# Public constants
#

# flags for RRASession.PutMultipleObjects, from syncmgr.h
RRA_SYNCMGR_NEW_OBJECT     = RRA_SYNCMGR_NEW_OBJECT_h
RRA_SYNCMGR_UPDATE_OBJECT  = RRA_SYNCMGR_UPDATE_OBJECT_h

# event types, from syncmgr.h
SYNCMGR_TYPE_EVENT_UNCHANGED = SYNCMGR_TYPE_EVENT_UNCHANGED_h
SYNCMGR_TYPE_EVENT_CHANGED   = SYNCMGR_TYPE_EVENT_CHANGED_h
SYNCMGR_TYPE_EVENT_DELETED   = SYNCMGR_TYPE_EVENT_DELETED_h

# from file.h
RRA_FILE_TYPE_UNKNOWN    = 0x00
RRA_FILE_TYPE_DIRECTORY  = 0x10
RRA_FILE_TYPE_FILE       = 0x20

class RRASyncObjectType(object):
	"""An RRA object type

	An object type that can be synchronised with a device
	using RRA"""
	def __init__(self,id,count,total_size,name1,name2):
		self.id = id
		self.count = count
		self.total_size = total_size
		self.name1 = name1
		self.name2 = name2

class RRAError(Exception):
	"""An error resulting from an RRA call"""
	def __init__(self, rc):
		self.rc = rc

	def __str__(self):
		return str(self.rc)

# callback processing
#
# Callback for types

cdef bool _CB_TypesCallback(RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t * ids, void *cookie):
	ida=[]
	cdef bool rc
	for i from 0 <= i < count:
		ida.append(ids[i])
	rc=(<object>cookie).CB_TypeCallback(<RRA_SyncMgrTypeEvent> event,type,ida)
	return rc

cdef bool _CB_WriterCallback(uint32_t type_id, uint32_t obj_id, const_uint8_ptr data, size_t data_size, void *cookie):
	pd=[]
	if data_size > 0:
		pd=PyString_FromStringAndSize(<char *>data,data_size)
	return (<object>cookie).CB_ObjectWriterCallback(type_id, obj_id, pd)

cdef ssize_t _CB_ReaderCallback(uint32_t type_id, unsigned index, uint8_t * data, size_t max_size, void *cookie):
	cdef char * pstr
	pd=(<object>cookie).CB_ObjectReaderCallback(type_id, index, max_size)
	rc = len(pd)
	if rc > 0:
		if rc > max_size:
			rc = max_size
		pstr = pd
		memcpy(data,pstr,rc)
#		for i from 0 <= i < rc:
#			data[i] = pstr[i]
	print "returning length rc=%d" % rc
	return rc

cdef class RRASession:
	"""An RRA connection to a Windows Mobile device."""

	cdef RRA_SyncMgr * instance
	cdef bool connected
	cdef uint32_t ntypes
	
	def __cinit__(self):
		self.instance = <RRA_SyncMgr *>rra_syncmgr_new()
		if not self.instance:
			raise RRAError("SyncMgr creation failed")
		self.connected = 0
		self.ntypes = 0

	def __dealloc__(self):
		rra_syncmgr_destroy(self.instance)

	#
	# Connection and disconnection
	#

	def Connect(self, rapisession=None):
		"""Connect to a device.

		Connect the RRA session to the device attached to the RAPISession
		given as an argument, or the default RAPI connection if none is
		specified. Raises an RRAError on failure."""
		cdef RapiConnection *rapiconn

		if rapisession == None:
			rapiconn = NULL
		else:
			rapiconn = PyCObject_AsVoidPtr(rapisession.rapi_connection)

		self.connected = rra_syncmgr_connect(self.instance, rapiconn)
		if self.connected == 0:
			raise RRAError("Connection failed")

		return

	def isConnected(self):
		"""The connection state of the session."""
		return self.connected

	def Disconnect(self):
		"""Disconnect from the device."""
		rra_syncmgr_disconnect(self.instance)
		self.connected=0
		return self.connected

	#
	# Event processing
	#

	def SubscribeObjectEvents(self,type_id):
		"""Subscribe to events."""

		"""Subscribe to events for object with a type of type_id. Raisess an RRAError on failure.
		CB_TypeCallback is called to process events, and should be overridden."""
		if self.connected == 0:
			raise RRAError("Not connected")

		rra_syncmgr_subscribe(self.instance, type_id, _CB_TypesCallback, <void *>self)
		return

	def StartEventListener(self):
		"""Start listening for events.

		Raisess an RRAError on failure."""

		if self.connected ==0:
			raise RRAError("Not connected")

		if rra_syncmgr_start_events(self.instance):
			return
		raise RRAError("Failed to start event listener")

	def GetEventDescriptor(self):
		"""Get connection descriptor.

		Returns the file descriptor associated with the connection."""
		return rra_syncmgr_get_event_descriptor(self.instance)

	def WaitEvent(self,timeout):
		"""Wait for an RRA event.

		Wait for an event from the device for timeout seconds. Returns true
		if an event has occured, false if not. Raises an RRAError otherwise."""
		cdef bool gotone
		cdef bool rc
		rc=rra_syncmgr_event_wait(self.instance,timeout, &gotone)
		if rc:
			return gotone
		else:
			raise RRAError("Failed to wait for events")

	def IsEventPresent(self):
		"""Is an RRA event waiting.

		Returns true if an event is waiting to be processed."""
		return rra_syncmgr_event_pending(self.instance)

	def HandleEvent(self):
		"""Handle a single pending RRA event.

		Raisess an RRAError on failure."""
		if rra_syncmgr_handle_event(self.instance):
			return

		raise RRAError("Failed to handle event")

	def HandleAllPendingEvents(self):
		"""Handle all pending RRA events.

		Raisess an RRAError on failure and stops processing."""
		if rra_syncmgr_handle_all_pending_events(self.instance):
			return

		raise RRAError("Failed to handle an event")

	#
	# Object type ID handling
	#

	def GetObjectTypeCount(self):
		"""The number of object types supported by the device.

		Raises an RRAError on failure."""

		if self.connected == 0:
			raise RRAError("Not connected")

		self.ntypes = rra_syncmgr_get_type_count(self.instance)
		return self.ntypes

	def GetObjectTypes(self):
		"""The object types supported by the device.

		Returns a list of RRASyncObjectType representing the types available.
		Raisess an RRAError on failure."""

		cdef RRA_SyncMgrType * thetypes
		rettypes = []

		if self.connected == 0:
			raise RRAError("Not connected")

		self.GetObjectTypeCount()
		thetypes = <RRA_SyncMgrType *>rra_syncmgr_get_types(self.instance)
		for i in range(self.ntypes):
			rettypes.append(RRASyncObjectType(thetypes[i].id,\
					thetypes[i].count,\
					thetypes[i].total_size,\
					thetypes[i].name1,\
					thetypes[i].name2))
		return rettypes

	#
	# Object retrieval and submission
	#

	def GetMultipleObjects(self,type_id, oids):
		"""Retrieve multiple data objects from device.

		Retrieve objects of type_id, the object ids of which are listed in oids.
		CB_ObjectWriterCallback is called for each object, and should be overridden.
		Raises an RRAError on failure."""
		cdef uint32_t * c_oids
		cdef uint32_t c_cnt

		c_cnt = len(oids)
		c_oids = <uint32_t *>malloc(sizeof(uint32_t)*c_cnt)
		rc=0
		if c_oids == NULL:
			raise MemoryError("Failed to allocate list of object ids")

		for i from 0 <= i < c_cnt:
			c_oids[i] = oids[i]
		rc = rra_syncmgr_get_multiple_objects(self.instance,type_id, c_cnt, c_oids,_CB_WriterCallback, <void *>self)
		free(c_oids)

		if rc != 0:
			return

		raise RRAError("Failed to get multiple objects")
	
	def GetSingleObject(self,type_id,obj_id):
		"""Retrieve an object from device.

		Retrieve a single object of type_id, with object id of obj_id, and
		return the buffer. Raises an RRAError on failure."""
		cdef uint8_t * arr
		cdef size_t cnt
		cdef bool rc
		data = ""
		rc=rra_syncmgr_get_single_object(self.instance,type_id,obj_id,&arr,&cnt)
		if rc != 0:
			data=PyString_FromStringAndSize(<char *>arr,cnt)
			free(arr)
			return data

		raise RRAError("Failed to get object")

	def PutMultipleObjects(self, type_id, oid_array, newoid_array, flags):
		"""Send multiple data objects to device.

		Send objects of type_id, the object ids of which are listed in oid_array.
		CB_ObjectReaderCallback is called for each object, and should be overridden.
		Raises an RRAError on failure."""
		cdef uint32_t * c_oids
		cdef uint32_t * c_newoids
		cdef uint32_t c_oidcount
		c_oidcount = len(oid_array)

		c_oids = <uint32_t *>malloc(sizeof(uint32_t)*c_oidcount)
		if c_oids == NULL:
			raise MemoryError("Failed to allocate list of object ids")

		c_newoids = <uint32_t *>malloc(sizeof(uint32_t)*c_oidcount)
		if c_newoids == NULL:
			free(c_oids)
			raise MemoryError("Failed to allocate list of object ids")

		rc = False

		for i from 0 <= i < c_oidcount:
			c_oids[i] = oid_array[i]

		rc = rra_syncmgr_put_multiple_objects(self.instance,type_id,c_oidcount,
						      c_oids,c_newoids,flags,_CB_ReaderCallback,<void *>self)
		if rc == True:
			for i from 0 <= i < c_oidcount:
				newoid_array.append(c_newoids[i])

		free(c_oids)
		free(c_newoids)

		if rc == True:
			return

		raise RRAError("Failed to put multiple objects")

	def MarkObjectUnchanged(self,type_id,obj_id):
		"""Mark an object unchanged.

		Mark the object as unchanged. Raises an RRAError on failure."""

		if rra_syncmgr_mark_object_unchanged(self.instance,type_id,obj_id):
			return

		raise RRAError("Failed to mark object unchanged")

	#
	# Object deletion
	#

	def DeleteObject(self,type_id, obj_id):
		"""Delete the object.

		Raises an RRAError on failure."""
		if rra_syncmgr_delete_object(self.instance,type_id,obj_id):
			return

		raise RRAError("Failed to delete object")

	#
	# Callbacks
	#
	# (to be overloaded by user)
	#

	def CB_TypeCallback(self, event, type_id, idarray):
		"""Callback for subscription events.

		This method should be overridden. It is called for events
		occuring for object types registered with SubscribeObjectEvents."""
		return False
		
	def CB_ObjectWriterCallback(self,type_id, obj_id, data):
		"""Callback for GetMultipleObjects.

		This method should be overridden. It is called for each object id
		passed in a call to GetMultipleObjects."""
		return False

	def CB_ObjectReaderCallback(self,type_id, index, maxlen):
		"""Callback for PutMultipleObjects.

		This method should be overridden. It is called for each object id
		passed in a call to PutMultipleObjects."""
		return False


def FileUnpack(data):
	cdef DWORD ftype
	cdef char *path
	cdef uint8_t *file_content
	cdef size_t file_size
	cdef uint8_t *tmp_data
	cdef Py_ssize_t data_len

	path = NULL
	file_content = NULL

	PyString_AsStringAndSize(data, <char**>&tmp_data, &data_len)

	retval = rra_file_unpack(tmp_data, data_len, &ftype, &path, &file_content, &file_size)
	if retval != 1:
		raise RRAError("Failed to unpack file data")

	if file_size > 0:
		value = PyString_FromStringAndSize(<char *>file_content, file_size)
	else:
		value = None

	return (ftype,path,value)


def FilePack(filetype,path,data=None):
	cdef uint8_t *tmp_data
	cdef Py_ssize_t data_len
	cdef uint8_t *out_data
	cdef size_t out_data_len

	out_data = NULL
	tmp_data = NULL
	data_len = 0

	if data != None:
		PyString_AsStringAndSize(data, <char**>&tmp_data, &data_len)

	retval = rra_file_pack(filetype, path, tmp_data, data_len, &out_data, &out_data_len)
	if retval != 1:
		raise RRAError("Failed to pack file data")

	value = PyString_FromStringAndSize(<char *>out_data, out_data_len)

	return value
