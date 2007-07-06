/*
 * Copyright (C) 2006-2007 Ole André Vadla Ravnås <oleavr@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gnet.h>
#include <synce.h>
#include <string.h>
#include <arpa/inet.h>

#include "odccm-device.h"
#include "odccm-device-private.h"
#include "odccm-device-legacy.h"
#include "odccm-device-signals-marshal.h"

#include "odccm-constants.h"
#include "odccm-connection-broker.h"
#include "odccm-errors.h"
#include "util.h"

#define DCCM_PORT               5679
#define DCCM_PING               0x12345678
#define DCCM_PING_INTERVAL      5       /* seconds */
#define DCCM_MAX_PING_COUNT     3       /* max number of pings without reply */
#define DCCM_MAX_PACKET_SIZE    512
#define DCCM_MIN_PACKET_SIZE    0x24
#define RAPI_PORT               990

G_DEFINE_TYPE (OdccmDeviceLegacy, odccm_device_legacy, ODCCM_TYPE_DEVICE)

typedef struct _OdccmDeviceLegacyPrivate OdccmDeviceLegacyPrivate;
struct _OdccmDeviceLegacyPrivate
{
  gint ping_count;
  gchar *password;
};

#define ODCCM_DEVICE_LEGACY_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), ODCCM_TYPE_DEVICE_LEGACY, OdccmDeviceLegacyPrivate))


static void conn_event_cb_legacy_impl (GConn *conn, GConnEvent *event, gpointer user_data);
static void odccm_device_request_connection_legacy_impl (OdccmDevice *self, DBusGMethodInvocation *ctx);
static void odccm_device_provide_password_impl (OdccmDevice *self, const gchar *password, DBusGMethodInvocation *ctx);

static void device_info_received_legacy (OdccmDeviceLegacy *self, const guchar *buf, gint length);
static gboolean odccm_device_legacy_send_ping(gpointer data);

/* class functions */

static void
odccm_device_legacy_init (OdccmDeviceLegacy *self)
{
  OdccmDeviceLegacyPrivate *priv = ODCCM_DEVICE_LEGACY_GET_PRIVATE (self);

  priv->ping_count = 0;
  priv->password = NULL;
}

static void
odccm_device_legacy_finalize (GObject *obj)
{
  OdccmDeviceLegacy *self = ODCCM_DEVICE_LEGACY (obj);
  OdccmDeviceLegacyPrivate *priv = ODCCM_DEVICE_LEGACY_GET_PRIVATE (self);

  g_free (priv->password);

  G_OBJECT_CLASS (odccm_device_legacy_parent_class)->finalize (obj);
}

static void
odccm_device_legacy_class_init (OdccmDeviceLegacyClass *dev_class)
{
  g_type_class_add_private (dev_class, sizeof (OdccmDeviceLegacyPrivate));

  G_OBJECT_CLASS(dev_class)->finalize = odccm_device_legacy_finalize;

  ODCCM_DEVICE_CLASS(dev_class)->conn_event_cb = conn_event_cb_legacy_impl;
  ODCCM_DEVICE_CLASS(dev_class)->odccm_device_request_connection = odccm_device_request_connection_legacy_impl;
  ODCCM_DEVICE_CLASS(dev_class)->odccm_device_provide_password = odccm_device_provide_password_impl;
}


/* methods */

static void
conn_event_cb_legacy_impl (GConn *conn,
               GConnEvent *event,
               gpointer user_data)
{
  OdccmDeviceLegacy *self = ODCCM_DEVICE_LEGACY (user_data);
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (ODCCM_DEVICE(self));
  OdccmDeviceLegacyPrivate *priv_legacy = ODCCM_DEVICE_LEGACY_GET_PRIVATE (self);

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
	    change_password_flags (ODCCM_DEVICE(self), ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE, 0);
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

	  device_info_received_legacy (self, (guchar *) event->buffer,
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
	      g_timeout_add((DCCM_PING_INTERVAL * 1000), odccm_device_legacy_send_ping, self);
            }
          else
            {
              change_password_flags (ODCCM_DEVICE(self), ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE,
                                     0);
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


static gboolean
odccm_device_legacy_send_ping(gpointer data)
{
  if (!(ODCCM_IS_DEVICE_LEGACY(data)))
    return FALSE;

  OdccmDeviceLegacy *self = ODCCM_DEVICE_LEGACY (data);
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (ODCCM_DEVICE(self));
  OdccmDeviceLegacyPrivate *priv_legacy = ODCCM_DEVICE_LEGACY_GET_PRIVATE (self);

  guint32 ping = GUINT32_TO_LE(DCCM_PING);
  gnet_conn_write(priv->conn, (gchar *) &ping, sizeof(ping));

  if (++priv_legacy->ping_count == DCCM_MAX_PING_COUNT) {
    g_warning("%s: Device not responded to %d pings, disconnected ?", G_STRFUNC, DCCM_MAX_PING_COUNT);
    return FALSE;
  }

  return TRUE;
}

static gchar *
device_legacy_info_string_to_string_at (const guchar *buf, const guchar *offset, const guchar *buf_max)
{
  gchar *ret;
  guint32 string_offset;

  if (offset + 4 > buf_max)
    return NULL;

  string_offset = GUINT32_FROM_LE (*((guint32 *) offset));
  if ((buf + string_offset) > buf_max)
    return NULL;

  ret = wstr_to_utf8 ((LPCWSTR) (buf + string_offset));

  return ret;
}

static void
device_info_received_legacy (OdccmDeviceLegacy *self, const guchar *buf, gint length)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (ODCCM_DEVICE(self));

  gchar *guid = NULL, *name = NULL, *platform_name = NULL, *model_name = NULL;
  guint os_major = 0, os_minor = 0, version = 0, cpu_type = 0, cur_partner_id = 0, id = 0;
  gchar *safe_guid, *obj_path;
  const gchar safe_chars[] = {
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789_"
  };
  const guchar *p = buf, *end_ptr = buf + length;

  priv->state = CTRL_STATE_GOT_INFO;

  g_debug (G_STRFUNC);
  _odccm_print_hexdump (buf, length);

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

  name = device_legacy_info_string_to_string_at (buf, p, end_ptr);
  if (name == NULL)
    {
      g_warning ("%s: DeviceName is out of bounds or too long", G_STRFUNC);
      goto ERROR;
    }
  p += sizeof (guint32);

  platform_name = device_legacy_info_string_to_string_at (buf, p, end_ptr);
  if (platform_name == NULL)
    {
      g_warning ("%s: PlatformName is out of bounds or too long", G_STRFUNC);
      goto ERROR;
    }
  p += sizeof (guint32);

  model_name = device_legacy_info_string_to_string_at (buf, p, end_ptr);
  if (model_name == NULL)
    {
      g_warning ("%s: ModelName is out of bounds or too long", G_STRFUNC);
      goto ERROR;
    }

  /* what are version, id ? */

  /* we dont have guid, we'll use name for now */
  g_object_set (self,
      "guid", name,
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

  /*
   * Register ourself with D-Bus.
   */
  safe_guid = g_strdup (priv->guid);
  g_strcanon (safe_guid, safe_chars, '_');
  obj_path = g_strdup_printf (DEVICE_BASE_OBJECT_PATH "/%s", safe_guid);
  g_free (safe_guid);

  g_debug ("%s: registering object path '%s'", G_STRFUNC, obj_path);

  dbus_g_connection_register_g_object (_odccm_get_dbus_conn (),
                                       obj_path, G_OBJECT (self));

  g_object_set (self, "object-path", obj_path, NULL);
  g_free (obj_path);

  if (priv->pw_key != 0)
    {
      priv->state = CTRL_STATE_AUTH;
      change_password_flags (ODCCM_DEVICE(self), ODCCM_DEVICE_PASSWORD_FLAG_SET |
			     ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE, 0);
    }
  else
    {
      priv->state = CTRL_STATE_CONNECTED;
      g_timeout_add((DCCM_PING_INTERVAL * 1000), odccm_device_legacy_send_ping, self);
    }

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
client_event_dummy_cb (GConn *conn,
                 GConnEvent *event,
                 gpointer user_data)
{
  return;
}

static void
client_event_password_cb (GConn *conn,
                 GConnEvent *event,
                 gpointer user_data)
{
  OdccmConnectionBroker *broker = ODCCM_CONNECTION_BROKER (user_data);
  guint8 result;
  GError *error = NULL;
  DBusGMethodInvocation *ctx;

  if (event->type != GNET_CONN_READ)
    return;

  g_object_get(broker, "context", &ctx, NULL);

  if (event->length != sizeof (guint8))
    {
      g_warning ("%s: Failed to get password reply, event->length != 1", G_STRFUNC);
      error = g_error_new (ODCCM_ERRORS, NotAvailable,
                           "Failed to authenticate connection");
      goto OUT;
    }

  result = *((guint8 *) event->buffer);
  if (result != 0) {
      _odccm_connection_broker_take_connection (broker, conn);
      gnet_conn_set_callback (conn, client_event_dummy_cb, NULL);
      return;
  } else {
    g_warning("%s: Password rejected", G_STRFUNC);
    error = g_error_new (ODCCM_ERRORS, NotAvailable,
                         "Failed to authenticate connection");
  }
OUT:
  if (error != NULL)
    dbus_g_method_return_error (ctx, error);
  return;
}

void
odccm_device_request_connection_legacy_impl (OdccmDevice *self, DBusGMethodInvocation *ctx)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (ODCCM_DEVICE(self));
  GError *error = NULL;
  OdccmConnectionBroker *broker;
  GConn *rapi_conn;
  GInetAddr *rapi_inet_addr;

  if (priv->state != CTRL_STATE_CONNECTED)
    {
      error = g_error_new (ODCCM_ERRORS, NotAvailable,
          (priv->pw_flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE) ?
          "Not authenticated, you need to call ProvidePassword with the "
          "correct password." : "Not yet connected.");
      goto OUT;
    }

  priv->req_id++;
  broker = g_object_new (ODCCM_TYPE_CONNECTION_BROKER,
                         "id", priv->req_id,
                         "context", ctx,
                         NULL);

  g_hash_table_insert (priv->requests, &priv->req_id, broker);

  g_signal_connect (broker, "done", (GCallback) conn_broker_done_cb, self);

  rapi_inet_addr = gnet_inetaddr_clone(priv->conn->inetaddr);
  gnet_inetaddr_set_port(rapi_inet_addr, RAPI_PORT);
  rapi_conn = gnet_conn_new_inetaddr(rapi_inet_addr, client_event_dummy_cb, NULL);
  gnet_conn_connect(rapi_conn);

  /* password stuff for new sock */

  OdccmDeviceLegacy *legacy_self = ODCCM_DEVICE_LEGACY(self);
  OdccmDeviceLegacyPrivate *legacy_priv = ODCCM_DEVICE_LEGACY_GET_PRIVATE (legacy_self);
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
      gnet_conn_write (rapi_conn, (gchar *) &buf_size, sizeof (buf_size));
      gnet_conn_write (rapi_conn, (gchar *) buf, buf_size);

      gnet_conn_set_callback (rapi_conn, client_event_password_cb, broker);
      gnet_conn_readn (rapi_conn, sizeof (guint8));

      goto OUT;
    }


  _odccm_connection_broker_take_connection (broker, rapi_conn);
OUT:
  if (error != NULL)
    dbus_g_method_return_error (ctx, error);
}

static void
odccm_device_provide_password_impl (OdccmDevice *self,
                               const gchar *password,
                               DBusGMethodInvocation *ctx)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);
  GError *error = NULL;

  if (priv->state != CTRL_STATE_AUTH ||
      (priv->pw_flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE) == 0)
    {
      error = g_error_new (ODCCM_ERRORS, NotAvailable,
          "No password expected in the current state.");
      goto OUT;
    }
  else if (priv->pw_ctx != NULL)
    {
      error = g_error_new (ODCCM_ERRORS, NotAvailable,
          "An authentication attempt is still in progress.");
      goto OUT;
    }

  OdccmDeviceLegacyPrivate *priv_legacy = ODCCM_DEVICE_LEGACY_GET_PRIVATE (ODCCM_DEVICE_LEGACY(self));
  priv_legacy->password = g_strdup(password);

  (ODCCM_DEVICE_CLASS(odccm_device_legacy_parent_class)->odccm_device_provide_password) (self, password, ctx);

OUT:
  if (error != NULL)
    dbus_g_method_return_error (ctx, error);
}
