/* $Id$ */
#ifndef __connection_h__
#define __connection_h__

#include <multisync.h>
#include <rra/syncmgr.h>
#include <rra/timezone.h>
#include <pthread.h>

enum 
{
  INDEX_APPOINTMENT,
  INDEX_CONTACT,
  INDEX_TASK,
  INDEX_MAX
};

typedef struct _SynceObject
{
  int type_index;
  uint32_t type_id;
  uint32_t object_id;
  RRA_SyncMgrTypeEvent event;
  char* data;
  int change_counter;
} SynceObject;

typedef struct _SynceConnection
{
  /** Data used by MultiSync */
  client_connection commondata;

  /** Handle used when talking to MultiSync */
  sync_pair* handle;

  /** Our connection */
  RRA_SyncMgr*  syncmgr;

  /** Timezone information */
  RRA_Timezone timezone;

  /** Type IDs */
  uint32_t type_ids[3];

  /** Updated by the event callback */
  GHashTable* objects[3];

  /** Thread */
  pthread_t thread;

  /** Boolean that should be set to false to cancel thread */
  bool thread_running;

  /** Lock for access to hashtables and change counters */
  pthread_mutex_t lock;

  int last_change_counter;
  int change_counter;

} SynceConnection;

int synce_index_from_sync_object_type(sync_object_type objtype);
void synce_free_object_data(SynceObject* object);

bool synce_subscribe(SynceConnection* connection);
bool synce_create_thread(SynceConnection* connection);
bool synce_join_thread(SynceConnection* connection);

bool synce_connect(SynceConnection* connection);
void synce_disconnect(SynceConnection* connection);

bool synce_get_all_changes(
    SynceConnection* connection, 
    sync_object_type newdbs, 
    change_info* info);

bool synce_mark_objects_as_unchanged(
    SynceConnection* connection);

#endif

