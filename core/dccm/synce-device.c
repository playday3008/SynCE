#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <synce.h>

#include "synce-device.h"
#include "synce-device-internal.h"
#include "synce-device-signals-marshal.h"
#include "synce-device-dbus.h"
#include "synce_gerrors.h"

G_DEFINE_TYPE (SynceDevice, synce_device, G_TYPE_OBJECT)


/* properties */
enum
{
  PROP_CONNECTION = 1,
  PROP_DEVICE_PATH,
  PROP_OBJ_PATH,
  PROP_IP_ADDRESS,
  PROP_IFACE_ADDRESS,

  PROP_GUID,
  PROP_OS_MAJOR,
  PROP_OS_MINOR,
  PROP_NAME,
  PROP_VERSION,
  PROP_CPU_TYPE,
  PROP_CURRENT_PARTNER_ID,
  PROP_ID,
  PROP_PLATFORM_NAME,
  PROP_MODEL_NAME,

  PROP_PASSWORD_FLAGS,

  LAST_PROPERTY
};


/* method overrides */

gboolean
synce_device_request_connection (SynceDbusDevice *interface, GDBusMethodInvocation *invocation, gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);
  return (SYNCE_DEVICE_GET_CLASS(self)->synce_device_request_connection (interface, invocation, userdata));
}


gboolean
synce_device_provide_password (SynceDbusDevice *interface, GDBusMethodInvocation *invocation, const gchar *password, gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);
  return (SYNCE_DEVICE_GET_CLASS(self)->synce_device_provide_password (interface, invocation, password, userdata));
}

void
synce_device_conn_event_cb(GObject *istream, GAsyncResult *res, gpointer user_data)
{
  SYNCE_DEVICE_GET_CLASS(user_data)->synce_device_conn_event_cb (istream, res, user_data);
}


/* standard functions */


static gboolean
synce_device_provide_password_impl (G_GNUC_UNUSED SynceDbusDevice *interface,
				    GDBusMethodInvocation *ctx,
				    const gchar *password,
				    gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);

  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE(self);

  g_return_val_if_fail(!(priv->dispose_has_run), TRUE);

  GError *error = NULL;
  guchar *buf;
  gsize buf_size;

  if (priv->state != CTRL_STATE_AUTH ||
      (priv->pw_flags & SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE) == 0)
    {
      error = g_error_new (SYNCE_DCCM_ERROR, SYNCE_DCCM_ERROR_NOT_AVAILABLE,
			   "No password expected in the current state.");
      goto OUT;
    }
  else if (priv->pw_ctx != NULL)
    {
      error = g_error_new (SYNCE_DCCM_ERROR, SYNCE_DCCM_ERROR_NOT_AVAILABLE,
			   "An authentication attempt is still in progress.");
      goto OUT;
    }

  priv->pw_ctx = ctx;

  synce_password_encode(password, priv->pw_key, &buf, &buf_size);

  buf_size = GUINT16_TO_LE (buf_size);
  gsize written = 0;
  GOutputStream *out_stream = g_io_stream_get_output_stream(G_IO_STREAM(priv->conn));
  if (!(g_output_stream_write_all(out_stream, (gchar *) &buf_size, sizeof (buf_size), &written, NULL, &error))) {
    g_critical("%s: failed to send password to device: %s", G_STRFUNC, error->message);
    goto OUT;
  }
  if (!(g_output_stream_write_all(out_stream, buf, buf_size, &written, NULL, &error))) {
    g_critical("%s: failed to send password to device: %s", G_STRFUNC, error->message);
    goto OUT;
  }

  GInputStream *in_stream = g_io_stream_get_input_stream(G_IO_STREAM(priv->conn));
  priv->iobuf = g_malloc(sizeof (guint16));
  g_input_stream_read_async(in_stream, priv->iobuf, sizeof (guint16), G_PRIORITY_DEFAULT, NULL, synce_device_conn_event_cb, g_object_ref(self));

  synce_device_change_password_flags (self, SYNCE_DEVICE_PASSWORD_FLAG_CHECKING);

 OUT:
  if (buf) wstr_free_string(buf);
  if (error != NULL)
    {
      g_dbus_method_invocation_return_gerror(ctx, error);
      g_error_free(error);
    }
  return TRUE;
}



static const gchar *
get_password_flag_text(SynceDevicePasswordFlags flag)
{
  const gchar *prop_str = NULL;

  switch (flag)
    {
    case SYNCE_DEVICE_PASSWORD_FLAG_UNSET:
      prop_str = "unset";
      g_debug("%s: setting password flags unset", G_STRFUNC);
      break;
    case SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE:
      prop_str = "provide";
      g_debug("%s: setting password flags provide", G_STRFUNC);
      break;
    case SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE:
      prop_str = "provide-on-device";
      g_debug("%s: setting password flags provide-on-device", G_STRFUNC);
      break;
    case SYNCE_DEVICE_PASSWORD_FLAG_CHECKING:
      prop_str = "checking";
      g_debug("%s: setting password flags checking", G_STRFUNC);
      break;
    case SYNCE_DEVICE_PASSWORD_FLAG_UNLOCKED:
      prop_str = "unlocked";
      g_debug("%s: setting password flags unlocked", G_STRFUNC);
      break;
    }

  return prop_str;
}


static gboolean
synce_device_get_name(SynceDbusDevice *interface,
		      GDBusMethodInvocation *invocation,
		      gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  synce_dbus_device_complete_get_name(interface, invocation, priv->name);

  return TRUE;
}

static gboolean
synce_device_get_platform_name(SynceDbusDevice *interface,
			       GDBusMethodInvocation *invocation,
			       gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  synce_dbus_device_complete_get_platform_name(interface, invocation, priv->platform_name);

  return TRUE;
}

static gboolean
synce_device_get_model_name(SynceDbusDevice *interface,
			    GDBusMethodInvocation *invocation,
			    gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  synce_dbus_device_complete_get_model_name(interface, invocation, priv->model_name);
  return TRUE;
}

static gboolean
synce_device_get_os_version(SynceDbusDevice *interface,
			    GDBusMethodInvocation *invocation,
			    gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  synce_dbus_device_complete_get_os_version(interface, invocation, priv->os_major, priv->os_minor);
  return TRUE;
}

static gboolean
synce_device_get_version(SynceDbusDevice *interface,
			 GDBusMethodInvocation *invocation,
			 gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  synce_dbus_device_complete_get_version(interface, invocation, priv->version);
  return TRUE;
}

static gboolean
synce_device_get_cpu_type(SynceDbusDevice *interface,
			  GDBusMethodInvocation *invocation,
			  gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  synce_dbus_device_complete_get_cpu_type(interface, invocation, priv->cpu_type);
  return TRUE;
}

static gboolean
synce_device_get_ip_address(SynceDbusDevice *interface,
			    GDBusMethodInvocation *invocation,
			    gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  synce_dbus_device_complete_get_ip_address(interface, invocation, priv->ip_address);
  return TRUE;
}

static gboolean
synce_device_get_iface_address(SynceDbusDevice *interface,
			       GDBusMethodInvocation *invocation,
			       gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  synce_dbus_device_complete_get_iface_address(interface, invocation, priv->iface_address);
  return TRUE;
}

static gboolean
synce_device_get_guid(SynceDbusDevice *interface,
		      GDBusMethodInvocation *invocation,
		      gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  synce_dbus_device_complete_get_guid(interface, invocation, priv->guid);
  return TRUE;
}

static gboolean
synce_device_get_current_partner_id(SynceDbusDevice *interface,
				    GDBusMethodInvocation *invocation,
				    gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  synce_dbus_device_complete_get_current_partner_id(interface, invocation, priv->cur_partner_id);
  return TRUE;
}

static gboolean
synce_device_get_password_flags(SynceDbusDevice *interface,
				GDBusMethodInvocation *invocation,
				gpointer userdata)
{
  SynceDevice *self = SYNCE_DEVICE (userdata);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  synce_dbus_device_complete_get_password_flags(interface, invocation, get_password_flag_text(priv->pw_flags));
  return TRUE;
}


void
synce_device_dbus_init(SynceDevice *self)
{
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);
  g_return_if_fail(!(priv->dispose_has_run));

  GError *error = NULL;
  gchar *safe_path = NULL;
  gchar *obj_path = NULL;
  GDBusConnection *system_bus = NULL;
  const gchar safe_chars[] = {
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789_"
  };

  gchar *guid = NULL, *name = NULL, *platform_name = NULL, *model_name = NULL,
    *ip_addr = NULL, *iface_addr = NULL;
  guint os_major, os_minor, version, cpu_type, cur_partner_id;
  SynceDevicePasswordFlags pw_flags;
  
  safe_path = g_strdup (priv->device_path);
  g_strcanon (safe_path, safe_chars, '_');
  obj_path = g_strdup_printf (DEVICE_BASE_OBJECT_PATH "/%s", safe_path);
  g_free (safe_path);

  g_message ("%s: registering object path '%s'", G_STRFUNC, obj_path);

  priv->interface = synce_dbus_device_skeleton_new();

  g_object_get(self,
	       "guid", &guid,
	       "os-major", &os_major,
	       "os-minor", &os_minor,
	       "name", &name,
	       "version", &version,
	       "cpu-type", &cpu_type,
	       "current-partner-id", &cur_partner_id,
	       "platform-name", &platform_name,
	       "model-name", &model_name,
	       "password-flags", &pw_flags,
	       "ip-address", &ip_addr,
	       "iface-address", &iface_addr,
	       NULL);

  g_object_set(priv->interface,
	       "guid", guid,
	       "os-major-version", os_major,
	       "os-minor-version", os_minor,
	       "name", name,
	       "version", version,
	       "cpu-type", cpu_type,
	       "current-partner-id", cur_partner_id,
	       "platform-name", platform_name,
	       "model-name", model_name,
	       "ip-address", ip_addr,
	       "iface-address", iface_addr,
	       NULL);
  synce_dbus_device_set_password_flags (priv->interface, get_password_flag_text(pw_flags));

  g_free(guid);
  g_free(name);
  g_free(platform_name);
  g_free(model_name);
  g_free(ip_addr);
  g_free(iface_addr);

  g_signal_connect(priv->interface,
		   "handle-get-name",
		   G_CALLBACK (synce_device_get_name),
		   self);

  g_signal_connect(priv->interface,
		   "handle-get-platform-name",
		   G_CALLBACK (synce_device_get_platform_name),
		   self);

  g_signal_connect(priv->interface,
		   "handle-get-model-name",
		   G_CALLBACK (synce_device_get_model_name),
		   self);

  g_signal_connect(priv->interface,
		   "handle-get-os-version",
		   G_CALLBACK (synce_device_get_os_version),
		   self);

  g_signal_connect(priv->interface,
		   "handle-get-version",
		   G_CALLBACK (synce_device_get_version),
		   self);

  g_signal_connect(priv->interface,
		   "handle-get-cpu-type",
		   G_CALLBACK (synce_device_get_cpu_type),
		   self);

  g_signal_connect(priv->interface,
		   "handle-get-ip-address",
		   G_CALLBACK (synce_device_get_ip_address),
		   self);

  g_signal_connect(priv->interface,
		   "handle-get-iface-address",
		   G_CALLBACK (synce_device_get_iface_address),
		   self);

  g_signal_connect(priv->interface,
		   "handle-get-guid",
		   G_CALLBACK (synce_device_get_guid),
		   self);

  g_signal_connect(priv->interface,
		   "handle-get-current-partner-id",
		   G_CALLBACK (synce_device_get_current_partner_id),
		   self);

  g_signal_connect(priv->interface,
		   "handle-get-password-flags",
		   G_CALLBACK (synce_device_get_password_flags),
		   self);

  g_signal_connect(priv->interface,
		   "handle-provide-password",
		   G_CALLBACK (synce_device_provide_password),
		   self);

  g_signal_connect(priv->interface,
		   "handle-request-connection",
		   G_CALLBACK (synce_device_request_connection),
		   self);

  system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
  if (system_bus == NULL) {
    g_critical("%s: Failed to connect to system bus: %s", G_STRFUNC, error->message);
    g_error_free(error);
    g_free(obj_path);
    return;
  }
  if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(priv->interface),
					system_bus,
                                        obj_path,
					&error)) {
    g_critical("%s: Failed to export interface on system bus: %s", G_STRFUNC, error->message);
    g_error_free(error);
    g_object_unref(system_bus);
    g_free(obj_path);
    return;
  }
  g_object_unref(system_bus);

  /* we set this through the property to emit notify::object-path */
  g_object_set (self, "object-path", obj_path, NULL);
  g_debug("%s: obj_path set to %s", G_STRFUNC, priv->obj_path);
  g_free(obj_path);

  return;
}

void
synce_device_change_password_flags (SynceDevice *self,
				    SynceDevicePasswordFlags new_flag)
{
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);
  g_return_if_fail(!(priv->dispose_has_run));

  const gchar *prop_str = NULL;

  g_object_set (self, "password-flags", new_flag, NULL);
  prop_str = get_password_flag_text(new_flag);
  g_signal_emit (self, SYNCE_DEVICE_GET_CLASS(SYNCE_DEVICE(self))->signals[SYNCE_DEVICE_SIGNAL_PASSWORD_FLAGS_CHANGED], 0, prop_str);
  synce_dbus_device_set_password_flags (priv->interface, prop_str);
  synce_dbus_device_emit_password_flags_changed(priv->interface, prop_str);

  return;
}

void
synce_device_conn_broker_done_cb (SynceConnectionBroker *broker,
				  gpointer user_data)
{
  SynceDevice *self = SYNCE_DEVICE (user_data);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);
  g_return_if_fail(!(priv->dispose_has_run));
  guint id;

  g_object_get (broker, "id", &id, NULL);

  g_hash_table_remove (priv->requests, &id);
}


/* class functions */


static void
synce_device_init (SynceDevice *self)
{
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  priv->dispose_has_run = FALSE;

  priv->state = CTRL_STATE_HANDSHAKE;
  priv->pw_flags = SYNCE_DEVICE_PASSWORD_FLAG_UNSET;
  priv->info_buf_size = -1;

  priv->req_id = 0;
  priv->requests = g_hash_table_new_full(g_int_hash,
					 g_int_equal,
					 g_free,
					 g_object_unref);

  priv->conn = NULL;
  priv->iobuf = NULL;
  priv->device_path = NULL;
  priv->guid = NULL;
  priv->name = NULL;
  priv->platform_name = NULL;
  priv->model_name = NULL;
  priv->ip_address = NULL;
  priv->iface_address = NULL;
  priv->os_major = 0;
  priv->os_minor = 0;
  priv->version = 0;
  priv->cpu_type = 0;
  priv->cur_partner_id = 0;
  priv->id = 0;
  priv->pw_key = 0;
  priv->pw_ctx = NULL;

  priv->obj_path = NULL;
  priv->interface = NULL;

  return;
}


static void
synce_device_dispose (GObject *obj)
{
  SynceDevice *self = SYNCE_DEVICE (obj);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  if (priv->interface) {
    g_dbus_interface_skeleton_unexport(G_DBUS_INTERFACE_SKELETON(priv->interface));
    g_object_unref(priv->interface);
    priv->interface = NULL;
  }

  g_io_stream_close(G_IO_STREAM(priv->conn), NULL, NULL);
  g_object_unref(priv->conn);

  g_hash_table_destroy (priv->requests);

  if (G_OBJECT_CLASS (synce_device_parent_class)->dispose)
    G_OBJECT_CLASS (synce_device_parent_class)->dispose (obj);
}

static void
synce_device_finalize (GObject *obj)
{
  SynceDevice *self = SYNCE_DEVICE (obj);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  g_free (priv->device_path);

  g_free (priv->guid);
  g_free (priv->name);
  g_free (priv->platform_name);
  g_free (priv->model_name);
  g_free (priv->obj_path);
  g_free (priv->ip_address);
  g_free (priv->iface_address);
  G_OBJECT_CLASS (synce_device_parent_class)->finalize (obj);
}

static void
synce_device_get_property (GObject    *obj,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  SynceDevice *self = SYNCE_DEVICE (obj);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  switch (property_id) {
  case PROP_CONNECTION:
    g_value_set_pointer (value, priv->conn);
    break;
  case PROP_DEVICE_PATH:
    g_value_set_string (value, priv->device_path);
    break;
  case PROP_OBJ_PATH:
    g_value_set_string (value, priv->obj_path);
    break;
  case PROP_IP_ADDRESS:
    g_value_set_string (value, priv->ip_address);
    break;
  case PROP_IFACE_ADDRESS:
    g_value_set_string (value, priv->iface_address);
    break;
  case PROP_GUID:
    g_value_set_string (value, priv->guid);
    break;
  case PROP_OS_MAJOR:
    g_value_set_uint (value, priv->os_major);
    break;
  case PROP_OS_MINOR:
    g_value_set_uint (value, priv->os_minor);
    break;
  case PROP_NAME:
    g_value_set_string (value, priv->name);
    break;
  case PROP_VERSION:
    g_value_set_uint (value, priv->version);
    break;
  case PROP_CPU_TYPE:
    g_value_set_uint (value, priv->cpu_type);
    break;
  case PROP_CURRENT_PARTNER_ID:
    g_value_set_uint (value, priv->cur_partner_id);
    break;
  case PROP_ID:
    g_value_set_uint (value, priv->id);
    break;
  case PROP_PLATFORM_NAME:
    g_value_set_string (value, priv->platform_name);
    break;
  case PROP_MODEL_NAME:
    g_value_set_string (value, priv->model_name);
    break;
  case PROP_PASSWORD_FLAGS:
    g_value_set_uint (value, priv->pw_flags);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
    break;
  }
}

static void
synce_device_set_property (GObject      *obj,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  SynceDevice *self = SYNCE_DEVICE (obj);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  switch (property_id) {
  case PROP_CONNECTION:
    if (priv->conn != NULL)
      {
	g_object_unref (priv->conn);
      }

    priv->conn = g_value_get_pointer (value);
    g_object_ref (priv->conn);

    GInputStream *in_stream = g_io_stream_get_input_stream(G_IO_STREAM(priv->conn));
    priv->iobuf = g_malloc(sizeof (guint32));
    g_input_stream_read_async(in_stream, priv->iobuf, sizeof (guint32), G_PRIORITY_DEFAULT, NULL, synce_device_conn_event_cb, g_object_ref(self));

    GInetSocketAddress *address = NULL;
    GInetAddress *inet_address = NULL;
    address = G_INET_SOCKET_ADDRESS(g_socket_connection_get_remote_address(priv->conn, NULL));
    inet_address = g_inet_socket_address_get_address(address);
    priv->ip_address =  g_inet_address_to_string(inet_address);
    g_object_unref(address);

    address = G_INET_SOCKET_ADDRESS(g_socket_connection_get_local_address(priv->conn, NULL));
    inet_address = g_inet_socket_address_get_address(address);
    priv->iface_address = g_inet_address_to_string(inet_address);
    g_object_unref(address);

    break;
  case PROP_DEVICE_PATH:
    g_free (priv->device_path);
    priv->device_path = g_value_dup_string (value);
    g_debug("%s: running for device %s", G_STRFUNC, priv->device_path);

    break;
  case PROP_OBJ_PATH:
    g_free (priv->obj_path);
    priv->obj_path = g_value_dup_string (value);
    break;
  case PROP_GUID:
    g_free (priv->guid);
    priv->guid = g_value_dup_string (value);
    break;
  case PROP_OS_MAJOR:
    priv->os_major = g_value_get_uint (value);
    break;
  case PROP_OS_MINOR:
    priv->os_minor = g_value_get_uint (value);
    break;
  case PROP_NAME:
    g_free (priv->name);
    priv->name = g_value_dup_string (value);
    break;
  case PROP_VERSION:
    priv->version = g_value_get_uint (value);
    break;
  case PROP_CPU_TYPE:
    priv->cpu_type = g_value_get_uint (value);
    break;
  case PROP_CURRENT_PARTNER_ID:
    priv->cur_partner_id = g_value_get_uint (value);
    break;
  case PROP_ID:
    priv->id = g_value_get_uint (value);
    break;
  case PROP_PLATFORM_NAME:
    g_free (priv->platform_name);
    priv->platform_name = g_value_dup_string (value);
    break;
  case PROP_MODEL_NAME:
    g_free (priv->model_name);
    priv->model_name = g_value_dup_string (value);
    break;
  case PROP_PASSWORD_FLAGS:
    priv->pw_flags = g_value_get_uint (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
    break;
  }
}


static void
synce_device_class_init (SynceDeviceClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GParamSpec *param_spec;

  g_type_class_add_private (klass, sizeof (SynceDevicePrivate));

  obj_class->get_property = synce_device_get_property;
  obj_class->set_property = synce_device_set_property;

  obj_class->dispose = synce_device_dispose;
  obj_class->finalize = synce_device_finalize;

  klass->synce_device_conn_event_cb = NULL;
  klass->synce_device_request_connection = NULL;
  klass->synce_device_provide_password = synce_device_provide_password_impl;

  param_spec = g_param_spec_pointer ("connection", "Connection object",
                                     "GSocketConnection object.",
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_NICK |
                                     G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_CONNECTION, param_spec);

  param_spec = g_param_spec_string ("device-path", "Device path",
				    "Sysfs path to the device.",
				    NULL,
				    G_PARAM_CONSTRUCT_ONLY |
				    G_PARAM_READWRITE |
				    G_PARAM_STATIC_NICK |
				    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_DEVICE_PATH, param_spec);

  param_spec = g_param_spec_string ("object-path", "DBus object path",
				    "The device' object path on DBus.",
				    NULL,
				    G_PARAM_READWRITE |
				    G_PARAM_STATIC_NICK |
				    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_OBJ_PATH, param_spec);

  param_spec = g_param_spec_string ("ip-address", "IP address",
				    "The device' IP address.",
				    NULL,
				    G_PARAM_READABLE |
				    G_PARAM_STATIC_NICK |
				    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_IP_ADDRESS, param_spec);

  param_spec = g_param_spec_string ("iface-address", "Interface IP address",
				    "The host's interface IP address.",
				    NULL,
				    G_PARAM_READABLE |
				    G_PARAM_STATIC_NICK |
				    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_IFACE_ADDRESS, param_spec);

  param_spec = g_param_spec_string ("guid", "Device GUID",
                                    "The device' unique ID.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_GUID, param_spec);

  param_spec = g_param_spec_uint ("os-major", "OS major version",
                                  "The device' OS major version.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_OS_MAJOR, param_spec);

  param_spec = g_param_spec_uint ("os-minor", "OS minor version",
                                  "The device' OS minor version.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_OS_MINOR, param_spec);

  param_spec = g_param_spec_string ("name", "Device name",
                                    "The device' name.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_NAME, param_spec);

  param_spec = g_param_spec_uint ("version", "Device version",
                                  "The device' version.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_VERSION, param_spec);

  param_spec = g_param_spec_uint ("cpu-type", "Device CPU type",
                                  "The device' CPU type.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_CPU_TYPE, param_spec);

  param_spec = g_param_spec_uint ("current-partner-id", "Current partner id",
                                  "The device' current partner id.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_CURRENT_PARTNER_ID, param_spec);

  param_spec = g_param_spec_uint ("id", "Device id",
                                  "The device' id.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_ID, param_spec);

  param_spec = g_param_spec_string ("platform-name", "Platform name",
                                    "The device' platform name.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_PLATFORM_NAME, param_spec);

  param_spec = g_param_spec_string ("model-name", "Model name",
                                    "The device' model name.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_MODEL_NAME, param_spec);

  param_spec = g_param_spec_uint ("password-flags", "Password flags",
                                  "The current password flags.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_PASSWORD_FLAGS, param_spec);

  klass->signals[SYNCE_DEVICE_SIGNAL_PASSWORD_FLAGS_CHANGED] =
    g_signal_new ("password-flags-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  klass->signals[SYNCE_DEVICE_SIGNAL_DISCONNECTED] =
    g_signal_new ("disconnected",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}


