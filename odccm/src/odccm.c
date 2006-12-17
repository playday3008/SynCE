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
#include "odccm-device-manager.h"
#include "util.h"

#define ODCCM_BUS_NAME "org.synce.odccm"

static void
print_usage (const gchar *name)
{
  printf ("Usage:\n"
          "\t%s [-f]\n\n"
          "\t-f           Do not run as a daemon\n\n",
          name);
}

gint main(gint argc, gchar *argv[])
{
  gint c;
  gboolean run_as_daemon = TRUE;
  GMainLoop *mainloop;
  OdccmDeviceManager *mgr;
  DBusGConnection *bus;

  while ((c = getopt (argc, argv, "f")) != -1)
    {
      switch (c)
        {
          case 'f':
            run_as_daemon = FALSE;
            break;
          default:
            print_usage (argv[0]);
            return EXIT_FAILURE;
        }
    }

  g_type_init ();

  mainloop = g_main_loop_new (NULL, FALSE);

  bus = _odccm_get_dbus_conn ();
  _odccm_request_dbus_name (ODCCM_BUS_NAME);

  mgr = g_object_new (ODCCM_TYPE_DEVICE_MANAGER, NULL);

  if (run_as_daemon)
    {
      pid_t pid = fork ();
      if (pid < 0)
        return EXIT_FAILURE;

      if (pid > 0)
        return EXIT_FAILURE;

      if (setsid () < 0)
        return EXIT_FAILURE;

      if (chdir ("/") < 0)
        return EXIT_FAILURE;

      close (STDIN_FILENO);
      close (STDOUT_FILENO);
      close (STDERR_FILENO);
    }

  g_main_loop_run (mainloop);

  return EXIT_SUCCESS;
}

