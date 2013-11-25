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
	ctypedef unsigned int time_t

cdef extern from "Python.h":
	object PyString_FromStringAndSize(char *, int)
	int PyString_AsStringAndSize(object, char**, Py_ssize_t*)
	void * PyCObject_AsVoidPtr(object)

cdef extern from "stdlib.h":
	void *malloc(size_t size)
	void *calloc(size_t nmemb, size_t size)
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

cdef extern from "rapi2.h":
	ctypedef void IRAPISession

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

	bool rra_syncmgr_connect(RRA_SyncMgr * instance, IRAPISession *session)

	void rra_syncmgr_disconnect(RRA_SyncMgr * instance)

	bool rra_syncmgr_is_connected(RRA_SyncMgr * instance)

	uint32_t rra_syncmgr_get_type_count(RRA_SyncMgr * instance)

	RRA_SyncMgrType * rra_syncmgr_get_types(RRA_SyncMgr * instance)

	RRA_SyncMgrType * rra_syncmgr_type_from_id(RRA_SyncMgr * instance, uint32_t type_id)
	RRA_SyncMgrType * rra_syncmgr_type_from_name(RRA_SyncMgr * instance, char* name)

	void rra_syncmgr_subscribe(RRA_SyncMgr * instance, uint32_t type_id,\
                                   RRA_SyncMgrTypeCallback callback,\
                                   void *cookie)

	void rra_syncmgr_unsubscribe(RRA_SyncMgr * instance, uint32_t type_id)

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
	bool rra_syncmgr_put_single_object(RRA_SyncMgr * instance, uint32_t type_id,\
                                           uint32_t object_id, uint32_t flags,\
                                           uint8_t* data, size_t data_size, uint32_t* new_object_id)
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

cdef extern from "../lib/matchmaker.h":
	ctypedef void  RRA_Matchmaker

	RRA_Matchmaker * rra_matchmaker_new(IRAPISession *session)
	void rra_matchmaker_destroy(RRA_Matchmaker * instance)
	bool rra_matchmaker_set_current_partner(RRA_Matchmaker * instance, uint32_t index)
	bool rra_matchmaker_get_current_partner(RRA_Matchmaker * instance, uint32_t* index)
	bool rra_matchmaker_get_partner_id(RRA_Matchmaker * instance, uint32_t index, uint32_t* id)
	bool rra_matchmaker_get_partner_name(RRA_Matchmaker * instance, uint32_t index, char** name)
	bool rra_matchmaker_new_partnership(RRA_Matchmaker * instance, uint32_t index)
	bool rra_matchmaker_clear_partnership(RRA_Matchmaker * instance, uint32_t index)
	bool rra_matchmaker_have_partnership_at(RRA_Matchmaker * instance, uint32_t index)
	bool rra_matchmaker_have_partnership(RRA_Matchmaker * instance, uint32_t* index)
	bool rra_matchmaker_create_partnership(RRA_Matchmaker * instance, uint32_t* index)

cdef extern from "../lib/timezone.h":
	ctypedef struct RRA_Timezone:
		pass

	bool rra_timezone_get(RRA_Timezone* timezone, IRAPISession *session)
	time_t rra_timezone_convert_from_utc(RRA_Timezone* tzi, time_t unix_time)
	time_t rra_timezone_convert_to_utc  (RRA_Timezone* tzi, time_t unix_time)
	void rra_timezone_create_id(RRA_Timezone* timezone, char** id)

cdef extern from "../lib/contact.h":
	bool rra_contact_to_vcard(uint32_t id, uint8_t* data, size_t data_size, char** vcard, uint32_t flags, char *codepage)
	bool rra_contact_from_vcard(char* vcard, uint32_t* id, uint8_t** data, size_t* data_size, uint32_t flags, char *codepage)

cdef extern from "../lib/appointment.h":
	bool rra_appointment_to_vevent(uint32_t id, uint8_t* data, size_t data_size, char** vevent, uint32_t flags, RRA_Timezone* tzi, char *codepage)
	bool rra_appointment_from_vevent(char* vevent, uint32_t* id, uint8_t** data, size_t* data_size, uint32_t flags, RRA_Timezone* tzi, char *codepage)

cdef extern from "../lib/task.h":
	bool rra_task_to_vtodo(uint32_t id, uint8_t* data, size_t data_size, char** vtodo, uint32_t flags, RRA_Timezone* tzi, char *codepage)
	bool rra_task_from_vtodo(char* vtodo, uint32_t* id, uint8_t** data, size_t* data_size, uint32_t flags, RRA_Timezone* tzi, char *codepage)

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

# Constants for use with rra_syncmgr_type_from_name()
RRA_SYNCMGR_TYPE_APPOINTMENT  = "Appointment"
RRA_SYNCMGR_TYPE_CONTACT      = "Contact"
RRA_SYNCMGR_TYPE_FAVORITE     = "Favorite"
RRA_SYNCMGR_TYPE_FILE         = "File"
RRA_SYNCMGR_TYPE_INBOX        = "Inbox"
RRA_SYNCMGR_TYPE_INK          = "Ink"
RRA_SYNCMGR_TYPE_MERLIN_MAIL  = "Merlin Mail"
RRA_SYNCMGR_TYPE_MS_TABLE     = "MS Table"
RRA_SYNCMGR_TYPE_TASK         = "Task"

# from timezone.h
RRA_TIMEZONE_INVALID_TIME = 0xffffffff

# from contact.h
RRA_CONTACT_ID_UNKNOWN  = 0

RRA_CONTACT_NEW     	= 0x1
RRA_CONTACT_UPDATE  	= 0x2

RRA_CONTACT_ISO8859_1	= 0x10
RRA_CONTACT_UTF8	= 0x20

RRA_CONTACT_VERSION_2_1 = 0x100
RRA_CONTACT_VERSION_3_0 = 0x200


# from appointment.h
RRA_APPOINTMENT_ID_UNKNOWN  = 0

RRA_APPOINTMENT_NEW     	= 0x1
RRA_APPOINTMENT_UPDATE  	= 0x2

RRA_APPOINTMENT_ISO8859_1	= 0x10
RRA_APPOINTMENT_UTF8		= 0x20

RRA_APPOINTMENT_VCAL_1_0	= 0x100
RRA_APPOINTMENT_VCAL_2_0	= 0x200


# from task.h
RRA_TASK_ID_UNKNOWN = 0

RRA_TASK_NEW     	= 0x1
RRA_TASK_UPDATE  	= 0x2

RRA_TASK_ISO8859_1	= 0x10
RRA_TASK_UTF8		= 0x20

RRA_TASK_VCAL_1_0	= 0x100
RRA_TASK_VCAL_2_0	= 0x200


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

	def Connect(self, rapisession):
		"""Connect to a device.

		Connect the RRA session to the device attached to the RAPISession
		given as an argument, or the default RAPI connection if none is
		specified. Raises an RRAError on failure."""
		cdef IRAPISession *session

		session = PyCObject_AsVoidPtr(rapisession.rapi_connection)

		self.connected = rra_syncmgr_connect(self.instance, session)
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
		"""Subscribe to events.

		Subscribe to events for object with a type of type_id. Raises an RRAError on failure.
		CB_TypeCallback is called to process events, and should be overridden."""
		if self.connected == 0:
			raise RRAError("Not connected")

		rra_syncmgr_subscribe(self.instance, type_id, _CB_TypesCallback, <void *>self)
		return

	def UnsubscribeObjectEvents(self,type_id):
		"""Unsubscribe to events.

		Unsubscribe to events for object with a type of type_id. Raises an RRAError on failure."""
		if self.connected == 0:
			raise RRAError("Not connected")

		rra_syncmgr_unsubscribe(self.instance, type_id)
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
		Raises an RRAError on failure."""

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

	def ObjectTypeFromId(self, id):
		"""The object type represented by the numeric id.

		Returns a RRASyncObjectType representing the type identified by
		the provided numeric id.
		Raises an RRAError on failure."""

		cdef RRA_SyncMgrType * thetype
		cdef uint32_t type_id

		if self.connected == 0:
			raise RRAError("Not connected")

		type_id = id
		thetype = <RRA_SyncMgrType *>rra_syncmgr_type_from_id(self.instance, type_id)
		rettype = RRASyncObjectType(thetype.id,\
					thetype.count,\
					thetype.total_size,\
					thetype.name1,\
					thetype.name2)
		return rettype

	def ObjectTypeFromName(self, name):
		"""The object type represented by the name.

		Returns a RRASyncObjectType representing the type identified by
		the provided text name.
		Raises an RRAError on failure."""

		cdef RRA_SyncMgrType * thetype
		cdef char * type_name

		if self.connected == 0:
			raise RRAError("Not connected")

		type_name = name
		thetype = <RRA_SyncMgrType *>rra_syncmgr_type_from_name(self.instance, type_name)
		rettype = RRASyncObjectType(thetype.id,\
					thetype.count,\
					thetype.total_size,\
					thetype.name1,\
					thetype.name2)
		return rettype

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

	def PutSingleObject(self,type_id,obj_id,data,flags):
		"""Send an object to device.

		Send a single object of type_id, with object id of obj_id, and
		return the new id. Raises an RRAError on failure."""
		cdef uint8_t * arr
		cdef Py_ssize_t cnt
		cdef bool rc
		cdef uint32_t new_object_id

		PyString_AsStringAndSize(data, <char**>&arr, &cnt)

		rc=rra_syncmgr_put_single_object(self.instance,type_id,obj_id,flags,arr,cnt,&new_object_id)

		if rc != 0:
			return new_object_id

		raise RRAError("Failed to send object")

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



cdef class RRAMatchmaker:
	"""An RRA matchmaker object to manage device partnerships with
	a Windows Mobile device.
	Connect to the device attached to the RAPISession
	given as an argument, or the default RAPI connection if none is
	specified. Raises an RRAError on failure."""

	cdef RRA_Matchmaker * instance
	
	def __cinit__(self, rapisession):
		cdef IRAPISession *session

		session = PyCObject_AsVoidPtr(rapisession.rapi_connection)

		self.instance = <RRA_Matchmaker *>rra_matchmaker_new(session)
		if not self.instance:
			raise RRAError("Matchmaker creation failed")

	def __dealloc__(self):
		rra_matchmaker_destroy(self.instance)

	def SetCurrentPartner(self,index):
		"""Set the index for the current partner.

		Set the index for the partnership that is to be 'current',
		ie. active for RRA synchronisation operations.
		Raises an RRAError on failure."""

		if rra_matchmaker_set_current_partner(self.instance,index):
			return

		raise RRAError("Failed to set current partner")

	def GetCurrentPartner(self):
		"""Get the index for the current partner.

		Get the index for the partnership that is 'current',
		ie. active for RRA synchronisation operations.
		Raises an RRAError on failure."""

		cdef uint32_t index
		cdef bool rc
		rc=rra_matchmaker_get_current_partner(self.instance,&index)
		if rc != 0:
			return index

		raise RRAError("Failed to get current partner")

	def GetPartnerId(self,index):
		"""Get the partnership ID.

		Get the partnership numeric ID for the partnership at the
		specified index (1 or 2).
		Raises an RRAError on failure."""

		cdef uint32_t id
		cdef bool rc
		rc=rra_matchmaker_get_partner_id(self.instance,index,&id)
		if rc != 0:
			return id

		raise RRAError("Failed to get partner id")

	def GetPartnerName(self,index):
		"""Get the partnership hostname.

		Get the hostname of the partner associated with the partnership
		at the specified index (1 or 2).
		Raises an RRAError on failure."""

		cdef char *name
		cdef bool rc
		data=""
		rc=rra_matchmaker_get_partner_name(self.instance,index,&name)
		if rc != 0:
			data=PyString_FromString(name)
			free(name)
			return data

		raise RRAError("Failed to get partner name")

	def NewPartnership(self,index):
		"""Create a partnership at specified empty index.

		Creates a new partnership at the index specified (1 or 2),
		which should be empty.
		Raises an RRAError on failure."""

		if rra_matchmaker_new_partnership(self.instance,index):
			return

		raise RRAError("Failed to create new partnership")

	def ClearPartnership(self,index):
		"""Clears a partnership at specified index.

		Clears an existing partnership at the index specified (1 or 2).
		Raises an RRAError on failure."""

		if rra_matchmaker_clear_partnership(self.instance,index):
			return

		raise RRAError("Failed to remove partnership")

	def HavePartnershipAt(self,index):
		"""Detect a valid partnership at specified index.

		Detects whether we have a valid partnership with this
		host at the index specified (1 or 2)."""

		if rra_matchmaker_have_partnership_at(self.instance,index):
			return True

		return False

	def HavePartnership(self):
		"""Detect a valid partnership.

		Detects whether we have a valid partnership with this
		host available, returning the index if found (1 or 2)."""

		cdef uint32_t index
		cdef bool rc
		rc=rra_matchmaker_have_partnership(self.instance,&index)
		if rc != 0:
			return index

		return 0

	def CreatePartnership(self):
		"""Create a partnership.

		Detects whether we have a valid partnership with this
		host available, and if we don't, attempts to create a
		partnership at any available index, returning the index
		(1 or 2) if an existing partnership is found or creation
		is successful.
		Raises an RRAError on failure."""

		cdef uint32_t index
		cdef bool rc
		rc=rra_matchmaker_create_partnership(self.instance,&index)
		if rc != 0:
			return index

		raise RRAError("Failed to find or create partnership")


cdef class RRATimezone:
	"""An RRA timezone, contains information about the timezone used on
	a Windows Mobile device.
	Raises an RRAError on failure."""

	cdef RRA_Timezone * instance
	
	def __cinit__(self):
		cdef RRA_Timezone tmp
		self.instance = <RRA_Timezone *>calloc(1, sizeof(tmp))
		if not self.instance:
			raise RRAError("Timezone creation failed")

	def __dealloc__(self):
		free(self.instance)

	def Get(self,rapisession):
		"""Get the timezone information.

		Gets the timezone information of the device attached to
		the RAPISession	given as an argument, or the default RAPI
		connection if none is specified.
		Raises an RRAError on failure."""

		cdef IRAPISession *session

		session = PyCObject_AsVoidPtr(rapisession.rapi_connection)

		if rra_timezone_get(self.instance,session):
			return

		raise RRAError("Failed to get timezone info.")

	def ConvertFromUTC(self,utc):
		"""Convert from UTC time to the timezone.

		Converts from the provided UTC time, as seconds since the
		epoch, to the time represented by the timezone.
		Raises an RRAError on failure."""

		cdef time_t tz_time_t
		cdef time_t utc_time_t

		utc_time_t = utc
		tz_time_t = rra_timezone_convert_from_utc(self.instance, utc_time_t)

		if tz_time_t == RRA_TIMEZONE_INVALID_TIME:
			raise RRAError("Failed to convert from UTC time.")

		return tz_time_t

	def ConvertToUTC(self,time):
		"""Convert to UTC time from the timezone.

		Converts from the provided time represented by the timezone,
		as seconds since the epoch, to the UTC time.
		Raises an RRAError on failure."""

		cdef time_t utc_time_t
		cdef time_t tz_time_t

		tz_time_t = time
		utc_time_t = rra_timezone_convert_to_utc(self.instance, tz_time_t)

		if utc_time_t == RRA_TIMEZONE_INVALID_TIME:
			raise RRAError("Failed to convert to UTC time.")

		return utc_time_t

	def CreateId(self):
		"""Create a timezone ID.

		Create a timezone ID for use in vCalendar objects."""

		cdef char *cid
		rra_timezone_create_id(self.instance, &cid)

		return cid


def ContactToVcard(id,data,flags,codepage):
	"""Convert an RRA contact to vcard.

	Convert a contact obtained from RRA to vcard format. Flags and codepage
	parameters control the formatting.
	Raises an RRAError on failure."""

	cdef uint8_t *tmp_data
	cdef Py_ssize_t data_len
	cdef char *vcard

	PyString_AsStringAndSize(data, <char**>&tmp_data, &data_len)

	vcard = NULL
	retval = rra_contact_to_vcard(id, tmp_data, data_len, &vcard, flags, codepage)
	if retval != 1:
		raise RRAError("Failed to convert contact to vcard")

	value = PyString_FromString(<char *>vcard)
	free(vcard)
	return value

def ContactFromVcard(vcard,flags,codepage):
	"""Convert an RRA contact from vcard.

	Convert a vcard to RRA contact format. Flags and codepage
	parameters control the formatting.
	Raises an RRAError on failure."""

	cdef uint32_t id
	cdef uint8_t *data
	cdef size_t data_len

	id = RRA_CONTACT_ID_UNKNOWN
	retval = rra_contact_from_vcard(vcard, &id, &data, &data_len, flags, codepage)
	if retval != 1:
		raise RRAError("Failed to convert contact from vcard")

	value = PyString_FromStringAndSize(<char *>data, data_len)
	free(data)
	return (id,value)


def AppointmentToVevent(id,data,flags,RRATimezone timezone,codepage):
	"""Convert an RRA appointment to vevent.

	Convert an appointment obtained from RRA to vevent format. Flags and codepage
	parameters control the formatting.
	Raises an RRAError on failure."""

	cdef uint8_t *tmp_data
	cdef Py_ssize_t data_len
	cdef char *vevent

	PyString_AsStringAndSize(data, <char**>&tmp_data, &data_len)

	vevent = NULL
	retval = rra_appointment_to_vevent(id, tmp_data, data_len, &vevent, flags, timezone.instance, codepage)
	if retval != 1:
		raise RRAError("Failed to convert appointment to vevent")

	value = PyString_FromString(<char *>vevent)
	free(vevent)
	return value

def AppointmentFromVevent(vevent,flags,RRATimezone timezone,codepage):
	"""Convert an RRA appointment from vevent.

	Convert a vevent to RRA appointment format. Flags and codepage
	parameters control the formatting.
	Raises an RRAError on failure."""

	cdef uint32_t id
	cdef uint8_t *data
	cdef size_t data_len

	id = RRA_APPOINTMENT_ID_UNKNOWN
	retval = rra_appointment_from_vevent(vevent, &id, &data, &data_len, flags, timezone.instance, codepage)
	if retval != 1:
		raise RRAError("Failed to convert appointment from vevent")

	value = PyString_FromStringAndSize(<char *>data, data_len)
	free(data)
	return (id,value)


def TaskToVtodo(id,data,flags,RRATimezone timezone,codepage):
	"""Convert an RRA task to vtodo.

	Convert a task obtained from RRA to vtodo format. Flags and codepage
	parameters control the formatting.
	Raises an RRAError on failure."""

	cdef uint8_t *tmp_data
	cdef Py_ssize_t data_len
	cdef char *vtodo

	PyString_AsStringAndSize(data, <char**>&tmp_data, &data_len)

	vtodo = NULL
	retval = rra_task_to_vtodo(id, tmp_data, data_len, &vtodo, flags, timezone.instance, codepage)
	if retval != 1:
		raise RRAError("Failed to convert task to vtodo")

	value = PyString_FromString(<char *>vtodo)
	free(vtodo)
	return value

def TaskFromVtodo(vtodo,flags,RRATimezone timezone,codepage):
	"""Convert to an RRA task from vtodo.

	Convert a vtodo to RRA taask format. Flags and codepage
	parameters control the formatting.
	Raises an RRAError on failure."""

	cdef uint32_t id
	cdef uint8_t *data
	cdef size_t data_len

	id = RRA_APPOINTMENT_ID_UNKNOWN
	retval = rra_task_from_vtodo(vtodo, &id, &data, &data_len, flags, timezone.instance, codepage)
	if retval != 1:
		raise RRAError("Failed to convert to task from vtodo")

	value = PyString_FromStringAndSize(<char *>data, data_len)
	free(data)
	return (id,value)


def FileUnpack(data):
	"""Unpack file data transferred via RRA.

	Unpack a packet of data transferred by RRA containing a file and
	it's metadata, returning the file type, relative path, and the file contents.
	Raises an RRAError on failure."""

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

	if ftype == RRA_FILE_TYPE_FILE:
		if file_size > 0:
			value = PyString_FromStringAndSize(<char *>file_content, file_size)
			free(file_content)
		else:
			value = PyString_FromStringAndSize(<char *>"", 0)
	else:
		value = None

	return (ftype,path,value)


def FilePack(filetype,path,data=None):
	"""Pack file data to be transferred via RRA.

	Pack a packet of data to be transferred by RRA containing a file and
	it's metadata. Packs the file type, relative path, and the file contents,
	returning the packed data.
	Raises an RRAError on failure."""

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
	free(out_data)

	return value
