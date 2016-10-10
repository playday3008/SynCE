/* $Id$ */
#define _BSD_SOURCE 1
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "synce.h"
#include "synce_log.h"
#include "config/config.h"
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#if ENABLE_ODCCM_SUPPORT || ENABLE_UDEV_SUPPORT
#include <glib-object.h>
#include <gio/gio.h>
#endif

/** 
 * @defgroup SynceUtils Synce utility functions API
 * @brief Low level functionality for communicating with a device and formatting data
 *
 */ 

/** 
 * @defgroup SynceInfo Hardware and OS information
 * @ingroup SynceUtils
 * @brief Obtaining basic information about a connected device
 *
 * @{ 
 */ 

/** @typedef struct _SynceInfo SynceInfo
 * @brief Information about a connected device
 * 
 * This is an opaque structure containing information about
 * a connected device, obtained using synce_info_new() and
 * synce_info_new_by_field(), and destroyed with synce_info_destroy().
 * It's contents should be accessed via the synce_info_get_*
 * series of functions.
 * 
 */ 
struct _SynceInfo
{
  pid_t dccm_pid;
  char* device_ip;
  char* local_iface_ip;
  char* password;
  int key;
  unsigned int os_major;
  unsigned int os_minor;
  unsigned int build_number;
  unsigned int processor_type;
  unsigned int partner_id_1;
  unsigned int partner_id_2;
  char* guid;
  char* name;
  char* os_name;
  char* model;
  char* transport;
  char* object_path;
};

#if ENABLE_ODCCM_SUPPORT || ENABLE_UDEV_SUPPORT
static const char* const DBUS_SERVICE       = "org.freedesktop.DBus";
static const char* const DBUS_IFACE         = "org.freedesktop.DBus";
static const char* const DBUS_PATH          = "/org/freedesktop/DBus";
#endif

#if ENABLE_ODCCM_SUPPORT
static const char* const ODCCM_SERVICE      = "org.synce.odccm";
static const char* const ODCCM_MGR_PATH     = "/org/synce/odccm/DeviceManager";
static const char* const ODCCM_MGR_IFACE    = "org.synce.odccm.DeviceManager";
static const char* const ODCCM_DEV_IFACE    = "org.synce.odccm.Device";
#endif

#if ENABLE_UDEV_SUPPORT
static const char* const DCCM_SERVICE      = "org.synce.dccm";
static const char* const DCCM_MGR_PATH     = "/org/synce/dccm/DeviceManager";
static const char* const DCCM_MGR_IFACE    = "org.synce.dccm.DeviceManager";
static const char* const DCCM_DEV_IFACE    = "org.synce.dccm.Device";
#endif

#define FREE(x)     if(x) free(x)

#if ENABLE_DCCM_FILE_SUPPORT
static char *STRDUP(const char* str)
{
  return str ? strdup(str) : NULL;
}

static SynceInfo* synce_info_from_file(SynceInfoIdField field, const char* data)
{
  SynceInfo* result = calloc(1, sizeof(SynceInfo));
  bool success = false;
  char* connection_filename;
  struct configFile* config = NULL;

  if (data) {
    if (field == INFO_NAME) {
      char *synce_dir;
      if (!synce_get_directory(&synce_dir)) {
        synce_error("unable to determine synce directory");
        goto exit;
      }
      int path_len = strlen(synce_dir) + strlen(data) + 2;
      connection_filename = (char *) malloc(sizeof(char) * path_len);

      if (snprintf(connection_filename, path_len, "%s/%s", synce_dir, data) >= path_len) {
        FREE(synce_dir);
        synce_error("error determining synce connection filename");
        goto exit;
      }
    }
    if (field == INFO_OBJECT_PATH) {
      connection_filename = strdup(data);
    }
  }
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
  result->os_major        = getConfigInt(config, "device", "os_version");
  result->os_minor        = getConfigInt(config, "device", "os_minor");
  result->build_number    = getConfigInt(config, "device", "build_number");
  result->processor_type  = getConfigInt(config, "device", "processor_type");
  result->partner_id_1    = getConfigInt(config, "device", "partner_id_1");
  result->partner_id_2    = getConfigInt(config, "device", "partner_id_2");

  result->device_ip       = STRDUP(getConfigString(config, "device", "ip"));
  result->local_iface_ip  = NULL;
  result->password  = STRDUP(getConfigString(config, "device", "password"));
  result->name      = STRDUP(getConfigString(config, "device", "name"));
  result->object_path     = STRDUP(connection_filename);

  if (!(result->os_name = STRDUP(getConfigString(config, "device", "os_name"))))
          result->os_name = STRDUP(getConfigString(config, "device", "class"));

  if (!(result->model = STRDUP(getConfigString(config, "device", "model"))))
          result->model = STRDUP(getConfigString(config, "device", "hardware"));

  result->transport = STRDUP(getConfigString(config, "connection", "transport"));

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

#endif /* ENABLE_DCCM_FILE_SUPPORT */

#if ENABLE_ODCCM_SUPPORT || ENABLE_UDEV_SUPPORT

static gboolean
synce_info_fields_from_dbus(SynceInfo *result, GDBusProxy *proxy)
{
  GError *error = NULL;
  GVariant *res = NULL;

  if (!(res = g_dbus_proxy_call_sync(proxy, "GetName",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_warning("%s: Failed to get device name: %s", G_STRFUNC, error->message);
      goto ERROR;
    }
  g_variant_get (res, "(s)", &(result->name));
  g_variant_unref (res);

  if (!(res = g_dbus_proxy_call_sync(proxy, "GetOsVersion",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_warning("%s: Failed to get device OS for %s: %s", G_STRFUNC, result->name, error->message);
      goto ERROR;
    }
  g_variant_get (res, "(uu)", &(result->os_major), &(result->os_minor));
  g_variant_unref (res);

  if (!(res = g_dbus_proxy_call_sync(proxy, "GetCpuType",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_warning("%s: Failed to get device cpu type for %s: %s", G_STRFUNC, result->name, error->message);
      goto ERROR;
    }
  g_variant_get (res, "(u)", &(result->processor_type));
  g_variant_unref (res);

  if (!(res = g_dbus_proxy_call_sync(proxy, "GetPlatformName",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_warning("%s: Failed to get device platform name for %s: %s", result->name, G_STRFUNC, error->message);
      goto ERROR;
    }
  g_variant_get (res, "(s)", &(result->os_name));
  g_variant_unref (res);

  if (!(res = g_dbus_proxy_call_sync(proxy, "GetModelName",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_warning("%s: Failed to get device model name for %s: %s", result->name, G_STRFUNC, error->message);
      goto ERROR;
    }
  g_variant_get (res, "(s)", &(result->model));
  g_variant_unref (res);

  if (!(res = g_dbus_proxy_call_sync(proxy, "GetIpAddress",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_warning("%s: Failed to get device IP address for %s: %s", result->name, G_STRFUNC, error->message);
      goto ERROR;
    }
  g_variant_get (res, "(s)", &(result->device_ip));
  g_variant_unref (res);

  if (!(res = g_dbus_proxy_call_sync(proxy, "GetGuid",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_warning("%s: Failed to get GUID for %s: %s", result->name, G_STRFUNC, error->message);
      goto ERROR;
    }
  g_variant_get (res, "(s)", &(result->guid));
  g_variant_unref (res);

  return TRUE;

ERROR:
  if (error != NULL)
    g_error_free(error);
  return FALSE;
}

static SynceInfo *synce_info_from_udev_or_odccm(SynceInfoIdField field, const char* data, gboolean udev)
{
  SynceInfo *result = NULL;
  GError *error = NULL;
  GDBusProxy *dbus_proxy = NULL;
  GDBusProxy *mgr_proxy = NULL;
  guint i;
  gboolean dccm_running = FALSE;
  GVariant *ret = NULL;
  gchar **devices = NULL;
  const gchar *service = NULL;
  const gchar *mgr_path = NULL;
  const gchar *mgr_iface = NULL;
  const gchar *dev_iface = NULL;
  const gchar *dccm_name = NULL;
  if (udev) {
#if ENABLE_UDEV_SUPPORT
    service = DCCM_SERVICE;
    mgr_path = DCCM_MGR_PATH;
    mgr_iface = DCCM_MGR_IFACE;
    dev_iface = DCCM_DEV_IFACE;
    dccm_name = "udev";
#endif
  } else {
#if ENABLE_ODCCM_SUPPORT
    service = ODCCM_SERVICE;
    mgr_path = ODCCM_MGR_PATH;
    mgr_iface = ODCCM_MGR_IFACE;
    dev_iface = ODCCM_DEV_IFACE;
    dccm_name = "odccm";
#endif
  }

#if !GLIB_CHECK_VERSION (2, 36, 0)
  g_type_init();
#endif

  dbus_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
					      G_DBUS_PROXY_FLAGS_NONE,
					      NULL, /* GDBusInterfaceInfo */
					      DBUS_SERVICE,
					      DBUS_PATH,
					      DBUS_IFACE,
					      NULL, /* GCancellable */
					      &error);
  if (dbus_proxy == NULL) {
    g_warning("%s: Failed to get dbus proxy object: %s", G_STRFUNC, error->message);
    goto ERROR;
  }

  ret = g_dbus_proxy_call_sync (dbus_proxy,
				"NameHasOwner",
				g_variant_new ("(s)", service),
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				NULL,
				&error);
  if (!ret) {
    g_critical("%s: Error checking owner of service %s: %s", G_STRFUNC, service, error->message);
    g_object_unref(dbus_proxy);
    goto ERROR;
  }
  g_variant_get (ret, "(b)", &dccm_running);
  g_variant_unref (ret);

  g_object_unref(dbus_proxy);
  if (!dccm_running) {
    g_message("%s is not running, ignoring", dccm_name);
    goto ERROR;
  }

  mgr_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
					     G_DBUS_PROXY_FLAGS_NONE,
					     NULL, /* GDBusInterfaceInfo */
					     service,
					     mgr_path,
					     mgr_iface,
					     NULL, /* GCancellable */
					     &error);
  if (mgr_proxy == NULL) {
    g_warning("%s: Failed to get DeviceManager proxy object: %s", G_STRFUNC, error->message);
    goto ERROR;
  }

  ret = g_dbus_proxy_call_sync (mgr_proxy,
				"GetConnectedDevices",
				g_variant_new ("()"),
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				NULL,
				&error);
  if (!ret) {
    g_warning("%s: Failed to get devices: %s", G_STRFUNC, error->message);
    goto ERROR;
  }
  g_variant_get (ret, "(^ao)", &devices);
  g_variant_unref (ret);

  if (g_strv_length(devices) == 0) {
    g_message("No devices connected to %s", dccm_name);
    goto ERROR;
  }

  for (i = 0; i < g_strv_length(devices); i++) {
    gchar *obj_path = devices[i];
    gchar *match_data = NULL;

    GDBusProxy *proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
					     G_DBUS_PROXY_FLAGS_NONE,
					     NULL, /* GDBusInterfaceInfo */
					     service,
					     obj_path,
					     dev_iface,
					     NULL, /* GCancellable */
					     &error);
    if (proxy == NULL) {
      g_warning("%s: Failed to get proxy for device '%s': %s", G_STRFUNC, obj_path, error->message);
      goto ERROR;
    }

    if (data != NULL)
    {
      switch (field)
        {
        case INFO_NAME:
	  ret = g_dbus_proxy_call_sync (proxy,
					"GetName",
					g_variant_new ("()"),
					G_DBUS_CALL_FLAGS_NONE,
					-1,
					NULL,
					&error);
	  if (!ret) {
	    g_warning("%s: Failed to get device name: %s", G_STRFUNC, error->message);
	    g_object_unref(proxy);
	    goto ERROR;
	  }
	  g_variant_get (ret, "(s)", &match_data);
	  g_variant_unref (ret);

          break;
        case INFO_OBJECT_PATH:
          match_data = g_strdup(obj_path);
          break;
        }

      if (strcasecmp(data, match_data) != 0) {
        g_free(match_data);
	g_object_unref(proxy);
        continue;
      }
    }

    g_free(match_data);

    if (!(result = calloc(1, sizeof(SynceInfo)))) {
      g_critical("%s: Failed to allocate SynceInfo", G_STRFUNC);
      g_object_unref(proxy);
      goto ERROR;
    }

    result->object_path = g_strdup(obj_path);

    if (!synce_info_fields_from_dbus(result, proxy)) {
      g_object_unref(proxy);
      goto ERROR;
    }

    if (udev) {
      ret = g_dbus_proxy_call_sync (proxy,
				    "GetIfaceAddress",
				    g_variant_new ("()"),
				    G_DBUS_CALL_FLAGS_NONE,
				    -1,
				    NULL,
				    &error);
      if (!ret) {
	g_warning("%s: Failed to get local interface IP address for %s: %s", G_STRFUNC, result->name, error->message);
	g_object_unref(proxy);
	goto ERROR;
      }
      g_variant_get (ret, "(s)", &(result->local_iface_ip));
      g_variant_unref (ret);
    } else {
      result->local_iface_ip = NULL;
    }

    g_object_unref(proxy);

    result->transport = g_strdup(dccm_name);

    break;
  }

  goto OUT;

ERROR:
  if (error != NULL)
    g_error_free(error);
  if (result) synce_info_destroy(result);
  result = NULL;

OUT:
  g_strfreev(devices);

  if (mgr_proxy != NULL)
    g_object_unref (mgr_proxy);

  return result;
}

#endif /* ENABLE_ODCCM_SUPPORT || ENABLE_UDEV_SUPPORT */


/** @brief Get device information for a named device
 * 
 * This function obtains a new SynceInfo struct containing
 * information about a connected device. If a device name is
 * specified, that device is queried, otherwise the first
 * device found is returned.
 * 
 * @param[in] device_name name of the device to query, or NULL for any device
 * @return a new SynceInfo struct, or NULL on error or if no device is found
 */ 
SynceInfo* synce_info_new(const char* device_name)
{
  return synce_info_new_by_field(INFO_NAME, device_name);
}

/** @brief Get device information for a specified device
 * 
 * This function obtains a new SynceInfo struct containing
 * information about a connected device. The device can be
 * specified by name, using INFO_NAME for the field parameter.
 * INFO_OBJECT_PATH specifies the device should be identified by
 * dbus object path, or full path to the connection file if
 * the legacy vdccm is being used. If identification data is not
 * specified, the first device found is returned.
 * 
 * @param[in] field INFO_NAME or INFO_OBJECT_PATH
 * @param[in] data identification of the device to query, or NULL for any device
 * @return a new SynceInfo struct, or NULL on error or if no device is found
 */ 
SynceInfo* synce_info_new_by_field(SynceInfoIdField field, const char* data)
{
  SynceInfo* result = NULL;

#if ENABLE_UDEV_SUPPORT
  result = synce_info_from_udev_or_odccm(field, data, TRUE);
#endif

#if ENABLE_ODCCM_SUPPORT
  if (!result)
    result = synce_info_from_udev_or_odccm(field, data, FALSE);

#if ENABLE_MIDASYNC
  if (!result)
    result = synce_info_from_midasyncd(field, data);
#endif
#endif

#if ENABLE_DCCM_FILE_SUPPORT
  if (!result)
    result = synce_info_from_file(field, data);
#endif

  return result;
}


/** @brief Destroy a SynceInfo struct
 * 
 * This function frees the memory used by a previously obtained
 * SynceInfo struct.
 * 
 * @param[in] info struct to destroy
 */ 
void synce_info_destroy(SynceInfo* info)
{
  if (info)
  {
    FREE(info->device_ip);
    FREE(info->local_iface_ip);
    FREE(info->password);
    FREE(info->name);
    FREE(info->os_name);
    FREE(info->model);
    FREE(info->transport);
    FREE(info->object_path);
    free(info);
  }
}

/** @brief Get device name from an info struct
 * 
 * This function obtains the device name from a previously
 * obtained SynceInfo struct.
 * 
 * @param[in] info struct to query
 * @return the device name, owned by the info object
 */ 
const char *
synce_info_get_name(SynceInfo *info)
{
  if (!info) return NULL;
  return info->name;
}

/** @brief Get device operation system version from an info struct
 * 
 * This function obtains the device operating system version
 * from a previously obtained SynceInfo struct.
 * 
 * @param[in] info struct to query
 * @param[out] os_major location to store the os major version
 * @param[out] os_minor location to store the os minor version
 * @return TRUE on success, FALSE on failure
 */ 
bool
synce_info_get_os_version(SynceInfo *info, unsigned int *os_major, unsigned int *os_minor)
{
  if (!info) return NULL;
  if ((!os_major) || (!os_minor))
    return false;

  *os_major = info->os_major;
  *os_minor = info->os_minor;
  return true;
}

/** @brief Get device build number from an info struct
 * 
 * This function obtains the device build number from a
 * previously obtained SynceInfo struct.
 * 
 * @param[in] info struct to query
 * @return the build number
 */ 
unsigned int
synce_info_get_build_number(SynceInfo *info)
{
  if (!info) return 0;
  return info->build_number;
}

/** @brief Get device processor type from an info struct
 * 
 * This function obtains the processor type from a
 * previously obtained SynceInfo struct.
 * 
 * @param[in] info struct to query
 * @return the processor type
 */ 
unsigned int
synce_info_get_processor_type(SynceInfo *info)
{
  if (!info) return 0;
  return info->processor_type;
}

/** @brief Get operating system name from an info struct
 * 
 * This function obtains the operating system name from a previously
 * obtained SynceInfo struct.
 * 
 * @param[in] info struct to query
 * @return the os name, owned by the info object
 */ 
const char *
synce_info_get_os_name(SynceInfo *info)
{
  if (!info) return NULL;
  return info->os_name;
}

/** @brief Get model name from an info struct
 * 
 * This function obtains the device model name from a previously
 * obtained SynceInfo struct.
 * 
 * @param[in] info struct to query
 * @return the model name, owned by the info object
 */ 
const char *
synce_info_get_model(SynceInfo *info)
{
  if (!info) return NULL;
  return info->model;
}

/** @brief Get device IP address from an info struct
 * 
 * This function obtains the IP address of a device, in
 * dotted quad notation, from a previously
 * obtained SynceInfo struct.
 * 
 * @param[in] info struct to query
 * @return the device IP address, owned by the info object
 */ 
const char *
synce_info_get_device_ip(SynceInfo *info)
{
  if (!info) return NULL;
  return info->device_ip;
}

/** @brief Get local interface IP address from an info struct
 * 
 * This function obtains the IP address of the local end of the
 * connection, in dotted quad notation, from a previously
 * obtained SynceInfo struct.
 * 
 * This is not available when using legacy odccm or vdccm, NULL
 * is returned.
 *
 * @param[in] info struct to query
 * @return the local IP address, owned by the info object
 */ 
const char *
synce_info_get_local_ip(SynceInfo *info)
{
  if (!info) return NULL;
  return info->local_iface_ip;
}

/** @brief Get GUID from an info struct
 * 
 * This function obtains the device GUID name from a previously
 * obtained SynceInfo struct.
 * 
 * Pre Windows Mobile 5 devices don't have a GUID, so this 
 * generates an appropriately formatted GUID from other 
 * identifying information from the device.
 *
 * @param[in] info struct to query
 * @return the GUID, owned by the info object
 */ 
const char *
synce_info_get_guid(SynceInfo *info)
{
  if (!info) return NULL;
  return info->guid;
}

/** @brief Get first partnership id from an info struct
 * 
 * This function obtains the first partnership id from a
 * previously obtained SynceInfo struct.
 * 
 * @param[in] info struct to query
 * @return the partnership id
 */ 
unsigned int
synce_info_get_partner_id_1(SynceInfo *info)
{
  if (!info) return 0;
  return info->partner_id_1;
}

/** @brief Get second partnership id from an info struct
 * 
 * This function obtains the second partnership id from a
 * previously obtained SynceInfo struct.
 * 
 * @param[in] info struct to query
 * @return the partnership id
 */ 
unsigned int
synce_info_get_partner_id_2(SynceInfo *info)
{
  if (!info) return 0;
  return info->partner_id_2;
}

/** @brief Get object path from an info struct
 * 
 * This function obtains the dbus object path, or in the case
 * of legacy vdccm the connection file name, from a previously
 * obtained SynceInfo struct.
 * 
 * @param[in] info struct to query
 * @return the object path, owned by the info object
 */ 
const char *
synce_info_get_object_path(SynceInfo *info)
{
  if (!info) return NULL;
  return info->object_path;
}

/** @brief Get dccm process id from an info struct
 * 
 * This function obtains the dccm process id from a
 * previously obtained SynceInfo struct.
 *
 * @deprecated This is only relevant when using a legacy
 * implementation of dccm, such as vdccm. In any other case
 * zero will be returned.
 * 
 * @param[in] info struct to query
 * @return the dccm pid, or zero
 */ 
pid_t
synce_info_get_dccm_pid(SynceInfo *info)
{
  if (!info) return 0;
  return info->dccm_pid;
}

/** @brief Get transport type from an info struct
 * 
 * This function obtains the transport type used by a device
 * from a previously obtained SynceInfo struct. This will be
 * udev, odccm, or vdccm.
 * 
 * @param[in] info struct to query
 * @return the transport type, owned by the info object
 */ 
const char *
synce_info_get_transport(SynceInfo *info)
{
  if (!info) return NULL;
  return info->transport;
}

/** @brief Get password from an info struct
 * 
 * This function obtains the password used to unlock
 * a password protected device from a previously obtained
 * SynceInfo struct..
 * 
 * @deprecated This is only valid when using a legacy
 * implementation of dccm, such as vdccm. In any other case
 * NULL will be returned.
 *
 * @param[in] info struct to query
 * @return the password, owned by the info object
 */ 
const char *
synce_info_get_password(SynceInfo *info)
{
  if (!info) return NULL;
  return info->password;
}

/** @brief Get password key from an info struct
 * 
 * This function obtains the password key used to unlock
 * a password protected device from a previously obtained
 * SynceInfo struct.
 * 
 * @deprecated This is only valid when using a legacy
 * implementation of dccm, such as vdccm. In any other case
 * zero will be returned.
 *
 * @param[in] info struct to query
 * @return the password key
 */ 
int
synce_info_get_key(SynceInfo *info)
{
  if (!info) return 0;
  return info->key;
}

/** @} */
