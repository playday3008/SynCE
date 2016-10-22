#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <synce.h>
#include <glib.h>
#include <gio/gio.h>
#include "utils.h"

static GDBusProxy *
synce_get_dbus_g_bus_proxy()
{
  static GDBusProxy *bus_proxy = NULL;

  if (bus_proxy == NULL) {
    GError *error = NULL;

    bus_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
					      G_DBUS_PROXY_FLAGS_NONE,
					      NULL,
					      "org.freedesktop.DBus",
					      "/org/freedesktop/DBus",
					      "org.freedesktop.DBus",
					      NULL,
					      &error);
    if (bus_proxy == NULL) {
      g_critical("%s: Failed to get proxy to system bus: %s", G_STRFUNC, error->message);
      g_error_free(error);
      return NULL;
    }
  }
  return bus_proxy;
}

void
synce_get_dbus_sender_uid(const gchar *sender, guint *uid)
{
  GError *error = NULL;
  GDBusProxy *dbus_proxy = synce_get_dbus_g_bus_proxy();
  if (!dbus_proxy) {
    *uid = 0;
    return;
  }

  GVariant *result =  g_dbus_proxy_call_sync(dbus_proxy,
					     "GetConnectionUnixUser",
					     g_variant_new ("(s)", sender),
					     G_DBUS_CALL_FLAGS_NONE,
					     -1,
					     NULL,
					     &error);
  if (!result) {
    g_critical("Failed to get dbus sender uid: %s", error->message);
    *uid = 0;
  } else {
    g_variant_get(result, "(u)", uid);
    g_variant_unref(result);
  }

  return;
}

void
synce_print_hexdump (const void *buf, gssize len)
{
  GString *s = g_string_sized_new (12);
  int i;

  for (i = 0; i < len; i++)
    {
      g_string_append_printf (s, "%s%02x",
			      (i > 0) ? " " : "", ((guchar *) buf)[i]);
    }

  g_debug ("%s", s->str);

  g_string_free (s, TRUE);
}

gchar *
synce_guid_to_string (const guchar *p)
{
  guint32 v1;
  guint16 v2, v3;

  v1 = GUINT32_FROM_LE (*((guint32 *) p));
  v2 = GUINT16_FROM_LE (*((guint32 *) (p + 4)));
  v3 = GUINT16_FROM_LE (*((guint32 *) (p + 6)));

  return g_strdup_printf ("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
			  v1, v2, v3, p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
}

gchar *
synce_rapi_unicode_string_to_string (const guchar *buf, const guchar *buf_max, guint max_len, gsize *bytes_consumed)
{
  gchar *ret;
  guint32 len;

  if (buf + 4 > buf_max)
    return NULL;

  len = GUINT32_FROM_LE (*((guint32 *) buf));
  if (buf + 4 + (len * sizeof (WCHAR)) > buf_max || len > max_len)
    return NULL;

  if (len > 0)
    {
      ret = wstr_to_utf8 ((LPCWSTR) (buf + 4));
    }
  else
    {
      ret = g_strdup ("");
    }

  if (bytes_consumed != NULL)
    *bytes_consumed = sizeof (len) + (len * sizeof (WCHAR));

  return ret;
}

gchar *
synce_rapi_ascii_string_to_string (const guchar *buf, const guchar *buf_max, guint max_len, gsize *bytes_consumed)
{
  gchar *ret;
  guint32 len;

  if (buf + 4 > buf_max)
    return NULL;

  len = GUINT32_FROM_LE (*((guint32 *) buf));
  if (buf + 4 + len > buf_max || len > max_len)
    return NULL;

  if (len > 0)
    {
      ret = g_strdup ((const gchar *) (buf + 4));
    }
  else
    {
      ret = g_strdup ("");
    }

  if (bytes_consumed != NULL)
    *bytes_consumed = sizeof (len) + len;

  return ret;
}

gchar *
synce_rapi_unicode_string_to_string_at_offset (const guchar *buf, const guchar *offset, const guchar *buf_max)
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

