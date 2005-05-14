/* $Id$ */
#define _BSD_SOURCE 1
#ifdef HAVE_CONFIG_H
#include "synce_config.h"
#endif
#include "synce.h"
#include "synce_log.h"
#include "config/config.h"
#if USE_DBUS
#define DBUS_API_SUBJECT_TO_CHANGE 1
#include <dbus/dbus.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#if USE_DBUS

static const char* const MIDASYNC_SERVICE   = "com.twogood.MidaSync";
static const char* const MANAGER_INTERFACE  = "com.twogood.MidaSync.Manager";
static const char* const MANAGER_PATH       = "/com/twogood/MidaSync/Manager";
static const char* const DEVICE_INTERFACE   = "com.twogood.MidaSync.Device";

#define STR_EQUAL(a,b)  (strcmp(a,b) == 0)

#endif

#define FREE(x)     if(x) free(x)

static char *STRDUP(const char* str)
{ 
  return str ? strdup(str) : NULL;
}

static SynceInfo* synce_info_from_file(const char* filename)
{
  SynceInfo* result = calloc(1, sizeof(SynceInfo));
  bool success = false;
  char* connection_filename;
 	struct configFile* config = NULL;
 
  if (filename)
    connection_filename = strdup(filename);
  else
    synce_get_connection_filename(&connection_filename);

	config = readConfigFile(connection_filename);
	if (!config)
	{
		synce_error("unable to open file: %s", connection_filename);
		goto exit;
	}

  result->dccm_pid        = getConfigInt(config, "dccm",   "pid");

  result->key             = getConfigInt(config, "device", "key");
  result->os_version      = getConfigInt(config, "device", "os_version");
  result->build_number    = getConfigInt(config, "device", "build_number");
  result->processor_type  = getConfigInt(config, "device", "processor_type");
  result->partner_id_1    = getConfigInt(config, "device", "partner_id_1");
  result->partner_id_2    = getConfigInt(config, "device", "partner_id_2");

  result->ip        = STRDUP(getConfigString(config, "device", "ip"));
  result->password  = STRDUP(getConfigString(config, "device", "password"));
  result->name      = STRDUP(getConfigString(config, "device", "name"));
  result->os_name   = STRDUP(getConfigString(config, "device", "os_name"));
  result->model     = STRDUP(getConfigString(config, "device", "model"));

  success = true;
  
exit:
  FREE(connection_filename);

  if (config)
    unloadConfigFile(config);

  if (success)
    return result;
  else 
  {
    synce_info_destroy(result);
    return NULL;
  }
}

#if USE_DBUS

static bool get_all_devices(DBusConnection* connection, char*** pDeviceNames, int* pDeviceCount)/*{{{*/
{
  bool success = false;
  DBusError error;
  DBusMessage *message = NULL;
  DBusMessage *reply = NULL;
  DBusMessageIter iter;
  
  dbus_error_init(&error);

  /* Create and send method call */
  message = dbus_message_new_method_call(
      MIDASYNC_SERVICE,
      MANAGER_PATH,
      MANAGER_INTERFACE,
      "GetAllDevices");
  if (message == NULL) 
  {
    synce_error("Couldn't allocate D-BUS message");
    goto exit;
  }

  reply = dbus_connection_send_with_reply_and_block(
      connection,
      message, -1,
      &error);
  if (dbus_error_is_set (&error)) 
  {
    synce_error("%s error '%s'", error.name, error.message);
    goto exit;
  }

  if (!reply)
    goto exit;

  /* Handle reply */
  dbus_message_iter_init(reply, &iter);
  if (!dbus_message_iter_get_string_array(
        &iter,
        pDeviceNames,
        pDeviceCount)) 
  {
    synce_error("Invalid reply from midasyncd.");
    return NULL;
  }

  synce_debug("Found %i devices registered by midasyncd", *pDeviceCount);
  
  success = true;

exit:
  if (message)
    dbus_message_unref(message);
  if (reply)
    dbus_message_unref(reply);

  return success;
}/*}}}*/

static bool get_all_properties(DBusConnection* connection, const char* path, DBusMessage** reply)
{
  bool success = false;
  DBusError error;
  DBusMessage *message = NULL;
  
  *reply = NULL;

  dbus_error_init(&error);

  /* Create and send method call */
  message = dbus_message_new_method_call(
      MIDASYNC_SERVICE,
      path,
      DEVICE_INTERFACE,
      "GetAllProperties");
  if (message == NULL) 
  {
    synce_error("Couldn't allocate D-BUS message");
    goto exit;
  }

  *reply = dbus_connection_send_with_reply_and_block(
      connection,
      message, -1,
      &error);
  if (dbus_error_is_set (&error)) 
  {
    synce_error("%s error '%s'", error.name, error.message);
    goto exit;
  }

  if (!*reply)
    goto exit;
  
  success = true;

exit:
  if (message)
    dbus_message_unref(message);
  if (!success && *reply)
  {
    dbus_message_unref(*reply);
    *reply = NULL;
  }

  return success;
}

static bool properties_to_info(DBusMessage* properties, SynceInfo* info)
{
  DBusMessageIter iter;
  DBusMessageIter iter_dict;

  dbus_message_iter_init(properties, &iter);

  for (dbus_message_iter_init_dict_iterator(&iter, &iter_dict); 
      ; 
      dbus_message_iter_next(&iter_dict))
  {
    char* str_value = NULL;
    char* key      = dbus_message_iter_get_dict_key(&iter_dict);
    int   type     = dbus_message_iter_get_arg_type(&iter_dict);
    int   int_value; 

    synce_debug("Key = %s", key);

    if (!key)
      continue;

    switch (type)
    {
      case DBUS_TYPE_STRING:
        str_value = dbus_message_iter_get_string(&iter_dict);

        if (STR_EQUAL(key, "address"))
          info->ip = STRDUP(str_value);
        else if (STR_EQUAL(key, "password"))
          info->password = STRDUP(str_value);

        /* TODO: handle more string properties */

        dbus_free(str_value);
        break;

      case DBUS_TYPE_INT32:
        int_value = dbus_message_iter_get_int32(&iter_dict);

        if (STR_EQUAL(key, "key"))
          info->key = int_value;

        /* TODO: handle more int32 properties */

        break;
    }

    dbus_free(key);

    if (!dbus_message_iter_has_next(&iter_dict))
      break;
  }

  /* Fake dccm PID! */
  info->dccm_pid = getpid();

  return info->ip != NULL;
}

static SynceInfo* synce_info_from_midasyncd(const char* path)
{
  bool success = false;
  SynceInfo* result = calloc(1, sizeof(SynceInfo));
  DBusConnection* connection = NULL;
  DBusError error;
  char** device_names = NULL;
  int device_count = 0;
  DBusMessage* properties = NULL;

  dbus_error_init(&error);

  /* Connect to D-BUS */
  connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
  if (dbus_error_is_set (&error)) 
  {
    synce_error("%s error '%s'", error.name, error.message);
    dbus_error_free(&error);
    goto exit;
  }

  if (!connection) {
    synce_error("Failed to connect to the D-BUS daemon");
    goto exit;
  }

  if (!path)
  {
    if (!get_all_devices(connection, &device_names, &device_count))
      goto exit;

    if (device_count < 1)
      goto exit;

    path = device_names[0];
  }

  if (!get_all_properties(connection, path, &properties))
    goto exit;

  if (!properties_to_info(properties, result))
    goto exit;
  
  success = true;
  
exit:
  /* TODO: clean up D-BUS connection */
  if (device_names)
    dbus_free_string_array(device_names);

  if (success)
    return result;
  else 
  {
    synce_info_destroy(result);
    return NULL;
  }
}

#endif

SynceInfo* synce_info_new(const char* path)
{
  SynceInfo* result = NULL;

  result = synce_info_from_file(path);

#if USE_DBUS
  if (!result)
    result = synce_info_from_midasyncd(path);
#endif
  
  return result;
}


void synce_info_destroy(SynceInfo* info)
{
  if (info)
  {
    FREE(info->ip);
    FREE(info->password);
    FREE(info->name);
    FREE(info->os_name);
    FREE(info->model);
    free(info);
  }
}

