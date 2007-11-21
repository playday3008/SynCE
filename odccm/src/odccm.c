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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>
#include "odccm-device-manager.h"
#include "util.h"

#define ODCCM_BUS_NAME "org.synce.odccm"

#define DEFAULT_DEVICE_IP_ADDRESS "169.254.2.1"
#define DEFAULT_LOCAL_IP_ADDRESS "169.254.2.2"
#define DEFAULT_LOCAL_NETMASK    "255.255.255.0"
#define DEFAULT_LOCAL_BROADCAST  "169.254.2.255"

static gboolean run_as_daemon = TRUE;
static gchar *device_ip = DEFAULT_DEVICE_IP_ADDRESS;
static gchar *local_ip = DEFAULT_LOCAL_IP_ADDRESS;
static gchar *local_netmask = DEFAULT_LOCAL_NETMASK;
static gchar *local_broadcast = DEFAULT_LOCAL_BROADCAST;

static GOptionEntry options[] =
  {
    { "foreground", 'f', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &run_as_daemon, "Do not run as a daemon", NULL },
    { "device-ip", 0, 0, G_OPTION_ARG_STRING, &device_ip, "Device's IP address in dotted quad (default " DEFAULT_DEVICE_IP_ADDRESS ")", NULL },
    { "local-ip", 0, 0, G_OPTION_ARG_STRING, &local_ip, "Local IP address in dotted quad (default " DEFAULT_LOCAL_IP_ADDRESS ")", NULL },
    { "local-netmask", 0, 0, G_OPTION_ARG_STRING, &local_netmask, "Local netmask in dotted quad (default " DEFAULT_LOCAL_NETMASK ")", NULL },
    { "local-broadcast", 0, 0, G_OPTION_ARG_STRING, &local_broadcast, "Local broadcast address in dotted quad (default " DEFAULT_LOCAL_BROADCAST ")", NULL },
    { NULL }
  };

gint main(gint argc, gchar *argv[])
{
  GMainLoop *mainloop;
  OdccmDeviceManager *mgr;
  DBusGConnection *bus;
  GError *error = NULL;

  g_type_init ();

  GOptionContext *option_context = g_option_context_new (" - keep connection to Windows Mobile device");
  g_option_context_add_main_entries (option_context, options, NULL);
  g_option_context_parse (option_context, &argc, &argv, &error);

  if (error) {
    switch (error->code)
      {
      case G_OPTION_ERROR_UNKNOWN_OPTION:
	g_printerr("%s\n", error->message);
	break;
      case G_OPTION_ERROR_BAD_VALUE:
	g_printerr("%s\n", error->message);
	break;
      default:
	g_printerr("Unexpected error: %s\n", error->message);
      }
    return EXIT_FAILURE;
  }
  g_option_context_free(option_context);

  mainloop = g_main_loop_new (NULL, FALSE);

  bus = _odccm_get_dbus_conn ();
  _odccm_request_dbus_name (ODCCM_BUS_NAME);

  mgr = g_object_new (ODCCM_TYPE_DEVICE_MANAGER,
		      "device-ip", device_ip,
		      "local-ip", local_ip,
		      "local-netmask", local_netmask,
		      "local-broadcast", local_broadcast,
		      NULL);

  if (run_as_daemon)
    {
      if (chdir ("/") < 0) {
	g_critical("%s: Failed to change to root directory", G_STRFUNC);
        return EXIT_FAILURE;
      }

      pid_t pid = fork ();
      if (pid < 0) {
	g_critical("%s: Failed to create child process", G_STRFUNC);
        return EXIT_FAILURE;
      }

      if (pid > 0)
        return EXIT_SUCCESS;

      if (setsid () < 0)
        return EXIT_FAILURE;

      close (STDIN_FILENO);
      close (STDOUT_FILENO);
      close (STDERR_FILENO);
    }

  g_main_loop_run (mainloop);

  return EXIT_SUCCESS;
}

