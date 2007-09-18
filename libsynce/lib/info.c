/* $Id$ */
#define _BSD_SOURCE 1
#ifdef HAVE_CONFIG_H
#include "synce_config.h"
#endif
#include "synce.h"
#include "synce_log.h"
#include "config/config.h"
#if ENABLE_DESKTOP_INTEGRATION
#define DBUS_API_SUBJECT_TO_CHANGE 1
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if ENABLE_DESKTOP_INTEGRATION
static const char* const ODCCM_SERVICE      = "org.synce.odccm";
static const char* const ODCCM_MGR_PATH     = "/org/synce/odccm/DeviceManager";
static const char* const ODCCM_MGR_IFACE    = "org.synce.odccm.DeviceManager";
static const char* const ODCCM_DEV_IFACE    = "org.synce.odccm.Device";
#endif

#define FREE(x)     if(x) free(x)

static char *STRDUP(const char* str)
{
  return str ? strdup(str) : NULL;
}

static SynceInfo* synce_info_from_file(const char* device_name)
{
  SynceInfo* result = calloc(1, sizeof(SynceInfo));
  bool success = false;
  char* connection_filename;
  struct configFile* config = NULL;

  if (device_name) {
    char *synce_dir;
    if (!synce_get_directory(&synce_dir)) {
      synce_error("unable to determine synce directory");
      goto exit;
    }
    int path_len = strlen(synce_dir) + strlen(device_name) + 2;
    connection_filename = (char *) malloc(sizeof(char) * path_len);

    if (snprintf(connection_filename, path_len, "%s/%s", synce_dir, device_name) >= path_len) {
      FREE(synce_dir);
      synce_error("error determining synce connection filename");
      goto exit;
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

#if ENABLE_DESKTOP_INTEGRATION

gint
get_socket_from_odccm(const gchar *unix_path)
{
  int fd = -1, dev_fd, ret;
  struct sockaddr_un sa;
  struct msghdr msg = { 0, };
  struct cmsghdr *cmsg;
  struct iovec iov;
  char cmsg_buf[512];
  char data_buf[512];

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0)
    goto ERROR;

  sa.sun_family = AF_UNIX;
  strcpy(sa.sun_path, unix_path);

  if (connect(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
    goto ERROR;

  msg.msg_control = cmsg_buf;
  msg.msg_controllen = sizeof(cmsg_buf);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_flags = MSG_WAITALL;

  iov.iov_base = data_buf;
  iov.iov_len = sizeof(data_buf);

  ret = recvmsg(fd, &msg, 0);
  if (ret < 0)
    goto ERROR;

  cmsg = CMSG_FIRSTHDR (&msg);
  if (cmsg == NULL || cmsg->cmsg_type != SCM_RIGHTS)
    goto ERROR;

  dev_fd = *((int *) CMSG_DATA(cmsg));
  goto OUT;

ERROR:
  dev_fd = -1;

OUT:
  if (fd >= 0)
    close(fd);

  return dev_fd;
}

#define ODCCM_TYPE_OBJECT_PATH_ARRAY \
  (dbus_g_type_get_collection("GPtrArray", DBUS_TYPE_G_OBJECT_PATH))

static SynceInfo *synce_info_from_odccm(const char* device_name)
{
  SynceInfo *result = NULL;
  GError *error = NULL;
  DBusGConnection *bus = NULL;
  DBusGProxy *mgr_proxy = NULL;
  GPtrArray *devices = NULL;
  guint i;

  g_type_init();

  bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
  if (bus == NULL)
  {
    g_warning("%s: Failed to connect to system bus: %s", G_STRFUNC, error->message);
    goto ERROR;
  }

  mgr_proxy = dbus_g_proxy_new_for_name(bus, ODCCM_SERVICE,
                                        ODCCM_MGR_PATH,
                                        ODCCM_MGR_IFACE);
  if (mgr_proxy == NULL) {
    g_warning("%s: Failed to get DeviceManager proxy object", G_STRFUNC);
    goto ERROR;
  }

  if (!dbus_g_proxy_call(mgr_proxy, "GetConnectedDevices", &error,
                         G_TYPE_INVALID,
                         ODCCM_TYPE_OBJECT_PATH_ARRAY, &devices,
                         G_TYPE_INVALID))
  {
    g_warning("%s: Failed to get devices: %s", G_STRFUNC, error->message);
    goto ERROR;
  }

  if (devices->len == 0) {
    g_warning("No devices connected to odccm");
    goto ERROR;
  }

  for (i = 0; i < devices->len; i++) {
    gchar *obj_path = g_ptr_array_index(devices, i);
    gchar *name;
    DBusGProxy *proxy = dbus_g_proxy_new_for_name(bus, ODCCM_SERVICE,
                                                  obj_path,
                                                  ODCCM_DEV_IFACE);
    if (proxy == NULL) {
      g_warning("%s: Failed to get proxy for device '%s'", G_STRFUNC, obj_path);
      goto ERROR;
    }

    if (!dbus_g_proxy_call(proxy, "GetName", &error,
                           G_TYPE_INVALID,
                           G_TYPE_STRING, &name,
                           G_TYPE_INVALID))
    {
      g_warning("%s: Failed to get device name: %s", G_STRFUNC, error->message);
      g_object_unref(proxy);
      goto ERROR;
    }

    if ( (device_name != NULL) && (strcmp(device_name, name) != 0) ) {
      g_free(name);
      continue;
    }

    if (!(result = calloc(1, sizeof(SynceInfo)))) {
      g_critical("%s: Failed to allocate SynceInfo", G_STRFUNC);
      g_object_unref(proxy);
      g_free(name);
      goto ERROR;
    }

    gchar *unix_path;
    guint os_major;
    guint os_minor;

    result->name = name;

    if (!dbus_g_proxy_call(proxy, "GetOsVersion", &error,
                           G_TYPE_INVALID,
                           G_TYPE_UINT, &os_major,
                           G_TYPE_UINT, &os_minor,
                           G_TYPE_INVALID))
    {
      g_warning("%s: Failed to get device OS for %s: %s", G_STRFUNC, result->name, error->message);
      g_object_unref(proxy);
      goto ERROR;
    }

    result->os_version = os_major;

    if (!dbus_g_proxy_call(proxy, "RequestConnection", &error,
                           G_TYPE_INVALID,
                           G_TYPE_STRING, &unix_path,
                           G_TYPE_INVALID))
    {
      g_warning("%s: Failed to get a connection for %s: %s", G_STRFUNC, result->name, error->message);
      g_object_unref(proxy);
      goto ERROR;
    }

    g_object_unref(proxy);

    result->fd = get_socket_from_odccm(unix_path);
    g_free(unix_path);

    if (result->fd < 0)
    {
      g_warning("%s: Failed to get file-descriptor from odccm for %s", G_STRFUNC, result->name);
      goto ERROR;
    }

    result->transport = g_strdup("odccm");

    break;
  }

  goto OUT;

ERROR:
  if (error != NULL)
    g_error_free(error);
  if (result) synce_info_destroy(result);
  result = NULL;

OUT:
  if (devices != NULL) {
    for (i = 0; i < devices->len; i++)
      g_free(g_ptr_array_index(devices, i));

    g_ptr_array_free(devices, TRUE);
  }

  if (mgr_proxy != NULL)
    g_object_unref (mgr_proxy);
  if (bus != NULL)
    dbus_g_connection_unref (bus);

  return result;
}
#endif /* ENABLE_DESKTOP_INTEGRATION */

SynceInfo* synce_info_new(const char* device_name)
{
  SynceInfo* result = NULL;

#if ENABLE_DESKTOP_INTEGRATION
  result = synce_info_from_odccm(device_name);

#if ENABLE_MIDASYNC
  if (!result)
    result = synce_info_from_midasyncd(device_name);
#endif
#endif

  if (!result)
    result = synce_info_from_file(device_name);

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
    FREE(info->transport);
    free(info);
  }
}

