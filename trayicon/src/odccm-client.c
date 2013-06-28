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
#if USE_GDBUS
#include <gio/gio.h>
#else
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#endif
#include <synce.h>
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
#if USE_GDBUS
  GDBusProxy *dbus_proxy;
  GDBusProxy *dev_mgr_proxy;
#else /* USE_GDBUS */
  DBusGConnection *dbus_connection;
  DBusGProxy *dbus_proxy;
  DBusGProxy *dev_mgr_proxy;
#endif /* USE_GDBUS */
  gboolean online;

  GPtrArray *dev_proxies;

  gboolean disposed;
};

#define ODCCM_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ODCCM_CLIENT_TYPE, OdccmClientPrivate))

#define DBUS_SERVICE "org.freedesktop.DBus"
#define DBUS_IFACE   "org.freedesktop.DBus"
#define DBUS_PATH    "/org/freedesktop/DBus"

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
#if USE_GDBUS
  GDBusProxy *proxy;
#else
  DBusGProxy *proxy;
#endif
};

/* methods */

#if USE_GDBUS

static void
on_dev_proxy_signal (GDBusProxy *proxy,
                     gchar *sender_name,
                     gchar *signal_name,
                     GVariant *parameters,
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

  if (strcmp(signal_name, "PasswordFlagsChanged") != 0) {
    return;
  }

  guint added, removed;
  const gchar *obj_path = NULL;
  obj_path = g_dbus_proxy_get_object_path(proxy);

  g_variant_get(parameters, "(uu)", &added, &removed);
#else
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

  const gchar *obj_path;

  obj_path = dbus_g_proxy_get_path(proxy);

#endif /* USE_GDBUS */

  gchar *pdaname = NULL;
  gint i;

  i = 0;
  while (i < priv->dev_proxies->len) {
#if USE_GDBUS
    if (!(g_ascii_strcasecmp(obj_path, g_dbus_proxy_get_object_path(((proxy_store *)g_ptr_array_index(priv->dev_proxies, i))->proxy)))) {
#else
    if (!(g_ascii_strcasecmp(obj_path, dbus_g_proxy_get_path(((proxy_store *)g_ptr_array_index(priv->dev_proxies, i))->proxy)))) {
#endif
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


  if ((removed & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE) || (removed & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE))
    g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_UNLOCKED], 0, pdaname);

  return;
}

static void
free_proxy_store(proxy_store *p_store)
{
  g_free(p_store->pdaname);
  g_object_unref(p_store->proxy);
  g_free(p_store);
}


static void
odccm_add_device(OdccmClient *self,
		 gchar *obj_path)
{
  GError *error = NULL;
#if USE_GDBUS
  GDBusProxy *new_proxy;
  GVariant *res = NULL;
#else
  DBusGProxy *new_proxy;
#endif
  WmDevice *device = NULL;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }
#if USE_GDBUS
  if (!priv->dbus_proxy) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return;
  }
#else
  if (!priv->dbus_connection) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return;
  }
#endif

  guint password_flags;
  gint os_major, os_minor,
    processor_type, partner_id_1;
  gchar *ip, *name, *os_name,
    *model, *transport;

  g_debug("%s: Received connect from odccm: %s", G_STRFUNC, obj_path);

#if USE_GDBUS

  new_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                            G_DBUS_PROXY_FLAGS_NONE,
                                            NULL, /* GDBusInterfaceInfo */
                                            ODCCM_SERVICE,
                                            obj_path,
                                            ODCCM_DEV_IFACE,
                                            NULL, /* GCancellable */
                                            &error);
  if (new_proxy == NULL) {
    g_critical("%s: Error getting proxy for device path '%s': %s", G_STRFUNC, obj_path, error->message);
    g_error_free(error);
    return;
  }

  if (!(res = g_dbus_proxy_call_sync(new_proxy, "GetName",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_critical("%s: Error getting device name from odccm: %s", G_STRFUNC, error->message);
      g_error_free(error);
      goto error_exit;
    }
  g_variant_get (res, "(s)", &name);
  g_variant_unref (res);

  if (!(res = g_dbus_proxy_call_sync(new_proxy, "GetIpAddress",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_critical("%s: Error getting device ip from odccm: %s", G_STRFUNC, error->message);
      g_error_free(error);
      goto error_exit;
    }
  g_variant_get (res, "(s)", &ip);
  g_variant_unref (res);

  if (!(res = g_dbus_proxy_call_sync(new_proxy, "GetCpuType",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_warning("%s: Error getting device processor type from odccm: %s", G_STRFUNC, error->message);
      g_error_free(error);
      goto error_exit;
    }
  g_variant_get (res, "(u)", &processor_type);
  g_variant_unref (res);

  if (!(res = g_dbus_proxy_call_sync(new_proxy, "GetOsVersion",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_warning("%s: Error getting device os version from odccm: %s", G_STRFUNC, error->message);
      g_error_free(error);
      goto error_exit;
    }
  g_variant_get (res, "(uu)", &os_major, &os_minor);
  g_variant_unref (res);

  if (!(res = g_dbus_proxy_call_sync(new_proxy, "GetCurrentPartnerId",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_warning("%s: Error getting device partner id from odccm: %s", G_STRFUNC, error->message);
      g_error_free(error);
      goto error_exit;
    }
  g_variant_get (res, "(u)", &partner_id_1);
  g_variant_unref (res);

  if (!(res = g_dbus_proxy_call_sync(new_proxy, "GetPlatformName",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_warning("%s: Error getting device platform name from odccm: %s", G_STRFUNC, error->message);
      g_error_free(error);
      goto error_exit;
    }
  g_variant_get (res, "(s)", &os_name);
  g_variant_unref (res);

  if (!(res = g_dbus_proxy_call_sync(new_proxy, "GetModelName",
				g_variant_new ("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error)))
    {
      g_warning("%s: Error getting device model name from odccm: %s", G_STRFUNC, error->message);
      g_error_free(error);
      goto error_exit;
    }
  g_variant_get (res, "(s)", &model);
  g_variant_unref (res);

#else /* USE_GDBUS */

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

#endif /* USE_GDBUS */

  transport = g_strdup("odccm");

  device = g_object_new(WM_DEVICE_TYPE,
                        "object-name", obj_path,
                        "dccm-type", "odccm",
                        "connection-status", DEVICE_STATUS_UNKNOWN,
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

#if USE_GDBUS
  g_signal_connect (new_proxy,
                    "g-signal",
                    G_CALLBACK (on_dev_proxy_signal),
                    self);

  res = g_dbus_proxy_call_sync (new_proxy,
                                "GetPasswordFlags",
                                g_variant_new ("()"),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
  if (!res) {
    g_critical("%s: Error getting password flags from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  g_variant_get (res, "(u)", &password_flags);
  g_variant_unref (res);


#else /* USE_GDBUS */

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

#endif /* USE_GDBUS */

  g_object_get(device, "name", &name, NULL);
  proxy_store *p_store = g_malloc0(sizeof(proxy_store));
  p_store->pdaname = g_strdup(name);
  p_store->proxy = new_proxy;

  g_ptr_array_add(priv->dev_proxies, p_store);

  if (password_flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE) {
          g_object_set(device, "connection-status", DEVICE_STATUS_PASSWORD_REQUIRED, NULL);
          g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_CONNECTED], 0, name, (gpointer)device);
          g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REQUIRED], 0, name);
          g_free(name);
          goto exit;
  }
  if (password_flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE) {
          g_object_set(device, "connection-status", DEVICE_STATUS_PASSWORD_REQUIRED_ON_DEVICE, NULL);
          g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_CONNECTED], 0, name, (gpointer)device);
          g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REQUIRED_ON_DEVICE], 0, name);
          g_free(name);
          goto exit;
  }

  g_object_set(device, "connection-status", DEVICE_STATUS_CONNECTED, NULL);
  g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_CONNECTED], 0, name, (gpointer)device);
  g_free(name);

  goto exit;
error_exit:
  g_object_unref(new_proxy);
  if (device) g_object_unref(device);
exit:
  return;
}

#if USE_GDBUS

static void
on_devmgr_proxy_signal (GDBusProxy *proxy,
			gchar *sender_name,
			gchar *signal_name,
			GVariant *parameters,
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
  if (!priv->dbus_proxy) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return;
  }

  gchar *obj_path = NULL;

  if (strcmp(signal_name, "DeviceConnected") == 0) {
    g_variant_get (parameters, "(s)", &obj_path);
    odccm_add_device(self, obj_path);
    g_free(obj_path);
  }

  if (strcmp(signal_name, "DeviceDisconnected") == 0) {
    g_variant_get (parameters, "(s)", &obj_path);

    gchar *pdaname = NULL;
    proxy_store *p_store = NULL;
    gint i;

    i = 0;
    while (i < priv->dev_proxies->len) {
      if (!(g_ascii_strcasecmp(obj_path, g_dbus_proxy_get_object_path(((proxy_store *)g_ptr_array_index(priv->dev_proxies, i))->proxy)))) {
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

    g_free(obj_path);
  }

  return;
}

#else /* USE_GDBUS */
static void
odccm_device_connected_cb(DBusGProxy *proxy,
			  gchar *obj_path,
			  gpointer user_data)
{
  OdccmClient *self = ODCCM_CLIENT(user_data);
  odccm_add_device(self, obj_path);
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
  if (!priv->dbus_connection) {
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

#endif /* USE_GDBUS */

void
odccm_client_provide_password_impl(OdccmClient *self, const gchar *pdaname, const gchar *password)
{
  GError *error = NULL;
  gboolean password_accepted = FALSE;
  gint i;
#if USE_GDBUS
  GVariant *ret = NULL;
  GDBusProxy *proxy = NULL;
#else
  gboolean result;
  DBusGProxy *proxy = NULL;
#endif

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

#if USE_GDBUS
  if (!priv->dbus_proxy) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return;
  }
#else
  if (!priv->dbus_connection) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return;
  }
#endif

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

#if USE_GDBUS

  ret = g_dbus_proxy_call_sync (proxy,
                                "ProvidePassword",
                                g_variant_new ("(s)", password),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
  if (!ret) {
    g_critical("%s: Error sending password to odccm for %s: %s", G_STRFUNC, pdaname, error->message);
    g_error_free(error);
    goto exit;
  }

  g_variant_get (ret, "(b)", &password_accepted);
  g_variant_unref (ret);

#else /* USE_GDBUS */

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
    goto exit;
  }

#endif /* USE_GDBUS */

  if (!(password_accepted)) {
    g_debug("%s: Password rejected for %s", G_STRFUNC, pdaname);
    g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REJECTED], 0, pdaname);
    goto exit;
  }

  g_debug("%s: Password accepted for %s", G_STRFUNC, pdaname);

exit:
  return;
}

gboolean
odccm_client_request_disconnect_impl(OdccmClient *self, const gchar *pdaname)
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
#if USE_GDBUS
  if (!priv->dbus_proxy) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return result;
  }
#else
  if (!priv->dbus_connection) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return result;
  }
#endif
  g_debug("%s: Odccm doesn't support a disconnect command", G_STRFUNC);

  result = FALSE;
  return result;
}

static void
odccm_disconnect(OdccmClient *self)
{
        gint i;
        proxy_store *p_store;

        OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

        if (priv->disposed) {
                g_warning("%s: Disposed object passed", G_STRFUNC);
                return;
        }

#if USE_GDBUS
	g_signal_handlers_disconnect_by_func(priv->dev_mgr_proxy, on_devmgr_proxy_signal, self);

	g_object_unref(priv->dev_mgr_proxy);
	priv->dev_mgr_proxy = NULL;

#else /* USE_GDBUS */
        dbus_g_proxy_disconnect_signal (priv->dev_mgr_proxy, "DeviceConnected",
                                        G_CALLBACK(odccm_device_connected_cb), self);

        dbus_g_proxy_disconnect_signal (priv->dev_mgr_proxy, "DeviceDisconnected",
                                        G_CALLBACK(odccm_device_disconnected_cb), self);

        g_object_unref(priv->dev_mgr_proxy);
        priv->dev_mgr_proxy = NULL;
#endif /* USE_GDBUS */

        for (i = 0; i < priv->dev_proxies->len; i++) {
                p_store = (proxy_store *)g_ptr_array_index(priv->dev_proxies, i);
                free_proxy_store(p_store);
        }
}

static void
odccm_connect(OdccmClient *self)
{
#if USE_GDBUS
  GVariant *ret = NULL;
  gchar **dev_list = NULL;
#else
  GPtrArray* dev_list = NULL;
#endif
  gint i;
  gchar *obj_path = NULL;
  GError *error = NULL;

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

#if USE_GDBUS

  priv->dev_mgr_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                       G_DBUS_PROXY_FLAGS_NONE,
                                                       NULL, /* GDBusInterfaceInfo */
                                                       ODCCM_SERVICE,
                                                       ODCCM_MGR_PATH,
                                                       ODCCM_MGR_IFACE,
                                                       NULL, /* GCancellable */
                                                       &error);
  if (priv->dev_mgr_proxy == NULL) {
    g_critical("%s: Failed to create proxy to device manager: %s", G_STRFUNC, error->message);
    priv->online = FALSE;
    g_error_free(error);
    return;
  }

  g_signal_connect (priv->dev_mgr_proxy,
                    "g-signal",
                    G_CALLBACK (on_devmgr_proxy_signal),
                    self);

  /* currently connected devices */

  ret = g_dbus_proxy_call_sync (priv->dev_mgr_proxy,
                                "GetConnectedDevices",
                                g_variant_new ("()"),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
  if (!ret) {
    g_critical("%s: Error getting device list from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    return;
  }

  g_variant_get (ret, "(^ao)", &dev_list);
  g_variant_unref (ret);

  for (i = 0; i < g_strv_length(dev_list); i++) {
    obj_path = dev_list[i];
    g_debug("%s: adding device: %s", G_STRFUNC, obj_path);
    odccm_add_device(self, obj_path);
  }
  g_strfreev(dev_list);

#else /* USE_GDBUS */
  priv->dev_mgr_proxy = dbus_g_proxy_new_for_name (priv->dbus_connection,
                                     ODCCM_SERVICE,
                                     ODCCM_MGR_PATH,
                                     ODCCM_MGR_IFACE);
  if (priv->dev_mgr_proxy == NULL) {
      g_critical("%s: Failed to create proxy to device manager", G_STRFUNC);
      priv->online = FALSE;
      return;
  }

  dbus_g_proxy_add_signal (priv->dev_mgr_proxy, "DeviceConnected",
			   G_TYPE_STRING, G_TYPE_INVALID);

  dbus_g_proxy_add_signal (priv->dev_mgr_proxy, "DeviceDisconnected",
			   G_TYPE_STRING, G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (priv->dev_mgr_proxy, "DeviceConnected",
			       G_CALLBACK(odccm_device_connected_cb), self, NULL);

  dbus_g_proxy_connect_signal (priv->dev_mgr_proxy, "DeviceDisconnected",
			       G_CALLBACK(odccm_device_disconnected_cb), self, NULL);

  /* currently connected devices */

  if (!(dbus_g_proxy_call(priv->dev_mgr_proxy, "GetConnectedDevices",
			  &error, G_TYPE_INVALID,
			  dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_OBJECT_PATH),
			  &dev_list,
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device list from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    return;
  }
  for (i = 0; i < dev_list->len; i++) {
    obj_path = (gchar *)g_ptr_array_index(dev_list, i);
    g_debug("%s: adding device: %s", G_STRFUNC, obj_path);
    odccm_add_device(self, obj_path);
    g_free(obj_path);
  }
  g_ptr_array_free(dev_list, TRUE);

#endif /* USE_GDBUS */

}

#if USE_GDBUS

static void
on_dbus_proxy_signal (GDBusProxy *proxy,
		      gchar *sender_name,
		      gchar *signal_name,
		      GVariant *parameters,
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

	gchar *name;
	gchar *old_owner;
	gchar *new_owner;

        if (strcmp(signal_name, "NameOwnerChanged") != 0)
	  return;

	g_variant_get (parameters, "(sss)", &name, &old_owner, &new_owner);


        if (strcmp(name, ODCCM_SERVICE) != 0)
          return;

        /* If this parameter is empty, odccm just came online */

        if (strcmp(old_owner, "") == 0) {
                priv->online = TRUE;
                g_debug("%s: odccm came online", G_STRFUNC);
                odccm_connect(self);

                g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STARTING], 0);
                return;
        }

        /* If this parameter is empty, odccm just went offline */

        if (strcmp(new_owner, "") == 0) {
                priv->online = FALSE;
                g_debug("%s: odccm went offline", G_STRFUNC);
                odccm_disconnect(self);

                g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
                return;
        }
}

#else /* USE_GDBUS */
static void
odccm_status_changed_cb(DBusGProxy *proxy,
                        gchar *name,
                        gchar *old_owner,
                        gchar *new_owner,
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

        if (strcmp(name, ODCCM_SERVICE) != 0)
                return;

        /* If this parameter is empty, odccm just came online */

        if (strcmp(old_owner, "") == 0) {
                priv->online = TRUE;
                g_debug("%s: odccm came online", G_STRFUNC);
                odccm_connect(self);

                g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STARTING], 0);
                return;
        }

        /* If this parameter is empty, odccm just went offline */

        if (strcmp(new_owner, "") == 0) {
                priv->online = FALSE;
                g_debug("%s: odccm went offline", G_STRFUNC);
                odccm_disconnect(self);

                g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
                return;
        }
}

#endif /* USE_GDBUS */

gboolean
odccm_client_uninit_comms_impl(OdccmClient *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return FALSE;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return FALSE;
  }
  if (priv->dev_mgr_proxy)
          odccm_disconnect(self);

#if USE_GDBUS
  g_signal_handlers_disconnect_by_func(priv->dbus_proxy, on_dbus_proxy_signal, self);
#else
  dbus_g_proxy_disconnect_signal (priv->dbus_proxy, "NameOwnerChanged",
                                  G_CALLBACK(odccm_status_changed_cb), NULL);
#endif

  g_object_unref(priv->dbus_proxy);
  priv->dbus_proxy = NULL;

#if !USE_GDBUS
  dbus_g_connection_unref(priv->dbus_connection);
  priv->dbus_connection = NULL;
#endif

  return TRUE;
}

gboolean
odccm_client_init_comms_impl(OdccmClient *self)
{
  GError *error = NULL;
  gboolean result = FALSE;
  gboolean has_owner = FALSE;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return result;
  }
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return result;
  }

#if USE_GDBUS
  if (priv->dbus_proxy) {
    g_warning("%s: Initialised object passed", G_STRFUNC);
    return result;
  }

  priv->dbus_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                    G_DBUS_PROXY_FLAGS_NONE,
                                                    NULL, /* GDBusInterfaceInfo */
                                                    DBUS_SERVICE,
                                                    DBUS_PATH,
                                                    DBUS_IFACE,
                                                    NULL, /* GCancellable */
                                                    &error);
  if (priv->dbus_proxy == NULL) {
    g_warning("%s: Failed to get dbus proxy object: %s", G_STRFUNC, error->message);
    g_error_free(error);
    return result;
  }

  g_signal_connect (priv->dbus_proxy,
                    "g-signal",
                    G_CALLBACK (on_dbus_proxy_signal),
                    self);

  GVariant *ret = NULL;
  ret = g_dbus_proxy_call_sync (priv->dbus_proxy,
                                "NameHasOwner",
                                g_variant_new ("(s)", ODCCM_SERVICE),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
  if (!ret) {
    g_critical("%s: Error checking owner of %s: %s", G_STRFUNC, ODCCM_SERVICE, error->message);
    g_error_free(error);
    return TRUE;
  }
  g_variant_get (ret, "(b)", &has_owner);
  g_variant_unref (ret);

#else /* USE_GDBUS */
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

  priv->dbus_proxy = dbus_g_proxy_new_for_name (priv->dbus_connection,
                                                DBUS_SERVICE,
                                                DBUS_PATH,
                                                DBUS_IFACE);

  dbus_g_proxy_add_signal (priv->dbus_proxy, "NameOwnerChanged",
			   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (priv->dbus_proxy, "NameOwnerChanged",
			       G_CALLBACK(odccm_status_changed_cb), self, NULL);

  if (!(dbus_g_proxy_call(priv->dbus_proxy, "NameHasOwner",
			  &error,
                          G_TYPE_STRING, ODCCM_SERVICE,
                          G_TYPE_INVALID,
			  G_TYPE_BOOLEAN, &has_owner,
			  G_TYPE_INVALID))) {
          g_critical("%s: Error checking owner of %s: %s", G_STRFUNC, ODCCM_SERVICE, error->message);
          g_error_free(error);
          return TRUE;
  }
#endif /* USE_GDBUS */

  if (has_owner) {
          priv->online = TRUE;
          odccm_connect(self);
  } else
          priv->online = FALSE;

  return TRUE;
}


/* class & instance functions */

static void
odccm_client_init(OdccmClient *self)
{
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

#if !USE_GDBUS
  priv->dbus_connection = NULL;
#endif
  priv->dbus_proxy = NULL;
  priv->dev_mgr_proxy = NULL;
  priv->dev_proxies = g_ptr_array_new();
  priv->online = FALSE;
}

static void
odccm_client_dispose (GObject *obj)
{
  OdccmClient *self = ODCCM_CLIENT(obj);
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (obj);

  if (priv->disposed) {
    return;
  }
#if USE_GDBUS
  if (priv->dbus_proxy)
#else
  if (priv->dbus_connection)
#endif
    dccm_client_uninit_comms(DCCM_CLIENT(self));

  priv->disposed = TRUE;

  /* unref other objects */

  g_ptr_array_free(priv->dev_proxies, TRUE);

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

#if !USE_GDBUS
  dbus_g_object_register_marshaller (dccm_client_marshal_VOID__UINT_UINT,
				     G_TYPE_NONE,
				     G_TYPE_UINT,
				     G_TYPE_UINT,
				     G_TYPE_INVALID);
#endif
}

static void
dccm_client_interface_init (gpointer g_iface, gpointer iface_data)
{
  DccmClientInterface *iface = (DccmClientInterface *)g_iface;

  iface->dccm_client_init_comms = (gboolean (*) (DccmClient *self)) odccm_client_init_comms_impl;
  iface->dccm_client_uninit_comms = (gboolean (*) (DccmClient *self)) odccm_client_uninit_comms_impl;
  iface->dccm_client_provide_password = (void (*) (DccmClient *self, const gchar *pdaname, const gchar *password)) odccm_client_provide_password_impl;
  iface->dccm_client_request_disconnect = (gboolean (*) (DccmClient *self, const gchar *pdaname)) odccm_client_request_disconnect_impl;
}
