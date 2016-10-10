#ifdef HAVE_CONFIG_H
#include "config.h"
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
#include <gio/gio.h>

#include "synce-device-manager.h"

#include "log.h"
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

SynceDeviceManager *device_manager = NULL;

static void
bus_acquired_handler(G_GNUC_UNUSED GDBusConnection *connection, G_GNUC_UNUSED const gchar *name, gpointer user_data)
{
  GMainLoop *mainloop = (GMainLoop*)user_data;
  GError *error = NULL;

  /* have a bus, set up device_manager */

  g_debug("%s: bus acquired, creating device manager", G_STRFUNC);

  device_manager = g_initable_new(SYNCE_TYPE_DEVICE_MANAGER, NULL, &error, NULL);
  if (!device_manager) {
    g_critical("Failed to create device manager: %s", error->message);
    g_error_free(error);
    g_main_loop_quit(mainloop);
    return;
  }
}

static void
name_acquired_handler(G_GNUC_UNUSED GDBusConnection *connection, G_GNUC_UNUSED const gchar *name, G_GNUC_UNUSED gpointer user_data)
{
  g_debug("%s: bus name acquired", G_STRFUNC);
}

static void
name_lost_handler(GDBusConnection *connection, G_GNUC_UNUSED const gchar *name, gpointer user_data)
{
  GMainLoop *mainloop = (GMainLoop*)user_data;

  /* probably quit, whatever the reason */

  if (!connection)
    if (!device_manager)
      g_critical("%s: unable to connect to dbus, exiting ...", G_STRFUNC);
    else
      g_critical("%s: lost connection to dbus, exiting ...", G_STRFUNC);
  else
    g_critical("%s: lost our name on dbus, exiting ...", G_STRFUNC);

  if (device_manager) g_object_unref(device_manager);
  g_main_loop_quit(mainloop);
  return;
}


gint
main(gint argc,
     gchar *argv[])
{
  GMainLoop *mainloop;
  GError *error = NULL;
  guint req_name_result;

#if !GLIB_CHECK_VERSION (2, 36, 0)
  g_type_init ();
#endif

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

  req_name_result = g_bus_own_name(G_BUS_TYPE_SYSTEM,
				   BUS_NAME,
				   G_BUS_NAME_OWNER_FLAGS_NONE,
				   bus_acquired_handler,
				   name_acquired_handler,
				   name_lost_handler,
				   mainloop,
				   NULL);

  g_main_loop_run (mainloop);

  g_bus_unown_name(req_name_result);

  g_debug("%s: exiting normally", G_STRFUNC);

  if (!log_to_foreground)
    closelog();

  return EXIT_SUCCESS;
}

