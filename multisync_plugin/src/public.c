/** $Id$ */
#ifdef HAVE_CONFIG_H
#include "multisync_plugin_config.h"
#endif

#include "internal.h"
#include <multisync.h>
#include <synce_log.h>
#include <rra/appointment.h>
#include <rra/contact.h>
#include <rra/task.h>
#include <string.h>
#include <gnome.h>

#define USE_SYNCOBJ_MODIFY_LIST 1

/*
   This file contains the public functions used by MultiSync
 */

/* sync_connect()

   This is called once every time the sync engine tries to get changes
   from the two plugins, or only once if always_connected() returns true.
   Typically, this is where you should try to connect to the device
   to be synchronized, as well as load any options and so on.
   The returned struct must contain a

   client_connection commondata; 

   first for common MultiSync data.

   Oddly enough (for historical reasons) this callback MUST
   return the connection handle from this function call (and NOT by
   using sync_set_requestdata()). The sync engine still waits for a 
   sync_set_requestdone() call (or _requestfailed) before continuing.
 */
SynceConnection* sync_connect(
    sync_pair* handle,
    connection_type type,
    sync_object_type object_types)
{
  SynceConnection* connection = g_new0(SynceConnection, 1);

  connection->handle                  = handle;
  connection->commondata.object_types = object_types;

	synce_trace("----->");

  if (!synce_connect(connection))
  {
    sync_set_requestfailederror(
        _("Failed to initialize the SynCE synchronization manager"),
        connection->handle);
		goto exit;
  }

  /* Success */
  sync_set_requestdone(connection->handle);

exit:
	synce_trace("<-----");
  return connection;
}

/* sync_disconnect()

   Called by the sync engine to free the connection handle and disconnect
   from the database client.
 */
void sync_disconnect(SynceConnection* connection) 
{
	synce_trace("----->");

  synce_disconnect(connection);

  sync_set_requestdone(connection->handle);

	synce_trace("<-----");
}

/* get_changes()

   The most important function in the plugin. This function is called
   periodically by the sync engine to poll for changes in the database to
   be synchronized. The function should return a pointer to a gmalloc'ed
   change_info struct (which will be freed by the sync engine after usage).
   using sync_set_requestdata(change_info*, sync_pair*).
   
   For all data types set in the argument "newdbs", ALL entries should
   be returned. This is used when the other end reports that a database has
   been reset (by e.g. selecting "Reset all data" in a mobile phone.)
   Testing for a data type is simply done by 
   
   if (newdbs & SYNC_OBJECT_TYPE_SOMETHING) ...

   The "commondata" field of the connection handle contains the field
   commondata.object_types which specifies which data types should
   be synchronized. Only return changes from these data types.

   The changes reported by this function should be the remembered
   and rereported every time until sync_done() (see below) has been 
   called with a success value. This ensures that no changes get lost
   if some connection fails.  
*/

void get_changes(SynceConnection* connection, sync_object_type newdbs) 
{
  change_info* info = g_new0(change_info, 1);

	synce_trace("----->");

  if (!connection->syncmgr || !rra_syncmgr_is_connected(connection->syncmgr))
  {
    sync_set_requestfailederror(
        _("The SynCE synchronization manager is not connected. Please restart MultiSync."),
        connection->handle);
		goto exit;
  }

  if (!synce_join_thread(connection))
  {
    sync_set_requestfailederror(
        _("Failed to wait for thread termination"),
        connection->handle);
		goto exit;
  }

  if (synce_get_all_changes(connection, newdbs, info))
  {
    sync_set_requestdata(info, connection->handle);
  }
  else
  {
    sync_free_change_info(info);
    sync_set_requestfailederror(
        _("Failed to get changes"),
        connection->handle);
  }

exit:
  synce_create_thread(connection);
	synce_trace("<-----");
}

#if USE_SYNCOBJ_MODIFY_LIST

typedef struct
{
  changed_object* object;
  syncobj_modify_result* result;
} ObjectAndResult;

typedef struct
{
  /* Global data */
  SynceConnection* connection;
  int type_index;
  GList* objects;
  /* Data for the current object */
  unsigned object_index;
  ObjectAndResult* oar;
  uint8_t* data;
  uint8_t* current;
  size_t data_left;
} ObjectReaderParameters;

static ssize_t
object_reader(/*{{{*/
    uint32_t type_id, unsigned object_index, uint8_t* data, size_t data_size, void* cookie)
{
  ssize_t result = -1;
  ObjectReaderParameters* parameters = (ObjectReaderParameters*)cookie;
  if (!parameters)
    goto exit;
  
  if (parameters->object_index != object_index)
  {
    bool success = false;
    uint32_t dummy_id;

    parameters->object_index = object_index;
    parameters->oar = (ObjectAndResult*)g_list_nth_data(
        parameters->objects,
        object_index);

    if (!parameters->oar)
    {
      synce_error("Failed to get object %i from list", object_index);
      goto exit;
    }

    /* Convert to native data */
    switch (parameters->type_index)/*{{{*/
    {
      case INDEX_APPOINTMENT:
        success = rra_appointment_from_vevent(
            parameters->oar->object->comp,
            &dummy_id,
            &parameters->data,
            &parameters->data_left,
            RRA_APPOINTMENT_UTF8,
            &parameters->connection->timezone);
        break;

      case INDEX_CONTACT:
        success = rra_contact_from_vcard(
            parameters->oar->object->comp,
            &dummy_id,
            &parameters->data,
            &parameters->data_left,
            RRA_CONTACT_UTF8 | RRA_CONTACT_VERSION_2_1);
        break;

      case INDEX_TASK:
        success = rra_task_from_vtodo(
            parameters->oar->object->comp,
            &dummy_id,
            &parameters->data,
            &parameters->data_left,
            RRA_TASK_UTF8,
            &parameters->connection->timezone);
        break;

      default:
        synce_error("Unexpected index: %i", index);
        sync_set_requestfailederror(
            _("Unexpected object type"),
            parameters->connection->handle);
        goto exit;
    }/*}}}*/

    if (!success)
    {
      synce_error("Data conversion failed for type %08x and object %d",
          parameters->connection->type_ids[parameters->type_index], object_index);
      sync_set_requestfailederror(
          _("Failed to convert object"),
          parameters->connection->handle);
      goto exit;
    }

    parameters->current = parameters->data;
  }

  result = MIN(parameters->data_left, data_size);

  if (result)
  {
    memcpy(data, parameters->current, result);

    parameters->current   += result;
    parameters->data_left -= result;

    if (parameters->data_left == 0)
    {
      /* Clean up! */
      switch (parameters->type_index)/*{{{*/
      {
        case INDEX_APPOINTMENT:
          rra_appointment_free_data(parameters->data);
          break;

        case INDEX_CONTACT:
          rra_contact_free_data(parameters->data);
          break;

        case INDEX_TASK:
          rra_task_free_data(parameters->data);
          break;
      }/*}}}*/

      parameters->data    = NULL;
      parameters->current = NULL;
    }
  }

exit:
  return result;
}/*}}}*/

/*
   The objects list contains points to ObjectAndResult structs
   */
static bool
put_objects(SynceConnection* connection, int index, GList* objects, uint32_t flags)/*{{{*/
{
  bool success = false;
  ObjectReaderParameters parameters;
  int count = g_list_length(objects);
  uint32_t* ids = g_new0(uint32_t, count);
  uint32_t* received_ids = g_new0(uint32_t, count);
  GList* item;
  int i;

  /* Provide IDs for existing objects */
  if (flags == RRA_SYNCMGR_UPDATE_OBJECT)
  {
    for (item = objects, i = 0; item != NULL; item = g_list_next(item), i++)
    {
      ObjectAndResult* oar = (ObjectAndResult*)item->data;

      if (oar->object->uid)
        ids[i] = strtol(oar->object->uid, NULL, 16);

      if (!ids[i])
      {
        synce_warning("Unexpected uid '%s', will become a new object!", oar->object->uid);
      }
    }
  }

  memset(&parameters, 0, sizeof(parameters));
  parameters.connection = connection;
  parameters.type_index = index;
  parameters.objects    = objects;
  parameters.object_index = -1;

  /* Put objects */
  if (!rra_syncmgr_put_multiple_objects(
        connection->syncmgr,
        connection->type_ids[index],
        count,
        ids,
        received_ids,
        flags,
        object_reader,
        &parameters))
  {
    synce_error("Failed to put %i objects of type %08x with flags %08x", 
        count, connection->type_ids[index], flags);
    goto exit;
  }

  /* Handle IDs for new objects */
  /*if (flags == RRA_SYNCMGR_NEW_OBJECT)*/
  {
    for (item = objects, i = 0; item != NULL; item = g_list_next(item), i++)
    {
      ObjectAndResult* oar = (ObjectAndResult*)item->data;

      if (received_ids[i] == 0xffffffff)
      {
        oar->result->result = SYNC_MSG_MODIFYERROR;
      }
      else
      {
        oar->result->result = SYNC_MSG_NOMSG;
        oar->result->returnuid = g_strdup_printf("%08x",
            received_ids[i]);
      }
    }
  }
 
  success = true;

exit:
  g_free(ids);
  g_free(received_ids);
  return success;
}/*}}}*/

static void
append_object_and_result(GList** oar_list, changed_object* object, GList** results)/*{{{*/
{
  ObjectAndResult* oar = g_new0(ObjectAndResult, 1);
  
  oar->object = object;
  oar->result = g_new0(syncobj_modify_result, 1);
  
  *oar_list = g_list_append(*oar_list, oar);
  *results  = g_list_append(*results, oar->result);
}/*}}}*/

static void g_free_GFunc(void* data, void* cookie)
{
  g_free(data);
}

/* syncobj_modify_list()

   Modify many objects in the database.

   The changes parameter is a list of changed_object structs.
*/
void 
syncobj_modify_list(SynceConnection* connection, GList* all_changes) 
{
  GList* item = NULL;
  GList* deleted = NULL;
  GList* added[INDEX_MAX];
  GList* modified[INDEX_MAX];
  GList *results = NULL;
  int i;

  memset(added,     0, sizeof(added));
  memset(modified,  0, sizeof(modified));

  synce_trace("----->");

  if (!synce_join_thread(connection))
  {
    sync_set_requestfailederror(
        _("Failed to wait for thread termination"),
        connection->handle);
    goto exit;
  }

  if (!connection->syncmgr || !rra_syncmgr_is_connected(connection->syncmgr))
  {
    sync_set_requestfailederror(
        _("The SynCE synchronization manager is not connected. Please restart MultiSync."),
        connection->handle);
    goto exit;
  }

  /* Separare changes into deleted, added and modified */
  synce_trace("%i changes total", g_list_length(all_changes));
  for (item = all_changes; item != NULL; item = g_list_next(item))/*{{{*/
  {
    changed_object* object = (changed_object*)item->data;
    int index = synce_index_from_sync_object_type(object->object_type);
    
    if (index == INDEX_MAX)
    {
      synce_error("Unexpected type: %i", object->object_type);
      continue;
    }

    switch (object->change_type)
    {
      case SYNC_OBJ_HARDDELETED:
        append_object_and_result(&deleted, object, &results);
        break;
        
      case SYNC_OBJ_SOFTDELETED:
        /* Ignore these */
        break;

      case SYNC_OBJ_ADDED:
        append_object_and_result(&added[index], object, &results);
        break;

      case SYNC_OBJ_MODIFIED:
        append_object_and_result(&modified[index], object, &results);
        break;

      default:
        synce_warning("Unknown change type: %i", object->change_type);
        break;
    }
  }/*}}}*/

  /* Can currently only delete one item at a time, so we do that... */
  synce_trace("%i items deleted", g_list_length(deleted));
  for (item = deleted; item != NULL; item = g_list_next(item))/*{{{*/
  {
    ObjectAndResult* oar = (ObjectAndResult*)item->data;
    changed_object* object = oar->object;
    int index;
    uint32_t object_id;
    index = synce_index_from_sync_object_type(object->object_type);
    
    if (INDEX_MAX == index)
    {
      synce_error("Unexpected type: %i", object->object_type);
      sync_set_requestfailederror(
          _("Unexpected object type"),
          connection->handle);
      goto exit;
    }

    object_id = strtol(object->uid, NULL, 16);

    if (!object_id)
    {
      synce_error("Unexpected uid: '%s'", object->uid);
      sync_set_requestfailederror(
          _("Unexpected uid"),
          connection->handle);
      goto exit;
    }

    if (!rra_syncmgr_delete_object(connection->syncmgr,
          connection->type_ids[index],
          object_id))
    {
      synce_error("Failed to delete object with type %08x and ID %08x", 
          connection->type_ids[index],
          object_id);
      oar->result->result = SYNC_MSG_REQFAILED;
    }
  }/*}}}*/
  
  for (i = 0; i < INDEX_MAX; i++)
  {
    synce_trace("%i items added of type index %i", g_list_length(added[i]), i);
    synce_trace("%i items modified of type index %i", g_list_length(modified[i]), i);

    if (!put_objects(connection, i, added[i], RRA_SYNCMGR_NEW_OBJECT))
      break;

    if (!put_objects(connection, i, modified[i], RRA_SYNCMGR_UPDATE_OBJECT))
      break;
  }

  sync_set_requestdata(results, connection->handle);
  /* implicited by the above? sync_set_requestdone(connection->handle); */

exit:
  g_list_foreach(deleted, g_free_GFunc, NULL);
  g_list_free(deleted);
  for (i = 0; i < INDEX_MAX; i++)
  {
    g_list_foreach(added[i], g_free_GFunc, NULL);
    g_list_free   (added[i]);
    g_list_foreach(modified[i], g_free_GFunc, NULL);
    g_list_free   (modified[i]);
  }
  synce_create_thread(connection);
  synce_trace("<-----");
}

#else /* USE_SYNCOBJ_MODIFY_LIST */

/* syncobj_modify() 

   Modify or add an object in the database. This is called by the sync
   engine when a change has been reported in the other end.

   object     

   A string containing the actual data of the object. E.g. for an objtype of
   SYNC_OBJECT_TYPE_CALENDAR, this is a vCALENDAR 2.0 string (see RFC 2445).

   uid        
   
   The unique ID of this entry. If it is new (i.e. the sync engine has not seen
   it before), this is NULL.

   objtype    

   The data type of this object.

   returnuid  

   If uid is NULL, then the ID of the newly created object should be returned
   in this buffer (if non-NULL). The length of the ID should be returned in
   returnuidlen.
 */

void syncobj_modify(/*{{{*/
    SynceConnection* connection,
    char* object, 
    char* uid,
    sync_object_type objtype,
    char* returnuid, 
    int* returnuidlen) 
{
  bool success = false;
  int index;
  uint32_t object_id;
  uint32_t flags = 0;
  uint8_t* data = NULL;
  size_t data_size = 0;
  uint32_t dummy_id;
  uint32_t new_object_id = 0;
  SynceObject* synce_object = NULL;
  
	synce_trace("----->");

  if (!synce_join_thread(connection))
  {
    sync_set_requestfailederror(
        _("Failed to wait for thread termination"),
        connection->handle);
		goto exit;
  }

  if (!connection->syncmgr || !rra_syncmgr_is_connected(connection->syncmgr))
  {
    sync_set_requestfailederror(
        _("The SynCE synchronization manager is not connected. Please restart MultiSync."),
        connection->handle);
		goto exit;
  }

  index = synce_index_from_sync_object_type(objtype);

  if (INDEX_MAX == index)
  {
    synce_error("Unexpected type: %i", objtype);
    sync_set_requestfailederror(
        _("Unexpected object type"),
        connection->handle);
    goto exit;
  }

  if (uid)
  {
    object_id = strtol(uid, NULL, 16);

    if (!object_id)
    {
      synce_error("Unexpected uid: '%s'", uid);
      sync_set_requestfailederror(
          _("Unexpected uid"),
          connection->handle);
      goto exit;
    }

    flags = RRA_SYNCMGR_UPDATE_OBJECT;
  }
  else
  {
    object_id = 0;
    flags = RRA_SYNCMGR_NEW_OBJECT;
  }

  switch (index)
  {
    case INDEX_APPOINTMENT:
      success = rra_appointment_from_vevent(
          object,
          &dummy_id,
          &data,
          &data_size,
          RRA_APPOINTMENT_UTF8,
          &connection->timezone);
      break;

    case INDEX_CONTACT:
      success = rra_contact_from_vcard(
          object,
          &dummy_id,
          &data,
          &data_size,
          RRA_CONTACT_UTF8 | RRA_CONTACT_VERSION_2_1);
      break;

    case INDEX_TASK:
      success = rra_task_from_vtodo(
          object,
          &dummy_id,
          &data,
          &data_size,
          RRA_TASK_UTF8,
          &connection->timezone);
      break;

    default:
      synce_error("Unexpected index: %i", index);
      sync_set_requestfailederror(
          _("Unexpected object type"),
          connection->handle);
      goto exit;
  }

  if (!success)
  {
    synce_error("Data conversion failed for type %08x and object %08x",
        connection->type_ids[index], object_id);
    sync_set_requestfailederror(
        _("Failed to convert object"),
        connection->handle);
    goto exit;
  }

  synce_trace("Sending object with type %08x and ID %08x to connected device",
      connection->type_ids[index],
      object_id);

  if (!rra_syncmgr_put_single_object(
      connection->syncmgr,
      connection->type_ids[index],
      object_id,
      flags,
      data,
      data_size,
      &new_object_id))
  {
    FILE* file;

    file = fopen("/tmp/failed_object.txt", "w");
    if (file)
    {
      fwrite(object, strlen(object), 1, file);
      fclose(file);
    }

    file = fopen("/tmp/failed_object.bin", "w");
    if (file)
    {
      fwrite(data, data_size, 1, file);
      fclose(file);
    }

    synce_error("Failed to put object %08x with type %08x", 
        object_id, connection->type_ids[index]);
    sync_set_requestfailederror(
        _("Failed to put object"),
        connection->handle);

    goto exit;
  }
      
  switch (index)
  {
    case INDEX_APPOINTMENT:
      rra_appointment_free_data(data);
      break;

    case INDEX_CONTACT:
      rra_contact_free_data(data);
      break;

    case INDEX_TASK:
      rra_task_free_data(data);
      break;
  }
  
  if (!uid)
  {
    object_id = new_object_id;
    sprintf(returnuid, "%08x", new_object_id);
    *returnuidlen = strlen(returnuid);
  }

  synce_object = (SynceObject*)g_hash_table_lookup(
      connection->objects[index],
      &object_id);

  if (!synce_object)
  {
    /* Create new object and add to hash table */
    synce_object = g_new0(SynceObject, 1);

    synce_object->type_index  = index;
    synce_object->type_id     = connection->type_ids[index];
    synce_object->object_id   = object_id;

    g_hash_table_insert(
        connection->objects[index],
        &synce_object->object_id,
        synce_object);  
  }

  synce_object->event = SYNCMGR_TYPE_EVENT_UNCHANGED;

  sync_set_requestdone(connection->handle);

exit:
  synce_create_thread(connection);
	synce_trace("<-----");
  return;
}/*}}}*/

/* syncobj_delete() 

   Delete an object from the database. If the argument softdelete is 
   true, then this object is deleted by the sync engine for storage reasons.
*/
void syncobj_delete(/*{{{*/
    SynceConnection* connection, 
    char* uid,
    sync_object_type objtype, 
    int softdelete)
{
  int index;
  uint32_t object_id;

  synce_trace("----->");

  if (!connection->syncmgr || !rra_syncmgr_is_connected(connection->syncmgr))
  {
    sync_set_requestfailederror(
        _("The SynCE synchronization manager is not connected. Please restart MultiSync."),
        connection->handle);
		goto exit;
  }

  if (softdelete)
  {
    /* Don't do soft deletes */
    sync_set_requestdone(connection->handle);
    goto exit;
  }

  index = synce_index_from_sync_object_type(objtype);

  if (INDEX_MAX == index)
  {
    synce_error("Unexpected type: %i", objtype);
    sync_set_requestfailederror(
        _("Unexpected object type"),
        connection->handle);
    goto exit;
  }

  object_id = strtol(uid, NULL, 16);

  if (!object_id)
  {
    synce_error("Unexpected uid: '%s'", uid);
    sync_set_requestfailederror(
        _("Unexpected uid"),
        connection->handle);
    goto exit;
  }

  if (!rra_syncmgr_delete_object(connection->syncmgr,
      connection->type_ids[index],
      object_id))
  {
    synce_error("Failed to delete object with type %08x and ID %08x", 
        connection->type_ids[index],
        object_id);
    sync_set_requestfailederror(
        _("Failed to delete object"),
        connection->handle);
    goto exit;
  }

  sync_set_requestdone(connection->handle);

exit:
	synce_trace("<-----");
}/*}}}*/

#endif /* USE_SYNCOBJ_MODIFY_LIST */

/* syncobj_get_recurring()

   This is a very optional function which may very well be removed in 
   the future. It should return a list of all recurrence instance of
   an object (such as all instances of a recurring calendar event).
   
   The recurring events should be returned as a GList of changed_objects
   with change type SYNC_OBJ_RECUR.
*/
void syncobj_get_recurring(
    SynceConnection* connection, 
    changed_object* obj) 
{
	synce_trace("here");
  /* not implemented */
  sync_set_requestdata(NULL, connection->handle);
}


/* sync_done()

   This function is called by the sync engine after a synchronization has
   been completed. If success is true, the sync was successful, and 
   all changes reported by get_changes can be forgot. If your database
   is based on a change counter, this can be done by simply saving the new
   change counter.
*/
void sync_done(SynceConnection* connection, gboolean success) 
{
	synce_trace("----->");

  if (!connection->syncmgr || !rra_syncmgr_is_connected(connection->syncmgr))
  {
    sync_set_requestfailederror(
        _("The SynCE synchronization manager is not connected. Please restart MultiSync."),
        connection->handle);
		goto exit;
  }

  if (success)
    synce_mark_objects_as_unchanged(connection);
  else
    synce_warning("sync_done called with success == false");

  if (!synce_create_thread(connection))
  {
    sync_set_requestfailederror(
        _("Failed to create event handling thread"), 
        connection->handle);
    goto exit;
  }

  sync_set_requestdone(connection->handle);

exit:
	synce_trace("<-----");
}

gboolean always_connected()
{
	return TRUE;
}

const char* short_name()
{
	return _("synce-plugin");
}

const char* long_name()
{
	return _("SynCE Plugin");
}

int object_types()
{
	return 
    SYNC_OBJECT_TYPE_CALENDAR | 
    SYNC_OBJECT_TYPE_PHONEBOOK |
    SYNC_OBJECT_TYPE_TODO;
}

void plugin_init()
{
	synce_trace("here");
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

}

const char* plugin_info()
{
	return _("This plugin allows synchronization with a device running Windows CE or Pocket PC.");
}

int plugin_API_version(void) {
    return(3);
}
