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

#include "odccm-device-manager.h"
#include "util.h"

#define ODCCM_BUS_NAME "org.synce.odccm"

gint main(gint argc, gchar *argv[])
{
  GMainLoop *mainloop;
  OdccmDeviceManager *mgr;
  DBusGConnection *bus;

  g_type_init ();

  mainloop = g_main_loop_new (NULL, FALSE);

  bus = _odccm_get_dbus_conn ();
  _odccm_request_dbus_name (ODCCM_BUS_NAME);

  mgr = g_object_new (ODCCM_TYPE_DEVICE_MANAGER, NULL);

  g_debug ("running mainloop");
  g_main_loop_run (mainloop);

  return 0;
}

