#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnet.h>
#include <libhal.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <synce.h>

#include "synce-device.h"
#include "synce-device-internal.h"
#include "synce-device-signals-marshal.h"
#include "synce-device-glue.h"
#include "synce-errors.h"

G_DEFINE_TYPE (SynceDevice, synce_device, G_TYPE_OBJECT)

/* properties */
enum
{
  PROP_CONNECTION = 1,
  PROP_IP_ADDRESS,

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

void
synce_device_request_connection (SynceDevice *self, DBusGMethodInvocation *ctx)
{
  SYNCE_DEVICE_GET_CLASS(self)->synce_device_request_connection (self, ctx);
}

static void
synce_device_conn_event_cb (GConn *conn, GConnEvent *event, gpointer user_data)
{
  SYNCE_DEVICE_GET_CLASS(user_data)->synce_device_conn_event_cb (conn, event, user_data);
}

void
synce_device_provide_password (SynceDevice *self, const gchar *password, DBusGMethodInvocation *ctx)
{
  SYNCE_DEVICE_GET_CLASS(self)->synce_device_provide_password (self, password, ctx);
}


/* standard functions */

static void
synce_device_provide_password_impl (SynceDevice *self,
				    const gchar *password,
				    DBusGMethodInvocation *ctx)
{
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE(self);
  GError *error = NULL;
  guchar *buf;
  guint16 buf_size;
  guint i;

  if (priv->state != CTRL_STATE_AUTH ||
      (priv->pw_flags & SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE) == 0)
    {
      error = g_error_new (SYNCE_ERRORS, NotAvailable,
			   "No password expected in the current state.");
      goto OUT;
    }
  else if (priv->pw_ctx != NULL)
    {
      error = g_error_new (SYNCE_ERRORS, NotAvailable,
			   "An authentication attempt is still in progress.");
      goto OUT;
    }

  priv->pw_ctx = ctx;

  buf = (guchar *) wstr_from_utf8 (password);
  buf_size = wstrlen ((LPCWSTR) buf) * sizeof (WCHAR);

  for (i = 0; i < buf_size; i++)
    {
      buf[i] ^= priv->pw_key;
    }

  buf_size = GUINT16_TO_LE (buf_size);
  gnet_conn_write (priv->conn, (gchar *) &buf_size, sizeof (buf_size));
  gnet_conn_write (priv->conn, (gchar *) buf, buf_size);
  gnet_conn_readn (priv->conn, sizeof (guint16));

  synce_device_change_password_flags (self, SYNCE_DEVICE_PASSWORD_FLAG_CHECKING);

 OUT:
  if (error != NULL)
    dbus_g_method_return_error (ctx, error);
}

void
synce_device_set_hal_props(SynceDevice *self)
{
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  GInetAddr *device_inetaddr;
  gchar *ip_bytes;
  gchar *ip_str;

  gchar *prop_name;
  DBusError error;
  dbus_bool_t result;

  dbus_error_init(&error);

  prop_name = "pda.pocketpc.name";
  result = libhal_device_set_property_string(priv->hal_ctx,
					     priv->udi,
					     prop_name,
					     priv->name,
					     &error);
  if (!result) {
    g_critical("%s: failed to set property %s: %s: %s", G_STRFUNC, prop_name, error.name, error.message);
    dbus_error_free(&error);
  }

  prop_name = "pda.pocketpc.platform";
  result = libhal_device_set_property_string(priv->hal_ctx,
					     priv->udi,
					     prop_name,
					     priv->platform_name,
					     &error);
  if (!result) {
    g_critical("%s: failed to set property %s: %s: %s", G_STRFUNC, prop_name, error.name, error.message);
    dbus_error_free(&error);
  }

  prop_name = "pda.pocketpc.model";
  result = libhal_device_set_property_string(priv->hal_ctx,
					     priv->udi,
					     prop_name,
					     priv->model_name,
					     &error);
  if (!result) {
    g_critical("%s: failed to set property %s: %s: %s", G_STRFUNC, prop_name, error.name, error.message);
    dbus_error_free(&error);
  }

  prop_name = "pda.pocketpc.os_major";
  result = libhal_device_set_property_uint64(priv->hal_ctx,
					     priv->udi,
					     prop_name,
					     priv->os_major,
					     &error);
  if (!result) {
    g_critical("%s: failed to set property %s: %s: %s", G_STRFUNC, prop_name, error.name, error.message);
    dbus_error_free(&error);
  }

  prop_name = "pda.pocketpc.os_minor";
  result = libhal_device_set_property_uint64(priv->hal_ctx,
					     priv->udi,
					     prop_name,
					     priv->os_minor,
					     &error);
  if (!result) {
    g_critical("%s: failed to set property %s: %s: %s", G_STRFUNC, prop_name, error.name, error.message);
    dbus_error_free(&error);
  }

  prop_name = "pda.pocketpc.version";
  result = libhal_device_set_property_uint64(priv->hal_ctx,
					     priv->udi,
					     prop_name,
					     priv->version,
					     &error);
  if (!result) {
    g_critical("%s: failed to set property %s: %s: %s", G_STRFUNC, prop_name, error.name, error.message);
    dbus_error_free(&error);
  }

  prop_name = "pda.pocketpc.cpu_type";
  result = libhal_device_set_property_uint64(priv->hal_ctx,
					     priv->udi,
					     prop_name,
					     priv->cpu_type,
					     &error);
  if (!result) {
    g_critical("%s: failed to set property %s: %s: %s", G_STRFUNC, prop_name, error.name, error.message);
    dbus_error_free(&error);
  }

  prop_name = "pda.pocketpc.current_partner_id";
  result = libhal_device_set_property_uint64(priv->hal_ctx,
					     priv->udi,
					     prop_name,
					     priv->cur_partner_id,
					     &error);
  if (!result) {
    g_critical("%s: failed to set property %s: %s: %s", G_STRFUNC, prop_name, error.name, error.message);
    dbus_error_free(&error);
  }

  prop_name = "pda.pocketpc.guid";
  result = libhal_device_set_property_string(priv->hal_ctx,
					     priv->udi,
					     prop_name,
					     priv->guid,
					     &error);
  if (!result) {
    g_critical("%s: failed to set property %s: %s: %s", G_STRFUNC, prop_name, error.name, error.message);
    dbus_error_free(&error);
  }

  device_inetaddr = gnet_tcp_socket_get_remote_inetaddr (priv->conn->socket);
  ip_bytes = g_malloc(gnet_inetaddr_get_length(device_inetaddr));
  gnet_inetaddr_get_bytes (device_inetaddr, ip_bytes);
  ip_str = g_strdup_printf("%u.%u.%u.%u", (guint8)ip_bytes[0], (guint8)ip_bytes[1], (guint8)ip_bytes[2], (guint8)ip_bytes[3]);
  g_free(ip_bytes);

  prop_name = "pda.pocketpc.ip_address";
  result = libhal_device_set_property_string(priv->hal_ctx,
					     priv->udi,
					     prop_name,
					     ip_str,
					     &error);
  if (!result) {
    g_critical("%s: failed to set property %s: %s: %s", G_STRFUNC, prop_name, error.name, error.message);
    dbus_error_free(&error);
  }
  g_free(ip_str);

  /* register object on hal dbus connection */

  dbus_g_connection_register_g_object (priv->hal_bus,
                                       priv->udi,
				       G_OBJECT (self));

  result = libhal_device_claim_interface(priv->hal_ctx,
					 priv->udi,
					 "org.freedesktop.Hal.Device.Synce",
					 "<method name=\"ProvidePassword\">"
					 "  <annotation name=\"org.freedesktop.DBus.GLib.Async\" value=\"\"/>"
					 "  <arg direction=\"in\" name=\"password\" type=\"s\"/>"
					 "  <arg direction=\"out\" type=\"b\"/>"
					 "</method>"
					 "<method name=\"RequestConnection\">"
					 "  <annotation name=\"org.freedesktop.DBus.GLib.Async\" value=\"\"/>"
					 "  <arg direction=\"out\" name=\"unix_socket_path\" type=\"s\"/>"
					 "</method>",
					 &error);
  if (!result) {
    g_critical("%s: failed to claim dbus interface: %s: %s", G_STRFUNC, error.name, error.message);
    dbus_error_free(&error);
  }

  dbus_error_free(&error);
}

void
synce_device_change_password_flags (SynceDevice *self,
				    SynceDevicePasswordFlags new_flag)
{
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);
  gchar *prop_str = NULL;
  DBusError dbus_error;

  g_object_set (self, "password-flags", new_flag, NULL);

  dbus_error_init(&dbus_error);

  switch (new_flag)
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

  if (!(libhal_device_set_property_string(priv->hal_ctx,
					  priv->udi,
					  "pda.pocketpc.password",
					  prop_str,
					  &dbus_error)))
    {
      g_critical("%s: failed to set property \"pda.pocketpc.password\": %s: %s", G_STRFUNC, dbus_error.name, dbus_error.message);
      dbus_error_free(&dbus_error);
    }
  return;
}

void
synce_device_conn_broker_done_cb (SynceConnectionBroker *broker,
				  gpointer user_data)
{
  SynceDevice *self = SYNCE_DEVICE (user_data);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);
  guint id;

  g_object_get (broker, "id", &id, NULL);

  g_hash_table_remove (priv->requests, &id);
}


/* class functions */

static void
synce_device_init (SynceDevice *self)
{
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  priv->udi = g_strdup(g_getenv("HAL_PROP_INFO_UDI"));
  g_debug("%s: running for udi %s", G_STRFUNC, priv->udi);

  priv->state = CTRL_STATE_HANDSHAKE;
  priv->pw_flags = SYNCE_DEVICE_PASSWORD_FLAG_UNSET;
  priv->info_buf_size = -1;

  priv->requests = g_hash_table_new_full(g_int_hash,
					 g_int_equal,
					 g_free,
					 g_object_unref);

  /* hal context setup */

  DBusError dbus_error;
  dbus_error_init(&dbus_error);

  priv->hal_ctx = libhal_ctx_init_direct(&dbus_error);
  if (!(priv->hal_ctx)) {
    g_critical("%s: failed to initialize direct hal context: %s: %s", G_STRFUNC, dbus_error.name, dbus_error.message);
    dbus_error_free (&dbus_error);
    goto exit;
  }

  dbus_connection_setup_with_g_main(libhal_ctx_get_dbus_connection(priv->hal_ctx), NULL);
  priv->hal_bus = dbus_connection_get_g_connection(libhal_ctx_get_dbus_connection(priv->hal_ctx));

  if (!libhal_ctx_set_user_data (priv->hal_ctx, self)) {
    g_critical("%s: failed to set user data for hal_ctx", G_STRFUNC);
    goto exit;
  }

exit:

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

  gnet_conn_unref (priv->conn);
  if (priv->hal_ctx) {
    libhal_ctx_shutdown(priv->hal_ctx, NULL);
    libhal_ctx_free(priv->hal_ctx);
  }

  g_hash_table_destroy (priv->requests);

  if (G_OBJECT_CLASS (synce_device_parent_class)->dispose)
    G_OBJECT_CLASS (synce_device_parent_class)->dispose (obj);
}

static void
synce_device_finalize (GObject *obj)
{
  SynceDevice *self = SYNCE_DEVICE (obj);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);

  g_free (priv->udi);

  g_free (priv->guid);
  g_free (priv->name);
  g_free (priv->platform_name);
  g_free (priv->model_name);

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
  guint32 addr;

  switch (property_id) {
  case PROP_CONNECTION:
    g_value_set_pointer (value, priv->conn);
    break;
  case PROP_IP_ADDRESS:
    gnet_inetaddr_get_bytes (gnet_tcp_socket_get_remote_inetaddr (priv->conn->socket),
			     (gchar *) &addr);
    g_value_set_uint (value, addr);
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
	gnet_conn_unref (priv->conn);
      }

    priv->conn = g_value_get_pointer (value);
    gnet_conn_ref (priv->conn);

    gnet_conn_set_callback(priv->conn, synce_device_conn_event_cb, self);
    gnet_conn_readn(priv->conn, sizeof (guint32));

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

  param_spec = g_param_spec_pointer ("connection", "GConn object",
                                     "GConn object.",
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_NICK |
                                     G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_CONNECTION, param_spec);

  param_spec = g_param_spec_uint ("ip-address", "IP address",
                                  "The device' IP address.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READABLE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_IP_ADDRESS, param_spec);

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
                  synce_device_marshal_VOID__UINT_UINT,
                  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

  klass->signals[SYNCE_DEVICE_SIGNAL_DISCONNECTED] =
    g_signal_new ("disconnected",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                   &dbus_glib_synce_device_object_info);
}


