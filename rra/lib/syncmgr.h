/* $Id$ */
#ifndef __syncmgr_h__
#define __syncmgr_h__

#include <synce.h>

struct _SyncMgr;
typedef struct _SyncMgr SyncMgr;

typedef struct 
{
  uint32_t  id;
  uint32_t  count;        /* number of objects in folder */
  uint32_t  total_size;   /* total size in bytes */
  time_t    modified;     /* 0 or last time any object was modified */
  char      name[100];
} SyncMgrType;

typedef enum 
{
  SYNCMGR_TYPE_EVENT_UNCHANGED,
  SYNCMGR_TYPE_EVENT_CHANGED,
  SYNCMGR_TYPE_EVENT_DELETED
} SyncMgrTypeEvent;

typedef bool (*SyncMgrTypeCallback)
  (SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie);

/* 
   Creation/destruction of SyncMgr object 
 */

SyncMgr* syncmgr_new();
void syncmgr_destroy(SyncMgr* self);

/* 
   Connect or disconnect from device 
 */

bool syncmgr_connect(SyncMgr* self);
void syncmgr_disconnect(SyncMgr* self);

/* 
   Get information about the available object types 
 */

uint32_t syncmgr_get_type_count(SyncMgr* self);
SyncMgrType* syncmgr_get_types(SyncMgr* self);
uint32_t syncmgr_type_from_name(SyncMgr* self, const char* name);

/** Select which object types we are interested in */
void syncmgr_subscribe(SyncMgr* self, 
  uint32_t type, SyncMgrTypeCallback callback, void* cookie);

/* 
   Event handling 
 */

/** Start listening for events for the subscribed types */
bool syncmgr_start_events(SyncMgr* self);

/** Get file descriptor for use with select() or poll() */
int syncmgr_get_event_descriptor(SyncMgr* self);

/** See if there is an event pending */
bool syncmgr_event_pending(SyncMgr* self);

/** See if there is an event pending */
bool syncmgr_event_wait(SyncMgr* self, int timeoutInSeconds);

/** Handle a pending event */
bool syncmgr_handle_event(SyncMgr* self);



#endif
