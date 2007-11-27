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
#include <syslog.h>
#include <string.h>
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
static gint log_level = 3;

static GOptionEntry options[] =
  {
    { "log-level", 'l', 0, G_OPTION_ARG_INT, &log_level, "Set log level 0 (none) to 6 (debug), default 3", NULL },
    { "foreground", 'f', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &run_as_daemon, "Do not run as a daemon, set log level 6", NULL },
    { "device-ip", 0, 0, G_OPTION_ARG_STRING, &device_ip, "Device's IP address in dotted quad (default " DEFAULT_DEVICE_IP_ADDRESS ")", NULL },
    { "local-ip", 0, 0, G_OPTION_ARG_STRING, &local_ip, "Local IP address in dotted quad (default " DEFAULT_LOCAL_IP_ADDRESS ")", NULL },
    { "local-netmask", 0, 0, G_OPTION_ARG_STRING, &local_netmask, "Local netmask in dotted quad (default " DEFAULT_LOCAL_NETMASK ")", NULL },
    { "local-broadcast", 0, 0, G_OPTION_ARG_STRING, &local_broadcast, "Local broadcast address in dotted quad (default " DEFAULT_LOCAL_BROADCAST ")", NULL },
    { NULL }
  };

void
odccm_log_to_syslog(const gchar *log_domain,
		    GLogLevelFlags log_level,
		    const gchar *message,
		    gpointer user_data)
{
  gboolean is_fatal = (log_level & G_LOG_FLAG_FATAL) != 0;
  gsize msg_len = 0;
  gchar msg_prefix[25];
  gchar *msg;
  gint priority;

  switch (log_level & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_ERROR:
      if (log_level < 1) return;
      strcpy (msg_prefix, "ERROR");
      priority = LOG_ERR;
      break;
    case G_LOG_LEVEL_CRITICAL:
      if (log_level < 2) return;
      strcpy (msg_prefix, "CRITICAL");
      priority = LOG_WARNING;
      break;
    case G_LOG_LEVEL_WARNING:
      if (log_level < 3) return;
      strcpy (msg_prefix, "WARNING");
      priority = LOG_WARNING;
      break;
    case G_LOG_LEVEL_MESSAGE:
      if (log_level < 4) return;
      strcpy (msg_prefix, "Message");
      priority = LOG_INFO;
      break;
    case G_LOG_LEVEL_INFO:
      if (log_level < 5) return;
      strcpy (msg_prefix, "INFO");
      priority = LOG_INFO;
      break;
    case G_LOG_LEVEL_DEBUG:
      if (log_level < 6) return;
      strcpy (msg_prefix, "DEBUG");
      priority = LOG_DEBUG;
      break;
    default:
      if (log_level < 6) return;
      strcpy (msg_prefix, "LOG");
      priority = LOG_DEBUG;
      break;
    }
  if (log_level & G_LOG_FLAG_RECURSION)
    strcat (msg_prefix, " (recursed)");
  strcat(msg_prefix, ": ");

  if (log_domain) {
    msg_len = strlen(msg_prefix) + strlen(log_domain) + 1;
    msg = malloc(msg_len + 1);
    strcpy(msg, log_domain);
    strcat(msg, "-");
  } else {
    msg_len = strlen(msg_prefix);
    msg = malloc(msg_len + 1);
    strcpy(msg, "");
  }
  strcat(msg, msg_prefix);

  if (!message) {
    msg_len = msg_len + 14;
    msg = realloc(msg, msg_len + 1);
    strcat(msg, "(NULL) message");
  } else {
    msg_len = msg_len + strlen(message);
    msg = realloc(msg, msg_len + 1);
    strcat(msg, message);
  }

  if (is_fatal) {
    msg_len = msg_len + 11;
    msg = realloc(msg, msg_len + 1);
    strcat(msg, "aborting...");
  }

  syslog(priority, "%s", msg);
  free(msg);
}

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

      openlog(g_get_prgname(), LOG_PID, LOG_DAEMON);
      g_log_set_default_handler(odccm_log_to_syslog, NULL);

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

