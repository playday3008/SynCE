#ifdef HAVE_CONFIG_H
#include "synce_config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "synce-device-manager.h"

#include "log.h"
#include "synce-device.h"
#include "synce-device-rndis.h"
#include "synce-device-legacy.h"
#include "utils.h"

#define BUS_NAME "org.synce.dccm"

/* args */

static gint log_level = 6;
static gboolean log_to_foreground = FALSE;

/* globals */

static GOptionEntry options[] =
  {
    { "log-level", 'l', 0, G_OPTION_ARG_INT, &log_level, "Set log level 0 (none) to 6 (debug), default 3", NULL },
    { "foreground", 'f', 0, G_OPTION_ARG_NONE, &log_to_foreground, "Do not log to system log", NULL },
    { NULL }
  };


gint
main(gint argc,
     gchar *argv[])
{
  GMainLoop *mainloop;
  GError *error = NULL;
  DBusGConnection *main_bus;

  DBusGProxy *main_bus_proxy = NULL;
  guint req_name_result;
  gchar *bus_name = NULL;

  SynceDeviceManager *device_manager = NULL;

  g_type_init ();

  GOptionContext *option_context = g_option_context_new (" - keep connection to Windows Mobile device");
  g_option_context_add_main_entries (option_context, options, NULL);
  g_option_context_parse (option_context, &argc, &argv, &error);

  if (error) {
    switch (error->code)
      {
      case G_OPTION_ERROR_UNKNOWN_OPTION:
	g_critical("%s\n", error->message);
	break;
      case G_OPTION_ERROR_BAD_VALUE:
	g_critical("%s\n", error->message);
	break;
      default:
	g_critical("Unexpected error: %s\n", error->message);
      }
    g_error_free(error);
    return EXIT_FAILURE;
  }
  g_option_context_free(option_context);

  if (!log_to_foreground) {
    openlog(g_get_prgname(), LOG_PID, LOG_DAEMON);
    g_log_set_default_handler(log_to_syslog, &log_level);
    close (STDIN_FILENO);
    close (STDOUT_FILENO);
    close (STDERR_FILENO);
  }

  g_debug("%s: starting ...", G_STRFUNC);

  mainloop = g_main_loop_new (NULL, FALSE);

  if (!(main_bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error))) {
    g_critical("%s: Failed to connect to system bus: %s", G_STRFUNC, error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }

  main_bus_proxy = dbus_g_proxy_new_for_name(main_bus, "org.freedesktop.DBus",
					     "/org/freedesktop/DBus",
					     "org.freedesktop.DBus");
  if (main_bus_proxy == NULL) {
    g_critical("Failed to get proxy to dbus");
    return EXIT_FAILURE;
  }

  bus_name = g_strdup(BUS_NAME);

  if (!dbus_g_proxy_call(main_bus_proxy, "RequestName", &error,
			 G_TYPE_STRING, bus_name,
			 G_TYPE_UINT, DBUS_NAME_FLAG_DO_NOT_QUEUE,
			 G_TYPE_INVALID,
			 G_TYPE_UINT, &req_name_result,
			 G_TYPE_INVALID))
    {
      g_critical("Failed to get bus name %s: %s", bus_name, error->message);
      g_free(bus_name);
      return EXIT_FAILURE;
    }

  device_manager = g_object_new(SYNCE_TYPE_DEVICE_MANAGER, NULL);

  g_main_loop_run (mainloop);

  if (!dbus_g_proxy_call(main_bus_proxy, "ReleaseName", &error,
			 G_TYPE_STRING, bus_name,
			 G_TYPE_INVALID,
			 G_TYPE_UINT, &req_name_result,
			 G_TYPE_INVALID))
    {
      g_critical("Failed to cleanly release bus name %s: %s", bus_name, error->message);
      g_free(bus_name);
      return EXIT_FAILURE;
    }
  g_free(bus_name);
  g_object_unref(main_bus_proxy);

  dbus_g_connection_unref(main_bus);

  g_debug("%s: exiting normally", G_STRFUNC);

  if (!log_to_foreground)
    closelog();

  return EXIT_SUCCESS;
}

