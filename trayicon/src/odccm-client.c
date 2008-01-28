/*
Copyright (c) 2007 Mark Ellis <mark@mpellis.org.uk>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib-object.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <synce.h>
#include <rapi.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "odccm-client.h"
#include "dccm-client-signals-marshal.h"
#include "dccm-client.h"
#include "device.h"
#include "utils.h"

static void dccm_client_interface_init (gpointer g_iface, gpointer iface_data);
G_DEFINE_TYPE_EXTENDED (OdccmClient, odccm_client, G_TYPE_OBJECT, 0, G_IMPLEMENT_INTERFACE (DCCM_CLIENT_TYPE, dccm_client_interface_init))

typedef struct _OdccmClientPrivate OdccmClientPrivate;
struct _OdccmClientPrivate {
  DBusGConnection *dbus_connection;
  DBusGProxy *dev_mgr_proxy;

  GPtrArray *dev_proxies;
  GHashTable *pending_devices;

  gboolean disposed;
};

#define ODCCM_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ODCCM_CLIENT_TYPE, OdccmClientPrivate))


#define ODCCM_SERVICE   "org.synce.odccm"
#define ODCCM_MGR_PATH  "/org/synce/odccm/DeviceManager"
#define ODCCM_MGR_IFACE "org.synce.odccm.DeviceManager"
#define ODCCM_DEV_IFACE "org.synce.odccm.Device"

enum {
  ODCCM_DEVICE_PASSWORD_FLAG_SET = 1,
  ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE = 2,
  ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE = 4
};

typedef struct _proxy_store proxy_store;
struct _proxy_store{
  gchar *pdaname;
  DBusGProxy *proxy;
};

/* methods */

static void 
password_flags_changed_cb(DBusGProxy *proxy,
			       guint added, guint removed,
			       gpointer user_data)
{
  if (!user_data) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  OdccmClient *self = ODCCM_CLIENT(user_data);
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }
  if (!priv->dev_mgr_proxy) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return;
  }

  gchar *pdaname = NULL;
  gint i;
  const gchar *obj_path;

  obj_path = dbus_g_proxy_get_path(proxy);

  i = 0;
  while (i < priv->dev_proxies->len) {
    if (!(g_ascii_strcasecmp(obj_path, dbus_g_proxy_get_path(((proxy_store *)g_ptr_array_index(priv->dev_proxies, i))->proxy)))) {
      pdaname = (((proxy_store *)g_ptr_array_index(priv->dev_proxies, i))->pdaname);
      break;
    }
    i++;
  }

  if (!pdaname) {
    g_warning("%s: Received password flags changed for unfound device: %s", G_STRFUNC, obj_path);
    return;
  }

  if (added & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE)
    g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REQUIRED], 0, pdaname);
  if (added & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE)
    g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REQUIRED_ON_DEVICE], 0, pdaname);

  return;
}

static void
free_proxy_store(proxy_store *p_store)
{
  g_free(p_store->pdaname);
  g_object_unref(p_store->proxy);
  g_free(p_store);
}


/* get_rapi_socket_from_odccm() is copied from get_socket_from_odccm()
   in libsynce, which doesn't export the function */

static gint
get_rapi_socket_from_odccm(const gchar *unix_path)
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

static gboolean
odccm_device_get_rapi_connection(OdccmClient *self, WmDevice *device)
{
  proxy_store *p_store = NULL;
  GError *error = NULL;
  RapiConnection *rapi_conn;
  HRESULT hr;
  gchar *unix_path = NULL;
  gchar *name = NULL;
  gchar *device_name;
  gint i = 0;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return FALSE;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return FALSE;
  }
  if (!priv->dev_mgr_proxy) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return TRUE;
  }

  g_object_get(device, "name", &name, NULL);
  while (i < priv->dev_proxies->len) {
    if (!(g_ascii_strcasecmp(name, ((proxy_store *)g_ptr_array_index(priv->dev_proxies, i))->pdaname))) {
      p_store = (proxy_store *)g_ptr_array_index(priv->dev_proxies, i);
      break;
    }
    i++;
  }
  if (!p_store) {
    g_critical("%s: called for unfound device %s", G_STRFUNC, name);
    g_free(name);
    return FALSE;
  }
  g_free(name);

  if (!(dbus_g_proxy_call(p_store->proxy, "RequestConnection",
			  &error, G_TYPE_INVALID,
			  G_TYPE_STRING, &unix_path,
			  G_TYPE_INVALID))) {
    g_critical("%s: Failed to request a connection: %s", G_STRFUNC, error->message);
    g_error_free(error);
    return FALSE;
  }

  SynceInfo *info = (SynceInfo *) g_malloc0(sizeof(SynceInfo));
  info->fd = get_rapi_socket_from_odccm(unix_path);
  g_free(unix_path);

  if (info->fd < 0) {
    g_warning("%s: Failed to get file descriptor from odccm", G_STRFUNC);
    g_free(info);
    return FALSE;
  }

  info->transport = g_strdup("odccm");

  rapi_conn = rapi_connection_from_info(info);

  rapi_connection_select(rapi_conn);
  CeRapiInit();

  hr = CeRapiInit();
  if (FAILED(hr)) {
    g_critical("%s: Rapi connection to %s failed: %d: %s", G_STRFUNC, info->name, hr, synce_strerror(hr));
    rapi_connection_destroy(rapi_conn);
    goto error_exit;;
  }

  device_name = get_device_name_via_rapi();
  if (!(device_name)) {
    CeRapiUninit();
    rapi_connection_destroy(rapi_conn);
    goto error_exit;
  }

  g_object_set(device, "rapi-conn", rapi_conn, NULL);
  g_object_set(device, "device-name", device_name, NULL);
  g_free(device_name);

  return TRUE;
error_exit:
  synce_info_destroy(info);
  return FALSE;
}

static void
odccm_device_connected_cb(DBusGProxy *proxy,
			 gchar *obj_path,
			 gpointer user_data)
{
  GError *error = NULL;
  DBusGProxy *new_proxy;
  WmDevice *device = NULL;

  OdccmClient *self = ODCCM_CLIENT(user_data);

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }
  if (!priv->dev_mgr_proxy) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return;
  }

  guint password_flags;
  gint os_major, os_minor,
    processor_type, partner_id_1;
  gchar *ip, *name, *os_name,
    *model, *transport;

  g_debug("%s: Received connect from odccm: %s", G_STRFUNC, obj_path);

  if (!(new_proxy = dbus_g_proxy_new_for_name (priv->dbus_connection,
					       ODCCM_SERVICE,
					       obj_path,
					       ODCCM_DEV_IFACE)))
    {
      g_critical("%s: Error getting proxy for device path %s", G_STRFUNC, obj_path);
      return;
    }

  if (!(dbus_g_proxy_call(new_proxy, "GetName",
			  &error, G_TYPE_INVALID,
			  G_TYPE_STRING, &(name),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device name from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  if (!(dbus_g_proxy_call(new_proxy, "GetIpAddress",
			  &error, G_TYPE_INVALID,
			  G_TYPE_STRING, &(ip),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device ip from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  if (!(dbus_g_proxy_call(new_proxy, "GetCpuType",
			  &error, G_TYPE_INVALID,
			  G_TYPE_UINT, &(processor_type),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device processor type from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  if (!(dbus_g_proxy_call(new_proxy, "GetOsVersion",
			  &error, G_TYPE_INVALID,
			  G_TYPE_UINT, &(os_major),
			  G_TYPE_UINT, &(os_minor),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device os version from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  if (!(dbus_g_proxy_call(new_proxy, "GetCurrentPartnerId",
			  &error,G_TYPE_INVALID,
			  G_TYPE_UINT, &(partner_id_1),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device partner id from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  if (!(dbus_g_proxy_call(new_proxy, "GetPlatformName",
			  &error, G_TYPE_INVALID,
			  G_TYPE_STRING, &(os_name),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device platform name from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  if (!(dbus_g_proxy_call(new_proxy, "GetModelName",
			  &error, G_TYPE_INVALID,
			  G_TYPE_STRING, &(model),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device model name from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  transport = g_strdup("odccm");

  device = g_object_new(WM_DEVICE_TYPE,
			"name", name,
			"os-major", os_major,
			"os-minor", os_minor,
			"build-number", 0,
			"processor-type", processor_type,
			"partner-id-1", partner_id_1,
			"partner-id-2", 0,
			"class", os_name,
			"hardware", model,
			"ip", ip,
			"transport", transport,
			NULL);

  g_free(name);
  g_free(ip);
  g_free(os_name);
  g_free(model);
  g_free(transport);

  if (!device) {
    g_critical("%s: Error creating new device", G_STRFUNC);
    goto error_exit;
  }

  dbus_g_proxy_add_signal (new_proxy, "PasswordFlagsChanged",
			   G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (new_proxy, "PasswordFlagsChanged",
			       G_CALLBACK(password_flags_changed_cb), self, NULL);

  if (!(dbus_g_proxy_call(new_proxy, "GetPasswordFlags",
			  &error, G_TYPE_INVALID,
			  G_TYPE_UINT, &(password_flags),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting password flags from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  g_object_get(device, "name", &name, NULL);
  proxy_store *p_store = g_malloc0(sizeof(proxy_store));
  p_store->pdaname = g_strdup(name);
  p_store->proxy = new_proxy;

  g_ptr_array_add(priv->dev_proxies, p_store);

  if (password_flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE) {
    g_hash_table_insert(priv->pending_devices, g_strdup(name), device);
    g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REQUIRED], 0, name);
    goto exit;
  }
  if (password_flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE) {
    g_hash_table_insert(priv->pending_devices, g_strdup(name), device);
    g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REQUIRED_ON_DEVICE], 0, name);
    goto exit;
  }

  /* get rapi connection */
  if (!(odccm_device_get_rapi_connection(self, device))) {
    g_ptr_array_remove(priv->dev_proxies, p_store);
    g_free(p_store->pdaname);
    g_free(p_store);
    goto error_exit;
  }

  g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_CONNECTED], 0, name, (gpointer)device);
  g_free(name);

  goto exit;
error_exit:
  g_object_unref(new_proxy);
  if (device) g_object_unref(device);
exit:
  return;
}

void
odccm_device_disconnected_cb(DBusGProxy *proxy,
		      gchar *obj_path,
		      gpointer user_data)
{
  OdccmClient *self = ODCCM_CLIENT(user_data);

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }
  if (!priv->dev_mgr_proxy) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return;
  }

  gchar *pdaname = NULL;
  proxy_store *p_store = NULL;
  gint i;

  i = 0;
  while (i < priv->dev_proxies->len) {
    if (!(g_ascii_strcasecmp(obj_path, dbus_g_proxy_get_path(((proxy_store *)g_ptr_array_index(priv->dev_proxies, i))->proxy)))) {
      p_store = (proxy_store *)g_ptr_array_index(priv->dev_proxies, i);
      pdaname = p_store->pdaname;
      break;
    }
    i++;
  }

  if (!pdaname) {
    g_warning("%s: Received disconnect from odccm from unfound device: %s", G_STRFUNC, obj_path);
    return;
  }

  g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_DISCONNECTED], 0, pdaname);

  free_proxy_store(p_store);
  g_ptr_array_remove_index_fast(priv->dev_proxies, i);

  return;
}

static gboolean
clear_pending_devices(gpointer key, gpointer value, gpointer data)
{
  g_object_unref(WM_DEVICE(value));
  return TRUE;
}

gboolean
odccm_client_uninit_comms_impl(OdccmClient *self)
{
  gint i;
  proxy_store *p_store;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return FALSE;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return FALSE;
  }
  if (!priv->dev_mgr_proxy) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return TRUE;
  }

  dbus_g_proxy_disconnect_signal (priv->dev_mgr_proxy, "DeviceConnected",
			       G_CALLBACK(odccm_device_connected_cb), NULL);

  dbus_g_proxy_disconnect_signal (priv->dev_mgr_proxy, "DeviceDisconnected",
			       G_CALLBACK(odccm_device_disconnected_cb), NULL);

  g_object_unref (priv->dev_mgr_proxy);
  priv->dev_mgr_proxy = NULL;

  dbus_g_connection_unref(priv->dbus_connection);
  priv->dbus_connection = NULL;

  for (i = 0; i < priv->dev_proxies->len; i++) {
    p_store = (proxy_store *)g_ptr_array_index(priv->dev_proxies, i);
    free_proxy_store(p_store);
  }
  g_ptr_array_free(priv->dev_proxies, TRUE);

  g_hash_table_foreach_remove(priv->pending_devices, clear_pending_devices, NULL);
  g_hash_table_destroy(priv->pending_devices);

  return TRUE;
}

void
odccm_client_provide_password_impl(OdccmClient *self, gchar *pdaname, gchar *password)
{
  GError *error = NULL;
  gboolean password_accepted = FALSE, result;
  gint i;
  DBusGProxy *proxy = NULL;
  WmDevice *device;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }
  if (!priv->dev_mgr_proxy) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return;
  }

  i = 0;
  while (i < priv->dev_proxies->len) {
    if (!(g_ascii_strcasecmp(pdaname, ((proxy_store *)g_ptr_array_index(priv->dev_proxies, i))->pdaname))) {
      proxy = (((proxy_store *)g_ptr_array_index(priv->dev_proxies, i))->proxy);
      break;
    }
    i++;
  }

  if (!proxy) {
    g_warning("%s: Password provided for unfound device: %s", G_STRFUNC, pdaname);
    return;
  }

  result = dbus_g_proxy_call(proxy,
			     "ProvidePassword",
			     &error,
			     G_TYPE_STRING, password,
			     G_TYPE_INVALID,
			     G_TYPE_BOOLEAN, &password_accepted,
			     G_TYPE_INVALID);

  if (result == FALSE) {
    g_critical("%s: Error sending password to odccm for %s: %s", G_STRFUNC, pdaname, error->message);
    g_error_free(error);
    device = g_hash_table_lookup(priv->pending_devices, pdaname);
    if (!device) {
      g_critical("%s: Unknown pending device %s", G_STRFUNC, pdaname);
    } else {
      g_object_unref(device);
      g_hash_table_remove(priv->pending_devices, pdaname);
    }
    goto exit;
  }

  if (!(password_accepted)) {
    g_debug("%s: Password rejected for %s", G_STRFUNC, pdaname);
    g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REJECTED], 0, pdaname);
    goto exit;
  }

  device = g_hash_table_lookup(priv->pending_devices, pdaname);
  g_hash_table_remove(priv->pending_devices, pdaname);

  /* get rapi connection */
  if (!(odccm_device_get_rapi_connection(self, device))) {
      g_object_unref(device);
      goto exit;
  }

  g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_CONNECTED], 0, pdaname, (gpointer)device);

exit:
  return;
}

gboolean
odccm_client_request_disconnect_impl(OdccmClient *self, gchar *pdaname)
{
  gboolean result = FALSE;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return result;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return result;
  }
  if (!priv->dev_mgr_proxy) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return result;
  }

  g_debug("%s: Odccm doesn't support a disconnect command", G_STRFUNC);

  result = FALSE;
  return result;
}


gboolean
odccm_client_init_comms_impl(OdccmClient *self)
{
  GError *error = NULL;
  gboolean result = FALSE;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return result;
  }
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return result;
  }
  if (priv->dbus_connection) {
    g_warning("%s: Initialised object passed", G_STRFUNC);
    return result;
  }

  priv->dbus_connection = dbus_g_bus_get (DBUS_BUS_SYSTEM,
                               &error);
  if (priv->dbus_connection == NULL)
    {
      g_critical("%s: Failed to open connection to bus: %s", G_STRFUNC, error->message);
      g_error_free (error);
      return result;
    }

  priv->dev_mgr_proxy = dbus_g_proxy_new_for_name (priv->dbus_connection,
                                     ODCCM_SERVICE,
                                     ODCCM_MGR_PATH,
                                     ODCCM_MGR_IFACE);
  if (priv->dev_mgr_proxy == NULL) {
      g_critical("%s: Failed to create proxy to device manager", G_STRFUNC);
    }

  dbus_g_proxy_add_signal (priv->dev_mgr_proxy, "DeviceConnected",
			   G_TYPE_STRING, G_TYPE_INVALID);

  dbus_g_proxy_add_signal (priv->dev_mgr_proxy, "DeviceDisconnected",
			   G_TYPE_STRING, G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (priv->dev_mgr_proxy, "DeviceConnected",
			       G_CALLBACK(odccm_device_connected_cb), self, NULL);

  dbus_g_proxy_connect_signal (priv->dev_mgr_proxy, "DeviceDisconnected",
			       G_CALLBACK(odccm_device_disconnected_cb), self, NULL);

  /* currently connected devices 
     TODO
     enumerate through these properly */

  GPtrArray* dev_list;
  if (!(dbus_g_proxy_call(priv->dev_mgr_proxy, "GetConnectedDevices",
			  &error, G_TYPE_INVALID,
			  dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_OBJECT_PATH),
			  &dev_list,
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device list from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    return TRUE;
  }
  int i;
  for (i = 0; i < dev_list->len; i++) {
    g_debug("%s: device %d: %s", G_STRFUNC, i, (char *)g_ptr_array_index(dev_list, i));
  }

  result = TRUE;

  return result;
}


/* class & instance functions */

static void
hash_key_str_destroy(gpointer data)
{
  g_free((gchar *)data);
}

static void
odccm_client_init(OdccmClient *self)
{
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  priv->dev_mgr_proxy = NULL;
  priv->dev_proxies = g_ptr_array_new();
  priv->pending_devices = g_hash_table_new_full(g_str_hash, g_str_equal, hash_key_str_destroy, NULL);
}

static void
odccm_client_dispose (GObject *obj)
{
  OdccmClient *self = ODCCM_CLIENT(obj);
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (obj);

  if (priv->disposed) {
    return;
  }
  if (priv->dev_mgr_proxy)
    dccm_client_uninit_comms(DCCM_CLIENT(self));

  priv->disposed = TRUE;

  /* unref other objects */

  if (G_OBJECT_CLASS (odccm_client_parent_class)->dispose)
    G_OBJECT_CLASS (odccm_client_parent_class)->dispose (obj);
}

static void
odccm_client_finalize (GObject *obj)
{
  if (G_OBJECT_CLASS (odccm_client_parent_class)->finalize)
    G_OBJECT_CLASS (odccm_client_parent_class)->finalize (obj);
}

static void
odccm_client_class_init (OdccmClientClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = odccm_client_dispose;
  gobject_class->finalize = odccm_client_finalize;

  g_type_class_add_private (klass, sizeof (OdccmClientPrivate));

  dbus_g_object_register_marshaller (dccm_client_marshal_VOID__UINT_UINT,
				     G_TYPE_NONE,
				     G_TYPE_UINT,
				     G_TYPE_UINT,
				     G_TYPE_INVALID);
}

static void
dccm_client_interface_init (gpointer g_iface, gpointer iface_data)
{
  DccmClientInterface *iface = (DccmClientInterface *)g_iface;

  iface->dccm_client_init_comms = (gboolean (*) (DccmClient *self)) odccm_client_init_comms_impl;
  iface->dccm_client_uninit_comms = (gboolean (*) (DccmClient *self)) odccm_client_uninit_comms_impl;
  iface->dccm_client_provide_password = (void (*) (DccmClient *self, gchar *pdaname, gchar *password)) odccm_client_provide_password_impl;
  iface->dccm_client_request_disconnect = (gboolean (*) (DccmClient *self, gchar *pdaname)) odccm_client_request_disconnect_impl;
}
