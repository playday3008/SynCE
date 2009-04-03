import sys

cdef extern from "stddef.h":
	ctypedef unsigned int  size_t

cdef extern from "time.h":
	ctypedef time_t

cdef extern from "Python.h":
	object PyString_FromStringAndSize(char *, int)

cdef extern from "stdlib.h":
	void *malloc(size_t size)
	void free(void *ptr)

cdef extern from "string.h":
	void * memcpy(void * dest, void * src, size_t n)

cdef extern from "stdint.h":
	ctypedef unsigned short int uint16_t
	ctypedef unsigned int uint32_t
	ctypedef unsigned char uint8_t
	
cdef extern from "synce.h":
	ctypedef uint16_t WCHAR
	ctypedef uint32_t bool
	ctypedef WCHAR *  LPCWSTR
	char *wstr_to_utf8(LPCWSTR unicode)
	LPCWSTR wstr_from_utf8(char * utf8)
	void wstr_free_string(void *str)

cdef extern from "synce_log.h":
	void synce_log_set_level(int level)

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
		SYNCMGR_TYPE_EVENT_UNCHANGED
		SYNCMGR_TYPE_EVENT_CHANGED
		SYNCMGR_TYPE_EVENT_DELETED

	ctypedef struct _RRA_Uint32Vector:
		pass

	
	RRA_SyncMgr * rra_syncmgr_new()

	void rra_syncmgr_destroy(RRA_SyncMgr * instance)

	bool rra_syncmgr_connect(RRA_SyncMgr * instance, char *ip_addr)

	void rra_syncmgr_disconnect(RRA_SyncMgr * instance)

	bool rra_syncmgr_is_connected(RRA_SyncMgr * instance)

	uint32_t rra_syncmgr_get_type_count(RRA_SyncMgr * instance)

	RRA_SyncMgrType * rra_syncmgr_get_types(RRA_SyncMgr * instance)

	void rra_syncmgr_subscribe(RRA_SyncMgr * instance, uint32_t type,\
                                   bool SyncMgrTypeCB(RRA_SyncMgrTypeEvent event,\
                                                      uint32_t, uint32_t,\
                                                      uint32_t * , context),\
                                   context)

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
					      bool SyncMgrWriter(uint32_t type_id, uint32_t object_id,\
								 uint8_t * data, size_t data_size, context),\
					      context)
	bool rra_syncmgr_put_multiple_objects(RRA_SyncMgr * instance, uint32_t type_id,\
					      uint32_t object_id_count, uint32_t * object_id_array,\
					      uint32_t * recv_object_id_array, uint32_t flags,
                                              size_t SyncMgrReader(uint32_t type_id, unsigned int index, \
                                                                   uint8_t * data, size_t data_size, context),\
					      context)

	void rra_syncmgr_free_data_buffer(uint8_t* buffer)

	bool rra_syncmgr_purge_deleted_object_ids(RRA_SyncMgr * instance, uint32_t type, uint32_t * oids)

	bool rra_syncmgr_mark_object_unchanged(RRA_SyncMgr * instance, uint32_t type_id,\
                                               uint32_t obj_id)

	bool rra_syncmgr_delete_object(RRA_SyncMgr * instance,\
                                       uint32_t type_id,\
                                       uint32_t object_id)

class RRASyncObjectType:
	def __init__(self,id,count,total_size,name1,name2):
		self.id = id
		self.count = count
		self.total_size = total_size
		self.name1 = name1
		self.name2 = name2

class RRAError:
	def __init__(self, rc):
		self.rc = rc

	def __str__(self):
		return str(self.rc)

# callback processing
#
# Callback for types

cdef bool _CB_TypesCallback(RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t * ids, context):
	ida=[]
	cdef bool rc
	for i from 0 <= i < count:
		ida.append(ids[i])
	rc=context.CB_TypeCallback(<RRA_SyncMgrTypeEvent> event,type,ida)
	return rc

cdef bool _CB_WriterCallback(uint32_t type_id, uint32_t obj_id, uint8_t * data, size_t data_size, context):
	pd=[]
	if data_size > 0:
		pd=PyString_FromStringAndSize(<char *>data,data_size)
	return context.CB_ObjectWriterCallback(type_id, obj_id, pd)

cdef size_t _CB_ReaderCallback(uint32_t type_id, uint32_t index, uint8_t * data, uint32_t max_size, context):
	cdef char * pstr
	pd=context.CB_ObjectReaderCallback(type_id, index, max_size)
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
	cdef RRA_SyncMgr * instance
	cdef bool connected
	cdef uint32_t ntypes
	
	def __cinit__(self):
		self.instance = <RRA_SyncMgr *>rra_syncmgr_new()
		if not self.instance:
			raise RRAError(-1)
		self.connected = 0
		self.ntypes = 0

	def __dealloc__(self):
		rra_syncmgr_destroy(self.instance)

	#
	# Connection and disconnection
	#

	def Connect(self,ip_addr="0.0.0.0"):
		self.connected = rra_syncmgr_connect(self.instance, ip_addr)
		return self.connected

	def isConnected(self):
		return self.connected

	def Disconnect(self):
		rra_syncmgr_disconnect(self.instance)
		self.connected=0
		return self.connected

	#
	# Event processing
	#

	def SubscribeObjectEvents(self,type_id):
		if self.connected != 0:
			rra_syncmgr_subscribe(self.instance, type_id, _CB_TypesCallback, self)
			return 0
		return -1

	def StartEventListener(self):
		if self.connected !=0:
			rra_syncmgr_start_events(self.instance)

	def GetEventDescriptor(self):
		return rra_syncmgr_get_event_descriptor(self.instance)
		return rc

	def WaitEvent(self,timeout):
		cdef bool gotone
		cdef bool rc
		rc=rra_syncmgr_event_wait(self.instance,timeout, &gotone)
		if rc:
			return gotone
		else:
			return 0

	def IsEventPresent(self):
		return rra_syncmgr_event_pending(self.instance)

	def HandleEvent(self):
		return rra_syncmgr_handle_event(self.instance)

	def HandleAllPendingEvents(self):
		return rra_syncmgr_handle_all_pending_events(self.instance)

	#
	# Object type ID handling
	#

	def GetObjectTypeCount(self):
		if self.connected !=0:
			self.ntypes = rra_syncmgr_get_type_count(self.instance)
			return self.ntypes
		return 0

	def GetObjectTypes(self):
		cdef RRA_SyncMgrType * thetypes
		rettypes = []
		if self.connected:
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
		cdef uint32_t * c_oids
		cdef uint32_t c_cnt

		c_cnt = len(oids)
		c_oids = <uint32_t *>malloc(sizeof(uint32_t)*c_cnt)
		rc=0
		if c_oids != NULL:
			for i from 0 <= i < c_cnt:
				c_oids[i] = oids[i]
			rc= rra_syncmgr_get_multiple_objects(self.instance,type_id, c_cnt, c_oids,_CB_WriterCallback, self)
			free(c_oids)
		return rc
	
	def GetSingleObject(self,type_id,obj_id):
		cdef uint8_t * arr
		cdef size_t cnt
		cdef bool rc
		data = ""
		rc=rra_syncmgr_get_single_object(self.instance,type_id,obj_id,&arr,<size_t *>&cnt)
		if rc:
			data=PyString_FromStringAndSize(<char *>arr,cnt)
			free(arr)
		return data


	def PutMultipleObjects(self, type_id, oid_array, newoid_array, flags):
		cdef uint32_t * c_oids
		cdef uint32_t * c_newoids
		cdef uint32_t c_oidcount
		c_oidcount = len(oid_array)
		c_oids = <uint32_t *>malloc(sizeof(uint32_t)*c_oidcount)
		rc = False
		if c_oids != NULL:
			c_newoids = <uint32_t *>malloc(sizeof(uint32_t)*c_oidcount)
			if c_newoids != NULL:
				rc = rra_syncmgr_put_multiple_objects(self.instance,type_id,c_oidcount,
								      c_oids,c_newoids,flags,_CB_ReaderCallback,self)
				if rc == True:
					for i from 0 <= i < c_oidcount:
						newoid_array.append(c_newoids[i])
			free(c_oids)
		return rc

	def MarkObjectUnchanged(self,type_id,obj_id):
		return rra_syncmgr_mark_object_unchanged(self.instance,type_id,obj_id)

	#
	# Object deletion
	#

	def DeleteObject(self,type_id, obj_id):
		return rra_syncmgr_delete_object(self.instance,type_id,obj_id)

	#
	# Callbacks
	#
	# (to be overloaded by user)
	#

	def CB_TypeCallback(self, event, type_id, idarray):
		return False
		
	def CB_ObjectWriterCallback(self,type_id, obj_id, data):
		return False

	def CB_ObjectReaderCallback(self,type_id, index, maxlen):
		return False
