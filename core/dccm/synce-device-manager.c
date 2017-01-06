#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <glib-object.h>
#include <gio/gio.h>

#if HAVE_GUDEV
#include <gudev/gudev.h>
#endif

#include "synce-device-manager.h"
#include "synce-device-manager-dbus.h"
#include "synce-device-manager-control.h"
#include "synce-device.h"
#include "synce-device-rndis.h"
#include "synce-device-legacy.h"
#include "utils.h"

static void     synce_device_manager_initable_iface_init (GInitableIface  *iface);
static gboolean synce_device_manager_initable_init       (GInitable       *initable,
							  GCancellable    *cancellable,
							  GError          **error);
static void     synce_device_manager_async_initable_iface_init (GAsyncInitableIface  *iface);
static void     synce_device_manager_init_async (GAsyncInitable *initable, int io_priority, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
static gboolean synce_device_manager_init_finish (GAsyncInitable      *initable,
						  GAsyncResult        *res,
						  GError             **error);

const gchar *udev_subsystems[] = { NULL };

G_DEFINE_TYPE_WITH_CODE (SynceDeviceManager, synce_device_manager, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, synce_device_manager_initable_iface_init)
			 G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, synce_device_manager_async_initable_iface_init))

/* private stuff */

typedef struct
{
  gchar *device_path;
  gchar *device_ip;
  gchar *local_ip;
  gboolean rndis;
  gboolean iface_pending;
  GSocketService *server;
  SynceDevice *device;
} DeviceEntry;

enum {
  NOT_INITIALISED,
  INITIALISING,
  INITIALISED
};

typedef struct _SynceDeviceManagerPrivate SynceDeviceManagerPrivate;

struct _SynceDeviceManagerPrivate
{
  guint init_state;
  gboolean init_success;
  GList *init_results;
  GError *init_error;
  gboolean inited;
  gboolean dispose_has_run;

  GSList *devices;
  SynceDeviceManagerControl *control_iface;
  gulong control_connect_id;
  gulong control_disconnect_id;
  guint iface_check_id;
#if HAVE_GUDEV
  GUdevClient *gudev_client;
#endif
  SynceDbusDeviceManager *interface;
};

#define SYNCE_DEVICE_MANAGER_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), SYNCE_TYPE_DEVICE_MANAGER, SynceDeviceManagerPrivate))


static void
synce_device_manager_device_entry_free(DeviceEntry *entry)
{
  g_free(entry->device_path);
  g_free(entry->device_ip);
  g_free(entry->local_ip);
  if (entry->server) {
    if (g_socket_service_is_active(entry->server)) {
      g_socket_service_stop(entry->server);
    }
    g_object_unref(entry->server);
  }
  if (entry->device) g_object_unref(entry->device);
  g_free(entry);

  return;
}

static void
synce_device_manager_remove_device(SynceDeviceManager *self, const gchar *device_path)
{
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);

  GSList *device_entry_iter = priv->devices;
  while (device_entry_iter != NULL) {

    if (g_str_has_suffix(device_path, ((DeviceEntry*)device_entry_iter->data)->device_path) == TRUE) 
      break;

    device_entry_iter = g_slist_next(device_entry_iter);
  }

  if (!device_entry_iter) {
    g_debug("%s: disconnect signal received for a non-managed device: %s", G_STRFUNC, device_path);
    return;
  }

  g_message("%s: received disconnect for one of our devices: %s", G_STRFUNC, device_path);

  DeviceEntry *deventry = device_entry_iter->data;
  priv->devices = g_slist_delete_link(priv->devices, device_entry_iter);

  if (deventry->device && SYNCE_IS_DEVICE(deventry->device)) {
    gchar *obj_path = NULL;
    g_object_get(deventry->device, "object-path", &obj_path, NULL);
    g_message("%s: emitting disconnect for object path %s", G_STRFUNC, obj_path);
    g_signal_emit (self, SYNCE_DEVICE_MANAGER_GET_CLASS(SYNCE_DEVICE_MANAGER(self))->signals[SYNCE_DEVICE_MANAGER_DEVICE_DISCONNECTED], 0, obj_path);
    synce_dbus_device_manager_emit_device_disconnected(priv->interface, obj_path);
    g_free(obj_path);
  } else {
    g_message("%s: removing uninitialised device: %s", G_STRFUNC, device_path);
  }
  synce_device_manager_device_entry_free(deventry);

  return;
}

#if HAVE_GUDEV

static void
gudev_uevent_callback(G_GNUC_UNUSED GUdevClient *client,
		      gchar *action,
		      GUdevDevice *device,
		      gpointer user_data)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER (user_data);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);
  g_return_if_fail(priv->inited && !(priv->dispose_has_run));

  const gchar *dev_path = NULL;

  g_debug("%s: received uevent %s for device %s", G_STRFUNC, action, g_udev_device_get_sysfs_path(device));

  if (strcmp("remove", action) != 0)
    return;

  dev_path = g_udev_device_get_sysfs_path(device);

  synce_device_manager_remove_device(self, dev_path);

  return;
}
#endif

static void
synce_device_manager_device_sends_disconnected_cb(SynceDevice *device,
						  gpointer user_data)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(user_data);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);

  g_debug("%s: received disconnect signal from device object", G_STRFUNC);

  GSList *device_entry_iter = priv->devices;
  while (device_entry_iter) {
    if (((DeviceEntry*)device_entry_iter->data)->device == device)
      break;
    device_entry_iter = g_slist_next(device_entry_iter);
  }

  if (!device_entry_iter) {
    g_critical("%s: disconnect signal received from a non-recognised device", G_STRFUNC);
    return;
  }

  DeviceEntry *deventry = device_entry_iter->data;
  priv->devices = g_slist_delete_link(priv->devices, device_entry_iter);

  gchar *obj_path = NULL;
  g_object_get(device, "object-path", &obj_path, NULL);
  g_message("%s: emitting disconnect for object path %s", G_STRFUNC, obj_path);
  g_signal_emit (self, SYNCE_DEVICE_MANAGER_GET_CLASS(SYNCE_DEVICE_MANAGER(self))->signals[SYNCE_DEVICE_MANAGER_DEVICE_DISCONNECTED], 0, obj_path);
  synce_dbus_device_manager_emit_device_disconnected(priv->interface, obj_path);
  g_free(obj_path);
  synce_device_manager_device_entry_free(deventry);

  return;
}


static void
synce_device_manager_device_obj_path_changed_cb(GObject    *obj,
						G_GNUC_UNUSED GParamSpec *param,
						gpointer    user_data)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(user_data);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);
  SynceDevice *dev = SYNCE_DEVICE(obj);

  gchar *obj_path = NULL;
  g_object_get (dev, "object-path", &obj_path, NULL);
  if (!obj_path) {
    g_debug("%s: device set object path to NULL", G_STRFUNC);
    return;
  }

  g_debug("%s: sending connected signal for %s", G_STRFUNC, obj_path); 
  g_signal_emit (self, SYNCE_DEVICE_MANAGER_GET_CLASS(SYNCE_DEVICE_MANAGER(self))->signals[SYNCE_DEVICE_MANAGER_DEVICE_CONNECTED], 0, obj_path);
  synce_dbus_device_manager_emit_device_connected(priv->interface, obj_path);
  g_free (obj_path);
}


static gboolean
synce_device_manager_client_connected_cb(GSocketService *server,
					 GSocketConnection *conn,
					 G_GNUC_UNUSED GObject *source_object,
					 gpointer user_data)
{
  if (conn == NULL) {
    g_critical("%s: a connection error occured", G_STRFUNC);
    return TRUE;
  }

  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(user_data);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);

  GSList *device_entry_iter = priv->devices;
  while (device_entry_iter) {
    if ( (((DeviceEntry*)device_entry_iter->data)->server == server) )
      break;
    device_entry_iter = g_slist_next(device_entry_iter);
  }

  if (!device_entry_iter) {
    g_critical("%s: connection from a non-recognised server", G_STRFUNC);
    return TRUE;
  }

  DeviceEntry *deventry = device_entry_iter->data;
  GError *error = NULL;

  GSocketAddress *local_inet_addr = g_socket_connection_get_local_address(conn, &error);
  if (!local_inet_addr) {
    g_critical("%s: failed to get address from new connection: %s", G_STRFUNC, error->message);
    g_error_free(error);
    return TRUE;
  }
  guint local_port = g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(local_inet_addr));
  g_object_unref(local_inet_addr);

  g_debug("%s: have a connection to port %d", G_STRFUNC, local_port);

  if (local_port == 5679) {
    if (!(deventry->device)) {
      /*
       * should close the socket listener on port 990 here, but can't see how to
       */
      g_debug("%s: creating device object for %s", G_STRFUNC, deventry->device_path);
      deventry->device = SYNCE_DEVICE(synce_device_legacy_new (conn, deventry->device_path));
    } else {
      g_warning("%s: unexpected secondary connection to port %d", G_STRFUNC, local_port);
      return TRUE;
    }
  } else if (local_port == 990) {
    if (!(deventry->device)) {
      /*
       * should close the socket listener on port 5679 here, but can't see how to
       */
      g_debug("%s: creating device object for %s", G_STRFUNC, deventry->device_path);
      deventry->device = SYNCE_DEVICE(synce_device_rndis_new (conn, deventry->device_path));
    } else {
      synce_device_rndis_client_connected (SYNCE_DEVICE_RNDIS(deventry->device), conn);
      return TRUE;
    }
  } else {
    g_warning("%s: unexpected connection to port %d", G_STRFUNC, local_port);
    return TRUE;
  }

  if (!deventry->device) {
    g_critical("%s: failed to create device object for new connection: %s", G_STRFUNC, error->message);
    g_error_free(error);
    return TRUE;
  }

  g_signal_connect(deventry->device, "disconnected", G_CALLBACK(synce_device_manager_device_sends_disconnected_cb), self);
  g_signal_connect(deventry->device, "notify::object-path", G_CALLBACK(synce_device_manager_device_obj_path_changed_cb), self);

  return TRUE;
}


static void
trigger_device_connection (const gchar *device_ip)
{
  GSocket *sock = NULL;
  GInetAddress *inet_address = NULL;
  GSocketAddress *sock_address = NULL;
  GError *error = NULL;
  gchar b = 0x7f;

  sock = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_DEFAULT, &error);
  if (!sock) {
    g_warning ("%s: failed to create socket: %s", G_STRFUNC, error->message);
    goto exit;
  }

  if (!(inet_address = g_inet_address_new_from_string(device_ip))) {
    g_critical("%s: failed to parse ip address: %s", G_STRFUNC, device_ip);
    goto exit;
  }
  sock_address = g_inet_socket_address_new(inet_address, 5679);

  if (g_socket_send_to (sock, sock_address, &b, sizeof(b), NULL, &error) != 1) {
    g_critical("%s: failed to send on socket: %s", G_STRFUNC, error->message);
    goto exit;
  }

exit:
  if (sock) g_object_unref(sock);
  if (inet_address) g_object_unref(inet_address);
  if (sock_address) g_object_unref(sock_address);

  return;
}


static gboolean
synce_device_manager_create_device(SynceDeviceManager *self,
				   GInetAddress *local_addr,
				   DeviceEntry *deventry)
{
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE(self);
  g_return_val_if_fail(priv->inited && !(priv->dispose_has_run), FALSE);

  GSocketAddress *sock_address = NULL;
  GError *error = NULL;

  g_debug("%s: found device interface for %s", G_STRFUNC, deventry->device_path);

  deventry->iface_pending = FALSE;

  deventry->server = g_socket_service_new();
  if (!(deventry->server)) {
    g_critical("%s: failed to create socket service", G_STRFUNC);
    goto error_exit;
  }

  sock_address = g_inet_socket_address_new(local_addr, 990);
  if (!(g_socket_listener_add_address(G_SOCKET_LISTENER(deventry->server),
				 sock_address, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP,
				 NULL, NULL, &error))) {
    g_critical("%s: failed to add port 990 socket to socket service: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  g_object_unref(sock_address);
  sock_address = g_inet_socket_address_new(local_addr, 5679);
  if (!(g_socket_listener_add_address(G_SOCKET_LISTENER(deventry->server),
				 sock_address, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP,
				 NULL, NULL, &error))) {
    g_critical("%s: failed to add port 5679 socket to socket service: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  g_object_unref(sock_address);

  g_signal_connect(deventry->server, "incoming", G_CALLBACK(synce_device_manager_client_connected_cb), self);

  g_socket_service_start(deventry->server);

  g_debug("%s: listening for device %s", G_STRFUNC, deventry->device_path);

  if (deventry->rndis) {
    g_debug("%s: triggering connection", G_STRFUNC);
    trigger_device_connection(deventry->device_ip);
  } else {
    g_debug("%s: NOT triggering connection", G_STRFUNC);
  }

  return TRUE;

 error_exit:
  if (sock_address) g_object_unref(sock_address);
  if (deventry->server) g_object_unref(deventry->server);

  return FALSE;
}

static gboolean
synce_device_manager_check_interfaces_cb (gpointer userdata)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(userdata);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE(self);
  g_return_val_if_fail(priv->inited && !(priv->dispose_has_run), FALSE);

  GError *error = NULL;
  GSList *device_entry_iter = NULL;

  device_entry_iter = priv->devices;
  while (device_entry_iter) {
    if ( !((DeviceEntry*)device_entry_iter->data)->iface_pending ) {
      device_entry_iter = g_slist_next(device_entry_iter);
      continue;
    }

    const gchar *local_ip = ((DeviceEntry*)device_entry_iter->data)->local_ip;

    GSocket *socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, &error);
    if (!socket) {
      g_critical("%s: failed to create a socket: %s", G_STRFUNC, error->message);
      g_error_free(error);
      return TRUE;
    }

    GInetAddress *local_addr = g_inet_address_new_from_string(local_ip);
    GInetSocketAddress *sockaddr = G_INET_SOCKET_ADDRESS(g_inet_socket_address_new(local_addr, 990));

    if (g_socket_bind(socket, G_SOCKET_ADDRESS(sockaddr), TRUE, &error)) {
      g_debug("%s: address ready", G_STRFUNC);
      g_object_unref(socket);
      synce_device_manager_create_device(self, local_addr, device_entry_iter->data);
    } else {
      g_debug("%s: address not yet ready, failed to bind: %s", G_STRFUNC, error->message);
      g_error_free(error);
      error = NULL;
      g_object_unref(socket);
    }
    g_object_unref(sockaddr);
    g_object_unref(local_addr);

    device_entry_iter = g_slist_next(device_entry_iter);
  }

  /* check if any interfaces are still outstanding */

  device_entry_iter = priv->devices;
  while (device_entry_iter) {
    if (((DeviceEntry*)device_entry_iter->data)->iface_pending)
      return TRUE;

    device_entry_iter = g_slist_next(device_entry_iter);
  }

  priv->iface_check_id = 0;
  return FALSE;
}


static void
synce_device_manager_device_connected_cb(G_GNUC_UNUSED SynceDeviceManagerControl *device_manager_control,
					 gchar *device_path,
					 gchar *device_ip,
					 gchar *local_ip,
					 gboolean rndis,
					 gpointer userdata)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(userdata);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE(self);
  g_return_if_fail(priv->inited && !(priv->dispose_has_run));

  g_debug("%s: receieved device connected signal for %s", G_STRFUNC, device_path);

  GSList *device_entry_iter = priv->devices;
  while (device_entry_iter != NULL) {

    if (strcmp(device_path, ((DeviceEntry*)device_entry_iter->data)->device_path) == 0) {
      break;
    }

    device_entry_iter = g_slist_next(device_entry_iter);
  }

  if (device_entry_iter) {
    g_debug("%s: ignoring connect signal for already known device %s", G_STRFUNC, device_path);
    return;
  }

  DeviceEntry *deventry = g_new0(DeviceEntry, 1);
  if (!deventry) {
    g_critical("%s: failed to allocate DeviceEntry", G_STRFUNC);
    return;
  }

  deventry->device_path = g_strdup(device_path);
  deventry->device_ip = g_strdup(device_ip);
  deventry->local_ip = g_strdup(local_ip);
  deventry->rndis = rndis;
  deventry->iface_pending = TRUE;

  priv->devices = g_slist_append(priv->devices, deventry);

  /* only set up the timeout callback if it doesn't already exist */
  if (priv->iface_check_id > 0)
    return;

  priv->iface_check_id = g_timeout_add (100, synce_device_manager_check_interfaces_cb, self);
}


static void
synce_device_manager_device_disconnected_cb(G_GNUC_UNUSED SynceDeviceManagerControl *device_manager_control,
					    gchar *device_path,
					    gpointer userdata)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(userdata);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE(self);
  g_return_if_fail(priv->inited && !(priv->dispose_has_run));

  g_debug("%s: receieved device disconnected signal for %s", G_STRFUNC, device_path);

  /*
   * this signal comes through the udev scripts, if we have gudev we ignore this as we
   * are monitoring udev directly
   */

#if HAVE_GUDEV
  g_debug("%s: ignored, listening through libgudev", G_STRFUNC);
#else
  synce_device_manager_remove_device(self, device_path);
#endif

  return;
}

gboolean
synce_device_manager_get_connected_devices (SynceDbusDeviceManager *interface,
                                            GDBusMethodInvocation *invocation,
                                            gpointer userdata)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(userdata);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);
  g_return_val_if_fail(priv->inited && !(priv->dispose_has_run), FALSE);

  gchar **device_list = g_malloc0((g_slist_length(priv->devices) + 1) * sizeof(gchar*));
  guint n = 0;
  GSList *device_entry_iter = priv->devices;
  while (device_entry_iter != NULL) {

    gchar *obj_path;
    SynceDevice *device = ((DeviceEntry*)device_entry_iter->data)->device;
    if (device == NULL) {
      /* interface is not yet ready */
      device_entry_iter = g_slist_next(device_entry_iter);
      continue;
    }

    g_object_get (device, "object-path", &obj_path, NULL);
    if (obj_path == NULL) {
      /* device is not yet ready */
      device_entry_iter = g_slist_next(device_entry_iter);
      continue;
    }

    g_debug("%s: found device %s with object path %s", G_STRFUNC, ((DeviceEntry*)device_entry_iter->data)->device_path, obj_path);

    device_list[n] = obj_path;
    n++;

    device_entry_iter = g_slist_next(device_entry_iter);
  }

  synce_dbus_device_manager_complete_get_connected_devices(interface, invocation, (const gchar**)device_list);

  g_strfreev(device_list);

  return TRUE;
}

/*
 * class / object functions
 */

static void
synce_device_manager_initable_iface_init (GInitableIface *iface)
{
  iface->init = synce_device_manager_initable_init;
}

static void
synce_device_manager_async_initable_iface_init (GAsyncInitableIface *iface)
{
  iface->init_async = synce_device_manager_init_async;
  iface->init_finish = synce_device_manager_init_finish;
}

static void
synce_device_manager_init (SynceDeviceManager *self)
{
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE(self);

  priv->devices = NULL;
  priv->control_iface = NULL;
  priv->control_connect_id = 0;
  priv->control_disconnect_id = 0;
  priv->iface_check_id = 0;
  priv->inited = FALSE;
  priv->init_state = NOT_INITIALISED;
  priv->init_success = FALSE;
  priv->init_results = NULL;
  priv->init_error = NULL;
#if HAVE_GUDEV
  priv->gudev_client = NULL;
#endif

  return;
}


static void
synce_device_manager_ctrl_ready_cb (GObject *source_object,
				    GAsyncResult *res,
				    gpointer user_data)
{
  g_return_if_fail (SYNCE_IS_DEVICE_MANAGER(user_data));
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(user_data);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);

  GError *error = NULL;

  priv->control_iface = synce_device_manager_control_new_finish(res, &error);
  if (!(priv->control_iface)) {
    g_critical("%s: failed to create manager control interface: %s", G_STRFUNC, error->message);
    goto out;
  }

  priv->control_connect_id = g_signal_connect(priv->control_iface, "device-connected", G_CALLBACK(synce_device_manager_device_connected_cb), self);
  priv->control_disconnect_id = g_signal_connect(priv->control_iface, "device-disconnected", G_CALLBACK(synce_device_manager_device_disconnected_cb), self);

  priv->init_success = TRUE;

 out:

  priv->inited = TRUE;
  if (!priv->init_success)
    priv->init_error = error;

  GList *l;

  priv->init_state = INITIALISED;

  for (l = priv->init_results; l != NULL; l = l->next)
    {
      GTask *task = l->data;

      if (priv->init_success)
	g_task_return_boolean (task, TRUE);
      else
	g_task_return_error (task, g_error_copy(priv->init_error));
      g_object_unref (task);
    }

  g_list_free (priv->init_results);
  priv->init_results = NULL;

  return;
}

static void
synce_device_manager_sysbus_ready_cb (GObject *source_object,
				      GAsyncResult *res,
				      gpointer user_data)
{
  g_return_if_fail (SYNCE_IS_DEVICE_MANAGER(user_data));
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(user_data);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);

  GError *error = NULL;
  GDBusConnection *system_bus = NULL;

  system_bus = g_bus_get_finish (res, &error);

  if (system_bus == NULL) {
    g_critical("%s: Failed to connect to system bus: %s", G_STRFUNC, error->message);
    goto out;
  }
  if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(priv->interface),
					system_bus,
                                        DEVICE_MANAGER_OBJECT_PATH,
					&error)) {
    g_critical("%s: Failed to export interface on system bus: %s", G_STRFUNC, error->message);
    g_object_unref(system_bus);
    goto out;
  }
  g_object_unref(system_bus);

  synce_device_manager_control_new_async(g_task_get_cancellable((GTask*)priv->init_results->data),
					 synce_device_manager_ctrl_ready_cb,
					 self);

  return;

 out:

  if (!priv->init_success)
    priv->init_error = error;

  GList *l;

  priv->init_state = INITIALISED;

  for (l = priv->init_results; l != NULL; l = l->next)
    {
      GTask *task = l->data;

      if (priv->init_success)
	g_task_return_boolean (task, TRUE);
      else
	g_task_return_error (task, g_error_copy(priv->init_error));
      g_object_unref (task);
    }

  g_list_free (priv->init_results);
  priv->init_results = NULL;

  return;
}


static void
synce_device_manager_init_async (GAsyncInitable *initable, int io_priority, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  g_return_if_fail (SYNCE_IS_DEVICE_MANAGER(initable));
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(initable);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);

  GTask *task = NULL;
  task = g_task_new (initable, cancellable, callback, user_data);

  switch (priv->init_state)
    {
    case NOT_INITIALISED:

#if HAVE_GUDEV
      g_debug("%s: connecting to udev", G_STRFUNC);
      if (!(priv->gudev_client = g_udev_client_new(udev_subsystems))) {
	g_critical("%s: failed to initialize connection to udev", G_STRFUNC);
	priv->init_error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to initialize connection to udev");
	g_task_return_error (task, g_error_copy(priv->init_error));
	priv->init_state = INITIALISED;
	g_object_unref(task);
	return;
      }

      if (g_signal_connect(priv->gudev_client, "uevent", G_CALLBACK(gudev_uevent_callback), self) < 1) {
	g_critical("%s: failed to connect to uevent signal", G_STRFUNC);
	priv->init_error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to initialize connection to udev");
	g_task_return_error (task, g_error_copy(priv->init_error));
	priv->init_state = INITIALISED;
	g_object_unref(task);
	return;
      }
#endif

      priv->interface = synce_dbus_device_manager_skeleton_new();
      g_signal_connect(priv->interface,
		       "handle-get-connected-devices",
		       G_CALLBACK (synce_device_manager_get_connected_devices),
		       self);

      /* should we ref self ? */
      g_bus_get (G_BUS_TYPE_SYSTEM,
		 cancellable,
		 synce_device_manager_sysbus_ready_cb,
		 self);

      priv->init_results = g_list_append(priv->init_results, task);
      priv->init_state = INITIALISING;
      break;
    case INITIALISING:
      priv->init_results = g_list_append(priv->init_results, task);
      break;
    case INITIALISED:
      if (!priv->init_success)
	g_task_return_error (task, g_error_copy(priv->init_error));
      else
	g_task_return_boolean (task, TRUE);
      g_object_unref (task);
      break;
    }
  return;
}


static gboolean
synce_device_manager_init_finish (GAsyncInitable      *initable,
				  GAsyncResult        *res,
				  GError             **error)
{
  g_return_val_if_fail (g_task_is_valid (res, initable), FALSE);

  return g_task_propagate_boolean (G_TASK (res), error);
}


static gboolean
synce_device_manager_initable_init (GInitable *initable, GCancellable *cancellable, GError **error)
{
  g_return_val_if_fail (SYNCE_IS_DEVICE_MANAGER(initable), FALSE);
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(initable);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);

  switch (priv->init_state)
    {
    case NOT_INITIALISED:
      priv->init_state = INITIALISING;

#if HAVE_GUDEV
      g_debug("%s: connecting to udev", G_STRFUNC);
      if (!(priv->gudev_client = g_udev_client_new(udev_subsystems))) {
	g_critical("%s: failed to initialize connection to udev", G_STRFUNC);
	priv->init_error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to initialize connection to udev");
	goto out;
      }

      if (g_signal_connect(priv->gudev_client, "uevent", G_CALLBACK(gudev_uevent_callback), self) < 1) {
	g_critical("%s: failed to connect to uevent signal", G_STRFUNC);
	priv->init_error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to initialize connection to udev");
	goto out;
      }
#endif

      priv->interface = synce_dbus_device_manager_skeleton_new();
      g_signal_connect(priv->interface,
		       "handle-get-connected-devices",
		       G_CALLBACK (synce_device_manager_get_connected_devices),
		       self);

      GDBusConnection *system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &(priv->init_error));
      if (system_bus == NULL) {
	g_critical("%s: Failed to connect to system bus: %s", G_STRFUNC, priv->init_error->message);
	goto out;
      }
      if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(priv->interface),
					    system_bus,
					    DEVICE_MANAGER_OBJECT_PATH,
					    &(priv->init_error))) {
	g_critical("%s: Failed to export interface on system bus: %s", G_STRFUNC, priv->init_error->message);
	g_object_unref(system_bus);
	goto out;;
      }
      g_object_unref(system_bus);

      priv->control_iface = synce_device_manager_control_new(NULL, &(priv->init_error));
      if (!(priv->control_iface)) {
	g_critical("%s: failed to create manager control interface: %s", G_STRFUNC, priv->init_error->message);
	goto out;;
      }

      priv->control_connect_id = g_signal_connect(priv->control_iface, "device-connected", G_CALLBACK(synce_device_manager_device_connected_cb), self);
      priv->control_disconnect_id = g_signal_connect(priv->control_iface, "device-disconnected", G_CALLBACK(synce_device_manager_device_disconnected_cb), self);

      priv->inited = TRUE;
      priv->init_success = TRUE;
      break;

    case INITIALISING:
      /* shouldn't ever have this unless initialised in 2 different threads ? */
      break;
    case INITIALISED:
      /* don't need to do anything here */
      break;
    }

 out:
  if (priv->init_success == FALSE)
    g_propagate_error (error, g_error_copy(priv->init_error));
  priv->init_state = INITIALISED;
  return priv->init_success;
}

static void
synce_device_manager_dispose (GObject *obj)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER (obj);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

#if HAVE_GUDEV
  g_object_unref(priv->gudev_client);
#endif

  if (priv->interface) {
    g_dbus_interface_skeleton_unexport(G_DBUS_INTERFACE_SKELETON(priv->interface));
    g_object_unref(priv->interface);
  }

  GSList *list_entry = priv->devices;
  while (list_entry) {
    synce_device_manager_device_entry_free(list_entry->data);
    list_entry = g_slist_next(list_entry);
  }
  g_slist_free(priv->devices);

  g_signal_handler_disconnect(priv->control_iface, priv->control_connect_id);
  g_signal_handler_disconnect(priv->control_iface, priv->control_disconnect_id);
  g_object_unref(priv->control_iface);

  g_list_free_full(priv->init_results, g_object_unref);
  g_clear_error(&(priv->init_error));

  if (G_OBJECT_CLASS (synce_device_manager_parent_class)->dispose)
    G_OBJECT_CLASS (synce_device_manager_parent_class)->dispose (obj);
}

static void
synce_device_manager_finalize (GObject *obj)
{
  G_OBJECT_CLASS (synce_device_manager_parent_class)->finalize (obj);
}

static void
synce_device_manager_class_init (SynceDeviceManagerClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (SynceDeviceManagerPrivate));

  obj_class->dispose = synce_device_manager_dispose;
  obj_class->finalize = synce_device_manager_finalize;

  klass->signals[SYNCE_DEVICE_MANAGER_DEVICE_CONNECTED] =
    g_signal_new ("device-connected",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  klass->signals[SYNCE_DEVICE_MANAGER_DEVICE_DISCONNECTED] =
    g_signal_new ("device-disconnected",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

}


SynceDeviceManager *
synce_device_manager_new (GCancellable *cancellable, GError **error)
{
  return SYNCE_DEVICE_MANAGER(g_initable_new(SYNCE_TYPE_DEVICE_MANAGER, cancellable, error, NULL));
}

void
synce_device_manager_new_async (GCancellable *cancellable,
				GAsyncReadyCallback callback,
				gpointer user_data)
{
  g_async_initable_new_async(SYNCE_TYPE_DEVICE_MANAGER, G_PRIORITY_DEFAULT, cancellable, callback, user_data, NULL);
}

SynceDeviceManager *
synce_device_manager_new_finish (GAsyncResult *res,
				 GError **error)
{
  GObject *object = NULL;
  GObject *source_object = NULL;

  source_object = g_async_result_get_source_object (res);
  g_assert (source_object != NULL);
  object = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object),
					res,
					error);
  g_object_unref (source_object);
  if (object != NULL)
    return SYNCE_DEVICE_MANAGER(object);
  else
    return NULL;
}

