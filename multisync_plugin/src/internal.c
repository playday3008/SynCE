#ifdef HAVE_CONFIG_H
#include "multisync_plugin_config.h"
#endif

#include "internal.h"
#include <synce_log.h>
#include <rra/appointment.h>
#include <rra/contact.h>
#include <rra/task.h>
#include <rra/matchmaker.h>
#include <rra/uint32vector.h>
#include <rapi.h>
#include <string.h>

typedef struct 
{
  sync_object_type type;
  const char* name;
} TypeToName;

static TypeToName types_and_names[] = 
{
  {SYNC_OBJECT_TYPE_CALENDAR,  RRA_SYNCMGR_TYPE_APPOINTMENT}, /* INDEX_APPOINTMENT */
  {SYNC_OBJECT_TYPE_PHONEBOOK, RRA_SYNCMGR_TYPE_CONTACT},     /* INDEX_CONTACT     */
  {SYNC_OBJECT_TYPE_TODO,      RRA_SYNCMGR_TYPE_TASK}         /* INDEX_TASK        */
};

int synce_index_from_sync_object_type(sync_object_type objtype)/*{{{*/
{
  int index;

  for (index = 0; index < INDEX_MAX; index++)
  {
    if (objtype == types_and_names[index].type)
      break;
  }

  return index;
}/*}}}*/

void synce_free_object_data(SynceObject* object)/*{{{*/
{
  if (!object)
    return;

  switch (object->type_index)
  {
    case INDEX_APPOINTMENT:
      rra_appointment_free_vevent(object->data);
      break;

    case INDEX_CONTACT:
      rra_contact_free_vcard(object->data);
      break;

    case INDEX_TASK:
      rra_task_free_vtodo(object->data);
      break;
  }

  object->data = NULL;
}/*}}}*/

static void synce_test_for_enough_ids(SynceConnection* connection, int index)
{
  if (!connection->enough_ids[index])
  {
    RRA_SyncMgrType* type = rra_syncmgr_type_from_id(
        connection->syncmgr, connection->type_ids[index]);

    if (g_hash_table_size(connection->objects[index]) == type->count)
      connection->enough_ids[index] = true;
  }
}

static bool synce_callback (/*{{{*/
    RRA_SyncMgrTypeEvent event, 
    uint32_t type, 
    uint32_t count, 
    uint32_t* ids, 
    void* cookie)
{
  SynceConnection* connection = (SynceConnection*)cookie;
  unsigned i;
  int index;
  SynceObject* object = NULL;

	synce_trace("----->");

  /* Find object type */
  for (index = 0; index < INDEX_MAX; index++)
  {
    if (type == connection->type_ids[index])
      break;
  }

  if (INDEX_MAX == index)
  {
    /* Unknown object type */
    return false;
  }
  
  /* Update the apropriate hashtable */
  for (i = 0; i < count; i++)
  {
    object = (SynceObject*)g_hash_table_lookup(
        connection->objects[index],
        &ids[i]);

    if (object)
    {
      synce_free_object_data(object);
    }
    else
    {
      /* Create new object and add to hash table */
      object = g_new0(SynceObject, 1);

      object->type_index  = index;
      object->type_id     = type;
      object->object_id   = ids[i];
      object->event       = event;
      
      g_hash_table_insert(
          connection->objects[index],
          &object->object_id,
          object);
    }

    object->event = event;
    object->change_counter = ++connection->change_counter;
  }

  synce_test_for_enough_ids(connection, index);
  
  if (count && SYNCMGR_TYPE_EVENT_UNCHANGED != event)
  {
    /* Notify MultiSync that something has changed */
    sync_object_changed(connection->handle);
  }

  synce_trace("<-----");

  return true;
}/*}}}*/

bool synce_subscribe(SynceConnection* connection)/*{{{*/
{
  bool success = false;
  int i;

  for (i = 0; i < INDEX_MAX; i++)
  {
    if (connection->commondata.object_types & types_and_names[i].type)
    {
      RRA_SyncMgrType* type = rra_syncmgr_type_from_name(
          connection->syncmgr, 
          types_and_names[i].name);
      
      if (type)
      {
        connection->type_ids[i] = type->id;

        rra_syncmgr_subscribe(
            connection->syncmgr, 
            connection->type_ids[i], 
            synce_callback, 
            connection);

        connection->objects[i] = g_hash_table_new(g_int_hash, g_int_equal);
      }
      else
      {
        /* Synchronization of this type is not supported! */
        synce_warning("Synchronization of '%s' events is not supported",
            types_and_names[i].name);
      }
    }
  }

  if (!rra_syncmgr_start_events(connection->syncmgr))
  {
    synce_error("Failed to subscribe to RRA synchronization events");
    goto exit;
  }

  success = true;

exit:
  return success;
}/*}}}*/

static void* synce_event_handling_thread(void* cookie)/*{{{*/
{
  SynceConnection* connection = (SynceConnection*)cookie;

  while (connection->thread_running)
  {
    bool got_event = false;

    if (rra_syncmgr_event_wait(connection->syncmgr, 1, &got_event))
    {
      if (got_event)
      {
        /*pthread_mutex_lock(&connection->lock);*/
        rra_syncmgr_handle_event(connection->syncmgr);
        /*pthread_mutex_unlock(&connection->lock);*/
      }
    }
    else
    {
      synce_error("Failed to wait for event, stopping event handling thread.");
      connection->thread_running = false;
    }
  }

  return NULL;
}/*}}}*/

bool synce_create_thread(SynceConnection* connection)/*{{{*/
{
  int error;
  
  pthread_mutex_init(&connection->lock, NULL);
  
  connection->thread_running = true;

  error = pthread_create(&connection->thread, NULL, synce_event_handling_thread, connection);

  if (error)
  {
    connection->thread_running = false;
    synce_error("Failed to create thread");
    return false;
  }

 
  return true;
}/*}}}*/

bool synce_join_thread(SynceConnection* connection)/*{{{*/
{
  int error;

  if (connection->thread_running)
  {
    connection->thread_running = false;

    error = pthread_join(connection->thread, NULL);

    if (error)
    {
      synce_error("Failed to wait for thread termination");
      return false;
    }

    pthread_mutex_destroy(&connection->lock);
  }
  else
    synce_warning("synce_join_thread called when no thread is running");

  return true;
}/*}}}*/

bool synce_connect(SynceConnection* connection)/*{{{*/
{
  bool success = false;
  HRESULT hr;
  RRA_Matchmaker* matchmaker = NULL;
  gchar* partner_filename = g_strdup_printf(
      "%s/synce-partner", 
      sync_get_datapath(connection->handle));

  if (!connection)
  {
    synce_error("Connection object is NULL");
    goto exit;
  }

  /* Make it possible to call this multiple times without harm... :-) */
  if (connection->syncmgr)
    return true;

  /* 
     Initialize RAPI 
     */

  hr = CeRapiInit();
  if (FAILED(hr))
  {
    synce_error("Failed to initialize RAPI");
    goto exit;
  }

  {
    FILE* file = fopen(partner_filename, "r");
    uint32_t partnership_id = 0;
    char id_buffer[10] = "";
    uint32_t index = 0;
    
    matchmaker = rra_matchmaker_new();

    if (file)
    {
     
      fgets(id_buffer, sizeof(id_buffer), file);
      partnership_id = strtol(id_buffer, NULL, 16);
      fclose(file);

      synce_trace("This synchronization pair is connected to partnership ID %08x",
          partnership_id);

      for (index = 1; index <= 2; index++)
      {
        uint32_t id;

        if (rra_matchmaker_get_partner_id(matchmaker, index, &id) && id == partnership_id)
          /* TODO: check hostname too */
          break;
      }

      if (index == 3)
      {
        synce_error("No partnership on the device matches this synchronization pair.");
        goto exit;
      }
      else
      {
        rra_matchmaker_set_current_partner(matchmaker, index);
      }
    }
    else
    {
      if (!rra_matchmaker_create_partnership(matchmaker, &index))
      {
        synce_error("Failed to create or select a partnership. Use the synce-matchmaker tool!");
        goto exit;
      }

      if (!rra_matchmaker_get_partner_id(matchmaker, index, &partnership_id))
      {
        synce_error("Failed to get partnership ID");
        goto exit;
      }

      synce_trace("This synchronization pair will be connected to partnership ID %08x",
          partnership_id);

      file = fopen(partner_filename, "w");
      if (!file)
      {
        synce_error("Failed to create file '%s'", partner_filename);
        goto exit;
      }

      snprintf(id_buffer, sizeof(id_buffer), "%08x", partnership_id);
      fwrite(id_buffer, strlen(id_buffer), 1, file);
      fclose(file);
    }
  }

  /*
     Get timezone info
   */

  if (!rra_timezone_get(&connection->timezone))
  {
    synce_error("Failed to get timezone information from device");
		goto exit;
  }

  /* 
     Initialize the synchronization manager in RRA 
   */

  connection->syncmgr = rra_syncmgr_new();

	if (!rra_syncmgr_connect(connection->syncmgr))
	{
    synce_error("Failed to initialize the synchronization manager");
		goto exit;
	}

  if (!synce_subscribe(connection))
  {
    synce_error("Failed to subscribe to synchronization events");
    goto exit;
  }

  if (!synce_create_thread(connection))
  {
    synce_error("Failed to create event handling thread");
		goto exit;
  }

  success = true;

exit:
  rra_matchmaker_destroy(matchmaker);
  g_free(partner_filename);
  return success;
}/*}}}*/

void synce_disconnect(SynceConnection* connection)/*{{{*/
{
  if (!connection)
  {
    synce_error("Connection object is NULL");
    return;
  }
  
  if (!synce_join_thread(connection))
  {
    synce_trace("Failed to wait for thread termination");
  }

  /* TODO: destroy hash tables */
  rra_syncmgr_destroy(connection->syncmgr);
  connection->syncmgr = NULL;
  /*g_free(connection);*/

  CeRapiUninit();
}/*}}}*/

static void synce_add_object_to_change_info(/*{{{*/
    SynceObject* object, 
    int change_type, 
    change_info* info)
{
  changed_object* changed = g_new0(changed_object, 1);

  synce_trace("Adding object with ID %08x and type %08x to change info", 
      object->object_id, object->type_id);

  if (object->event == SYNCMGR_TYPE_EVENT_DELETED)
    changed->comp       = NULL;
  else
    changed->comp       = g_strdup(object->data);

  changed->uid          = g_strdup_printf("%08x", object->object_id);
  changed->change_type  = change_type;
  changed->object_type  = types_and_names[object->type_index].type;

  info->changes = g_list_prepend(info->changes, changed);
}/*}}}*/

static bool synce_save_object_data(/*{{{*/
    uint32_t type_id, 
    uint32_t object_id, 
    const uint8_t* data, 
    size_t data_size, 
    void* cookie)
{
  bool success;
  SynceConnection* connection = (SynceConnection*)cookie;
  SynceObject* object = NULL;
  int index;

  synce_trace("Saving data for object with ID %08x and type %08x", 
      object_id, type_id);

  for (index = 0; index < INDEX_MAX; index++)
  {
    if (connection->type_ids[index] == type_id)
      break;
  }

  if (INDEX_MAX == index)
  {
    synce_warning("Unexpected type ID: %08x", type_id);
    goto exit;
  }

  object = (SynceObject*)g_hash_table_lookup(
      connection->objects[index],
      &object_id);

  if (!object)
  {
    synce_warning("Cannot find object with ID: %08x", object_id);
    goto exit;
  }

  switch (index)
  {
    case INDEX_APPOINTMENT:
      success = rra_appointment_to_vevent(
          object_id,
          data,
          data_size,
          &object->data,
          RRA_APPOINTMENT_UTF8,
          &connection->timezone);
      break;

    case INDEX_CONTACT:
      success = rra_contact_to_vcard(
          object_id,
          data,
          data_size,
          &object->data,
          RRA_CONTACT_UTF8 | RRA_CONTACT_VERSION_2_1);
      break;

    case INDEX_TASK:
      success = rra_task_to_vtodo(
          object_id,
          data,
          data_size,
          &object->data,
          RRA_TASK_UTF8,
          &connection->timezone);
      break;

    default:
      synce_error("Unhandled index: %i", index);
      goto exit;
  }

  if (!success)
  {
    synce_error("Data conversion failed for type %08x and object %08x",
        type_id, object_id);
    goto exit;
  }

exit:
  return true;
}/*}}}*/

static void synce_retrieve_object_data(/*{{{*/
    SynceConnection* connection,
    int index,
    RRA_Uint32Vector* vector)
{
  bool success;

  synce_trace("Retrieving %i objects of type %08x", 
      vector->used, connection->type_ids[index]);

  success = rra_syncmgr_get_multiple_objects(
      connection->syncmgr,
      connection->type_ids[index],
      vector->used,
      vector->items,
      synce_save_object_data,
      connection
      );

  if (!success)
  {
    synce_error("Failed to retrieve object data");
  }
}/*}}}*/

static void synce_add_changed_to_id_vector_if_data_needed_GHFunc(/*{{{*/
    gpointer key,
    gpointer value, 
    gpointer cookie)
{
  RRA_Uint32Vector* vector = (RRA_Uint32Vector*)cookie;
  SynceObject* object = (SynceObject*)value;

  if (object->event == SYNCMGR_TYPE_EVENT_CHANGED && !object->data)
  {
    /*synce_trace("Adding ID %08x of type %08x to ID array", 
        object->object_id, object->type_id);*/

    rra_uint32vector_add(vector, object->object_id);
  }
}

/** Proxy to for synce_add_object_to_change_info */
static void synce_add_changed_to_change_info_GHFunc(/*{{{*/
    gpointer key,
    gpointer value, 
    gpointer cookie)
{
  change_info* info = (change_info*)cookie;
  SynceObject* object = (SynceObject*)value;
  int change_type;

  switch (object->event)
  {
    case SYNCMGR_TYPE_EVENT_UNCHANGED:
      /* ignoring */
      return;

    case SYNCMGR_TYPE_EVENT_CHANGED:
      change_type = SYNC_OBJ_MODIFIED;
      break;

    case SYNCMGR_TYPE_EVENT_DELETED:
      change_type = SYNC_OBJ_HARDDELETED;
      break;

    default:
      synce_warning("Unknown event for changed object");
      return;
  }

  synce_add_object_to_change_info(object, change_type, info);
}/*}}}*/

/** Proxy to synce_add_existing_to_id_vector for hash table entries */
static void synce_add_existing_to_id_vector_GHFunc(/*{{{*/
    gpointer key,
    gpointer value,
    gpointer cookie)
{
  RRA_Uint32Vector* vector = (RRA_Uint32Vector*)cookie;
  SynceObject* object = (SynceObject*)value;

  if (object->event != SYNCMGR_TYPE_EVENT_DELETED)
  {
    rra_uint32vector_add(vector, object->object_id);
  }
}/*}}}*/
 
static void synce_add_deleted_items_to_change_info(
    SynceConnection* connection,
    int index,
    change_info* info)
{
  RRA_Uint32Vector* current_ids = rra_uint32vector_new();
  RRA_Uint32Vector* deleted_ids = rra_uint32vector_new();

  synce_trace("----->");

  g_hash_table_foreach(
      connection->objects[index], 
      synce_add_existing_to_id_vector_GHFunc, 
      current_ids);

  if (rra_syncmgr_get_deleted_object_ids(
        connection->syncmgr,
        connection->type_ids[index],
        current_ids,
        deleted_ids))
  {
    /* Add deleted object IDs to change_info */
    unsigned i;

    for (i = 0; i < deleted_ids->used; i++)
    {
      /* Create new object and add to hash table */
      SynceObject* object = g_new0(SynceObject, 1);

      synce_trace("Object with type %08x and ID %08x has been deleted",
          connection->type_ids[index], deleted_ids->items[i]);

      object->type_index  = index;
      object->type_id     = connection->type_ids[index];
      object->object_id   = deleted_ids->items[i];
      object->event       = SYNCMGR_TYPE_EVENT_DELETED;
      object->change_counter = ++connection->change_counter;

      g_hash_table_insert(
          connection->objects[index],
          &object->object_id,
          object);
    }
  }
  else
  {
    synce_warning("Failed to find out which items had been deleted.");
  }

  rra_uint32vector_destroy(current_ids, true);
  rra_uint32vector_destroy(deleted_ids, true);

  synce_trace("<-----");
}

static bool synce_get_changes(/*{{{*/
    SynceConnection* connection,
    int index,
    change_info* info)
{
  RRA_Uint32Vector* changed_ids = rra_uint32vector_new();

  synce_trace("Get changes for type %08x", 
      connection->type_ids[index]);

  synce_test_for_enough_ids(connection, index);

  if (connection->enough_ids[index])
  {
    synce_add_deleted_items_to_change_info(
        connection, index, info);
  }

  g_hash_table_foreach(
      connection->objects[index], 
      synce_add_changed_to_id_vector_if_data_needed_GHFunc, 
      changed_ids);

  synce_retrieve_object_data(connection, index, changed_ids);

  g_hash_table_foreach(
      connection->objects[index], 
      synce_add_changed_to_change_info_GHFunc, 
      info);

  rra_uint32vector_destroy(changed_ids, true);
  return true;
}/*}}}*/

static void synce_add_any_to_id_vector_if_data_needed_GHFunc(/*{{{*/
    gpointer key,
    gpointer value, 
    gpointer cookie)
{
  RRA_Uint32Vector* vector = (RRA_Uint32Vector*)cookie;
  SynceObject* object = (SynceObject*)value;

  if (object->event != SYNCMGR_TYPE_EVENT_DELETED && !object->data)
  {
    /*synce_trace("Adding ID %08x of type %08x to ID array", 
        object->object_id, object->type_id);*/

    rra_uint32vector_add(vector, object->object_id);
  }
}


/** Proxy to synce_add_object_to_change_info for hash table entries */
static void synce_add_any_to_change_info_GHFunc(/*{{{*/
    gpointer key,
    gpointer value,
    gpointer cookie)
{
  change_info* info = (change_info*)cookie;
  SynceObject* object = (SynceObject*)value;
  
  if (object->event != SYNCMGR_TYPE_EVENT_DELETED)
    synce_add_object_to_change_info(object, SYNC_OBJ_ADDED, info);
}/*}}}*/

static bool synce_get_everything(/*{{{*/
    SynceConnection* connection,
    int index,
    change_info* info)
{
  RRA_Uint32Vector* existing_ids = rra_uint32vector_new();

  synce_trace("Get all data for type %08x", 
      connection->type_ids[index]);

  g_hash_table_foreach(
      connection->objects[index], 
      synce_add_any_to_id_vector_if_data_needed_GHFunc, 
      existing_ids);

  synce_retrieve_object_data(connection, index, existing_ids);

  g_hash_table_foreach(
      connection->objects[index], 
      synce_add_any_to_change_info_GHFunc, 
      info);

  rra_uint32vector_destroy(existing_ids, true);
  return true;
}/*}}}*/

bool synce_get_all_changes(/*{{{*/
    SynceConnection* connection, 
    sync_object_type newdbs, 
    change_info* info)
{
  bool result = false;
  int i;
  
  /*pthread_mutex_lock(&connection->lock);*/

  for (i = 0; i < INDEX_MAX; i++)
  {
    if (connection->commondata.object_types & types_and_names[i].type)
    {
      if (newdbs & types_and_names[i].type)
      {
        result = synce_get_everything(
            connection, 
            i,
            info);
      }
      else
      {
        result = synce_get_changes(
            connection, 
            i,
            info);
      }

      if (!result)
        break;
    }
  }

  synce_trace("Updating last change counter from %i to %i", 
      connection->last_change_counter, connection->change_counter);

  connection->last_change_counter = connection->change_counter;

  /*pthread_mutex_unlock(&connection->lock);*/

  return result;
}/*}}}*/

static gboolean synce_mark_as_unchanged_GHRFunc(/*{{{*/
    gpointer key,
    gpointer value,
    gpointer cookie)
{
  gboolean remove_item = FALSE;
  SynceConnection* connection = (SynceConnection*)cookie;
  SynceObject* object = (SynceObject*)value;

  if (object->change_counter <= connection->last_change_counter)
  {
    switch (object->event)
    {
      case SYNCMGR_TYPE_EVENT_CHANGED:
        synce_trace("Marking object %08x of type %08x as unchanged",
            object->object_id, object->type_id);

        if (!rra_syncmgr_mark_object_unchanged(
              connection->syncmgr,
              object->type_id,
              object->object_id))
        {
          synce_warning("Failed to mark object %08x of type %08x as unchanged",
              object->type_id,
              object->object_id);
        }

        object->event = SYNCMGR_TYPE_EVENT_UNCHANGED;
        remove_item = FALSE;
        break;

      case SYNCMGR_TYPE_EVENT_DELETED:
        synce_trace("Forgetting all about object %08x of type %08x",
            object->object_id, object->type_id);

        g_free(object->data);
        g_free(object);
        remove_item = TRUE;
        break;

      default:
        /* ignore */
        break;
    }
  }

  return remove_item;
}/*}}}*/
 
bool synce_mark_objects_as_unchanged(/*{{{*/
    SynceConnection* connection)
{
  int i;

  /*pthread_mutex_lock(&connection->lock);*/

  for (i = 0; i < INDEX_MAX; i++)
  {
    if (connection->commondata.object_types & types_and_names[i].type)
    {
      g_hash_table_foreach_remove(
          connection->objects[i], 
          synce_mark_as_unchanged_GHRFunc, 
          connection);
    }
  }

  /*pthread_mutex_unlock(&connection->lock);*/

  return true;
}/*}}}*/
