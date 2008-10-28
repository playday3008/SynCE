/*
 * Copyright (C) 2006 Ole André Vadla Ravnås <oleavr@gmail.com>
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <synce.h>
#include "util.h"

void
_odccm_print_hexdump (const void *buf, gint len)
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

DBusGConnection *
_odccm_get_dbus_conn ()
{
  static DBusGConnection *bus = NULL;

  if (bus == NULL)
    {
      GError *error = NULL;

      bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
      if (bus == NULL)
        {
          g_error ("Failed to connect to system bus: %s", error->message);
          exit (EXIT_FAILURE);
        }
    }

  return bus;
}

static DBusGProxy *
get_dbus_proxy ()
{
  static DBusGProxy *bus_proxy = NULL;

  if (bus_proxy == NULL)
    {
      DBusGConnection *bus = _odccm_get_dbus_conn ();

      bus_proxy = dbus_g_proxy_new_for_name (bus, "org.freedesktop.DBus",
                                             "/org/freedesktop/DBus",
                                             "org.freedesktop.DBus");
      if (bus_proxy == NULL)
        {
          g_error ("Failed to get bus proxy object");
          exit (EXIT_FAILURE);
        }
    }

  return bus_proxy;
}

void
_odccm_request_dbus_name (const gchar *bus_name)
{
  DBusGProxy *bus_proxy = get_dbus_proxy ();
  GError *error = NULL;
  guint req_name_result;

  if (!dbus_g_proxy_call (bus_proxy, "RequestName", &error,
                          G_TYPE_STRING, bus_name,
                          G_TYPE_UINT, DBUS_NAME_FLAG_DO_NOT_QUEUE,
                          G_TYPE_INVALID,
                          G_TYPE_UINT, &req_name_result,
                          G_TYPE_INVALID))
    {
      g_error ("Failed to get bus name: %s", error->message);
      exit (EXIT_FAILURE);
    }
}

void
_odccm_get_dbus_sender_uid (const gchar *sender, guint *uid)
{
  DBusGProxy *bus_proxy = get_dbus_proxy ();
  GError *error = NULL;

  if (!dbus_g_proxy_call (bus_proxy, "GetConnectionUnixUser", &error,
                          G_TYPE_STRING, sender,
                          G_TYPE_INVALID,
                          G_TYPE_UINT, uid,
                          G_TYPE_INVALID))
    {
      g_error ("Failed to get dbus sender uid: %s", error->message);
      exit (EXIT_FAILURE);
    }
}

gchar *
_odccm_guid_to_string (const guchar *p)
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
_odccm_rapi_unicode_string_to_string (const guchar *buf, const guchar *buf_max, guint max_len, guint *bytes_consumed)
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
_odccm_rapi_ascii_string_to_string (const guchar *buf, const guchar *buf_max, guint max_len, guint *bytes_consumed)
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

gboolean
_odccm_configure_interface (const gchar *ifname,
                            const gchar *ip_addr,
                            const gchar *netmask,
                            const gchar *bcast_addr)
{
  gboolean result = FALSE;
  const gchar *op = "socket()";
  gint fd = -1;
  struct ifreq ifr;
  struct sockaddr_in *addr;

  if ((fd = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    goto ERROR;

  memset (&ifr, 0, sizeof (ifr));
  strcpy (ifr.ifr_name, ifname);

  addr = (struct sockaddr_in *) &ifr.ifr_addr;
  addr->sin_family = AF_INET;

  inet_aton (ip_addr, &addr->sin_addr);
  if (ioctl (fd, SIOCSIFADDR, &ifr) < 0)
    {
      op = "ioctl(SIOCSIFADDR)";
      goto ERROR;
    }

  inet_aton (netmask, &addr->sin_addr);
  if (ioctl (fd, SIOCSIFNETMASK, &ifr) < 0)
    {
      op = "ioctl(SIOCSIFNETMASK)";
      goto ERROR;
    }

  inet_aton (bcast_addr, &addr->sin_addr);
  if (ioctl (fd, SIOCSIFBRDADDR, &ifr) < 0)
    {
      op = "ioctl(SIOCSIFBRDADDR)";
      goto ERROR;
    }

  if (ioctl (fd, SIOCGIFFLAGS, &ifr) < 0)
    {
      op = "ioctl(SIOCGIFFLAGS)";
      goto ERROR;
    }

  ifr.ifr_flags |= IFF_UP;

  if (ioctl (fd, SIOCSIFFLAGS, &ifr) < 0)
    {
      op = "ioctl(SIOCSIFFLAGS)";
      goto ERROR;
    }

  result = TRUE;
  goto OUT;

ERROR:
  g_warning ("%s: failed to configure %s. %s failed: %s",
             G_STRFUNC, ifname, op, strerror (errno));

OUT:
  if (fd != -1)
    close (fd);

  return result;
}

gboolean
_odccm_interface_is_configured (const gchar *ifname,
                                const gchar *expected_address)
{
  gboolean result = FALSE;
  const gchar *op = "socket()";
  gint fd = -1;
  struct ifreq ifr;
  struct sockaddr_in *addr;

  if ((fd = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    goto ERROR;

  memset (&ifr, 0, sizeof (ifr));
  strcpy (ifr.ifr_name, ifname);
  ifr.ifr_addr.sa_family = AF_INET;

  if (ioctl (fd, SIOCGIFADDR, &ifr) < 0)
    {
      op = "ioctl(SIOCGIFADDR)";
      goto ERROR;
    }

  addr = (struct sockaddr_in *) &ifr.ifr_addr;
  if (addr->sin_addr.s_addr != inet_addr (expected_address))
    goto OUT;

  if (ioctl (fd, SIOCGIFFLAGS, &ifr) < 0)
    {
      op = "ioctl(SIOCGIFFLAGS)";
      goto ERROR;
    }

  if ((ifr.ifr_flags & IFF_UP) == 0)
    goto OUT;

  result = TRUE;
  goto OUT;

ERROR:
  /*g_warning ("failed to get configuration for %s. %s failed: %s",
             ifname, op, strerror (errno));*/

OUT:
  if (fd != -1)
    close (fd);

  return result;
}

gboolean
_odccm_interface_address (const gchar *ifname,
                          const gchar *expected_address)
{
#define SETTLE_TIME 5 
  gboolean result = FALSE, found = FALSE;
  const gchar *op = "socket()";
  gint fd = -1;
  struct ifreq ifr[10];
  int i,p;
  struct ifconf ifc;
  struct sockaddr_in *addr;

  // Give interface time to get IP address
  // by default not more than 10 seconds
  for (p=0; p<SETTLE_TIME; p++) {
    if ((fd = socket (PF_INET, SOCK_STREAM, 0)) < 0)
      goto ERROR;

    memset (&ifr, 0, sizeof (ifr));
    memset (&ifc, 0, sizeof (ifc));
    ifc.ifc_len = sizeof(ifr);
    ifc.ifc_req = ifr;

    if (ioctl (fd, SIOCGIFCONF, &ifc) < 0)
      {
        op = "ioctl(SIOCGIFCONF)";
        goto ERROR;
      }

    if (ifc.ifc_len == sizeof(ifr))
      {
        g_warning ("%s: too many interfaces",G_STRFUNC);
      }

    for (i=0; i<ifc.ifc_len/sizeof(struct ifreq); i++) {
      addr = (struct sockaddr_in *) &ifr[i].ifr_addr;
      if (strcmp(ifname,ifr[i].ifr_name)==0) {
        if (addr->sin_addr.s_addr == inet_addr (expected_address)) {
          found = TRUE;
	      g_debug("%s: found matching interface", G_STRFUNC);
          break;
        }
      }
    }
    if (found == TRUE) {
      break;
    } else {
      g_debug("%s: waiting for IP address on %s",G_STRFUNC,ifname);
      close(fd);
      sleep(1);
    }
  }

  if (found == FALSE) goto OUT;

  if (ioctl (fd, SIOCGIFFLAGS, &ifr[i]) < 0)
    {
      op = "ioctl(SIOCGIFFLAGS)";
      goto ERROR;
    }

  if ((ifr[i].ifr_flags & IFF_UP) == 0)
    goto OUT;

  result = TRUE;
  goto OUT;

ERROR:
  g_warning ("failed to get configuration for %s. %s failed: %s",
             ifname, op, strerror (errno));

OUT:
  if (fd != -1)
    close (fd);

  return result;
}

gboolean
_odccm_trigger_connection (const gchar *device_ip)
{
  gboolean result = FALSE;
  const gchar *op = "socket()";
  gint fd = -1;
  struct sockaddr_in sa;
  guchar b = 0x7f;

  if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
    goto ERROR;

  sa.sin_family = AF_INET;
  sa.sin_port = htons (5679);
  inet_aton (device_ip, &sa.sin_addr);

  if (sendto (fd, &b, sizeof (b), 0, (const struct sockaddr *) &sa,
              sizeof (sa)) != 1)
    {
      op = "sendto()";
      goto ERROR;
    }

  result = TRUE;
  goto OUT;

ERROR:
  g_warning ("failed to send trigger packet. %s failed: %s",
             op, strerror (errno));

OUT:
  if (fd != -1)
    close (fd);

  return result;
}

