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

#include "odccm-device-glue.h"
#include "odccm-device-signals-marshal.h"

#include "odccm-connection-broker.h"
#include "odccm-errors.h"
#include "util.h"

G_DEFINE_TYPE (OdccmDevice, odccm_device, G_TYPE_OBJECT)

/* properties */
enum
{
  PROP_CONNECTION = 1,
  PROP_IP_ADDRESS,
  PROP_OBJECT_PATH,

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

/* signals */
enum
{
  PASSWORD_FLAGS_CHANGED,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

/* private stuff */
typedef enum {
    CTRL_STATE_HANDSHAKE,
    CTRL_STATE_GETTING_INFO,
    CTRL_STATE_GOT_INFO,
    CTRL_STATE_AUTH,
    CTRL_STATE_CONNECTED,
} ControlState;

typedef struct _OdccmDevicePrivate OdccmDevicePrivate;

struct _OdccmDevicePrivate
{
  gboolean dispose_has_run;

  GConn *conn;
  gchar *obj_path;

  gchar *guid;
  guint os_major;
  guint os_minor;
  gchar *name;
  guint version;
  guint cpu_type;
  guint cur_partner_id;
  guint id;
  gchar *platform_name;
  gchar *model_name;

  /* state */
  ControlState state;
  gint32 info_buf_size;
  OdccmDevicePasswordFlags pw_flags;

  guint32 pw_key;
  DBusGMethodInvocation *pw_ctx;

  GHashTable *requests;
  guint req_id;
};

#define ODCCM_DEVICE_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), ODCCM_TYPE_DEVICE, OdccmDevicePrivate))

static void
odccm_device_init (OdccmDevice *self)
{
}

static void
odccm_device_get_property (GObject    *obj,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  OdccmDevice *self = ODCCM_DEVICE (obj);
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);
  guint32 addr;

  switch (property_id) {
    case PROP_CONNECTION:
      g_value_set_pointer (value, priv->conn);
      break;
    case PROP_IP_ADDRESS:
      gnet_inetaddr_get_bytes (
          gnet_tcp_socket_get_remote_inetaddr (priv->conn->socket),
          (gchar *) &addr);
      g_value_set_uint (value, addr);
      break;
    case PROP_OBJECT_PATH:
      g_value_set_string (value, priv->obj_path);
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
odccm_device_set_property (GObject      *obj,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  OdccmDevice *self = ODCCM_DEVICE (obj);
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  switch (property_id) {
    case PROP_CONNECTION:
      if (priv->conn != NULL)
        {
          gnet_conn_unref (priv->conn);
        }

      priv->conn = g_value_get_pointer (value);
      gnet_conn_ref (priv->conn);

      break;
    case PROP_OBJECT_PATH:
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

static void conn_event_cb (GConn *conn, GConnEvent *event, gpointer user_data);

static GObject *
odccm_device_constructor (GType type, guint n_props,
                                 GObjectConstructParam *props)
{
  GObject *obj;
  OdccmDevicePrivate *priv;

  obj = G_OBJECT_CLASS (odccm_device_parent_class)->
    constructor (type, n_props, props);

  priv = ODCCM_DEVICE_GET_PRIVATE (obj);

  priv->state = CTRL_STATE_HANDSHAKE;
  priv->info_buf_size = -1;

  priv->requests = g_hash_table_new_full (g_int_hash,
                                          g_int_equal,
                                          NULL,
                                          g_object_unref);

  gnet_conn_set_callback (priv->conn, conn_event_cb, obj);
  gnet_conn_readn (priv->conn, sizeof (guint32));

  return obj;
}

static void
odccm_device_dispose (GObject *obj)
{
  OdccmDevice *self = ODCCM_DEVICE (obj);
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  gnet_conn_unref (priv->conn);

  if (G_OBJECT_CLASS (odccm_device_parent_class)->dispose)
    G_OBJECT_CLASS (odccm_device_parent_class)->dispose (obj);
}

static void
odccm_device_finalize (GObject *obj)
{
  OdccmDevice *self = ODCCM_DEVICE (obj);
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  g_free (priv->obj_path);

  g_free (priv->guid);
  g_free (priv->name);
  g_free (priv->platform_name);
  g_free (priv->model_name);

  g_hash_table_destroy (priv->requests);

  G_OBJECT_CLASS (odccm_device_parent_class)->finalize (obj);
}

static void
odccm_device_class_init (OdccmDeviceClass *dev_class)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (dev_class);
  GParamSpec *param_spec;

  g_type_class_add_private (dev_class, sizeof (OdccmDevicePrivate));

  obj_class->constructor = odccm_device_constructor;

  obj_class->get_property = odccm_device_get_property;
  obj_class->set_property = odccm_device_set_property;

  obj_class->dispose = odccm_device_dispose;
  obj_class->finalize = odccm_device_finalize;

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

  param_spec = g_param_spec_string ("object-path", "Object path",
                                    "The D-Bus object path.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_OBJECT_PATH, param_spec);

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

  signals[PASSWORD_FLAGS_CHANGED] =
    g_signal_new ("password-flags-changed",
                  G_OBJECT_CLASS_TYPE (dev_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  odccm_device_marshal_VOID__UINT_UINT,
                  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (dev_class),
                                   &dbus_glib_odccm_device_object_info);
}

static void
change_password_flags (OdccmDevice *self,
                       OdccmDevicePasswordFlags add,
                       OdccmDevicePasswordFlags remove)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);
  OdccmDevicePasswordFlags flags, added, removed;

  flags = priv->pw_flags;

  added = add & ~flags;
  flags |= added;

  removed = remove & flags;
  flags &= ~removed;

  if (added != 0 || removed != 0)
    {
      g_object_set (self, "password-flags", flags, NULL);
      g_signal_emit (self, signals[PASSWORD_FLAGS_CHANGED], 0, added, removed);
    }
}

static void device_info_received (OdccmDevice *self, const guchar *buf, gint length);

static void
conn_event_cb (GConn *conn,
               GConnEvent *event,
               gpointer user_data)
{
  OdccmDevice *self = ODCCM_DEVICE (user_data);
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

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

              device_info_received (self, (guchar *) event->buffer,
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
              change_password_flags (self, ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE,
                                     0);
            }

          dbus_g_method_return (priv->pw_ctx, result != 0);
          priv->pw_ctx = NULL;
        }
    }
}

static void
device_info_received (OdccmDevice *self, const guchar *buf, gint length)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);
  gchar *guid = NULL, *name = NULL, *platform_name = NULL, *model_name = NULL;
  guint os_major, os_minor, version, cpu_type, cur_partner_id, id, comp_count;
  gchar *safe_guid, *obj_path;
  const gchar safe_chars[] = {
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789_"
  };
  const guchar *p = buf, *end_ptr = buf + length;
  guint consumed;

  priv->state = CTRL_STATE_GOT_INFO;

  g_debug (G_STRFUNC);
  _odccm_print_hexdump (buf, length);

  /*
   * Parse and set device properties.
   */
  if (p + 24 > end_ptr)
    {
      g_warning ("%s: short read trying to read GUID/OsMajor/OsMinor",
          G_STRFUNC);
      goto ERROR;
    }

  guid = _odccm_guid_to_string (p);
  p += 16;

  os_major = GUINT32_FROM_LE (*((guint32 *) p));
  p += sizeof (guint32);

  os_minor = GUINT32_FROM_LE (*((guint32 *) p));
  p += sizeof (guint32);

  name = _odccm_rapi_unicode_string_to_string (p, end_ptr, 31, &consumed);
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
  platform_name = _odccm_rapi_ascii_string_to_string (p, end_ptr, 255,
      &consumed);
  if (platform_name == NULL)
    {
      g_warning ("%s: PlatformName is out of bounds or too long", G_STRFUNC);
      goto ERROR;
    }
  p += consumed;

  model_name = _odccm_rapi_ascii_string_to_string (p, end_ptr, 255, &consumed);
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
      _odccm_print_hexdump (p, n);

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
      if (priv->pw_key != 0xffffffff)
        {
          priv->state = CTRL_STATE_AUTH;
          change_password_flags (self, ODCCM_DEVICE_PASSWORD_FLAG_SET |
                                 ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE, 0);
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

gboolean
odccm_device_get_ip_address (OdccmDevice *self, gchar **ip_address,
                             GError **error)
{
  struct in_addr addr;

  g_object_get (self, "ip-address", &(addr.s_addr), NULL);
  *ip_address = g_strdup (inet_ntoa (addr));

  return TRUE;
}

gboolean
odccm_device_get_guid (OdccmDevice *self, gchar **guid, GError **error)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  *guid = g_strdup (priv->guid);

  return TRUE;
}

gboolean
odccm_device_get_os_version (OdccmDevice *self,
                             guint *os_major, guint *os_minor,
                             GError **error)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  *os_major = priv->os_major;
  *os_minor = priv->os_minor;

  return TRUE;
}

gboolean
odccm_device_get_name (OdccmDevice *self, gchar **name, GError **error)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  *name = g_strdup (priv->name);

  return TRUE;
}

gboolean
odccm_device_get_version (OdccmDevice *self, guint *version, GError **error)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  *version = priv->version;

  return TRUE;
}

gboolean
odccm_device_get_cpu_type (OdccmDevice *self, guint *cpu_type, GError **error)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  *cpu_type = priv->cpu_type;

  return TRUE;
}

gboolean
odccm_device_get_current_partner_id (OdccmDevice *self, guint *cur_partner_id,
                                     GError **error)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  *cur_partner_id = priv->cur_partner_id;

  return TRUE;
}

gboolean
odccm_device_get_id (OdccmDevice *self, guint *id, GError **error)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  *id = priv->id;

  return TRUE;
}

gboolean
odccm_device_get_platform_name (OdccmDevice *self, gchar **platform_name,
                                GError **error)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  *platform_name = g_strdup (priv->platform_name);

  return TRUE;
}

gboolean
odccm_device_get_model_name (OdccmDevice *self, gchar **model_name,
                             GError **error)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  *model_name = g_strdup (priv->model_name);

  return TRUE;
}

gboolean
odccm_device_get_password_flags (OdccmDevice *self, guint *pw_flags,
                                 GError **error)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  *pw_flags = priv->pw_flags;

  return TRUE;
}

void
odccm_device_provide_password (OdccmDevice *self,
                               const gchar *password,
                               DBusGMethodInvocation *ctx)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);
  GError *error = NULL;
  guchar *buf;
  guint16 buf_size;
  guint i;

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

  change_password_flags (self, 0, ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE);

OUT:
  if (error != NULL)
    dbus_g_method_return_error (ctx, error);
}

static void
conn_broker_done_cb (OdccmConnectionBroker *broker,
                     gpointer user_data)
{
  OdccmDevice *self = ODCCM_DEVICE (user_data);
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);
  guint id;

  g_object_get (broker, "id", &id, NULL);

  g_hash_table_remove (priv->requests, &id);
}

void
odccm_device_request_connection (OdccmDevice *self, DBusGMethodInvocation *ctx)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);
  GError *error = NULL;
  OdccmConnectionBroker *broker;
  guint32 buf[3];

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

  /* FIXME: have OdccmConnectionBroker emit a signal when the request has
   *        timed out so that we don't risk zombie requests hanging around. */
  g_hash_table_insert (priv->requests, &priv->req_id, broker);

  g_signal_connect (broker, "done", (GCallback) conn_broker_done_cb, self);

  buf[0] = GUINT32_TO_LE (5);
  buf[1] = GUINT32_TO_LE (4);
  buf[2] = GUINT32_TO_LE (priv->req_id);

  gnet_conn_write (priv->conn, (gchar *) buf, sizeof (buf));

OUT:
  if (error != NULL)
    dbus_g_method_return_error (ctx, error);
}

gboolean
_odccm_device_is_identified (OdccmDevice *self)
{
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  return (priv->state >= CTRL_STATE_GOT_INFO);
}

static void
client_event_cb (GConn *conn,
                 GConnEvent *event,
                 gpointer user_data)
{
  OdccmDevice *self = ODCCM_DEVICE (user_data);
  OdccmDevicePrivate *priv = ODCCM_DEVICE_GET_PRIVATE (self);

  if (event->type == GNET_CONN_READ)
    {
      if (event->length == sizeof (guint32))
        {
          guint32 id = GUINT32_FROM_LE (*((guint32 *) event->buffer));
          OdccmConnectionBroker *broker = g_hash_table_lookup (priv->requests, &id);

          if (broker != NULL)
            {
              _odccm_connection_broker_take_connection (broker, conn);

              return;
            }
        }
    }

  g_warning ("%s: unhandled event", G_STRFUNC);
}

void
_odccm_device_client_connected (OdccmDevice *self, GConn *conn)
{
  gnet_conn_set_callback (conn, client_event_cb, self);
  gnet_conn_readn (conn, sizeof (guint32));
}

