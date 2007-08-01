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

#include <glib.h>
#include <dbus/dbus-glib.h>

void _odccm_print_hexdump (const void *buf, gint len);
DBusGConnection *_odccm_get_dbus_conn ();
void _odccm_request_dbus_name (const gchar *bus_name);
void _odccm_get_dbus_sender_uid (const gchar *sender, guint *uid);
gchar *_odccm_guid_to_string (const guchar *p);

/* TODO: make these two handle strings that are optionally null-terminated,
 *       or disambiguate on whether the prefixed length includes the NUL. */
gchar *_odccm_rapi_unicode_string_to_string (const guchar *buf, const guchar *buf_max, guint max_len, guint *bytes_consumed);
gchar *_odccm_rapi_ascii_string_to_string (const guchar *buf, const guchar *buf_max, guint max_len, guint *bytes_consumed);

gboolean _odccm_configure_interface (const gchar *ifname, const gchar *ip_addr, const gchar *netmask, const gchar *bcast_addr);
gboolean _odccm_interface_is_configured (const gchar *ifname, const gchar *expected_address);
gboolean _odccm_interface_address (const gchar *ifname, const gchar *expected_address);
gboolean _odccm_trigger_connection (const gchar *device_ip);
