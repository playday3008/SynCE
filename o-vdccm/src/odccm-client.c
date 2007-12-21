#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib-object.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <synce.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <synce.h>
#include <gnet.h>

#include "odccm-client.h"
#include "odccm-client-signals-marshal.h"

G_DEFINE_TYPE (OdccmClient, odccm_client, G_TYPE_OBJECT)

typedef struct _OdccmClientPrivate OdccmClientPrivate;
struct _OdccmClientPrivate {
  DBusGConnection *dbus_connection;
  DBusGProxy *dev_mgr_proxy;
  gboolean use_ip;

  GUnixSocket *control_sock_server;
  guint control_sock_watch;
  GPtrArray *client_conns;

  GPtrArray *devices;
  GPtrArray *pending_devices;

  gboolean disposed;
};

#define ODCCM_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ODCCM_CLIENT_TYPE, OdccmClientPrivate))

/* properties */
enum
{
  PROP_USE_IP = 1,

  LAST_PROPERTY
};

#define ODCCM_SERVICE   "org.synce.odccm"
#define ODCCM_MGR_PATH  "/org/synce/odccm/DeviceManager"
#define ODCCM_MGR_IFACE "org.synce.odccm.DeviceManager"
#define ODCCM_DEV_IFACE "org.synce.odccm.Device"

enum {
  ODCCM_DEVICE_PASSWORD_FLAG_SET = 1,
  ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE
};

typedef struct _device device;
struct _device {
  gchar *real_name;
  gchar *obj_path;
  DBusGProxy *proxy;
  gchar *ip;
  gint port;
  gchar *name;
  gchar *class;
  gchar *hardware;
  gchar *transport;
  gint os_major, os_minor, build_number,
    processor_type, partner_id_1, partner_id_2;
  guint password_flags;
  gchar *password;
  gint key;
  gchar *connection_filename;
  GKeyFile *connection_file;
};

/* methods */

static void
free_device(device *dev)
{
  g_free(dev->real_name);
  g_free(dev->ip);
  g_free(dev->name);
  g_free(dev->class);
  g_free(dev->hardware);
  g_free(dev->transport);
  g_free(dev->password);
  g_free(dev->connection_filename);
  g_free(dev->obj_path);
  if (dev->connection_file)
    g_key_file_free(dev->connection_file);
  if (dev->proxy)
    g_object_unref(dev->proxy);

  g_free(dev);
}



static void
client_signal_device_connected(OdccmClient *self, device *dev)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  GError *error = NULL;
  gchar *connect_str;
  if (priv->use_ip)
    connect_str = g_strdup_printf("%s%s", "C", dev->ip);
  else
    connect_str = g_strdup_printf("%s%s", "C", dev->name);

  GIOChannel *client_chann;
  gsize bytes_written;
  GIOStatus status;
  gint i;
  for (i = 0; i < priv->client_conns->len; i++) {
    client_chann = gnet_unix_socket_get_io_channel(g_ptr_array_index(priv->client_conns, i));
    status = g_io_channel_write_chars(client_chann,
				      connect_str,
				      -1,
				      &bytes_written,
				      &error);
  }
  if (status != G_IO_STATUS_NORMAL) {
    g_warning("%s: write failed: %s", G_STRFUNC, error->message);
    g_error_free(error);
  }

  g_free(connect_str);
}

static void
client_signal_password_required(OdccmClient *self, device *dev)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  GError *error = NULL;
  gchar *connect_str;
  if (priv->use_ip)
    connect_str = g_strdup_printf("%s%s", "P", dev->ip);
  else
    connect_str = g_strdup_printf("%s%s", "P", dev->name);
  GIOChannel *client_chann;
  gsize bytes_written;
  GIOStatus status;
  gint i;
  for (i = 0; i < priv->client_conns->len; i++) {
    client_chann = gnet_unix_socket_get_io_channel(g_ptr_array_index(priv->client_conns, i));
    status = g_io_channel_write_chars(client_chann,
				      connect_str,
				      -1,
				      &bytes_written,
				      &error);
  }
  if (status != G_IO_STATUS_NORMAL) {
    g_warning("%s: write failed: %s", G_STRFUNC, error->message);
    g_error_free(error);
  }

  g_free(connect_str);
}

static void
client_signal_password_rejected(OdccmClient *self, device *dev)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  GError *error = NULL;
  gchar *connect_str;
  if (priv->use_ip)
    connect_str = g_strdup_printf("%s%s", "R", dev->ip);
  else
    connect_str = g_strdup_printf("%s%s", "R", dev->name);
  GIOChannel *client_chann;
  gsize bytes_written;
  GIOStatus status;
  gint i;
  for (i = 0; i < priv->client_conns->len; i++) {
    client_chann = gnet_unix_socket_get_io_channel(g_ptr_array_index(priv->client_conns, i));
    status = g_io_channel_write_chars(client_chann,
				      connect_str,
				      -1,
				      &bytes_written,
				      &error);
  }
  if (status != G_IO_STATUS_NORMAL) {
    g_warning("%s: write failed: %s", G_STRFUNC, error->message);
    g_error_free(error);
  }

  g_free(connect_str);
}

static gboolean
write_connection_file(OdccmClient *self, device *new_device)
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

  GError *error = NULL;

  new_device->connection_file = g_key_file_new();
  g_key_file_set_comment(new_device->connection_file, NULL, NULL, "Modifications to this file will be lost next time a client connects to dccm", NULL);
  g_key_file_set_integer(new_device->connection_file, "dccm", "pid", getpid());
  g_key_file_set_integer(new_device->connection_file, "device", "os_version", new_device->os_major);
  g_key_file_set_integer(new_device->connection_file, "device", "build_number", new_device->build_number);
  g_key_file_set_integer(new_device->connection_file, "device", "processor_type", new_device->processor_type);
  g_key_file_set_integer(new_device->connection_file, "device", "partner_id_1", new_device->partner_id_1);
  g_key_file_set_integer(new_device->connection_file, "device", "partner_id_2", new_device->partner_id_2);
  g_key_file_set_string(new_device->connection_file, "device", "name", new_device->name);
  g_key_file_set_string(new_device->connection_file, "device", "real_name", new_device->real_name);
  g_key_file_set_string(new_device->connection_file, "device", "class", new_device->class);
  g_key_file_set_string(new_device->connection_file, "device", "hardware", new_device->hardware);
  g_key_file_set_string(new_device->connection_file, "device", "ip", new_device->ip);
  g_key_file_set_integer(new_device->connection_file, "device", "port", new_device->port);
  g_key_file_set_string(new_device->connection_file, "device", "password", new_device->password ? new_device->password : "");
  g_key_file_set_integer(new_device->connection_file, "device", "key", new_device->key);
  g_key_file_set_string(new_device->connection_file, "connection", "transport", new_device->transport);

  if (priv->use_ip)
    new_device->connection_filename = g_strdup_printf("%s/.synce/%s", g_get_home_dir(), new_device->ip);
  else
    new_device->connection_filename = g_strdup_printf("%s/.synce/%s", g_get_home_dir(), new_device->name);

  g_debug("%s: connection filename: %s", G_STRFUNC, new_device->connection_filename);

  gchar *file_string;
  gsize file_length;
  file_string = g_key_file_to_data(new_device->connection_file, &file_length, &error);
  if (error) {
    g_critical("%s: error creating connection file: %s", G_STRFUNC, error->message);
    g_error_free(error);
    g_free(file_string);
    return FALSE;
  }

  gint filep = open(new_device->connection_filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
  if (filep == -1) {
    g_critical("%s: failed to open file %s: %s", G_STRFUNC, new_device->connection_filename, strerror(errno));
    return FALSE;
  }

  ssize_t written;
  written = write(filep, file_string, file_length);
  if (written < file_length)
    g_critical("%s: only wrote %d of %d", G_STRFUNC, written, file_length);

  close(filep);
  g_free(file_string);
  return TRUE;
}


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

  device *dev = NULL;
  gint i;
  const gchar *obj_path;

  obj_path = dbus_g_proxy_get_path(proxy);

  i = 0;
  while (i < priv->devices->len) {
    dev = (device*)g_ptr_array_index(priv->devices, i);
    if (!(g_ascii_strcasecmp(obj_path, dbus_g_proxy_get_path(dev->proxy)))) {
      break;
    }
    dev = NULL;
    i++;
  }

  if (!dev) {
    g_warning("%s: Received password flags changed for unfound device: %s", G_STRFUNC, obj_path);
    return;
  }

  if (added & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE)
    client_signal_password_required(self, dev);

  if (removed & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE) {
    if (!write_connection_file(self, dev)) {
      g_ptr_array_remove_index_fast(priv->devices, i);
      free_device(dev);
      return;
    }
    client_signal_device_connected(self, dev);
  }
  return;
}


static void
odccm_device_connected_cb(DBusGProxy *proxy,
			  gchar *obj_path,
			  gpointer user_data)
{
  GError *error = NULL;
  device *new_device = NULL;

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

  g_debug("%s: Received connect from odccm: %s", G_STRFUNC, obj_path);
  new_device = g_new0(device, 1);
  new_device->obj_path = g_strdup(obj_path);

  if (!(new_device->proxy = dbus_g_proxy_new_for_name (priv->dbus_connection,
						       ODCCM_SERVICE,
						       obj_path,
						       ODCCM_DEV_IFACE)))
    {
      g_critical("%s: Error getting proxy for device path %s", G_STRFUNC, obj_path);
      goto error_exit;
    }

  if (!(dbus_g_proxy_call(new_device->proxy, "GetName",
			  &error, G_TYPE_INVALID,
			  G_TYPE_STRING, &(new_device->name),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device name from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  if (!(dbus_g_proxy_call(new_device->proxy, "GetIpAddress",
			  &error, G_TYPE_INVALID,
			  G_TYPE_STRING, &(new_device->ip),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device ip from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  if (!(dbus_g_proxy_call(new_device->proxy, "GetCpuType",
			  &error, G_TYPE_INVALID,
			  G_TYPE_UINT, &(new_device->processor_type),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device processor type from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  if (!(dbus_g_proxy_call(new_device->proxy, "GetOsVersion",
			  &error, G_TYPE_INVALID,
			  G_TYPE_UINT, &(new_device->os_major),
			  G_TYPE_UINT, &(new_device->os_minor),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device os version from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  if (!(dbus_g_proxy_call(new_device->proxy, "GetCurrentPartnerId",
			  &error,G_TYPE_INVALID,
			  G_TYPE_UINT, &(new_device->partner_id_1),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device partner id from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  if (!(dbus_g_proxy_call(new_device->proxy, "GetPlatformName",
			  &error, G_TYPE_INVALID,
			  G_TYPE_STRING, &(new_device->class),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device platform name from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  if (!(dbus_g_proxy_call(new_device->proxy, "GetModelName",
			  &error, G_TYPE_INVALID,
			  G_TYPE_STRING, &(new_device->hardware),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting device model name from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  new_device->transport = g_strdup("odccm");

  new_device->real_name = g_strdup(new_device->name);
  new_device->build_number = 0;
  new_device->partner_id_2 = 0;

  new_device->port = 0;
  new_device->password_flags = 0;
  new_device->password = NULL;
  new_device->key = 0;


  g_debug("%s: name: %s", G_STRFUNC, new_device->name);
  g_debug("%s: ip: %s", G_STRFUNC, new_device->ip);
  g_debug("%s: processor type: %d", G_STRFUNC, new_device->processor_type);
  g_debug("%s: os: %d %d", G_STRFUNC, new_device->os_major, new_device->os_minor);
  g_debug("%s: partner id 1: %d", G_STRFUNC, new_device->partner_id_1);
  g_debug("%s: class: %s", G_STRFUNC, new_device->class);
  g_debug("%s: hardware: %s", G_STRFUNC, new_device->hardware);
  g_debug("%s: transport: %s", G_STRFUNC, new_device->transport);
  g_debug("%s: real name: %s", G_STRFUNC, new_device->real_name);


  dbus_g_proxy_add_signal (new_device->proxy, "PasswordFlagsChanged",
			   G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (new_device->proxy, "PasswordFlagsChanged",
			       G_CALLBACK(password_flags_changed_cb), self, NULL);

  if (!(dbus_g_proxy_call(new_device->proxy, "GetPasswordFlags",
			  &error, G_TYPE_INVALID,
			  G_TYPE_UINT, &(new_device->password_flags),
			  G_TYPE_INVALID))) {
    g_critical("%s: Error getting password flags from odccm: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto error_exit;
  }

  if (new_device->password_flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE) {
    g_ptr_array_add(priv->devices, new_device);
    client_signal_password_required(self, new_device);
    goto exit;
  }

  g_ptr_array_add(priv->devices, new_device);

  if (!write_connection_file(self, new_device))
    goto error_exit;

  client_signal_device_connected(self, new_device);

  goto exit;
error_exit:
  free_device(new_device);
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

  GError *error = NULL;
  gint i;
  device *dev = NULL;
  i = 0;
  while (i < priv->devices->len) {
    dev = (device *)g_ptr_array_index(priv->devices, i);
    if (!(g_ascii_strcasecmp(obj_path, dev->obj_path))) {
      break;
    }
    dev = NULL;
    i++;
  }

  if (!dev) {
    g_warning("%s: Received disconnect from odccm from unfound device: %s", G_STRFUNC, obj_path);
    return;
  }
  g_ptr_array_remove_index_fast(priv->devices, i);

  g_unlink(dev->connection_filename);

  gchar *disconnect_str;
  if (priv->use_ip)
    disconnect_str = g_strdup_printf("%s%s", "D", dev->ip);
  else
    disconnect_str = g_strdup_printf("%s%s", "D", dev->name);
  GIOChannel *client_chann;
  gsize bytes_written;
  GIOStatus status;
  for (i = 0; i < priv->client_conns->len; i++) {
    client_chann = gnet_unix_socket_get_io_channel(g_ptr_array_index(priv->client_conns, i));
    status = g_io_channel_write_chars(client_chann,
				      disconnect_str,
				      -1,
				      &bytes_written,
				      &error);
  }
  if (status != G_IO_STATUS_NORMAL) {
    g_warning("%s: write failed: %s", G_STRFUNC, error->message);
    g_error_free(error);
  }

  g_free(disconnect_str);

  free_device(dev);

  return;
}

static void
device_send_password(OdccmClient *self, gchar *name, gchar *password)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  device *dev = NULL;
  GError *error = NULL;
  gboolean password_accepted = FALSE, result;

  gint i = 0;
  while (i < priv->devices->len) {
    dev = (device*)g_ptr_array_index(priv->devices, i);

    if (priv->use_ip)
      {
	if (!(g_ascii_strcasecmp(name, dev->ip)))
	  break;
      }
    else
      {
	if (!(g_ascii_strcasecmp(name, dev->name)))
	  break;
      }
    dev = NULL;
    i++;
  }

  if (!dev) {
    g_warning("%s: Password provided for unfound device: %s", G_STRFUNC, name);
    return;
  }

  result = dbus_g_proxy_call(dev->proxy,
                             "ProvidePassword",
                             &error,
                             G_TYPE_STRING, password,
                             G_TYPE_INVALID,
                             G_TYPE_BOOLEAN, &password_accepted,
                             G_TYPE_INVALID);

  if (result == FALSE) {
    g_critical("%s: Error sending password to odccm for %s: %s", G_STRFUNC, name, error->message);
    g_error_free(error);
    g_ptr_array_remove_index_fast(priv->devices, i);
    free_device(dev);
    return;
  }

  if (!(password_accepted)) {
    g_debug("%s: Password rejected for %s", G_STRFUNC, name);
    client_signal_password_rejected(self, dev);
    g_ptr_array_remove_index_fast(priv->devices, i);
    free_device(dev);
    return;
  }

  return;
}

static gboolean
client_readable(GIOChannel *source,
		GIOCondition condition,
		gpointer data)
{
  OdccmClient *self = ODCCM_CLIENT(data);

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  gsize count = 256;
  gsize bytes_read;
  gchar *buffer = g_malloc0(count);
  GIOStatus status;
  GError *error = NULL;

  status = g_io_channel_read_chars(source,
				   buffer,
				   count,
				   &bytes_read,
				   &error);
  if (status != G_IO_STATUS_NORMAL) {
    g_warning("%s: read failed: %s", G_STRFUNC, error->message);
    g_error_free(error);
    goto exit;
  }

  buffer[bytes_read] = '\0';
  gchar *tmp = buffer + 1;
  gchar **tokens;

  switch(buffer[0]) {
  case 'R':
    /* password provided */
    tokens = g_strsplit(tmp, "=", 2);
    device_send_password(self, tokens[0], tokens[1]);
    g_strfreev(tokens);
    break;
  case 'D':
    /* disconnection requested */
    g_message("%s: Disconnection requested for %s, not supported under odccm", G_STRFUNC, tmp);
    break;
  default:
    g_warning("%s: Unknown response from client: %c", G_STRFUNC, buffer[0]);
    break;
  }

exit:
  g_free(buffer);

  return TRUE;
}

static gboolean
client_disconnected(GIOChannel *source,
		    GIOCondition condition,
		    gpointer data)
{
  OdccmClient *self = ODCCM_CLIENT(data);

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  gint i;
  for (i = 0; i < priv->client_conns->len; i++) {
    if (source == gnet_unix_socket_get_io_channel(g_ptr_array_index(priv->client_conns, i)))
      {
	gnet_unix_socket_delete(g_ptr_array_index(priv->client_conns, i));
	g_ptr_array_remove_index_fast(priv->client_conns, i);
	break;
      }
  }
  return FALSE;
}

static gboolean
client_connected(GIOChannel *source,
		 GIOCondition condition,
		 gpointer data)
{
  OdccmClient *self = ODCCM_CLIENT(data);

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  GUnixSocket* new_client_conn = gnet_unix_socket_server_accept(priv->control_sock_server);

  GIOChannel* new_client_io_chann = gnet_unix_socket_get_io_channel (new_client_conn);

  g_io_add_watch(new_client_io_chann,
		 G_IO_HUP,
		 client_disconnected,
		 self);

  g_io_add_watch(new_client_io_chann,
		 G_IO_IN,
		 client_readable,
		 self);

  g_ptr_array_add(priv->client_conns, new_client_conn);

  device *dev;

  gint i;
  for (i = 0; i < priv->devices->len; i++) {
    dev = (device *)g_ptr_array_index(priv->devices, i);
    client_signal_device_connected(self, dev);
  }
}

/* class & instance functions */

static void
odccm_client_init(OdccmClient *self)
{
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);
  GError *error = NULL;

  priv->dbus_connection = dbus_g_bus_get (DBUS_BUS_SYSTEM,
                               &error);
  if (priv->dbus_connection == NULL)
    {
      g_critical("%s: Failed to open connection to bus: %s", G_STRFUNC, error->message);
      g_error_free (error);
      return;
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

  priv->devices = g_ptr_array_new();


  gchar *synce_dir;
  if (!synce_get_directory(&synce_dir))
    g_error("%s: Failed to get configuration directory name.", G_STRFUNC);

  gchar *control_sock_path = g_strdup_printf("%s/%s", synce_dir, "csock");
  g_free(synce_dir);

  priv->control_sock_server = gnet_unix_socket_server_new(control_sock_path);
  GIOChannel *control_sock_iochan = gnet_unix_socket_get_io_channel (priv->control_sock_server);
  priv->control_sock_watch = g_io_add_watch(control_sock_iochan,
					    G_IO_IN,
					    client_connected,
					    self);
  g_free(control_sock_path);

  priv->client_conns = g_ptr_array_new();
}

static void
odccm_client_dispose (GObject *obj)
{
  OdccmClient *self = ODCCM_CLIENT(obj);
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (obj);

  if (priv->disposed) {
    return;
  }

  gint i;
  for (i = 0; i < priv->client_conns->len; i++) {
    gnet_unix_socket_delete(g_ptr_array_index(priv->client_conns, i));
  }

  g_ptr_array_free(priv->client_conns, TRUE);


  gnet_unix_socket_delete(priv->control_sock_server);

  dbus_g_proxy_disconnect_signal (priv->dev_mgr_proxy, "DeviceConnected",
			       G_CALLBACK(odccm_device_connected_cb), NULL);

  dbus_g_proxy_disconnect_signal (priv->dev_mgr_proxy, "DeviceDisconnected",
			       G_CALLBACK(odccm_device_disconnected_cb), NULL);

  g_object_unref (priv->dev_mgr_proxy);
  priv->dev_mgr_proxy = NULL;
  dbus_g_connection_unref(priv->dbus_connection);
  priv->dbus_connection = NULL;

  device *dev;

  for (i = 0; i < priv->devices->len; i++) {
    dev = (device *)g_ptr_array_index(priv->devices, i);
    free_device(dev);
  }
  g_ptr_array_free(priv->devices, TRUE);

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
odccm_client_get_property (GObject    *obj,
			   guint       property_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  OdccmClient *self = ODCCM_CLIENT(obj);
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  switch (property_id) {

  case PROP_USE_IP:
    g_value_set_boolean(value, priv->use_ip);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
    break;
  }
}

static void
odccm_client_set_property (GObject      *obj,
			   guint         property_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  OdccmClient *self = ODCCM_CLIENT (obj);
  OdccmClientPrivate *priv = ODCCM_CLIENT_GET_PRIVATE (self);

  switch (property_id) {
  case PROP_USE_IP:
    priv->use_ip = g_value_get_boolean(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
    break;
  }
}

static void
odccm_client_class_init (OdccmClientClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = odccm_client_get_property;
  gobject_class->set_property = odccm_client_set_property;

  gobject_class->dispose = odccm_client_dispose;
  gobject_class->finalize = odccm_client_finalize;

  g_type_class_add_private (klass, sizeof (OdccmClientPrivate));

  GParamSpec *param_spec;
  param_spec = g_param_spec_boolean("use-ip", "Use IPs",
                                    "Address devices by IP.",
                                    FALSE,
                                    G_PARAM_READWRITE |
                                    G_PARAM_CONSTRUCT_ONLY |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_USE_IP, param_spec);

  dbus_g_object_register_marshaller (odccm_client_marshal_VOID__UINT_UINT,
				     G_TYPE_NONE,
				     G_TYPE_UINT,
				     G_TYPE_UINT,
				     G_TYPE_INVALID);
}

