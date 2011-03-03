/*
Copyright (c) 2008 Mark Ellis <mark@mpellis.org.uk>

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
#include <dbus/dbus-glib-lowlevel.h>
#include <libhal.h>
#include <synce.h>
#include <rapi.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "hal-client.h"
#include "dccm-client-signals-marshal.h"
#include "dccm-client.h"
#include "device.h"
#include "utils.h"

static void dccm_client_interface_init (gpointer g_iface, gpointer iface_data);
G_DEFINE_TYPE_EXTENDED (HalClient, hal_client, G_TYPE_OBJECT, 0, G_IMPLEMENT_INTERFACE (DCCM_CLIENT_TYPE, dccm_client_interface_init))

typedef struct _HalClientPrivate HalClientPrivate;
struct _HalClientPrivate {
  DBusGConnection *dbus_connection;
  DBusGProxy *dbus_proxy;
  LibHalContext *hal_ctx;

  gboolean online;
  GHashTable *udi_name_table;

  gboolean disposed;
};

#define HAL_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAL_CLIENT_TYPE, HalClientPrivate))

#define DBUS_SERVICE "org.freedesktop.DBus"
#define DBUS_IFACE   "org.freedesktop.DBus"
#define DBUS_PATH    "/org/freedesktop/DBus"

#define HAL_SERVICE   "org.freedesktop.Hal"
#define HAL_MGR_PATH  "/org/freedesktop/Hal/Manager"
#define HAL_MGR_IFACE "org.freedesktop.Hal.Manager"

/* methods */

static void 
password_status_changed_cb(LibHalContext *ctx,
			  const char *udi,
			  const char *key,
			  dbus_bool_t is_removed,
			  dbus_bool_t is_added)
{
  void *user_data = libhal_ctx_get_user_data(ctx);

  if (!user_data) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  HalClient *self = HAL_CLIENT(user_data);
  HalClientPrivate *priv = HAL_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  if (strcmp(key, "pda.pocketpc.password") != 0) 
    return;

  gchar *pdaname = NULL;
  DBusError dbus_error;
  gchar *pw_status;

  dbus_error_init(&dbus_error);

  pdaname = (gchar*) g_hash_table_lookup(priv->udi_name_table, udi);
  if (!pdaname) {
    g_warning("%s: Received password flags changed for unfound device: %s", G_STRFUNC, udi);
    goto error_exit;;
  }

  pw_status = libhal_device_get_property_string(priv->hal_ctx, udi, "pda.pocketpc.password", &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    g_critical("%s: error getting property from device %s: %s: %s", G_STRFUNC, udi, dbus_error.name, dbus_error.message);
    goto error_exit;
  }

  if (strcmp(pw_status, "provide") == 0) {
    g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REQUIRED], 0, pdaname);
    goto exit;
  }

  if (strcmp(pw_status, "provide-on-device") == 0) {
    g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REQUIRED_ON_DEVICE], 0, pdaname);
    goto exit;
  }

  if ( (strcmp(pw_status, "unset") == 0) || (strcmp(pw_status, "unlocked") == 0) ) {

          if (!libhal_device_remove_property_watch(priv->hal_ctx, udi, &dbus_error)) {
                  g_warning("%s: Failed to remove property watch for device %s: %s: %s", G_STRFUNC, udi, dbus_error.name, dbus_error.message);
                  if (dbus_error_is_set(&dbus_error))
                          dbus_error_free(&dbus_error);
          }

          g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_UNLOCKED], 0, pdaname);
  }
  goto exit;

error_exit:
  if (dbus_error_is_set(&dbus_error))
    dbus_error_free(&dbus_error);

exit:
  return;
}


static void
hal_add_device(HalClient *self, const char *udi)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  HalClientPrivate *priv = HAL_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  WmDevice *device = NULL;
  gint os_major, os_minor,
    processor_type, partner_id_1;
  gchar *ip = NULL;
  gchar *name = NULL;
  gchar *os_name = NULL;
  gchar *model = NULL;
  const gchar *transport;
  gchar *pw_status = NULL;
  DBusError dbus_error;
  dbus_bool_t dbus_ret;

  dbus_error_init(&dbus_error);

  dbus_ret = libhal_device_property_exists(priv->hal_ctx,
					   udi,
					   "pda.pocketpc.name",
					   &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    g_critical("%s: error checking property pda.pocketpc.name from device %s: %s: %s", G_STRFUNC, udi, dbus_error.name, dbus_error.message);
    goto error_exit;
  }

  /* is it a pocketpc ? */
  if (!dbus_ret)
    return;

  g_debug("%s: Received device connected from hal: %s", G_STRFUNC, udi);

  name = libhal_device_get_property_string(priv->hal_ctx, udi, "pda.pocketpc.name", &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    g_critical("%s: Failed to obtain property pda.pocketpc.name for device %s: %s: %s", G_STRFUNC, udi, dbus_error.name, dbus_error.message);
    goto error_exit;
  }

  ip = libhal_device_get_property_string(priv->hal_ctx, udi, "pda.pocketpc.ip_address", &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    g_critical("%s: Failed to obtain property pda.pocketpc.ip_address for device %s: %s: %s", G_STRFUNC, udi, dbus_error.name, dbus_error.message);
    goto error_exit;
  }

  processor_type = libhal_device_get_property_uint64(priv->hal_ctx, udi, "pda.pocketpc.cpu_type", &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    g_critical("%s: Failed to obtain property pda.pocketpc.cpu_type for device %s: %s: %s", G_STRFUNC, udi, dbus_error.name, dbus_error.message);
    goto error_exit;
  }

  os_major = libhal_device_get_property_uint64(priv->hal_ctx, udi, "pda.pocketpc.os_major", &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    g_critical("%s: Failed to obtain property pda.pocketpc.os_major for device %s: %s: %s", G_STRFUNC, udi, dbus_error.name, dbus_error.message);
    goto error_exit;
  }

  os_minor = libhal_device_get_property_uint64(priv->hal_ctx, udi, "pda.pocketpc.os_minor", &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    g_critical("%s: Failed to obtain property pda.pocketpc.os_minor for device %s: %s: %s", G_STRFUNC, udi, dbus_error.name, dbus_error.message);
    goto error_exit;
  }

  partner_id_1 = libhal_device_get_property_uint64(priv->hal_ctx, udi, "pda.pocketpc.current_partner_id", &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    g_critical("%s: Failed to obtain property pda.pocketpc.current_partner_id for device %s: %s: %s", G_STRFUNC, udi, dbus_error.name, dbus_error.message);
    goto error_exit;
  }

  os_name = libhal_device_get_property_string(priv->hal_ctx, udi, "pda.pocketpc.platform", &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    g_critical("%s: Failed to obtain property pda.pocketpc.platform for device %s: %s: %s", G_STRFUNC, udi, dbus_error.name, dbus_error.message);
    goto error_exit;
  }

  model = libhal_device_get_property_string(priv->hal_ctx, udi, "pda.pocketpc.model", &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    g_critical("%s: Failed to obtain property pda.pocketpc.model for device %s: %s: %s", G_STRFUNC, udi, dbus_error.name, dbus_error.message);
    goto error_exit;
  }

  transport = "hal";

  device = g_object_new(WM_DEVICE_TYPE,
			"object-name", udi,
                        "dccm-type", "hal",
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

  if (!device) {
    g_critical("%s: Error creating new device", G_STRFUNC);
    goto error_exit;
  }

  pw_status = libhal_device_get_property_string(priv->hal_ctx, udi, "pda.pocketpc.password", &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    g_critical("%s: Failed to obtain password status for device %s: %s: %s", G_STRFUNC, udi, dbus_error.name, dbus_error.message);
    goto error_exit;
  }

  if ((pw_status) && (strcmp(pw_status, "unset") != 0) && (strcmp(pw_status, "unlocked") != 0)) {
    if (!libhal_device_add_property_watch(priv->hal_ctx, udi, &dbus_error)) {
      g_critical("%s: Failed to set property watch for device %s: %s: %s", G_STRFUNC, udi, dbus_error.name, dbus_error.message);
      goto error_exit;
    }

    g_hash_table_insert(priv->udi_name_table, g_strdup(udi), g_strdup(name));

    if ((strcmp(pw_status, "provide") == 0)) {
            g_object_set(device, "connection-status", DEVICE_STATUS_PASSWORD_REQUIRED, NULL);
            g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_CONNECTED], 0, name, (gpointer)device);
            g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REQUIRED], 0, name);
    }
    if ((strcmp(pw_status, "provide-on-device") == 0)) {
            g_object_set(device, "connection-status", DEVICE_STATUS_PASSWORD_REQUIRED_ON_DEVICE, NULL);
            g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_CONNECTED], 0, name, (gpointer)device);
            g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REQUIRED_ON_DEVICE], 0, name);
    }
    goto exit;
  }

  g_object_set(device, "connection-status", DEVICE_STATUS_CONNECTED, NULL);
  g_hash_table_insert(priv->udi_name_table, g_strdup(udi), g_strdup(name));
  g_signal_emit(self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_CONNECTED], 0, name, (gpointer)device);

  goto exit;
error_exit:
  if (dbus_error_is_set(&dbus_error))
    dbus_error_free(&dbus_error);
  if (device)
    g_object_unref(device);

exit:
  g_free(name);
  g_free(ip);
  g_free(os_name);
  g_free(model);
  g_free(pw_status);

  return;
}

static void
hal_device_connected_cb(LibHalContext *ctx, const char *udi)
{
  HalClient *self = HAL_CLIENT(libhal_ctx_get_user_data(ctx));
  hal_add_device(self, udi);
}


void
hal_device_disconnected_cb(LibHalContext *ctx, const char *udi)
{
  void *user_data = libhal_ctx_get_user_data(ctx);
  HalClient *self = HAL_CLIENT(user_data);

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  HalClientPrivate *priv = HAL_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  gchar *pdaname = NULL;
  DBusError dbus_error;

  pdaname = (gchar*) g_hash_table_lookup(priv->udi_name_table, udi);

  if (!pdaname)
    return;

  g_debug("%s: Received device disconnected from hal: %s", G_STRFUNC, udi);

  dbus_error_init(&dbus_error);

  if (!(libhal_device_remove_property_watch(priv->hal_ctx, udi, &dbus_error))) {
          g_warning("%s: Error removing watch on device: %s: %s", G_STRFUNC, dbus_error.name, dbus_error.message);
          if (dbus_error_is_set(&dbus_error))
                  dbus_error_free(&dbus_error);
  }

  g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_DISCONNECTED], 0, pdaname);

  g_hash_table_remove(priv->udi_name_table, udi);

  return;
}

void
hal_client_provide_password_impl(HalClient *self, const gchar *pdaname, const gchar *password)
{
  GError *error = NULL;
  gboolean password_accepted = FALSE, result;
  gint num_devices;
  DBusGProxy *proxy = NULL;
  DBusError dbus_error;
  gchar **device_list;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  HalClientPrivate *priv = HAL_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }
  if (!priv->dbus_connection) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return;
  }

  dbus_error_init(&dbus_error);

  device_list = libhal_manager_find_device_string_match(priv->hal_ctx,
                                                        "pda.pocketpc.name",
                                                        pdaname,
                                                        &num_devices,
                                                        &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    g_critical("%s: Failed to obtain list of attached devices: %s: %s", G_STRFUNC, dbus_error.name, dbus_error.message);
    goto error_exit;
  }

  if (num_devices < 1) {
    g_warning("%s: Password provided for unfound device: %s", G_STRFUNC, pdaname);
    return;
  }

  proxy = dbus_g_proxy_new_for_name(priv->dbus_connection,
				    "org.freedesktop.Hal",
				    device_list[0],
				    "org.freedesktop.Hal.Device.Synce");
  if (proxy == NULL) {
    g_critical("%s: Failed to get proxy for device '%s'", G_STRFUNC, device_list[0]);
    goto error_exit;
  }

  result = dbus_g_proxy_call(proxy,
			     "ProvidePassword",
			     &error,
			     G_TYPE_STRING, password,
			     G_TYPE_INVALID,
			     G_TYPE_BOOLEAN, &password_accepted,
			     G_TYPE_INVALID);

  if (result == FALSE) {
    g_critical("%s: Error sending password to Hal for %s: %s", G_STRFUNC, pdaname, error->message);
    goto error_exit;
  }

  if (!(password_accepted)) {
    g_debug("%s: Password rejected for %s", G_STRFUNC, pdaname);
    g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REJECTED], 0, pdaname);
    goto exit;
  }

  g_debug("%s: Password accepted for %s", G_STRFUNC, pdaname);
  goto exit;
error_exit:
  if (dbus_error_is_set(&dbus_error))
    dbus_error_free(&dbus_error);
  if (error)
    g_error_free(error);

exit:
  if (device_list)
    libhal_free_string_array(device_list);
  if (proxy)
    g_object_unref(proxy);
  return;
}

gboolean
hal_client_request_disconnect_impl(HalClient *self, const gchar *pdaname)
{
  gboolean result = FALSE;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return result;
  }

  HalClientPrivate *priv = HAL_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return result;
  }

  g_debug("%s: Hal backend doesn't support a disconnect command", G_STRFUNC);

  result = FALSE;
  return result;
}


static void
hal_disconnect(HalClient *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  HalClientPrivate *priv = HAL_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  DBusError dbus_error;
  dbus_error_init(&dbus_error);

  if (!libhal_ctx_shutdown(priv->hal_ctx, &dbus_error)) {
    g_critical("%s: Failed to shutdown hal context: %s: %s", G_STRFUNC, dbus_error.name, dbus_error.message);
    dbus_error_free(&dbus_error);
  }

  libhal_ctx_free(priv->hal_ctx);
  priv->hal_ctx = NULL;

  g_hash_table_remove_all(priv->udi_name_table);
}

static void
hal_connect(HalClient *self)
{
  DBusError dbus_error;
  gchar **dev_list = NULL;
  gint i, num_devices;
  gchar *udi = NULL;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  HalClientPrivate *priv = HAL_CLIENT_GET_PRIVATE (self);

  dbus_error_init(&dbus_error);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  if (!(priv->hal_ctx = libhal_ctx_new())) {
    g_critical("%s: Failed to get hal context", G_STRFUNC);
    goto error_exit;
  }

  if (!libhal_ctx_set_dbus_connection(priv->hal_ctx, dbus_g_connection_get_connection(priv->dbus_connection))) {
    g_critical("%s: Failed to set DBus connection for hal context", G_STRFUNC);
    goto error_exit;
  }

  if (!libhal_ctx_set_user_data(priv->hal_ctx, self)) {
    g_critical("%s: Failed to set user data for hal context", G_STRFUNC);
    goto error_exit;
  }

  if (!libhal_ctx_init(priv->hal_ctx, &dbus_error)) {
    g_critical("%s: Failed to initialise hal context: %s: %s", G_STRFUNC, dbus_error.name, dbus_error.message);
    goto error_exit;
  }

  if (!libhal_ctx_set_device_added(priv->hal_ctx, hal_device_connected_cb)) {
    g_critical("%s: Failed to set hal device added callback", G_STRFUNC);
    goto error_exit;
  }

  if (!libhal_ctx_set_device_removed(priv->hal_ctx, hal_device_disconnected_cb)) {
    g_critical("%s: Failed to set hal device removed callback", G_STRFUNC);
    goto error_exit;
  }

  if (!libhal_ctx_set_device_property_modified(priv->hal_ctx, password_status_changed_cb)) {
    g_critical("%s: Failed to set hal device property modified callback", G_STRFUNC);
    goto error_exit;
  }

  /* currently connected devices */

  dev_list = libhal_manager_find_device_string_match(priv->hal_ctx,
						     "pda.platform",
						     "pocketpc",
						     &num_devices,
						     &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    g_warning("%s: Failed to obtain list of attached devices: %s: %s", G_STRFUNC, dbus_error.name, dbus_error.message);
    dbus_error_free(&dbus_error);
  }

  for (i = 0; i < num_devices; i++) {
    udi = dev_list[i];
    g_debug("%s: adding device: %s", G_STRFUNC, udi);
    hal_add_device(self, udi);
  }
  libhal_free_string_array(dev_list);

  return;

error_exit:
  priv->online = FALSE;
  if (dbus_error_is_set(&dbus_error))
    dbus_error_free(&dbus_error);
  if (priv->hal_ctx) {
    libhal_ctx_shutdown(priv->hal_ctx, NULL);
    libhal_ctx_free(priv->hal_ctx);
  }
  return;
}

static void
hal_status_changed_cb(DBusGProxy *proxy,
                      gchar *name,
                      gchar *old_owner,
                      gchar *new_owner,
                      gpointer user_data)
{
        HalClient *self = HAL_CLIENT(user_data);
        if (!self) {
                g_warning("%s: Invalid object passed", G_STRFUNC);
                return;
        }
        HalClientPrivate *priv = HAL_CLIENT_GET_PRIVATE (self);

        if (priv->disposed) {
                g_warning("%s: Disposed object passed", G_STRFUNC);
                return;
        }

        if (strcmp(name, HAL_SERVICE) != 0)
                return;

        /* If this parameter is empty, hal just came online */

        if (strcmp(old_owner, "") == 0) {
                priv->online = TRUE;
                g_debug("%s: hal came online", G_STRFUNC);
                hal_connect(self);

                g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STARTING], 0);
                return;
        }

        /* If this parameter is empty, hal just went offline */

        if (strcmp(new_owner, "") == 0) {
                priv->online = FALSE;
                g_debug("%s: hal went offline", G_STRFUNC);
                hal_disconnect(self);

                g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
                return;
        }
}

gboolean
hal_client_uninit_comms_impl(HalClient *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return FALSE;
  }

  HalClientPrivate *priv = HAL_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return FALSE;
  }
  if (!priv->dbus_connection) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return TRUE;
  }

  if (priv->hal_ctx)
          hal_disconnect(self);

  dbus_g_proxy_disconnect_signal (priv->dbus_proxy, "NameOwnerChanged",
                                  G_CALLBACK(hal_status_changed_cb), NULL);

  g_object_unref(priv->dbus_proxy);
  priv->dbus_proxy = NULL;

  dbus_g_connection_unref(priv->dbus_connection);
  priv->dbus_connection = NULL;

  return TRUE;
}

gboolean
hal_client_init_comms_impl(HalClient *self)
{
  GError *error = NULL;
  gboolean result = FALSE;
  gboolean has_owner = FALSE;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return FALSE;
  }
  HalClientPrivate *priv = HAL_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return FALSE;
  }
  if (priv->dbus_connection) {
    g_warning("%s: Initialised object passed", G_STRFUNC);
    return FALSE;
  }

  priv->dbus_connection = dbus_g_bus_get (DBUS_BUS_SYSTEM,
					  &error);
  if (priv->dbus_connection == NULL) {
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
			       G_CALLBACK(hal_status_changed_cb), self, NULL);

  if (!(dbus_g_proxy_call(priv->dbus_proxy, "NameHasOwner",
			  &error,
                          G_TYPE_STRING, HAL_SERVICE,
                          G_TYPE_INVALID,
			  G_TYPE_BOOLEAN, &has_owner,
			  G_TYPE_INVALID))) {
          g_critical("%s: Error checking owner of %s: %s", G_STRFUNC, HAL_SERVICE, error->message);
          g_error_free(error);
          return TRUE;
  }

  if (has_owner) {
          priv->online = TRUE;
          hal_connect(self);
  } else
          priv->online = FALSE;

  return TRUE;
}


/* class & instance functions */

static void
hal_client_init(HalClient *self)
{
  HalClientPrivate *priv = HAL_CLIENT_GET_PRIVATE (self);

  priv->dbus_connection = NULL;
  priv->dbus_proxy = NULL;
  priv->hal_ctx = NULL;

  priv->online = FALSE;
  priv->udi_name_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

static void
hal_client_dispose (GObject *obj)
{
  HalClient *self = HAL_CLIENT(obj);
  HalClientPrivate *priv = HAL_CLIENT_GET_PRIVATE (obj);

  if (priv->disposed) {
    return;
  }
  if (priv->dbus_connection)
    dccm_client_uninit_comms(DCCM_CLIENT(self));

  g_hash_table_destroy(priv->udi_name_table);

  priv->disposed = TRUE;

  /* unref other objects */

  if (G_OBJECT_CLASS (hal_client_parent_class)->dispose)
    G_OBJECT_CLASS (hal_client_parent_class)->dispose (obj);
}

static void
hal_client_finalize (GObject *obj)
{
  if (G_OBJECT_CLASS (hal_client_parent_class)->finalize)
    G_OBJECT_CLASS (hal_client_parent_class)->finalize (obj);
}

static void
hal_client_class_init (HalClientClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = hal_client_dispose;
  gobject_class->finalize = hal_client_finalize;

  g_type_class_add_private (klass, sizeof (HalClientPrivate));

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

  iface->dccm_client_init_comms = (gboolean (*) (DccmClient *self)) hal_client_init_comms_impl;
  iface->dccm_client_uninit_comms = (gboolean (*) (DccmClient *self)) hal_client_uninit_comms_impl;
  iface->dccm_client_provide_password = (void (*) (DccmClient *self, const gchar *pdaname, const gchar *password)) hal_client_provide_password_impl;
  iface->dccm_client_request_disconnect = (gboolean (*) (DccmClient *self, const gchar *pdaname)) hal_client_request_disconnect_impl;
}
