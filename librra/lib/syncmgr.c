/* $Id$ */
#define _BSD_SOURCE 1
#include "syncmgr.h"
#include "rrac.h"
#include "uint32vector.h"
#include <parser.h>
#include <synce_hash.h>
#include <synce_log.h>
#include <synce_socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h> /* for MIN(a,b) */

#define VERBOSE 0

static const char* RRA_DIRECTORY    = "rra";

#define SEND_COMMAND_6F_6 1

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
  RRA_SyncMgrTypeCallback callback;
  void* cookie;
} Subscription;

static Subscription* subscription_new(
    uint32_t type, RRA_SyncMgrTypeCallback callback, void* cookie)
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

static void subscription_destroy(Subscription* self)
{
  FREE(self);
}

struct _RRA_SyncMgr
{
  RRAC* rrac;
  SHashTable* subscriptions;
  bool receiving_events;

  uint32_t type_count;
  RRA_SyncMgrType* types;

  SyncPartners partners;
};

static unsigned uint32_hash(const void *key)/*{{{*/
{
  return *(uint32_t*)key;
}/*}}}*/

static int uint32_compare(const void* a, const void* b)/*{{{*/
{
  return *(uint32_t*)a == *(uint32_t*)b;
}/*}}}*/

static bool rra_syncmgr_retrieve_types(RRA_SyncMgr* self)/*{{{*/
{
  RawObjectType* raw_object_types = NULL;
  unsigned i = 0;
  bool success = false;

  /* 0x7c1 is sent by ActiveSync 3.7.1 */
  if (!rrac_send_6f(self->rrac, 0x7c1))
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

  if (self->types)
    free(self->types);

  self->types = malloc(self->type_count * sizeof(RRA_SyncMgrType));

  for (i = 0; i < self->type_count; i++)
  {
    char* ascii = NULL;

    self->types[i].id          = raw_object_types[i].id;
    self->types[i].count       = raw_object_types[i].count;
    self->types[i].total_size  = raw_object_types[i].total_size;

    if (!parser_filetime_to_unix_time(&raw_object_types[i].filetime, &self->types[i].modified))
      self->types[i].modified = 0;

    ascii = wstr_to_ascii(raw_object_types[i].name1);
    strcpy(self->types[i].name1, ascii);
    wstr_free_string(ascii);

    ascii = wstr_to_ascii(raw_object_types[i].name2);
    strcpy(self->types[i].name2, ascii);
    wstr_free_string(ascii);
  }

  success = true;

exit:
	rrac_free(raw_object_types);
  return success;
}/*}}}*/

RRA_SyncMgr* rra_syncmgr_new()/*{{{*/
{
  RRA_SyncMgr* self = (RRA_SyncMgr*)calloc(1, sizeof(RRA_SyncMgr));

  self->rrac = rrac_new();
  self->subscriptions = 
    s_hash_table_new(uint32_hash, uint32_compare, sizeof(uint32_t));

  return self;
}/*}}}*/

void rra_syncmgr_destroy(RRA_SyncMgr* self)/*{{{*/
{
  if (self)
  {
    FREE(self->types);
    rrac_destroy(self->rrac);
    s_hash_table_destroy(self->subscriptions, 
        (SHashTableDataDestroy)subscription_destroy);
    free(self);
  }
}/*}}}*/

bool rra_syncmgr_connect(RRA_SyncMgr* self, const char *ip_addr)/*{{{*/
{
  if (self)
  {
    return 
            rrac_connect(self->rrac, ip_addr) &&
      rra_syncmgr_retrieve_types(self);
  }
  else
  {
    synce_error("RRA_SyncMgr pointer is NULL");
    return false;
  }
}/*}}}*/

void rra_syncmgr_disconnect(RRA_SyncMgr* self)/*{{{*/
{
  if (self) 
  {
    rrac_disconnect(self->rrac);
    self->receiving_events = FALSE;
  }
}/*}}}*/

bool rra_syncmgr_is_connected(RRA_SyncMgr* self)
{
  return self && rrac_is_connected(self->rrac);
}

uint32_t rra_syncmgr_get_type_count(RRA_SyncMgr* self)/*{{{*/
{
  if (self)
    return self->type_count;
  else
    return 0;
}/*}}}*/

RRA_SyncMgrType* rra_syncmgr_get_types(RRA_SyncMgr* self)/*{{{*/
{
  if (self)
    return self->types;
  else
    return NULL;
}/*}}}*/

RRA_SyncMgrType* rra_syncmgr_type_from_id(RRA_SyncMgr* self, uint32_t type_id)/*{{{*/
{
  RRA_SyncMgrType* result = NULL;
  unsigned i;
  
  if (!self || !self->types)
  {
    synce_error("Not connected.");
    goto exit;
  }

  for (i = 0; i < self->type_count; i++)
  {
    if (self->types[i].id == type_id)
    {
      result = &self->types[i];
      break;
    }
  }

exit:
  return result;
}/*}}}*/

RRA_SyncMgrType* rra_syncmgr_type_from_name(RRA_SyncMgr* self, const char* name)/*{{{*/
{
  RRA_SyncMgrType* result = NULL;
  unsigned i;
  
  if (!self || !self->types)
  {
    synce_error("Not connected.");
    goto exit;
  }

  for (i = 0; i < self->type_count; i++)
  {
    if (0 == strcasecmp(name, self->types[i].name1) ||
        0 == strcasecmp(name, self->types[i].name2) )
    {
      result = &self->types[i];
      break;
    }
  }

exit:
  return result;
}/*}}}*/

bool rra_syncmgr_get_deleted_object_ids(/*{{{*/
    RRA_SyncMgr* self,
    uint32_t type_id,
    RRA_Uint32Vector* current_ids,
    RRA_Uint32Vector* deleted_ids)
{
  bool success = false;
  char* directory = NULL;	
  char filename[256];
  FILE* file = NULL;
  unsigned current;
  unsigned previous;
  RRA_Uint32Vector* previous_ids = rra_uint32vector_new();

  if (self->partners.current != 1 &&
      self->partners.current != 2)
  {
    synce_error("No current partnership");
    goto exit;
  }

  if (!synce_get_subdirectory(RRA_DIRECTORY, &directory))
  {
    synce_error("Failed to get rra directory path");
    goto exit;
  }

  snprintf(filename, sizeof(filename), "%s/partner-%08x-type-%08x", directory,
      self->partners.ids[self->partners.current - 1], type_id);

  /*
     Create list of previous IDs
   */

  file = fopen(filename, "r");
  if (file)
  {
    char buffer[16];
    while (fgets(buffer, sizeof(buffer), file))
    {
      rra_uint32vector_add(previous_ids, strtol(buffer, NULL, 16));
    }

    fclose(file);
  }

  /* Sort vectors */
  rra_uint32vector_sort(previous_ids);
  rra_uint32vector_sort(current_ids);

  /*
     Iterate both vectors and see what is missing from the previous
   */

  for (current = 0, previous = 0;
      current < current_ids->used && previous < previous_ids->used; )
  {
    /* synce_trace("current id: %08x    previous id: %08x", 
       current_ids->items[current], previous_ids->items[previous]); */

    if (current_ids->items[current] > previous_ids->items[previous])
    {
      /* deleted item */
      rra_uint32vector_add(deleted_ids, previous_ids->items[previous]);
      /* synce_trace("deleted item: %08x", previous_ids->items[previous]); */
      previous++;
    }
    else if (current_ids->items[current] < previous_ids->items[previous])
    {
      /* new item */
      current++;
    }
    else
    {
      /* the IDs are equal */
      current++;
      previous++;
    }
  }

  /*
     Any IDs left at the end of the previous_ids vector are deleted
   */

  for (; previous < previous_ids->used; previous++)
  {
    rra_uint32vector_add(deleted_ids, previous_ids->items[previous]);
    /* synce_trace("deleted item: %08x", previous_ids->items[previous]); */
  }

  /*
     Save current ID list
   */

  file = fopen(filename, "w");
  if (!file)
  {
    synce_error("Failed to open '%s' for writing.", filename);
    goto exit;
  }

  if (file)
  {
    for (current = 0; current < current_ids->used; current++)
    {
      char buffer[16];
      snprintf(buffer, sizeof(buffer), "%08x\n", current_ids->items[current]);
      fwrite(buffer, strlen(buffer), 1, file);
    }

    fclose(file);
  }


  success = true;

exit:
  if (directory)
    free(directory);
  rra_uint32vector_destroy(previous_ids, true);
  return success;
}/*}}}*/

bool rra_syncmgr_purge_deleted_object_ids(/*{{{*/
    RRA_SyncMgr* self,
    uint32_t type_id,
    struct _RRA_Uint32Vector* deleted_ids)
{
  bool success = false;
  char* directory = NULL;
  char filename[256];
  FILE* file = NULL;
  unsigned deleted;
  unsigned previous;
  unsigned current;
  RRA_Uint32Vector* previous_ids = rra_uint32vector_new();
  RRA_Uint32Vector* new_current_ids = rra_uint32vector_new();

  if (self->partners.current != 1 &&
    self->partners.current != 2)
  {
    synce_error("No current partnership");
    goto exit;
  }

  if (!synce_get_subdirectory(RRA_DIRECTORY, &directory))
  {
    synce_error("Failed to get rra directory path");
    goto exit;
  }
  snprintf(filename, sizeof(filename), "%s/partner-%08x-type-%08x", directory,
  self->partners.ids[self->partners.current - 1], type_id);

  /*
     Create list of previous IDs
   */

  file = fopen(filename, "r");
  if (file)
  {
    char buffer[16];
    while (fgets(buffer, sizeof(buffer), file))
    {
      rra_uint32vector_add(previous_ids, strtol(buffer, NULL, 16));
    }
    fclose(file);
  }

  /* Sort vectors */
  rra_uint32vector_sort(previous_ids);
  rra_uint32vector_sort(deleted_ids);

  for (previous = 0, deleted = 0;
      previous < previous_ids->used && deleted < deleted_ids->used; ) {
    if (deleted_ids->items[deleted] > previous_ids->items[previous]) {
      /* previous item not deleted, so append to new_current_ids */
      rra_uint32vector_add(new_current_ids, previous_ids->items[previous]);
      previous++;
    } else if (deleted_ids->items[deleted] == previous_ids->items[previous]) {
      /* previous item deleted, don't append, but proceed with the next items */
      deleted++;
      previous++;
    } else {
      /* should not happen - deleted item not found in current_ids */
      deleted++;
    }
  }

  /*
      Any IDs left at the end of the previous_ids vector are new_current_ids
   */

  for (; previous < previous_ids->used; previous++)
  {
    rra_uint32vector_add(new_current_ids, previous_ids->items[previous]);
  }

  /*
      Save current ID list
   */

  file = fopen(filename, "w");
  if (!file)
  {
    synce_error("Failed to open '%s' for writing.", filename);
    goto exit;
  }

  if (file)
  {
    for (current = 0; current < new_current_ids->used; current++)
    {
      char buffer[16];
      snprintf(buffer, sizeof(buffer), "%08x\n", new_current_ids->items[current]);
      fwrite(buffer, strlen(buffer), 1, file);
    }

    fclose(file);
  }


  success = true;

exit:
  if (directory)
    free(directory);
  rra_uint32vector_destroy(previous_ids, true);
  rra_uint32vector_destroy(new_current_ids, true);
  return success;
}/*}}}*/

bool rra_syncmgr_register_added_object_ids(/*{{{*/
    RRA_SyncMgr* self,
    uint32_t type_id,
    struct _RRA_Uint32Vector* added_ids)
{
  bool success = false;
  char* directory = NULL;
  char filename[256];
  FILE* file = NULL;
  unsigned previous;
  unsigned added;
  RRA_Uint32Vector* previous_ids = rra_uint32vector_new();

  if (self->partners.current != 1 &&
    self->partners.current != 2)
  {
    synce_error("No current partnership");
    goto exit;
  }

  if (!synce_get_subdirectory(RRA_DIRECTORY, &directory))
  {
    synce_error("Failed to get rra directory path");
    goto exit;
  }
  snprintf(filename, sizeof(filename), "%s/partner-%08x-type-%08x", directory,
  self->partners.ids[self->partners.current - 1], type_id);

  /*
     Create list of previous IDs
   */

  file = fopen(filename, "r");
  if (file)
  {
    char buffer[16];
    while (fgets(buffer, sizeof(buffer), file))
    {
      rra_uint32vector_add(previous_ids, strtol(buffer, NULL, 16));
    }
    fclose(file);
  }

  for (added = 0; added < added_ids->used; added++)
  {
    rra_uint32vector_add(previous_ids, added_ids->items[added]);
  }

  /* Sort vector */
  rra_uint32vector_sort(previous_ids);

  /*
      Save current ID list
   */

  file = fopen(filename, "w");
  if (!file)
  {
    synce_error("Failed to open '%s' for writing.", filename);
    goto exit;
  }

  if (file)
  {
    for (previous = 0; previous < previous_ids->used; previous++)
    {
      char buffer[16];
      snprintf(buffer, sizeof(buffer), "%08x\n", 
previous_ids->items[previous]);
      fwrite(buffer, strlen(buffer), 1, file);
    }

    fclose(file);
  }


  success = true;

exit:
  if (directory)
    free(directory);
  rra_uint32vector_destroy(previous_ids, true);
  return success;
}/*}}}*/

void rra_syncmgr_subscribe(RRA_SyncMgr* self, /*{{{*/
  uint32_t type, RRA_SyncMgrTypeCallback callback, void* cookie)
{
  if (self)
  {
    Subscription* subscription =
      subscription_new(type, callback, cookie);

    synce_trace("Subcribing to type %08x", type);

    s_hash_table_insert(self->subscriptions, &subscription->type, subscription);
  }
  else
    synce_error("RRA_SyncMgr pointer is NULL");
}/*}}}*/

void rra_syncmgr_unsubscribe(RRA_SyncMgr* self, uint32_t type) /*{{{*/
{
  if (self)
  {
    Subscription* subscription =
      s_hash_table_remove(self->subscriptions, &type);
    
    if (subscription) {
      synce_trace("Unsubscribed from type %08x", type);
      subscription_destroy(subscription);
    }
  }
}/*}}}*/   

bool rra_syncmgr_start_events(RRA_SyncMgr* self)/*{{{*/
{
  bool success = false;
  unsigned i;
  uint32_t ignored_count;
  uint32_t* ignored_ids = NULL;

  if (self->receiving_events)
  {
    synce_warning("Already receiving events");
    return true;
  }

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

  success = self->receiving_events = true;

exit:
  FREE(ignored_ids);
  return success;
}/*}}}*/

int rra_syncmgr_get_event_descriptor(RRA_SyncMgr* self)/*{{{*/
{
  if (self && self->rrac)
    return rrac_get_event_descriptor(self->rrac);
  else
    return SYNCE_SOCKET_INVALID_DESCRIPTOR;
}/*}}}*/

bool rra_syncmgr_event_pending(RRA_SyncMgr* self)/*{{{*/
{
  if (self && self->rrac)
    return rrac_event_pending(self->rrac);
  else
    return false;
}/*}}}*/

bool rra_syncmgr_event_wait(RRA_SyncMgr* self, int timeoutInSeconds, bool* got_event)/*{{{*/
{
  if (self && self->rrac)
    return rrac_event_wait(self->rrac, timeoutInSeconds, got_event);
  else
    return false;
}/*}}}*/

static bool rra_syncmgr_make_callback(/*{{{*/
    RRA_SyncMgr* self, 
    RRA_SyncMgrTypeEvent event,
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
    synce_error("RRA_SyncMgr object is NULL");

  return success;
}/*}}}*/

static bool rra_syncmgr_on_notify_ids(RRA_SyncMgr* self, SyncCommand* command)/*{{{*/
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

  /* assume success in case none of the following calls get called */
  success = TRUE;

  /* Unchanged and deleted are never set in the same message */
  if (header.unchanged)
  {
    success = rra_syncmgr_make_callback(
        self,
        SYNCMGR_TYPE_EVENT_UNCHANGED,
        header.type,
        header.unchanged,
        ids);

    if (!success)
      goto exit;
  }
  else if (header.deleted)
  {
    success = rra_syncmgr_make_callback(
        self,
        SYNCMGR_TYPE_EVENT_DELETED,
        header.type,
        header.deleted,
        ids);

    if (!success)
      goto exit;
  }

  if (header.changed)
  {
    success = rra_syncmgr_make_callback(
        self,
        SYNCMGR_TYPE_EVENT_CHANGED,
        header.type,
        header.changed,
        ids + header.deleted + header.unchanged);

    if (!success)
      goto exit;
  }

  if (SYNC_COMMAND_NOTIFY_IDS_4 == header.notify_code ||
      SYNC_COMMAND_NOTIFY_IDS_6 == header.notify_code)
  {
    /* TODO: Figure out missing items and send SYNCMGR_TYPE_EVENT_DELETED for those */
  }

  if (SYNC_COMMAND_NOTIFY_UPDATE == header.notify_code)
  {
  
  }

exit:
  FREE(ids);
  return success;
}/*}}}*/

static bool rra_syncmgr_on_negotiation(RRA_SyncMgr* self, SyncCommand* command)/*{{{*/
{
  bool success = false;
  SyncNegotiation negotiation;

  if (!sync_command_negotiation_get(command, &negotiation))
  {
    synce_error("Failed to get negotiation info");
    goto exit;
  }

  synce_trace("%08x %08x %08x %08x",
      negotiation.type_id,
      negotiation.old_id,
      negotiation.new_id,
      negotiation.flags);

  if (negotiation.old_id != negotiation.new_id)
  {
    synce_error("We are supposed to reply!");
  }

  success = true;
 
exit:
  return success;
}/*}}}*/

static bool rra_syncmgr_on_notify(RRA_SyncMgr* self, SyncCommand* command)/*{{{*/
{
  bool success = false;

  synce_trace("Notify code = %08x", sync_command_notify_code(command));

  switch (sync_command_notify_code(command))
  {
    case SYNC_COMMAND_NOTIFY_PARTNERS:
      success = sync_command_notify_partners(command, &self->partners);
      break;

    case SYNC_COMMAND_NOTIFY_UPDATE:
    case SYNC_COMMAND_NOTIFY_IDS_4:
    case SYNC_COMMAND_NOTIFY_IDS_6:
      success = rra_syncmgr_on_notify_ids(self, command);
      break;

    case SYNC_COMMAND_NOTIFY_INVALID:
    default:
      synce_error("Unknown notify code: %08x",
          sync_command_notify_code(command));
      break;
  }
  
  return success;
}/*}}}*/

bool rra_syncmgr_handle_event(RRA_SyncMgr* self)/*{{{*/
{
  bool success = false;
  SyncCommand* command = rrac_recv_command(self->rrac);

  if (command)
  {
    synce_trace("code = %08x", sync_command_code(command));

    switch (sync_command_code(command))
    {
      case SYNC_COMMAND_NEGOTIATION:
        success = rra_syncmgr_on_negotiation(self, command);
        break;
      
      case SYNC_COMMAND_NOTIFY:
        success = rra_syncmgr_on_notify(self, command);
        break;

      case SYNC_COMMAND_ERROR:
        break;

      default:
        synce_error("Unhandled command: %4x", sync_command_code(command));
        break;
    }

    sync_command_destroy(command);
  }
  else
  {
    synce_error("Failed to receive event, closing connection!");
    rra_syncmgr_disconnect(self);
  }
  
  return success;
}/*}}}*/

bool rra_syncmgr_handle_all_pending_events(RRA_SyncMgr* self)/*{{{*/
{
  if (!self)
  {
    synce_error("RRA_SyncMgr pointer is NULL");
    return false;
  }
  
  while (rra_syncmgr_event_pending(self))
  {
    if (!rra_syncmgr_handle_event(self))
    {
      synce_error("Failed to handle event");
      return false;
    }
  }

  return true;
}/*}}}*/

bool rra_syncmgr_get_multiple_objects(RRA_SyncMgr* self, /*{{{*/
    uint32_t type_id,
    uint32_t object_id_count,
    uint32_t* object_id_array,
    RRA_SyncMgrWriter writer,
    void* cookie)
{
  bool success = false;
	uint32_t recv_object_id;
	uint32_t recv_type_id;
  uint8_t* data;
  size_t data_size;
  unsigned i;

  /* do absolutely nothing if object_id_count is zero! */
  if (!object_id_count)
    return true;

  if (self->receiving_events)
    if (!rra_syncmgr_handle_all_pending_events(self))
    {
      synce_error("Failed to handle pending events");
      goto exit;
    }

	/* Ask for object data */
	if (!rrac_send_67(self->rrac, type_id, object_id_array, object_id_count))
	{
    synce_error("Failed to request object data");
		goto exit;
	}

  for (i = 0; i < object_id_count; i++)
  {
    /* Receive object data */
    if (!rrac_recv_data(self->rrac, &recv_object_id, &recv_type_id, &data, &data_size))
    {
      synce_error("Failed to receive data");
      goto exit;
    }

    if (recv_type_id != type_id)
    {
      synce_error("Unexpected object type");
      goto exit;
    }

    /* Write to calling application */
    if (!writer(recv_type_id, recv_object_id, data, data_size, cookie))
    {
      synce_error("Writer callback failed");
      goto exit;
    }

    rrac_free(data);
  }

	/* Receive end-of-data object */
	if (!rrac_recv_data(self->rrac, NULL, NULL, NULL, NULL))
	{
		synce_error("rrac_recv_data failed");
		goto exit;
	}
	
  success = true;

exit:
  return success;
}/*}}}*/

typedef struct
{
	uint32_t object_id;
  uint8_t* data;
  size_t data_size;
} ObjectData;

static bool rra_syncmgr_get_single_object_writer(/*{{{*/
    uint32_t type_id, uint32_t object_id, const uint8_t* data, size_t data_size, void* cookie)
{
  ObjectData* object = (ObjectData*)cookie;

  object->object_id = object_id;
  object->data = malloc(data_size);
  memcpy(object->data, data, data_size);
  object->data_size = data_size;

  return true;
}/*}}}*/

bool rra_syncmgr_mark_object_unchanged(
    RRA_SyncMgr* self, 
    uint32_t type_id,
    uint32_t object_id)
{
  bool success = false;

  if (!rrac_send_65(
        self->rrac, 
        type_id, 
        object_id, 
        object_id,
        0))
  {
    synce_error("Failed to send command 65");
    goto exit;
  }

  success = true;

exit:
  return success;
}

bool rra_syncmgr_get_single_object(RRA_SyncMgr* self, /*{{{*/
    uint32_t type_id,
    uint32_t object_id,
    uint8_t** data,
    size_t* data_size)
{
  bool success = false;
  ObjectData object;

  memset(&object, 0, sizeof(ObjectData));

  if (!rra_syncmgr_get_multiple_objects(
        self,
        type_id, 
        1, 
        &object_id, 
        rra_syncmgr_get_single_object_writer, 
        &object))
  {
    synce_error("Failed to get object");
    goto exit;
  }

  if (object.object_id != object_id)
  {
    synce_error("Unexpected object received");
    goto exit;
  }

  *data       = object.data;
  *data_size  = object.data_size;

  success = true;

exit:
  return success;
}/*}}}*/

#define SYNCMGRREADER_BUFFER_SIZE   32768

bool rra_syncmgr_put_multiple_objects(/*{{{*/
    RRA_SyncMgr* self,  
    uint32_t type_id,
    uint32_t object_id_count,
    uint32_t* object_id_array,
    uint32_t* recv_object_id_array,
    uint32_t flags,
    RRA_SyncMgrReader reader,
    void* cookie)
{
  bool success = false;
  unsigned i;
  uint32_t recv_type_id;
  uint32_t recv_object_id1;
  uint32_t recv_object_id2;
  uint32_t recv_flags;
  uint8_t* data = NULL;
  size_t max_data_size = 0;
  uint32_t adjusted_flags;

  /* do absolutely nothing if object_id_count is zero! */
  if (!object_id_count)
    return true;

  if (self->receiving_events)
    if (!rra_syncmgr_handle_all_pending_events(self))
    {
      synce_error("Failed to handle pending events");
      goto exit;
    }

  /* Write all data */
  for (i = 0; i < object_id_count; i++)
  {
    size_t data_size = 0;
    ssize_t bytes_read = 0;

    /* Read data with reader callback */
    for (;;)
    {
      if (max_data_size < data_size + SYNCMGRREADER_BUFFER_SIZE)
      {
        max_data_size = data_size + SYNCMGRREADER_BUFFER_SIZE;
        data = realloc(data, max_data_size);
      }

      bytes_read = reader(
          type_id, 
          i, 
          data + data_size, 
          SYNCMGRREADER_BUFFER_SIZE, 
          cookie);

      if (bytes_read < 0)
      {
        synce_error("Reader callback failed");
        /* trigger error handler below this loop */
        data_size = 0;
        break;
      }

      if (bytes_read == 0)
        break;

      data_size += bytes_read;
    }

    if (data_size == 0)
    {
      synce_error("Empty object of type %08x with ID %08x, ignoring.",
          type_id, object_id_array[i]);
      object_id_array[i] = 0xffffffff;
      continue;
    }

    if (object_id_array[i] == 0 && flags == RRA_SYNCMGR_UPDATE_OBJECT)
      adjusted_flags = RRA_SYNCMGR_NEW_OBJECT;
    else
      adjusted_flags = flags;

    /* Write data to device */
    if (!rrac_send_data(
          self->rrac, 
          object_id_array[i], 
          type_id, 
          adjusted_flags, 
          data, 
          data_size))
    {
      synce_error("Failed to send data for object of type %08x and ID %08x",
          type_id, object_id_array[i]);
      object_id_array[i] = 0xffffffff;
    }
  }

#if 0
  /* Write end-of-data marker */
  if (!rrac_send_data(self->rrac, OBJECT_ID_STOP, type_id, 0, NULL, 0))
  {
    synce_error("Failed to send stop entry");
    goto exit;	
  }
#endif

  /* Negotiate object IDs */
  for (i = 0; i < object_id_count; i++)
  {
    if (object_id_array[i] == 0xffffffff)
    {
      /* Mark object as invalid */
      if (recv_object_id_array)
        recv_object_id_array[i] = 0xffffffff;
      continue;
    }
    
    if (!rrac_recv_65(
          self->rrac, 
          &recv_type_id, 
          &recv_object_id1, 
          &recv_object_id2,
          &recv_flags))
    {
      synce_error("Failed to receive command 65");
      goto exit;
    }

#if VERBOSE
    synce_trace("Received command 65: type = %08x, id1 = %08x, id2 = %08x, flags = %08x",
        recv_type_id, recv_object_id1, recv_object_id2, recv_flags);
#endif

    if (recv_type_id != type_id || recv_object_id1 != object_id_array[i])
    {
      synce_error("Unexpected type or object id");
      goto exit;
    }

    if (recv_flags != RRA_SYNCMGR_NEW_OBJECT &&
        recv_flags != RRA_SYNCMGR_UPDATE_OBJECT)
    {
      synce_warning("Unexpected flags: %08x", recv_flags);
    }

    if (recv_object_id1 != recv_object_id2)
    {
      if (!rrac_send_65(
            self->rrac, 
            type_id, 
            recv_object_id2, 
            recv_object_id2,
            0x08000000))
      {
        synce_error("Failed to send command 65");
        goto exit;
      }
    }

    if (recv_object_id_array)
      recv_object_id_array[i] = recv_object_id2;
  }

#if SEND_COMMAND_6F_6
	if (!rrac_send_6f(self->rrac, 6))
	{
		synce_error("rrac_send_6f failed");
		goto exit;
	}
#endif

#if SEND_COMMAND_6F_6
	if (!rrac_recv_reply_6f_6(self->rrac))
	{
		synce_error("rrac_recv_reply_6f_6 failed");
		goto exit;
	}
#endif


  success = true;

exit:
  if (data)
    free(data);

  return success;
}/*}}}*/

static ssize_t rra_syncmgr_put_single_object_reader(/*{{{*/
    uint32_t type_id, unsigned index, uint8_t* data, size_t data_size, void* cookie)
{
  if (index == 0)
  {
    ObjectData* object = (ObjectData*)cookie;
    ssize_t result = MIN(data_size, object->data_size);

    /* TODO: handle that object->data_size is > data_size and this function is
       called more than once for the same object!  */

    if (result)
    {
      memcpy(data, object->data, result);
      object->data_size -= result;
    }

    return result;
  }
  else
  {
    synce_error("Unexpected index: %i", index);
    return -1;
  }
}/*}}}*/

bool rra_syncmgr_put_single_object(/*{{{*/
    RRA_SyncMgr* self,  
    uint32_t type_id,
    uint32_t object_id,
    uint32_t flags,
    uint8_t* data,
    size_t data_size,
    uint32_t* new_object_id)
{
  bool success = false;
  ObjectData object;

  object.object_id  = object_id;
  object.data       = data;
  object.data_size  = data_size;

  if (!rra_syncmgr_put_multiple_objects(
        self,
        type_id,
        1,
        &object_id,
        new_object_id,
        flags,
        rra_syncmgr_put_single_object_reader,
        &object))
  {
    synce_error("Failed to put object");
    goto exit;
  }

  success = true;

exit:
  return success;
}/*}}}*/

bool rra_syncmgr_delete_object(/*{{{*/
  RRA_SyncMgr* self, 
  uint32_t type_id, 
  uint32_t object_id)
{
  bool success = false;
  uint32_t recv_type_id;
  uint32_t recv_object_id1;
  uint32_t recv_object_id2;
  uint32_t recv_flags;
  uint32_t index = 1;

  if (!rrac_send_66(self->rrac, type_id, object_id, index))
  {
    synce_error("Failed to senmd command 66");
    goto exit;
  }

  if (!rrac_recv_65(
        self->rrac, 
        &recv_type_id, 
        &recv_object_id1,
        &recv_object_id2,
        &recv_flags))
  {
    synce_error("Failed to receive command 65");
    goto exit;
  }

  if (recv_object_id1 != recv_object_id2)
  {
    synce_error("Unexpected object ids");
    goto exit;
  }

  if (recv_flags != (index | 0x80000000))
  {
    synce_warning("Unexpected flags: %08x", recv_flags);
  }

  success = true;

exit:
  return success;
}/*}}}*/

