/** $Id$ */
#include "internal.h"
#include <multisync.h>
#include <synce_log.h>
#include <rra/appointment.h>
#include <rra/contact.h>
#include <rra/task.h>
#include <string.h>

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
        "Failed to initialize the SynCE synchronization manager",
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

  if (!synce_join_thread(connection))
  {
    sync_set_requestfailederror(
        "Failed to wait for thread termination",
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
        "Failed to get changes",
        connection->handle);
  }

exit:
	synce_trace("<-----");
}

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

void syncobj_modify(
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
  
	synce_trace("----->");

  if (!synce_connect(connection))
  {
    sync_set_requestfailederror(
        "Failed to initialize the SynCE synchronization manager",
        connection->handle);
		goto exit;
  }

  index = synce_index_from_sync_object_type(objtype);

  if (INDEX_MAX == index)
  {
    synce_error("Unexpected type: %i", objtype);
    sync_set_requestfailederror(
        "Unexpected object type",
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
          "Unexpected uid",
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
      synce_error("Unhandled index: %i", index);
      goto exit;
  }

  if (!success)
  {
    synce_error("Data conversion failed for type %08x and object %08x",
        connection->type_ids[index], object_id);
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
        "Failed to put object",
        connection->handle);

    /* This error was probably fatal, so close connection */
    synce_disconnect(connection);
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
    sprintf(returnuid, "%08x", new_object_id);
    *returnuidlen = strlen(returnuid);
  }
 
  sync_set_requestdone(connection->handle);

exit:
	synce_trace("<-----");
  return;
}


/* syncobj_delete() 

   Delete an object from the database. If the argument softdelete is 
   true, then this object is deleted by the sync engine for storage reasons.
*/
void syncobj_delete(
    SynceConnection* connection, 
    char* uid,
    sync_object_type objtype, 
    int softdelete)
{
  int index;
  uint32_t object_id;

  synce_trace("----->");

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
        "Unexpected object type",
        connection->handle);
    goto exit;
  }

  object_id = strtol(uid, NULL, 16);

  if (!object_id)
  {
    synce_error("Unexpected uid: '%s'", uid);
    sync_set_requestfailederror(
        "Unexpected uid",
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
        "Failed to delete object",
        connection->handle);
    goto exit;
  }

  sync_set_requestdone(connection->handle);

exit:
	synce_trace("<-----");
}

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

  if (success)
    synce_mark_objects_as_unchanged(connection);
  else
    synce_warning("sync_done called with success == false");

  if (!synce_create_thread(connection))
  {
    sync_set_requestfailederror(
        "Failed to create event handling thread", 
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
	return "synce-plugin";
}

const char* long_name()
{
	return "SynCE Plugin";
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
}

const char* plugin_info()
{
	return "This plugin allows synchronization with a device running Windows CE or Pocket PC.";
}

int plugin_API_version(void) {
    return(3);
}
