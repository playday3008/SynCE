#include <gnet.h>
#include <string.h>
#include <unistd.h>

#include "synce-device-manager.h"
#include "synce-device-manager-glue.h"
#include "synce-device-manager-control.h"
#include "synce-device.h"
#include "synce-device-rndis.h"
#include "synce-device-legacy.h"
#include "utils.h"

G_DEFINE_TYPE (SynceDeviceManager, synce_device_manager, G_TYPE_OBJECT)

/* private stuff */

typedef struct
{
  gchar *device_path;
  gchar *device_ip;
  gchar *local_ip;
  gboolean rndis;
  gboolean iface_pending;
  GServer *server_990;
  GServer *server_5679;
  SynceDevice *device;
} DeviceEntry;

typedef struct _SynceDeviceManagerPrivate SynceDeviceManagerPrivate;

struct _SynceDeviceManagerPrivate
{
  gboolean dispose_has_run;

  GSList *devices;
  SynceDeviceManagerControl *control_iface;
  gint control_connect_id;
  guint iface_check_id;
};

#define SYNCE_DEVICE_MANAGER_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), SYNCE_TYPE_DEVICE_MANAGER, SynceDeviceManagerPrivate))

static void
device_entry_free(DeviceEntry *entry)
{
  g_free(entry->device_path);
  g_free(entry->device_ip);
  g_free(entry->local_ip);
  if (entry->server_990) gnet_server_delete(entry->server_990);
  if (entry->server_5679) gnet_server_delete(entry->server_5679);
  if (entry->device) g_object_unref(entry->device);
  g_free(entry);

  return;
}



static void
device_disconnected_cb(SynceDevice *device,
		       gpointer user_data)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(user_data);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);

  g_debug("%s: receieved disconnect from device", G_STRFUNC);

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

  gchar *obj_path = NULL;
  g_object_get(device, "object-path", &obj_path, NULL);
  g_signal_emit (self, SYNCE_DEVICE_MANAGER_GET_CLASS(SYNCE_DEVICE_MANAGER(self))->signals[SYNCE_DEVICE_MANAGER_DEVICE_DISCONNECTED], 0, obj_path);
  g_free(obj_path);
  device_entry_free(deventry);
  priv->devices = g_slist_delete_link(priv->devices, device_entry_iter);
}


static void
device_obj_path_changed_cb(GObject    *obj,
			   GParamSpec *param,
			   gpointer    user_data)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(user_data);
  SynceDevice *dev = SYNCE_DEVICE(obj);

  gchar *obj_path = NULL;
  g_object_get (dev, "object-path", &obj_path, NULL);
  if (!obj_path)
    return;

  g_debug("%s: sending connected signal for %s", G_STRFUNC, obj_path); 
  g_signal_emit (self, SYNCE_DEVICE_MANAGER_GET_CLASS(SYNCE_DEVICE_MANAGER(self))->signals[SYNCE_DEVICE_MANAGER_DEVICE_CONNECTED], 0, obj_path);
  g_free (obj_path);
}


static void
client_connected_cb (GServer *server,
                     GConn *conn,
                     gpointer user_data)
{
  if (conn == NULL) {
    g_critical("%s: a connection error occured", G_STRFUNC);
    return;
  }

  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(user_data);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);

  GSList *device_entry_iter = priv->devices;
  while (device_entry_iter) {
    if ((((DeviceEntry*)device_entry_iter->data)->server_990 = server) || (((DeviceEntry*)device_entry_iter->data)->server_5679 = server))
      break;
    device_entry_iter = g_slist_next(device_entry_iter);
  }

  if (!device_entry_iter) {
    g_critical("%s: connection from a non-recognised server", G_STRFUNC);
    return;
  }

  DeviceEntry *deventry = device_entry_iter->data;

  GInetAddr *local_inet_addr = gnet_tcp_socket_get_local_inetaddr (conn->socket);
  gint local_port = gnet_inetaddr_get_port(local_inet_addr);
  gnet_inetaddr_unref(local_inet_addr);

  g_debug("%s: have a connection to port %d", G_STRFUNC, local_port);

  if (local_port == 5679) {
    if (!(deventry->device)) {
      gnet_server_delete(deventry->server_990);
      deventry->server_990 = NULL;
      g_debug("%s: creating device object for %s", G_STRFUNC, deventry->device_path);
      deventry->device = g_object_new (SYNCE_TYPE_DEVICE_LEGACY, "connection", conn, "device-path", deventry->device_path, NULL);
      g_signal_connect(deventry->device, "disconnected", G_CALLBACK(device_disconnected_cb), self);
    }
  } else {
    if (!(deventry->device)) {
      gnet_server_delete(deventry->server_5679);
      deventry->server_5679 = NULL;
      g_debug("%s: creating device object for %s", G_STRFUNC, deventry->device_path);
      deventry->device = g_object_new (SYNCE_TYPE_DEVICE_RNDIS, "connection", conn, "device-path", deventry->device_path, NULL);
      g_signal_connect(deventry->device, "disconnected", G_CALLBACK(device_disconnected_cb), self);
    } else {
      synce_device_rndis_client_connected (SYNCE_DEVICE_RNDIS(deventry->device), conn);
      return;
    }
  }

  gnet_conn_unref (conn);

  g_signal_connect (deventry->device, "notify::object-path",
		    (GCallback) device_obj_path_changed_cb,
		    self);

  return;
}


static gboolean
create_device(SynceDeviceManager *self, GInetAddr *local_iface, DeviceEntry *deventry)
{
  g_debug("%s: found device interface", G_STRFUNC);

  GInetAddr *server_990_addr = gnet_inetaddr_clone(local_iface);
  gnet_inetaddr_set_port(server_990_addr, 990);

  deventry->server_990 = gnet_server_new (server_990_addr, 990, client_connected_cb, self);
  if (!(deventry->server_990)) {
    g_critical("%s: unable to listen on rndis port (990), server invalid", G_STRFUNC);
    return FALSE;
  }

  GInetAddr *server_5679_addr = gnet_inetaddr_clone(local_iface);
  gnet_inetaddr_set_port(server_5679_addr, 5679);

  deventry->server_5679 = gnet_server_new (server_5679_addr, 5679, client_connected_cb, self);
  if (!(deventry->server_5679)) {
    g_critical("%s: unable to listen on legacy port (5679), server invalid", G_STRFUNC);
    return FALSE;
  }

  gnet_inetaddr_unref(server_990_addr);
  gnet_inetaddr_unref(server_5679_addr);

  g_debug("%s: listening for device", G_STRFUNC);

  if (deventry->rndis)
    synce_trigger_connection(deventry->device_ip);

  return TRUE;
}

static void
iface_list_free_func(gpointer data,
		     gpointer user_data)
{
  gnet_inetaddr_unref((GInetAddr*)data);
}


static gboolean
check_interface_cb (gpointer userdata)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(userdata);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE(self);

  gchar *iface_bytes, *local_ip_bytes;
  GInetAddr *local_iface = NULL;

  GList *iface_list = gnet_inetaddr_list_interfaces();
  GList *iface_list_iter = g_list_first(iface_list);

  GSList *device_entry_iter = NULL;

  while (iface_list_iter) {
    iface_bytes = g_malloc0(gnet_inetaddr_get_length((GInetAddr*)iface_list_iter->data));
    gnet_inetaddr_get_bytes((GInetAddr*)iface_list_iter->data, iface_bytes);

    device_entry_iter = priv->devices;
    while (device_entry_iter) {
      local_ip_bytes = ip4_bytes_from_dotted_quad(((DeviceEntry*)device_entry_iter->data)->local_ip);

      if ( (((DeviceEntry*)device_entry_iter->data)->iface_pending) && ((*(guint32*)iface_bytes) == (*(guint32*)local_ip_bytes)) ) {
	local_iface = (GInetAddr*)iface_list_iter->data;
	create_device(self, local_iface, device_entry_iter->data);
	((DeviceEntry*)device_entry_iter->data)->iface_pending = FALSE;
	g_free(local_ip_bytes);
	break;
      }

      g_free(local_ip_bytes);
      device_entry_iter = g_slist_next(device_entry_iter);
    }

    g_free(iface_bytes);
    iface_list_iter = g_list_next(iface_list_iter);
  }

  g_list_foreach(iface_list, iface_list_free_func, NULL);
  g_list_free(iface_list);

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
device_connected_cb(SynceDeviceManagerControl *device_manager_control, gchar *device_path, gchar *device_ip, gchar *local_ip, gboolean rndis, gpointer userdata)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER(userdata);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE(self);

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

  priv->iface_check_id = g_timeout_add (100, check_interface_cb, self);
}

static void
synce_device_manager_init (SynceDeviceManager *self)
{
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE(self);

  GError *error = NULL;
  DBusGConnection *system_bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
  if (system_bus == NULL) {
    g_critical("%s: Failed to connect to system bus: %s", G_STRFUNC, error->message);
    g_error_free(error);
    return;
  }

  dbus_g_connection_register_g_object (system_bus,
                                       DEVICE_MANAGER_OBJECT_PATH,
				       G_OBJECT(self));
  dbus_g_connection_unref(system_bus);

  priv->devices = NULL;
  priv->control_iface = g_object_new(SYNCE_TYPE_DEVICE_MANAGER_CONTROL, NULL);

  priv->control_connect_id = g_signal_connect(priv->control_iface, "device-connected", G_CALLBACK(device_connected_cb), self);
  priv->iface_check_id = 0;

  return;
}

static void
synce_device_manager_dispose (GObject *obj)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER (obj);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  GSList *list_entry = priv->devices;
  while (list_entry) {
    device_entry_free(list_entry->data);
    list_entry = g_slist_next(list_entry);
  }
  g_slist_free(priv->devices);

  g_object_unref(priv->control_iface);

  if (G_OBJECT_CLASS (synce_device_manager_parent_class)->dispose)
    G_OBJECT_CLASS (synce_device_manager_parent_class)->dispose (obj);
}

static void
synce_device_manager_finalize (GObject *obj)
{
  SynceDeviceManager *self = SYNCE_DEVICE_MANAGER (obj);
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);

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

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                   &dbus_glib_synce_device_manager_object_info);
}



gboolean
synce_device_manager_get_connected_devices (SynceDeviceManager *self,
                                            GPtrArray **ret,
                                            GError **error)
{
  SynceDeviceManagerPrivate *priv = SYNCE_DEVICE_MANAGER_GET_PRIVATE (self);

  *ret = g_ptr_array_new ();

  GSList *device_entry_iter = priv->devices;
  while (device_entry_iter != NULL) {

    /* maybe check what auth state device is in */

    gchar *obj_path;
    SynceDevice *device = ((DeviceEntry*)device_entry_iter->data)->device;
    g_object_get (device, "object-path", &obj_path, NULL);

    g_debug("%s: found device %s with object path %s", G_STRFUNC, ((DeviceEntry*)device_entry_iter->data)->device_path, obj_path);

    g_ptr_array_add (*ret, obj_path);

    device_entry_iter = g_slist_next(device_entry_iter);
  }

  return TRUE;
}


