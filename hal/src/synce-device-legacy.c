#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dbus/dbus-glib.h>
#include <synce.h>
#include <arpa/inet.h>
#include <string.h>

#include "synce-device.h"
#include "synce-device-internal.h"
#include "synce-device-legacy.h"
#include "synce-connection-broker.h"
#include "synce-errors.h"
#include "utils.h"

G_DEFINE_TYPE(SynceDeviceLegacy, synce_device_legacy, SYNCE_TYPE_DEVICE)

typedef struct _SynceDeviceLegacyPrivate SynceDeviceLegacyPrivate;
struct _SynceDeviceLegacyPrivate {
  gint ping_count;
  gchar *password;
};

#define SYNCE_DEVICE_LEGACY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SYNCE_TYPE_DEVICE_LEGACY, SynceDeviceLegacyPrivate))


#define DCCM_PORT               5679
#define DCCM_PING               0x12345678
#define DCCM_PING_INTERVAL      5       /* seconds */
#define DCCM_MAX_PING_COUNT     3       /* max number of pings without reply */
#define DCCM_MAX_PACKET_SIZE    512
#define DCCM_MIN_PACKET_SIZE    0x24
#define RAPI_PORT               990


/* methods */

static gboolean
synce_device_legacy_send_ping(gpointer data)
{
  if (!(SYNCE_IS_DEVICE_LEGACY(data)))
    return FALSE;

  SynceDeviceLegacy *self = SYNCE_DEVICE_LEGACY (data);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (SYNCE_DEVICE(self));
  SynceDeviceLegacyPrivate *priv_legacy = SYNCE_DEVICE_LEGACY_GET_PRIVATE (self);

  guint32 ping = GUINT32_TO_LE(DCCM_PING);
  gnet_conn_write(priv->conn, (gchar *) &ping, sizeof(ping));

  if (++priv_legacy->ping_count == DCCM_MAX_PING_COUNT) {
    gchar *name = NULL;

    g_object_get(self, "name", &name, NULL);

    g_warning("%s: Device %s not responded to %d pings, assume disconnected", G_STRFUNC, name, DCCM_MAX_PING_COUNT);
    g_signal_emit (self, SYNCE_DEVICE_GET_CLASS(SYNCE_DEVICE(self))->signals[SYNCE_DEVICE_SIGNAL_DISCONNECTED], 0);
    g_free(name);
    return FALSE;
  }

  return TRUE;
}

static void
synce_device_legacy_info_received (SynceDeviceLegacy *self, const guchar *buf, gint length)
{
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (SYNCE_DEVICE(self));

  gchar *guid = NULL, *name = NULL, *platform_name = NULL, *model_name = NULL;
  guint os_major = 0, os_minor = 0, version = 0, cpu_type = 0, cur_partner_id = 0, id = 0;
  const guchar *p = buf, *end_ptr = buf + length;
  DBusError error;

  dbus_error_init(&error);

  priv->state = CTRL_STATE_GOT_INFO;

  g_debug("%s", G_STRFUNC);
  synce_print_hexdump (buf, length);

  /*
   * Parse and set device properties.
   */

  if (p + 24 > end_ptr)
    {
      g_warning ("%s: short read for initial information",
		 G_STRFUNC);
      goto ERROR;
    }

  /* skip first 4 bytes */
  p += sizeof(guint32);

  os_major = *((guint8 *) p);
  p += sizeof (guint8);

  os_minor = *((guint8 *) p);
  p += sizeof (guint8);

  /* skip build number ? */
  p += sizeof (guint16);

  cpu_type = GUINT16_FROM_LE (*((guint16 *) p));
  p += sizeof (guint16);

  /* skip 2 bytes */
  p += sizeof (guint16);
  /* skip 4 bytes */
  p += sizeof(guint32);

  cur_partner_id = GUINT32_FROM_LE (*((guint32 *) p));
  p += sizeof (guint32);

  /* second partner id */
  p += sizeof (guint32);

  /* string information */

  name = synce_rapi_unicode_string_to_string_at_offset (buf, p, end_ptr);
  if (name == NULL)
    {
      g_warning ("%s: DeviceName is out of bounds or too long", G_STRFUNC);
      goto ERROR;
    }
  p += sizeof (guint32);

  platform_name = synce_rapi_unicode_string_to_string_at_offset (buf, p, end_ptr);
  if (platform_name == NULL)
    {
      g_warning ("%s: PlatformName is out of bounds or too long", G_STRFUNC);
      goto ERROR;
    }
  p += sizeof (guint32);

  model_name = synce_rapi_unicode_string_to_string_at_offset (buf, p, end_ptr);
  if (model_name == NULL)
    {
      g_warning ("%s: ModelName is out of bounds or too long", G_STRFUNC);
      goto ERROR;
    }

  /* what are version, id ? */

  /* we dont have guid, we'll use ip address */
  guint32 addr;
  g_object_get(self, "ip-address", &addr, NULL);

  struct in_addr tmp_addr;
  tmp_addr.s_addr = addr;
  gchar *str_addr = inet_ntoa(tmp_addr);

  g_object_set (self,
		"guid", str_addr,
		"os-major", os_major,
		"os-minor", os_minor,
		"name", name,
		"version", version,
		"cpu-type", cpu_type,
		"current-partner-id", cur_partner_id,
		"id", id,
		"platform-name", platform_name,
		"model-name", model_name,
		NULL);

  synce_device_set_hal_props(SYNCE_DEVICE(self));

  if (priv->pw_key != 0)
    {
      priv->state = CTRL_STATE_AUTH;
      synce_device_change_password_flags (SYNCE_DEVICE(self), SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE);
    }
  else
    {
      synce_device_change_password_flags (SYNCE_DEVICE(self), SYNCE_DEVICE_PASSWORD_FLAG_UNSET);
      g_debug("%s: setting CTRL_STATE_CONNECTED", G_STRFUNC);
      priv->state = CTRL_STATE_CONNECTED;
      g_timeout_add((DCCM_PING_INTERVAL * 1000), synce_device_legacy_send_ping, self);
    }

  /* tell hal device is ready */

  g_debug("%s: notify hal that device is ready to be advertised", G_STRFUNC);

  if (!(libhal_device_addon_is_ready(priv->hal_ctx,
				     priv->udi,
				     &error))) {
    g_critical("%s: failed to notify hal that device is ready: %s: %s", G_STRFUNC, error.name, error.message);
    dbus_error_free(&error);
  }

  goto OUT;

 ERROR:
  /* TODO: do something sensible here */

 OUT:
  g_free (guid);
  g_free (name);
  g_free (platform_name);
  g_free (model_name);
}

static void
synce_device_legacy_conn_event_cb_impl (GConn *conn,
					GConnEvent *event,
					gpointer user_data)
{
  SynceDeviceLegacy *self = SYNCE_DEVICE_LEGACY (user_data);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (SYNCE_DEVICE(self));
  SynceDeviceLegacyPrivate *priv_legacy = SYNCE_DEVICE_LEGACY_GET_PRIVATE (self);

  if (priv->state < CTRL_STATE_GETTING_INFO)
    {
      /* CTRL_STATE_HANDSHAKE */
      if (event->type == GNET_CONN_READ)
        {
          guint32 header;

          if (event->length != 4)
            {
              g_warning ("%s: unexpected length", G_STRFUNC);
              return;
            }

          header = GUINT32_FROM_LE (*((guint32 *) event->buffer));

          if (header == 0) {
            /* empty packet header */
            gnet_conn_readn (conn, 4);
            return;
          } else if (header < DCCM_MAX_PACKET_SIZE) {
            if (header < DCCM_MIN_PACKET_SIZE) {
              g_warning ("%s: undersize packet", G_STRFUNC);
              return;
            }
            /* info message */
            priv->state = CTRL_STATE_GETTING_INFO;
            priv->info_buf_size = header;
            gnet_conn_readn (conn, priv->info_buf_size);
          } else {
            /* password challenge */
            priv->pw_key = header & 0xff;
	    /*
            synce_device_change_password_flags (SYNCE_DEVICE(self), SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE);
	    */
            gnet_conn_readn (conn, 4);
          }
        }
      else if (event->type == GNET_CONN_WRITE)
        {
          gnet_conn_readn (conn, 4);
        }
    }
  else if (priv->state < CTRL_STATE_AUTH)
    {
      /* CTRL_STATE_INFO */
      if (event->type == GNET_CONN_READ)
        {
          if (event->length != priv->info_buf_size)
            {
              g_warning ("%s: event->length=%d != info_buf_size=%d",
                         G_STRFUNC, event->length, priv->info_buf_size);
              return;
            }

          synce_device_legacy_info_received (self, (guchar *) event->buffer,
					     event->length);
        }
    }
  else if (priv->state == CTRL_STATE_AUTH)
    {
      /* CTRL_STATE_AUTH */
      if (event->type == GNET_CONN_READ)
        {
          guint16 result;

          if (event->length != sizeof (guint16))
            {
              g_warning ("%s: event->length != 2", G_STRFUNC);
              return;
            }

          result = GUINT16_FROM_LE (*((guint16 *) event->buffer));

          if (result != 0)
            {
              priv->state = CTRL_STATE_CONNECTED;
              synce_device_change_password_flags (SYNCE_DEVICE(self), SYNCE_DEVICE_PASSWORD_FLAG_UNLOCKED);
              g_timeout_add((DCCM_PING_INTERVAL * 1000), synce_device_legacy_send_ping, self);
            }
          else
            {
	      g_free(priv_legacy->password);
              synce_device_change_password_flags (SYNCE_DEVICE(self), SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE);
            }

          dbus_g_method_return (priv->pw_ctx, result != 0);

          priv->pw_ctx = NULL;
        }
    }
  else if (priv->state == CTRL_STATE_CONNECTED)
    {
      /* CTRL_STATE_CONNECTED */
      if (event->type == GNET_CONN_READ)
        {
          guint32 req;

          if (event->length != 4)
            {
              g_warning ("%s: unexpected length", G_STRFUNC);
              return;
            }
          req = GUINT32_FROM_LE (*((guint32 *) event->buffer));

          if (req == 0) {
            /* empty packet header */
            gnet_conn_readn (conn, 4);
            return;
          }
          if (req == DCCM_PING) {
	    /* received a ping */

            priv_legacy->ping_count = 0;
          } else {
            g_warning("%s: Received header, not a ping reply: %d", G_STRFUNC, req);
          }
        }
      else
        {
          gnet_conn_readn (conn, 4);
        }
    }
}

static void
synce_device_legacy_client_event_cb (GConn *conn,
				     GConnEvent *event,
				     gpointer user_data)
{
  if (event->type == GNET_CONN_CONNECT) {
    if (SYNCE_IS_CONNECTION_BROKER(user_data)) {
      _synce_connection_broker_take_connection (SYNCE_CONNECTION_BROKER(user_data), conn);
    } else {
      g_warning("%s: not passed a connection broker", G_STRFUNC);
    }
    return;
  }

  g_warning ("%s: unhandled event", G_STRFUNC);

  return;
}

static void
synce_device_legacy_client_event_password_cb (GConn *conn,
			  GConnEvent *event,
			  gpointer user_data)
{
  SynceConnectionBroker *broker = SYNCE_CONNECTION_BROKER (user_data);
  guint8 result;
  GError *error = NULL;
  DBusGMethodInvocation *ctx;

  if (event->type != GNET_CONN_READ)
    return;

  g_object_get(broker, "context", &ctx, NULL);

  if (event->length != sizeof (guint8))
    {
      g_warning ("%s: Failed to get password reply, event->length != 1", G_STRFUNC);
      error = g_error_new (SYNCE_ERRORS, NotAvailable,
                           "Failed to authenticate connection");
      goto OUT;
    }

  result = *((guint8 *) event->buffer);
  if (result != 0) {
    _synce_connection_broker_take_connection (broker, conn);

    gnet_conn_set_callback (conn, synce_device_legacy_client_event_cb, broker);
    return;
  } else {
    g_warning("%s: Password rejected", G_STRFUNC);
    error = g_error_new (SYNCE_ERRORS, NotAvailable,
                         "Failed to authenticate connection");
  }
 OUT:
  if (error != NULL)
    dbus_g_method_return_error (ctx, error);
  return;
}

void
synce_device_legacy_request_connection_impl (SynceDevice *self, DBusGMethodInvocation *ctx)
{
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);
  GError *error = NULL;
  SynceConnectionBroker *broker;
  GConn *rapi_conn;
  GInetAddr *rapi_inet_addr;

  if (priv->state != CTRL_STATE_CONNECTED)
    {
      error = g_error_new (SYNCE_ERRORS, NotAvailable,
			   (priv->pw_flags & SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE) ?
          "Not authenticated, you need to call ProvidePassword with the "
			   "correct password." : "Not yet connected.");
      goto OUT;
    }

  /* 
   * Create a local copy of the global req_id variable to avoid
   * the chances of race conditions
   */
  guint *req_id_local = (guint *) g_malloc (sizeof (guint));
  *req_id_local = ++(priv->req_id) ;

  broker = g_object_new (SYNCE_TYPE_CONNECTION_BROKER,
                         "id", *req_id_local,
                         "context", ctx,
                         NULL);

  g_hash_table_insert (priv->requests, req_id_local, broker);

  g_signal_connect (broker, "done", (GCallback) synce_device_conn_broker_done_cb, self);

  SynceDeviceLegacyPrivate *legacy_priv = SYNCE_DEVICE_LEGACY_GET_PRIVATE (SYNCE_DEVICE_LEGACY(self));

  rapi_inet_addr = gnet_inetaddr_clone(priv->conn->inetaddr);
  gnet_inetaddr_set_port(rapi_inet_addr, RAPI_PORT);

  if (legacy_priv->password && strlen(legacy_priv->password))
    {
      guchar *buf;
      guint16 buf_size;
      guint i;

      buf = (guchar *) wstr_from_utf8 (legacy_priv->password);
      buf_size = wstrlen ((LPCWSTR) buf) * sizeof (WCHAR);
      for (i = 0; i < buf_size; i++)
        {
          buf[i] ^= priv->pw_key;
        }

      buf_size = GUINT16_TO_LE (buf_size);

      rapi_conn = gnet_conn_new_inetaddr(rapi_inet_addr, synce_device_legacy_client_event_password_cb, broker);
      gnet_conn_connect(rapi_conn);

      gnet_conn_write (rapi_conn, (gchar *) &buf_size, sizeof (buf_size));
      gnet_conn_write (rapi_conn, (gchar *) buf, buf_size);

      gnet_conn_readn (rapi_conn, sizeof (guint8));

      goto OUT;
    }

  rapi_conn = gnet_conn_new_inetaddr(rapi_inet_addr, synce_device_legacy_client_event_cb, broker);
  gnet_conn_connect(rapi_conn);

 OUT:
  if (error != NULL)
    dbus_g_method_return_error (ctx, error);
}

static void
synce_device_legacy_provide_password_impl (SynceDevice *self,
				    const gchar *password,
				    DBusGMethodInvocation *ctx)
{
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (self);
  GError *error = NULL;

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

  SynceDeviceLegacyPrivate *priv_legacy = SYNCE_DEVICE_LEGACY_GET_PRIVATE (SYNCE_DEVICE_LEGACY(self));
  priv_legacy->password = g_strdup(password);

  (SYNCE_DEVICE_CLASS(synce_device_legacy_parent_class)->synce_device_provide_password) (SYNCE_DEVICE(self), password, ctx);

 OUT:
  if (error != NULL)
    dbus_g_method_return_error (ctx, error);
}


/* class & instance functions */

static void
synce_device_legacy_init(SynceDeviceLegacy *self)
{
  SynceDeviceLegacyPrivate *priv = SYNCE_DEVICE_LEGACY_GET_PRIVATE (self);

  priv->ping_count = 0;
  priv->password = NULL;
}

static void
synce_device_legacy_dispose (GObject *obj)
{
  SynceDeviceLegacy *self = SYNCE_DEVICE_LEGACY(obj);
  SynceDevicePrivate *parent_priv = SYNCE_DEVICE_GET_PRIVATE (SYNCE_DEVICE(self));

  if (parent_priv->dispose_has_run) {
    return;
  }

  /* unref other objects */

  if (G_OBJECT_CLASS (synce_device_legacy_parent_class)->dispose)
    G_OBJECT_CLASS (synce_device_legacy_parent_class)->dispose (obj);
}

static void
synce_device_legacy_finalize (GObject *obj)
{
  SynceDeviceLegacy *self = SYNCE_DEVICE_LEGACY(obj);
  SynceDeviceLegacyPrivate *priv = SYNCE_DEVICE_LEGACY_GET_PRIVATE (self);

  g_free(priv->password);

  if (G_OBJECT_CLASS (synce_device_legacy_parent_class)->finalize)
    G_OBJECT_CLASS (synce_device_legacy_parent_class)->finalize (obj);
}

static void
synce_device_legacy_class_init (SynceDeviceLegacyClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  SynceDeviceClass *synce_device_class = SYNCE_DEVICE_CLASS(klass);

  gobject_class->dispose = synce_device_legacy_dispose;
  gobject_class->finalize = synce_device_legacy_finalize;

  g_type_class_add_private (klass, sizeof (SynceDeviceLegacyPrivate));

  synce_device_class->synce_device_conn_event_cb = synce_device_legacy_conn_event_cb_impl;
  synce_device_class->synce_device_request_connection = synce_device_legacy_request_connection_impl;
  synce_device_class->synce_device_provide_password = synce_device_legacy_provide_password_impl;
}


