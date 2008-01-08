#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dbus/dbus-glib.h>
#include <synce.h>

#include "synce-device.h"
#include "synce-device-internal.h"
#include "synce-device-rndis.h"
#include "synce-connection-broker.h"
#include "synce-errors.h"
#include "utils.h"

G_DEFINE_TYPE(SynceDeviceRndis, synce_device_rndis, SYNCE_TYPE_DEVICE)


/* methods */


static void
synce_device_rndis_info_received(SynceDeviceRndis *self, const guchar *buf, gint length)
{
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (SYNCE_DEVICE(self));
  gchar *guid = NULL, *name = NULL, *platform_name = NULL, *model_name = NULL;
  guint os_major, os_minor, version, cpu_type, cur_partner_id, id, comp_count;
  const guchar *p = buf, *end_ptr = buf + length;
  guint consumed;

  priv->state = CTRL_STATE_GOT_INFO;

  g_debug("%s", G_STRFUNC);
  synce_print_hexdump (buf, length);

  /*
   * Parse and set device properties.
   */
  if (p + 24 > end_ptr)
    {
      g_warning ("%s: short read trying to read GUID/OsMajor/OsMinor",
		 G_STRFUNC);
      goto ERROR;
    }

  guid = synce_guid_to_string (p);
  p += 16;

  os_major = GUINT32_FROM_LE (*((guint32 *) p));
  p += sizeof (guint32);

  os_minor = GUINT32_FROM_LE (*((guint32 *) p));
  p += sizeof (guint32);

  name = synce_rapi_unicode_string_to_string (p, end_ptr, 31, &consumed);
  if (name == NULL)
    {
      g_warning ("%s: DeviceName is out of bounds or too long", G_STRFUNC);
      goto ERROR;
    }
  p += consumed + sizeof (WCHAR);

  if (p + 20 > end_ptr)
    {
      g_warning ("%s: short read trying to read Version/CpuType/"
		 "Flags/CurPartnerId/Id", G_STRFUNC);
      goto ERROR;
    }

  version = GUINT32_FROM_LE (*((guint32 *) p));
  p += sizeof (guint32);

  cpu_type = GUINT32_FROM_LE (*((guint32 *) p));
  p += sizeof (guint32);

  /* counter or flags? */
  p += sizeof (guint32);

  cur_partner_id = GUINT32_FROM_LE (*((guint32 *) p));
  p += sizeof (guint32);

  id = GUINT32_FROM_LE (*((guint32 *) p));
  p += sizeof (guint32);

  /* TODO: PlatformName is actually a list of strings,
   *       terminated with an extra NUL byte */
  platform_name = synce_rapi_ascii_string_to_string (p, end_ptr, 255,
						      &consumed);
  if (platform_name == NULL)
    {
      g_warning ("%s: PlatformName is out of bounds or too long", G_STRFUNC);
      goto ERROR;
    }
  p += consumed;

  model_name = synce_rapi_ascii_string_to_string (p, end_ptr, 255, &consumed);
  if (model_name == NULL)
    {
      g_warning ("%s: ModelName is out of bounds or too long", G_STRFUNC);
      goto ERROR;
    }
  p += consumed + 1;

  /* TODO: parse the platform component versions,
   *       for now we just ignore them */
  if (p + 4 > end_ptr)
    {
      g_warning ("%s: short read trying to read ComponentCount", G_STRFUNC);
      goto ERROR;
    }
  comp_count = GUINT32_FROM_LE (*((guint32 *) p));
  if (comp_count > 6)
    {
      g_warning ("%s: ComponentCount %d is out of range", G_STRFUNC, comp_count);
      goto ERROR;
    }
  p += sizeof (guint32) + (comp_count * 8);

  if (p + 4 > end_ptr)
    {
      g_warning ("%s: short read trying to read Components/PasswordKey",
		 G_STRFUNC);
      goto ERROR;
    }
  priv->pw_key = GUINT32_FROM_LE (*((guint32 *) p));
  p += sizeof (guint32);

  if (p < buf + length)
    {
      guint n;

      if (p + 4 > end_ptr)
        {
          g_warning ("%s: short read trying to read ExtraData size", G_STRFUNC);
          goto ERROR;
        }
      n = GUINT32_FROM_LE (*((guint32 *) p));
      if (p + n > end_ptr)
        {
          g_warning ("%s: short read trying to read ExtraData data", G_STRFUNC);
          goto ERROR;
        }

      g_debug ("extradata:");
      synce_print_hexdump (p, n);

      p += n;
    }

  g_object_set (self,
		"guid", guid,
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

  if (priv->pw_key != 0)
    {
      if (priv->pw_key != 0xffffffff)
        {
          priv->state = CTRL_STATE_AUTH;
          synce_device_change_password_flags (SYNCE_DEVICE(self), SYNCE_DEVICE_PASSWORD_FLAG_SET |
                                 SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE, 0);
        }
      else
        {
          /* TODO: extend the API to handle this (introduced by WM6) */
          g_warning ("%s: device is locked, please unlock it", G_STRFUNC);
          priv->state = CTRL_STATE_CONNECTED;
        }
    }
  else
    {
      priv->state = CTRL_STATE_CONNECTED;
    }

  synce_device_set_hal_props(SYNCE_DEVICE(self));

  goto OUT;

 ERROR:
  /* TODO: do something sensible here */

 OUT:
  if (guid != NULL)
    g_free (guid);
  if (name != NULL)
    g_free (name);
  if (platform_name != NULL)
    g_free (platform_name);
  if (model_name != NULL)
    g_free (model_name);
}


static void
synce_device_rndis_conn_event_cb_impl (GConn *conn,
				 GConnEvent *event,
				 gpointer user_data)
{
  SynceDeviceRndis *self = SYNCE_DEVICE_RNDIS (user_data);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (SYNCE_DEVICE(self));

  if (priv->state == CTRL_STATE_HANDSHAKE)
    {
      if (event->type == GNET_CONN_READ)
        {
          guint32 req, v;
          GArray *resp;
          gint i, len;
          gchar *buf;

          if (event->length != 4)
            {
              g_warning ("%s: unexpected length", G_STRFUNC);
              return;
            }

          req = GUINT32_FROM_LE (*((guint32 *) event->buffer));
          resp = g_array_sized_new (FALSE, FALSE, sizeof (guint32), 1);

          switch (req)
	    {
            case 0:
              v = 3; g_array_append_val (resp, v);
              break;
            case 4:
              priv->state = CTRL_STATE_GETTING_INFO;
              gnet_conn_readn (conn, 4);
              break;
            case 6:
              v = 7; g_array_append_val (resp, v);
              v = 8; g_array_append_val (resp, v);
              v = 4; g_array_append_val (resp, v);
              v = 5; g_array_append_val (resp, v);
              break;
            default:
              g_warning ("%s: unknown request 0x%08x", G_STRFUNC, req);
              break;
	    }

          if (resp->len > 0)
            {
              len = resp->len * sizeof (guint32);
              buf = g_new (gchar, len);

              for (i = 0; i < resp->len; i++)
                {
                  v = g_array_index (resp, guint32, i);
                  *((guint32 *)(buf + (i * sizeof (guint32)))) = GUINT32_TO_LE (v);
                }

              gnet_conn_write (conn, buf, len);

              g_free (buf);
            }

          g_array_free (resp, TRUE);
        }
      else if (event->type == GNET_CONN_WRITE)
        {
          gnet_conn_readn (conn, 4);
        }
    }
  else if (priv->state < CTRL_STATE_AUTH)
    {
      if (event->type == GNET_CONN_READ)
        {
          if (priv->info_buf_size == -1)
            {
              if (event->length != 4)
                {
                  g_warning ("%s: event->length != 4", G_STRFUNC);
                  return;
                }

              priv->info_buf_size = GUINT32_FROM_LE (*((guint32 *) event->buffer));
              gnet_conn_readn (conn, priv->info_buf_size);
            }
          else
            {
              if (event->length != priv->info_buf_size)
                {
                  g_warning ("%s: event->length=%d != info_buf_size=%d",
			     G_STRFUNC, event->length, priv->info_buf_size);
                  return;
                }

              synce_device_rndis_info_received (self, (guchar *) event->buffer,
                                    event->length);
            }
        }
    }
  else if (priv->state == CTRL_STATE_AUTH)
    {
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
            }
          else
            {
              synce_device_change_password_flags (SYNCE_DEVICE(self), SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE,
                                     0);
            }
          dbus_g_method_return (priv->pw_ctx, result != 0);
          priv->pw_ctx = NULL;
        }
    }
}

void
synce_device_rndis_request_connection_impl (SynceDevice *self, DBusGMethodInvocation *ctx)
{
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE(self);
  GError *error = NULL;
  SynceConnectionBroker *broker;
  guint32 buf[3];

  if (priv->state != CTRL_STATE_CONNECTED)
    {
      error = g_error_new (SYNCE_ERRORS, NotAvailable,
			   (priv->pw_flags & SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE) ?
          "Not authenticated, you need to call ProvidePassword with the "
			   "correct password." : "Not yet connected.");
      goto OUT;
    }

  priv->req_id++;

  broker = g_object_new (SYNCE_TYPE_CONNECTION_BROKER,
                         "id", priv->req_id,
                         "context", ctx,
                         NULL);

  /* FIXME: have SynceConnectionBroker emit a signal when the request has
   *        timed out so that we don't risk zombie requests hanging around. */
  g_hash_table_insert (priv->requests, &priv->req_id, broker);

  g_signal_connect (broker, "done", (GCallback) synce_device_conn_broker_done_cb, self);

  buf[0] = GUINT32_TO_LE (5);
  buf[1] = GUINT32_TO_LE (4);
  buf[2] = GUINT32_TO_LE (priv->req_id);

  gnet_conn_write (priv->conn, (gchar *) buf, sizeof (buf));

 OUT:
  if (error != NULL)
    dbus_g_method_return_error (ctx, error);
}

static void
synce_device_rndis_client_event_cb (GConn *conn,
                 GConnEvent *event,
                 gpointer user_data)
{
  SynceDeviceRndis *self = SYNCE_DEVICE_RNDIS (user_data);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE (SYNCE_DEVICE(self));

  if (event->type == GNET_CONN_READ)
    {
      if (event->length == sizeof (guint32))
        {
          guint32 id = GUINT32_FROM_LE (*((guint32 *) event->buffer));
          SynceConnectionBroker *broker = g_hash_table_lookup (priv->requests, &id);

          if (broker != NULL)
            {
              _synce_connection_broker_take_connection (broker, conn);

              return;
            }
        }
    }

  g_warning ("%s: unhandled event", G_STRFUNC);
}

void
synce_device_rndis_client_connected (SynceDeviceRndis *self, GConn *conn)
{
  gnet_conn_set_callback (conn, synce_device_rndis_client_event_cb, self);
  gnet_conn_readn (conn, sizeof (guint32));
}


/* class & instance functions */

static void
synce_device_rndis_init(SynceDeviceRndis *self)
{

}

static void
synce_device_rndis_dispose (GObject *obj)
{
  SynceDeviceRndis *self = SYNCE_DEVICE_RNDIS(obj);
  SynceDevicePrivate *priv = SYNCE_DEVICE_GET_PRIVATE(self);

  if (priv->dispose_has_run) {
    return;
  }

  /* unref other objects */

  if (G_OBJECT_CLASS (synce_device_rndis_parent_class)->dispose)
    G_OBJECT_CLASS (synce_device_rndis_parent_class)->dispose (obj);
}

static void
synce_device_rndis_finalize (GObject *obj)
{
  if (G_OBJECT_CLASS (synce_device_rndis_parent_class)->finalize)
    G_OBJECT_CLASS (synce_device_rndis_parent_class)->finalize (obj);
}

static void
synce_device_rndis_class_init (SynceDeviceRndisClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  SynceDeviceClass *synce_device_class = SYNCE_DEVICE_CLASS(klass);

  gobject_class->dispose = synce_device_rndis_dispose;
  gobject_class->finalize = synce_device_rndis_finalize;

  synce_device_class->synce_device_conn_event_cb = synce_device_rndis_conn_event_cb_impl;
  synce_device_class->synce_device_request_connection = synce_device_rndis_request_connection_impl;
}

