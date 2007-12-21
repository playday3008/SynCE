#include <glib.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"
#include "odccm-client.h"

static gboolean run_as_daemon = TRUE;
static gint max_log_level = 3;
static gboolean address_as_ip = FALSE;

static GOptionEntry options[] =
  {
    { "log-level", 'l', 0, G_OPTION_ARG_INT, &max_log_level, "Set log level 0 (none) to 6 (debug), default 3", NULL },
    { "foreground", 'f', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &run_as_daemon, "Do not run as a daemon, set log level 6", NULL },
    { "use-ip", 'i', 0, G_OPTION_ARG_NONE, &address_as_ip, "Use ip-address of device for identification", NULL },
    { NULL }
  };


gint main(gint argc, gchar *argv[])
{
  GMainLoop *mainloop;
  GError *error = NULL;

  g_type_init ();

  GOptionContext *option_context = g_option_context_new (" - translate odccm dbus comms to vdccm socket");
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

  OdccmClient *client = g_object_new (ODCCM_CLIENT_TYPE,
				      "use-ip", address_as_ip,
				      NULL);

  if (run_as_daemon)
    {
      pid_t pid = fork ();
      if (pid < 0) {
	g_critical("%s: Failed to create child process", G_STRFUNC);
        return EXIT_FAILURE;
      }

      if (pid > 0)
        return EXIT_SUCCESS;

      if (setsid () < 0)
        return EXIT_FAILURE;

      openlog(g_get_prgname(), LOG_PID, LOG_DAEMON);
      g_log_set_default_handler(log_to_syslog, &max_log_level);

      close (STDIN_FILENO);
      close (STDOUT_FILENO);
      close (STDERR_FILENO);
    }

  g_main_loop_run (mainloop);

  if (run_as_daemon) {
    closelog();
  }

  return EXIT_SUCCESS;
}

