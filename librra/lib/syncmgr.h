/* $Id$ */
#ifndef __rra_syncmgr_h__
#define __rra_syncmgr_h__

#include <synce.h>

/* Constants for use with rra_syncmgr_type_from_name() */
#define RRA_SYNCMGR_TYPE_APPOINTMENT  "Appointment"
#define RRA_SYNCMGR_TYPE_CONTACT      "Contact"
#define RRA_SYNCMGR_TYPE_FAVORITE     "Favorite"
#define RRA_SYNCMGR_TYPE_FILE         "File"
#define RRA_SYNCMGR_TYPE_INBOX        "Inbox"
#define RRA_SYNCMGR_TYPE_INK          "Ink"
#define RRA_SYNCMGR_TYPE_MERLIN_MAIL  "Merlin Mail"
#define RRA_SYNCMGR_TYPE_MS_TABLE     "MS Table"
#define RRA_SYNCMGR_TYPE_TASK         "Task"

struct _RRA_Uint32Vector;

struct _RRA_SyncMgr;
typedef struct _RRA_SyncMgr RRA_SyncMgr;

typedef struct 
{
  uint32_t  id;
  uint32_t  count;        /* number of objects in folder */
  uint32_t  total_size;   /* total size in bytes */
  time_t    modified;     /* 0 or last time any object was modified */
  char      name1[100];
  char      name2[80];
} RRA_SyncMgrType;

typedef enum 
{
  SYNCMGR_TYPE_EVENT_UNCHANGED,
  SYNCMGR_TYPE_EVENT_CHANGED,
  SYNCMGR_TYPE_EVENT_DELETED
} RRA_SyncMgrTypeEvent;

typedef bool (*RRA_SyncMgrTypeCallback)
  (RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie);

/* 
   Creation/destruction of RRA_SyncMgr object 
 */

RRA_SyncMgr* rra_syncmgr_new();
void rra_syncmgr_destroy(RRA_SyncMgr* self);

/* 
   Connect or disconnect from device 
 */

bool rra_syncmgr_connect(RRA_SyncMgr* self, const char *ip_addr);
void rra_syncmgr_disconnect(RRA_SyncMgr* self);
bool rra_syncmgr_is_connected(RRA_SyncMgr* self);

/* 
   Get information about the available object types 
 */

uint32_t rra_syncmgr_get_type_count(RRA_SyncMgr* self);
RRA_SyncMgrType* rra_syncmgr_get_types(RRA_SyncMgr* self);
RRA_SyncMgrType* rra_syncmgr_type_from_id(RRA_SyncMgr* self, uint32_t type_id);
RRA_SyncMgrType* rra_syncmgr_type_from_name(RRA_SyncMgr* self, const char* name);

/*#define RRA_SYNCMGR_INVALID_TYPE_ID   ((uint32_t)0xffffffff)*/

bool rra_syncmgr_get_deleted_object_ids(
    RRA_SyncMgr* self,
    uint32_t type_id,
    struct _RRA_Uint32Vector* current_ids,
    struct _RRA_Uint32Vector* deleted_ids);

bool rra_syncmgr_purge_deleted_object_ids(
    RRA_SyncMgr* self,
    uint32_t type_id,
    struct _RRA_Uint32Vector* deleted_ids);

bool rra_syncmgr_register_added_object_ids(
    RRA_SyncMgr* self,
    uint32_t type_id,
    struct _RRA_Uint32Vector* added_ids);

/** Select which object types we are interested in */
void rra_syncmgr_subscribe(RRA_SyncMgr* self, 
  uint32_t type, RRA_SyncMgrTypeCallback callback, void* cookie);

/** Unsubscribe from a previously selected object type */
void rra_syncmgr_unsubscribe(RRA_SyncMgr* self, 
  uint32_t type);

/* 
   Event handling 
 */

/** Start listening for events for the subscribed types */
bool rra_syncmgr_start_events(RRA_SyncMgr* self);

/** Get file descriptor for use with select() or poll() */
int rra_syncmgr_get_event_descriptor(RRA_SyncMgr* self);

/** See if there is an event pending */
bool rra_syncmgr_event_pending(RRA_SyncMgr* self);

/** See if there is an event pending */
bool rra_syncmgr_event_wait(RRA_SyncMgr* self, int timeoutInSeconds, bool* got_event);

/** Handle a pending event */
bool rra_syncmgr_handle_event(RRA_SyncMgr* self);

/** Empty queue of pending events */
bool rra_syncmgr_handle_all_pending_events(RRA_SyncMgr* self);

/*
   Get, add, update object data
 */

/** 
  Should work like the read(2) system call.
  No rra_syncmgr_* functions should be called from this callback!

  The object ID is object_id_array[index]
 */
typedef ssize_t (*RRA_SyncMgrReader)
  (uint32_t type_id, unsigned index, uint8_t* data, size_t data_size, void* cookie);

/** 
  Should work like the write(2) system call, but must take care of all data
  at once and return true/false for success/failure.
  No rra_syncmgr_* functions should be called from this callback! 
 */
typedef bool (*RRA_SyncMgrWriter)
  (uint32_t type_id, uint32_t object_id, const uint8_t* data, size_t data_size, void* cookie);

/** Get multiple objects. The 'writer' callback is called exactly once for each object. */
bool rra_syncmgr_get_multiple_objects(RRA_SyncMgr* self, 
    uint32_t type_id,
    uint32_t object_id_count,
    uint32_t* object_id_array,
    RRA_SyncMgrWriter writer,
    void* cookie);

/** Get a single object */
bool rra_syncmgr_get_single_object(RRA_SyncMgr* self, 
    uint32_t type_id,
    uint32_t object_id,
    uint8_t** data,
    size_t* data_size);

/** Free data buffer returned by rra_syncmgr_get_single_object */
void rra_syncmgr_free_data_buffer(uint8_t* buffer);

/** In order for an object to be marked unchanged, this function has to be called! */
bool rra_syncmgr_mark_object_unchanged(
    RRA_SyncMgr* self, 
    uint32_t type_id,
    uint32_t object_id);

#define RRA_SYNCMGR_NEW_OBJECT        2
#define RRA_SYNCMGR_UPDATE_OBJECT  0x40

/** Put multiple objects. The 'reader' call back is called at least once for each object. */
bool rra_syncmgr_put_multiple_objects(
    RRA_SyncMgr* self,  
    uint32_t type_id,
    uint32_t object_id_count,
    uint32_t* object_id_array,
    uint32_t* recv_object_id_array,
    uint32_t flags,
    RRA_SyncMgrReader reader,
    void* cookie);

bool rra_syncmgr_put_single_object(
    RRA_SyncMgr* self,
    uint32_t type_id,
    uint32_t object_id,
    uint32_t flags,
    uint8_t* data,
    size_t data_size,
    uint32_t* new_object_id);

/** Same thing as calling rra_syncmgr_put_single_object with flags set to RRA_SYNCMGR_NEW_OBJECT */
bool rra_syncmgr_new_object(
    RRA_SyncMgr* rra,  
    uint32_t type_id,
    uint8_t* data,
    size_t data_size,
    uint32_t* new_object_id);

/** Same thing as calling rra_syncmgr_put_single_object with flags set to RRA_SYNCMGR_UPDATE_OBJECT */
bool rra_syncmgr_update_object(
    RRA_SyncMgr* rra,  
    uint32_t type_id,
    uint32_t object_id,
    uint8_t* data,
    size_t data_size);

/** Delete an object */
bool rra_syncmgr_delete_object(
    RRA_SyncMgr* self, 
    uint32_t type_id, 
    uint32_t object_id);

#endif
