/* $Id$ */
#define _BSD_SOURCE 1
#include "syncmgr.h"
#include "rrac.h"
#include <synce_hash.h>
#include <synce_log.h>
#include <synce_socket.h>
#include <stdlib.h>
#include <string.h>

#define FREE(ptr)   if (ptr) free(ptr)

/*
   Generic SynCE list - move to libsynce later
 */

struct _SList
{
  void* next;
  void* data;
};
typedef struct _SList SList;

SList* s_list_new()
{
  return (SList*)calloc(1, sizeof(SList));
}

SList* s_list_append(SList* a, SList* b)
{
  if (a) 
  {
    while (a->next)
      a = a->next;
    a->next = b;
    return a;
  }
  else
    return b;
}

SList* s_list_prepend(SList* a, SList* b)
{
  return s_list_append(b, a);
}



typedef struct
{
  uint32_t type;
  SyncMgrTypeCallback callback;
  void* cookie;
} Subscription;

static Subscription* subscription_new(
    uint32_t type, SyncMgrTypeCallback callback, void* cookie)
{
  Subscription* self = (Subscription*)calloc(1, sizeof(Subscription));
  
  if (self)
  {
    self->type      = type;
    self->callback  = callback;
    self->cookie    = cookie;
  }
  
  return self;
}

void subscription_destroy(Subscription* self)
{
  FREE(self);
}

struct _SyncMgr
{
  RRAC* rrac;
  SHashTable* subscriptions;

  uint32_t type_count;
  SyncMgrType* types;

  SyncPartners partners;
};

static unsigned uint32_hash(const void *key)
{
  return *(uint32_t*)key;
}

static int uint32_compare(const void* a, const void* b)
{
  return *(uint32_t*)a == *(uint32_t*)b;
}

static bool syncmgr_retrieve_types(SyncMgr* self)
{
  RawObjectType* raw_object_types = NULL;
  unsigned i = 0;
  bool success = false;

  if (!rrac_send_6f(self->rrac, 0x3c1))
  {
    synce_error("Failed to send command 6f");
    goto exit;
  }

  if (!rrac_recv_reply_6f_c1(
        self->rrac, 
        &raw_object_types, 
        &self->type_count))
  {
    synce_error("Failed to receive reply");
    goto exit;
  }

  self->types = malloc(self->type_count * sizeof(SyncMgrType));

  for (i = 0; i < self->type_count; i++)
  {
    char* ascii = NULL;

    self->types[i].id          = raw_object_types[i].id;
    self->types[i].count       = raw_object_types[i].count;
    self->types[i].total_size  = raw_object_types[i].total_size;
    self->types[i].modified    = 
      filetime_to_unix_time(&raw_object_types[i].filetime);

    ascii = wstr_to_ascii(raw_object_types[i].name1);
    strcpy(self->types[i].name, ascii);
    wstr_free_string(ascii);
  }

  success = true;

exit:
	rrac_free(raw_object_types);
  return success;
}

SyncMgr* syncmgr_new()
{
  SyncMgr* self = (SyncMgr*)calloc(1, sizeof(SyncMgr));

  self->rrac = rrac_new();
  self->subscriptions = 
    s_hash_table_new(uint32_hash, uint32_compare, sizeof(uint32_t));

  return self;
}

void syncmgr_destroy(SyncMgr* self)
{
  if (self)
  {
    FREE(self->types);
    rrac_destroy(self->rrac);
    s_hash_table_destroy(self->subscriptions, 
        (SHashTableDataDestroy)subscription_destroy);
    free(self);
  }
}

bool syncmgr_connect(SyncMgr* self)
{
  if (self)
  {
    return 
      rrac_connect(self->rrac) &&
      syncmgr_retrieve_types(self);
  }
  else
  {
    synce_error("SyncMgr pointer is NULL");
    return false;
  }
}

void syncmgr_disconnect(SyncMgr* self)
{
  if (self) 
    rrac_disconnect(self->rrac);
}

uint32_t syncmgr_get_type_count(SyncMgr* self)
{
  return self->type_count;
}

SyncMgrType* syncmgr_get_types(SyncMgr* self)
{
  return self->types;
}

uint32_t syncmgr_type_from_name(SyncMgr* self, const char* name)/*{{{*/
{
  uint32_t result = 0xffffffff;
  unsigned i;
  
  if (!self->types)
  {
    synce_error("Not connected.");
    goto exit;
  }

  for (i = 0; i < self->type_count; i++)
  {
    if (0 == strcasecmp(name, self->types[i].name))
    {
      result = self->types[i].id;
      break;
    }
  }

exit:
  return result;
}/*}}}*/

void syncmgr_subscribe(SyncMgr* self, /*{{{*/
  uint32_t type, SyncMgrTypeCallback callback, void* cookie)
{
  if (self)
  {
    Subscription* subscription =
      subscription_new(type, callback, cookie);

    synce_trace("Subcribing to type %08x", type);

    s_hash_table_insert(self->subscriptions, &subscription->type, subscription);
  }
  else
    synce_error("SyncMgr pointer is NULL");
}/*}}}*/

bool syncmgr_start_events(SyncMgr* self)
{
  bool success = false;
  unsigned i;
  uint32_t ignored_count;
  uint32_t* ignored_ids = NULL;

  /*
   * Ignore object types
   */

  ignored_ids = malloc(self->type_count * sizeof(uint32_t));

  for (i = 0, ignored_count = 0; i < self->type_count; i++)
  {
    Subscription* subscription = (Subscription*)s_hash_table_lookup(
        self->subscriptions, &self->types[i].id);

    if (!subscription)
      ignored_ids[ignored_count++] = self->types[i].id;
  }

  if (ignored_count == self->type_count)
  {
    synce_error("No valid subscriptions");
  }

  rrac_send_70_3(self->rrac, ignored_ids, ignored_count);

  if (!rrac_recv_reply_70(self->rrac))
  {
    synce_error("rrac_recv_reply_70 failed");
    goto exit;
  }

  success = true;

exit:
  FREE(ignored_ids);
  return success;
}

int syncmgr_get_event_descriptor(SyncMgr* self)/*{{{*/
{
  if (self && self->rrac)
    return rrac_get_event_descriptor(self->rrac);
  else
    return SYNCE_SOCKET_INVALID_DESCRIPTOR;
}/*}}}*/

bool syncmgr_event_pending(SyncMgr* self)/*{{{*/
{
  if (self && self->rrac)
    return rrac_event_pending(self->rrac);
  else
    return false;
}/*}}}*/

bool syncmgr_event_wait(SyncMgr* self, int timeoutInSeconds)
{
  if (self && self->rrac)
    return rrac_event_wait(self->rrac, timeoutInSeconds);
  else
    return false;
}

static bool syncmgr_make_callback(/*{{{*/
    SyncMgr* self, 
    SyncMgrTypeEvent event,
    uint32_t type,
    uint32_t count,
    uint32_t* ids)
{
  bool success = false;

  if (self)
  {
    Subscription* subscription = (Subscription*)s_hash_table_lookup(
        self->subscriptions, &type);

    synce_trace("type = %08x, subscription %08x", type, subscription);

    if (subscription)
    {
      success = subscription->callback(
          event, type, count, ids, subscription->cookie);
    }
    else
      /* no subscription found is OK! */
      success = true;
  }
  else
    synce_error("SyncMgr object is NULL");

  return success;
}/*}}}*/

static bool syncmgr_on_notify_ids(SyncMgr* self, SyncCommand* command)/*{{{*/
{
  bool success = false;
  SyncNotifyHeader header;
  uint32_t* ids = NULL;

  if (!sync_command_notify_header(command, &header))
  {
    synce_error("Failed to get notify header");
    goto exit;
  }

  ids = calloc(header.total, sizeof(uint32_t));

  if (!sync_command_notify_ids(command, ids))
  {
    synce_error("Failed to get notify IDs");
    goto exit;
  }

  if (header.unchanged)
  {
    success = syncmgr_make_callback(
        self,
        SYNCMGR_TYPE_EVENT_UNCHANGED,
        header.type,
        header.unchanged,
        ids);

    if (!success)
      goto exit;
  }

  if (header.changed)
  {
    success = syncmgr_make_callback(
        self,
        SYNCMGR_TYPE_EVENT_CHANGED,
        header.type,
        header.changed,
        ids + header.unchanged);

    if (!success)
      goto exit;
  }

  /* TODO: find out deleted IDs */

exit:
  FREE(ids);
  return success;
}/*}}}*/

static bool syncmgr_on_notify(SyncMgr* self, SyncCommand* command)/*{{{*/
{
  bool success = false;

  synce_trace("Notify code = %08x", sync_command_notify_code(command));

  switch (sync_command_notify_code(command))
  {
    case SYNC_COMMAND_NOTIFY_PARTNERS:
      success = sync_command_notify_partners(command, &self->partners);
      break;

    case SYNC_COMMAND_NOTIFY_IDS_4:
    case SYNC_COMMAND_NOTIFY_IDS_6:
      success = syncmgr_on_notify_ids(self, command);
      break;

    case SYNC_COMMAND_NOTIFY_INVALID:
    default:
      synce_error("Unknown notify code: %08x",
          sync_command_notify_code(command));
      break;
  }
  
  return success;
}/*}}}*/

bool syncmgr_handle_event(SyncMgr* self)/*{{{*/
{
  bool success = false;
  SyncCommand* command = rrac_recv_command(self->rrac);

  if (command)
  {
    synce_trace("code = %08x", sync_command_code(command));

    switch (sync_command_code(command))
    {
      case SYNC_COMMAND_NOTIFY:
        success = syncmgr_on_notify(self, command);
        break;

      case SYNC_COMMAND_ERROR:
        break;

      default:
        synce_error("Unhandled command: %4x", sync_command_code(command));
        break;
    }

    sync_command_destroy(command);
  }
  
  return success;
}/*}}}*/
