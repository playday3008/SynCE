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

#include "odccm-client.h"
#include "dccm-client-signals-marshal.h"
#include "dccm-client.h"

static void dccm_client_interface_init (gpointer g_iface, gpointer iface_data);
G_DEFINE_TYPE_EXTENDED (OdccmClient, odccm_client, G_TYPE_OBJECT, 0, G_IMPLEMENT_INTERFACE (DCCM_CLIENT_TYPE, dccm_client_interface_init))

typedef struct _OdccmClientPrivate OdccmClientPrivate;
struct _OdccmClientPrivate {
  DBusGConnection *dbus_connection;
  DBusGProxy *dev_mgr_proxy;

  GPtrArray *dev_proxies;

  gboolean disposed;
};

#define ODCCM_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ODCCM_CLIENT_TYPE, OdccmClientPrivate))


#define ODCCM_SERVICE   "org.synce.odccm"
#define ODCCM_MGR_PATH  "/org/synce/odccm/DeviceManager"
#define ODCCM_MGR_IFACE "org.synce.odccm.DeviceManager"
#define ODCCM_DEV_IFACE "org.synce.odccm.Device"

typedef struct _dev_proxy dev_proxy;
struct _dev_proxy{
  gchar *pdaname;
  DBusGProxy *proxy;
};


/* methods */

void password_flags_changed_cb(DBusGProxy *proxy,
			       guint added, guint removed,
			       gpointer user_data)
{
  OdccmClient *self = ODCCM_CLIENT(user_data);
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);
  gchar *pdaname = NULL;
  gint i;
  const gchar *obj_path;

  obj_path = dbus_g_proxy_get_path(proxy);

  g_debug("Received password flags changed from odccm: %s: %s", obj_path, G_STRFUNC);

  i = 0;
  while (i < priv->dev_proxies->len) {
    if (!(g_ascii_strcasecmp(obj_path, dbus_g_proxy_get_path(((dev_proxy *)g_ptr_array_index(priv->dev_proxies, i))->proxy)))) {
      pdaname = (((dev_proxy *)g_ptr_array_index(priv->dev_proxies, i))->pdaname);
      break;
    }
    i++;
  }

  if (!pdaname) {
    g_warning("Received password flags changed from odccm for unfound device: %s: %s", obj_path, G_STRFUNC);
    return;
  }

  if (added == 2) /* provide */
    g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REQUIRED], 0, pdaname); 
}

void
odccm_device_connected_cb(DBusGProxy *proxy,
			 gchar *obj_path,
			 gpointer user_data)
{
  OdccmClient *self = ODCCM_CLIENT(user_data);
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);
  SynceInfo *synce_info = NULL;
  GError *error;
  DBusGProxy *new_proxy;

  new_proxy = dbus_g_proxy_new_for_name (priv->dbus_connection,
                                     ODCCM_SERVICE,
                                     obj_path,
                                     ODCCM_DEV_IFACE);

  g_debug("Received connect from odccm: %s: %s", obj_path, G_STRFUNC);

  synce_info = (SynceInfo *)g_malloc0(sizeof(SynceInfo));

  if (!(dbus_g_proxy_call(new_proxy,
			  "GetName",
			  &error,
			  G_TYPE_INVALID,
			  G_TYPE_STRING,
			  synce_info->name,
			  G_TYPE_INVALID))) {
    g_critical("Error getting device name from odccm: %s: %s", error->message, G_STRFUNC);
    g_error_free(error);
    goto dbus_error;
  }
  if (!(dbus_g_proxy_call(new_proxy,
			  "GetIpAddress",
			  &error,
			  G_TYPE_INVALID,
			  G_TYPE_STRING,
			  synce_info->ip,
			  G_TYPE_INVALID))) {
    g_critical("Error getting device ip from odccm: %s: %s", error->message, G_STRFUNC);
    g_error_free(error);
    goto dbus_error;
  }
  if (!(dbus_g_proxy_call(new_proxy,
			  "GetCpuType",
			  &error,
			  G_TYPE_INVALID,
			  G_TYPE_UINT,
			  synce_info->processor_type,
			  G_TYPE_INVALID))) {
    g_critical("Error getting device processor type from odccm: %s: %s", error->message, G_STRFUNC);
    g_error_free(error);
    goto dbus_error;
  }
  if (!(dbus_g_proxy_call(new_proxy,
			  "GetPlatformName",
			  &error,
			  G_TYPE_INVALID,
			  G_TYPE_UINT,
			  synce_info->os_name,
			  G_TYPE_INVALID))) {
    g_critical("Error getting device platform name from odccm: %s: %s", error->message, G_STRFUNC);
    g_error_free(error);
    goto dbus_error;
  }
  if (!(dbus_g_proxy_call(new_proxy,
			  "GetModelName",
			  &error,
			  G_TYPE_INVALID,
			  G_TYPE_UINT,
			  synce_info->model,
			  G_TYPE_INVALID))) {
    g_critical("Error getting device model name from odccm: %s: %s", error->message, G_STRFUNC);
    g_error_free(error);
    goto dbus_error;
  }

  dev_proxy *new_dev_proxy = g_malloc0(sizeof(dev_proxy));
  new_dev_proxy->pdaname = g_strdup(synce_info->name);
  new_dev_proxy->proxy = new_proxy;

  g_ptr_array_add(priv->dev_proxies, new_dev_proxy);

  dbus_g_proxy_add_signal (new_proxy, "PasswordFlagsChanged",
			   G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (new_proxy, "PasswordFlagsChanged",
			       G_CALLBACK(password_flags_changed_cb), self, NULL);

  /*
  pid_t dccm_pid;
  char* password;
  int key;
  int os_version;
  int build_number;
  int partner_id_1;
  int partner_id_2;
  char* transport;
  int fd;
  */

  g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_CONNECTED], 0, synce_info->name, (gpointer)synce_info);
  goto exit;
dbus_error:
  g_object_unref(new_proxy);
  synce_info_destroy(synce_info);

exit:
  return;
}

void
free_dev_proxy(OdccmClient *self, gint index)
{
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  g_free(((dev_proxy *)g_ptr_array_index(priv->dev_proxies, index))->pdaname);
  g_object_unref(((dev_proxy *)g_ptr_array_index(priv->dev_proxies, index))->proxy);
  g_free(g_ptr_array_index(priv->dev_proxies, index));
}

void
odccm_device_disconnected_cb(DBusGProxy *proxy,
		      gchar *obj_path,
		      gpointer user_data)
{
  OdccmClient *self = ODCCM_CLIENT(user_data);
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);
  gchar *pdaname = NULL;
  gint i;

  i = 0;
  while (i < priv->dev_proxies->len) {
    if (!(g_ascii_strcasecmp(obj_path, dbus_g_proxy_get_path(((dev_proxy *)g_ptr_array_index(priv->dev_proxies, i))->proxy)))) {
      pdaname = (((dev_proxy *)g_ptr_array_index(priv->dev_proxies, i))->pdaname);
      break;
    }
    i++;
  }

  if (!pdaname) {
    g_warning("Received disconnect from odccm from unfound device: %s: %s", obj_path, G_STRFUNC);
    return;
  }

  g_debug("Received disconnect from odccm dbus: %s: %s", pdaname, G_STRFUNC);
  g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_DISCONNECTED], 0, pdaname);

  g_ptr_array_remove_index_fast(priv->dev_proxies, i);

  return;
}

gboolean
odccm_client_uninit_comms_impl(OdccmClient *self)
{
  gint i;

  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return FALSE;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return FALSE;
  }
  if (!priv->dev_mgr_proxy) {
    g_warning("Uninitialised object passed: %s", G_STRFUNC);
    return TRUE;
  }

  dbus_g_proxy_disconnect_signal (priv->dev_mgr_proxy, "DeviceConnected",
			       G_CALLBACK(odccm_device_connected_cb), NULL);

  dbus_g_proxy_disconnect_signal (priv->dev_mgr_proxy, "DeviceDisconnected",
			       G_CALLBACK(odccm_device_disconnected_cb), NULL);

  g_object_unref (priv->dev_mgr_proxy);

  for (i = 0; i < priv->dev_proxies->len; i++)
    free_dev_proxy(self, i);

  g_ptr_array_free(priv->dev_proxies, TRUE);

  return TRUE;
}

gboolean
odccm_client_provide_password_impl(OdccmClient *self, gchar *pdaname, gchar *password)
{
  GError *error = NULL;
  gboolean password_accepted = FALSE;
  gboolean result = FALSE;
  gint i;
  DBusGProxy *proxy = NULL;

  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return result;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return result;
  }
  if (!priv->dev_mgr_proxy) {
    g_warning("Uninitialised object passed: %s", G_STRFUNC);
    return result;
  }


  i = 0;
  while (i < priv->dev_proxies->len) {
    if (!(g_ascii_strcasecmp(pdaname, ((dev_proxy *)g_ptr_array_index(priv->dev_proxies, i))->pdaname))) {
      proxy = (((dev_proxy *)g_ptr_array_index(priv->dev_proxies, i))->proxy);
      break;
    }
    i++;
  }

  if (!proxy) {
    g_warning("Password provided for unfound device: %s: %s", pdaname, G_STRFUNC);
    return FALSE;
  }

  result = dbus_g_proxy_call(proxy,
			     "ProvidePassword",
			     &error,
			     G_TYPE_STRING,
			     password,
			     G_TYPE_INVALID,
			     G_TYPE_BOOLEAN,
			     password_accepted,			     
			     G_TYPE_INVALID);

  if (result == FALSE) {
    g_critical("Error sending password to odccm for %s: %s: %s", pdaname, error->message, G_STRFUNC);
    g_error_free(error);
    goto exit;
  }

  if (!password_accepted) {
    g_debug("Password rejected for %s: %s", pdaname, G_STRFUNC);
    g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REJECTED], 0, pdaname);
  }

  result = TRUE;
exit:
  return result;
}

gboolean
odccm_client_request_disconnect_impl(OdccmClient *self, gchar *pdaname)
{
  gboolean result = FALSE;

  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return result;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return result;
  }
  if (!priv->dev_mgr_proxy) {
    g_warning("Uninitialised object passed: %s", G_STRFUNC);
    return result;
  }

  g_debug("Odccm doesn't support a disconnect command: %s", G_STRFUNC);

  result = FALSE;
  return result;
}


gboolean
odccm_client_init_comms_impl(OdccmClient *self)
{
  GError *error = NULL;
  gboolean result = FALSE;

  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return result;
  }
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return result;
  }
  if (priv->dbus_connection) {
    g_warning("Initialised object passed: %s", G_STRFUNC);
    return result;
  }

  priv->dbus_connection = dbus_g_bus_get (DBUS_BUS_SESSION,
                               &error);
  if (priv->dbus_connection == NULL)
    {
      g_critical("Failed to open connection to bus: %s: %s", error->message, G_STRFUNC);
      g_error_free (error);
      return result;
    }

  priv->dev_mgr_proxy = dbus_g_proxy_new_for_name (priv->dbus_connection,
                                     ODCCM_SERVICE,
                                     ODCCM_MGR_PATH,
                                     ODCCM_MGR_IFACE);
  dbus_g_proxy_add_signal (priv->dev_mgr_proxy, "DeviceConnected",
			   G_TYPE_STRING, G_TYPE_INVALID);

  dbus_g_proxy_add_signal (priv->dev_mgr_proxy, "DeviceDisconnected",
			   G_TYPE_STRING, G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (priv->dev_mgr_proxy, "DeviceConnected",
			       G_CALLBACK(odccm_device_connected_cb), self, NULL);

  dbus_g_proxy_connect_signal (priv->dev_mgr_proxy, "DeviceDisconnected",
			       G_CALLBACK(odccm_device_disconnected_cb), self, NULL);

  result = TRUE;

  return result;
}


/* class & instance functions */

static void
odccm_client_init(OdccmClient *self)
{
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  priv->dev_mgr_proxy = NULL;
  priv->dev_proxies = g_ptr_array_new();
}

static void
odccm_client_dispose (GObject *obj)
{
  OdccmClient *self = ODCCM_CLIENT(obj);
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (obj);

  if (priv->disposed) {
    return;
  }
  priv->disposed = TRUE;

  if (priv->dev_mgr_proxy)
    dccm_client_uninit_comms(DCCM_CLIENT(self));

  /* unref other objects */

  if (G_OBJECT_CLASS (odccm_client_parent_class)->dispose)
    G_OBJECT_CLASS (odccm_client_parent_class)->dispose (obj);
}

static void
odccm_client_finalize (GObject *obj)
{
  OdccmClient *self = ODCCM_CLIENT(obj);
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);


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
}

static void
dccm_client_interface_init (gpointer g_iface, gpointer iface_data)
{
  DccmClientInterface *iface = (DccmClientInterface *)g_iface;

  iface->dccm_client_init_comms = (gboolean (*) (DccmClient *self)) odccm_client_init_comms_impl;
  iface->dccm_client_uninit_comms = (gboolean (*) (DccmClient *self)) odccm_client_uninit_comms_impl;
  iface->dccm_client_provide_password = (gboolean (*) (DccmClient *self, gchar *pdaname, gchar *password)) odccm_client_provide_password_impl;
  iface->dccm_client_request_disconnect = (gboolean (*) (DccmClient *self, gchar *pdaname)) odccm_client_request_disconnect_impl;
}
