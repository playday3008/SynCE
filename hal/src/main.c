#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <glib.h>
#include <glib-object.h>
#include <gnet.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libhal.h>

#include "log.h"
#include "synce-device.h"
#include "synce-device-rndis.h"
#include "synce-device-legacy.h"
#include "utils.h"

/* args */

static gchar *device_ip = NULL;
static gchar *local_ip = NULL;
static gint log_level = 3;
static gboolean rndis_device = FALSE;

/* globals */

static GServer *server_990;
static GServer *server_5679;
static SynceDevice *synce_dev;

static int signal_pipe[2];

static GOptionEntry options[] =
  {
    { "log-level", 'l', 0, G_OPTION_ARG_INT, &log_level, "Set log level 0 (none) to 6 (debug), default 3", NULL },
    { "device-ip", 0, 0, G_OPTION_ARG_STRING, &device_ip, "Device's IP address in dotted quad format", NULL },
    { "local-ip", 0, 0, G_OPTION_ARG_STRING, &local_ip, "Interface's local IP address in dotted quad format", NULL },
    { "rndis", 0, 0, G_OPTION_ARG_NONE, &rndis_device, "Interface is rndis", NULL },
    { NULL }
  };


/* handle the sigterm we get from hal when device is removed */

static void
pipe_posix_signals(int signal)
{
  if(write(signal_pipe[1], &signal, sizeof(int)) != sizeof(int))
    g_error("%s: Could not handle SIGTERM", G_STRFUNC);
}


static gboolean
deliver_posix_signal(GIOChannel *source, GIOCondition condition, gpointer data)
{
  GMainLoop *mainloop = (GMainLoop *)data;
  GError *error = NULL;

  GIOStatus status;
  gsize bytes_read;
  int sig;
  
  gchar *buf = g_malloc0(sizeof(int));

  while((status = g_io_channel_read_chars(source, buf, sizeof(int), &bytes_read, &error)) == G_IO_STATUS_NORMAL)
    {
      if(bytes_read != sizeof(int)){
	g_critical("%s: lost data in signal pipe (expected %lu, received %lu)", G_STRFUNC, sizeof(int), bytes_read);
	continue;
      }

      sig = *((int*)buf);

      if (sig == SIGTERM) {
	g_debug("%s: received SIGTERM, exiting normally", G_STRFUNC);
	g_main_loop_quit(mainloop);
      } else {
	g_warning("%s: handling signal %d, not supposed to happen", G_STRFUNC, sig);
      }

    }

  if (status == G_IO_STATUS_ERROR) {
    g_critical("%s: reading signal pipe failed: %s", G_STRFUNC, error->message);
    g_main_loop_quit(mainloop);
  }
  if (status == G_IO_STATUS_EOF) {
    g_critical("%s: signal pipe has been closed", G_STRFUNC);
    g_main_loop_quit(mainloop);
  }

  g_free(buf);
  return TRUE;
}


static void
device_disconnected_cb(SynceDevice *device,
		       gpointer user_data)
{
  g_debug("%s: receieved disconnect from device", G_STRFUNC);
  g_object_unref(synce_dev);
  g_main_loop_quit((GMainLoop*)user_data);
}


static void
client_connected_cb (GServer *server,
                     GConn *conn,
                     gpointer user_data)
{
  if (conn == NULL) {
    g_critical("%s: a connection error occured", G_STRFUNC);
    return;
  }

  GMainLoop *mainloop = (GMainLoop *)user_data;

  GInetAddr *local_inet_addr = gnet_tcp_socket_get_local_inetaddr (conn->socket);
  gint local_port = gnet_inetaddr_get_port(local_inet_addr);
  gnet_inetaddr_unref(local_inet_addr);

  g_debug("%s: have a connection to port %d", G_STRFUNC, local_port);

  if (local_port == 5679) {
    if (!synce_dev) {
      gnet_server_delete(server_990);
      synce_dev = g_object_new (SYNCE_TYPE_DEVICE_LEGACY, "connection", conn, NULL);
      g_signal_connect(synce_dev, "disconnected", G_CALLBACK(device_disconnected_cb), mainloop);
    }
  } else {
    if (!synce_dev) {
      gnet_server_delete(server_5679);
      synce_dev = g_object_new (SYNCE_TYPE_DEVICE_RNDIS, "connection", conn, NULL);
      g_signal_connect(synce_dev, "disconnected", G_CALLBACK(device_disconnected_cb), mainloop);
    } else {
      synce_device_rndis_client_connected (SYNCE_DEVICE_RNDIS(synce_dev), conn);
      return;
    }
  }

  gnet_conn_unref (conn);
}

static void
hal_device_removed_callback(LibHalContext *ctx,
			    const char *udi)
{
  const gchar *our_udi = g_getenv("HAL_PROP_INFO_UDI");
  if (strcmp(our_udi, udi) != 0) 
    return;

  g_debug("%s: received hal disconnect for our device", G_STRFUNC);
  GMainLoop *mainloop = libhal_ctx_get_user_data(ctx);
  g_main_loop_quit(mainloop);

  return;
}

static void
iface_list_free_func(gpointer data,
		     gpointer user_data)
{
  gnet_inetaddr_unref((GInetAddr*)data);
}

static gboolean
check_interface_cb (gpointer data)
{
  GMainLoop *mainloop = (GMainLoop *)data;

  gchar *tmp_bytes, *local_ip_bytes;
  GInetAddr *local_iface = NULL;
  gint addr_length;

  GList *iface_list = gnet_inetaddr_list_interfaces();
  GList *iface_list_iter = g_list_first(iface_list);

  local_ip_bytes = ip4_bytes_from_dotted_quad(local_ip);

  while (iface_list_iter) {
    addr_length = gnet_inetaddr_get_length((GInetAddr*)iface_list_iter->data);
    tmp_bytes = g_malloc0(addr_length);
    gnet_inetaddr_get_bytes((GInetAddr*)iface_list_iter->data, tmp_bytes);

    if ((*(guint32*)tmp_bytes) == (*(guint32*)local_ip_bytes)) {
      g_free(tmp_bytes);
      local_iface = (GInetAddr*)iface_list_iter->data;
      gnet_inetaddr_ref(local_iface);
      break;
    }

    g_free(tmp_bytes);

    iface_list_iter = g_list_next(iface_list_iter);
  }

  g_list_foreach(iface_list, iface_list_free_func, NULL);
  g_list_free(iface_list);
  g_free(local_ip_bytes);

  if (!local_iface) {
    return TRUE;
  }

  g_debug("%s: found device interface", G_STRFUNC);

  GInetAddr *server_990_addr = gnet_inetaddr_clone(local_iface);
  gnet_inetaddr_set_port(server_990_addr, 990);

  server_990 = gnet_server_new (server_990_addr, 990, client_connected_cb, mainloop);
  if (!server_990) {
    g_critical("%s: unable to listen on rndis port (990), server invalid", G_STRFUNC);
    g_main_loop_quit(mainloop);
  }

  GInetAddr *server_5679_addr = gnet_inetaddr_clone(local_iface);
  gnet_inetaddr_set_port(server_5679_addr, 5679);

  server_5679 = gnet_server_new (server_5679_addr, 5679, client_connected_cb, mainloop);
  if (!server_5679) {
    g_critical("%s: unable to listen on legacy port (5679), server invalid", G_STRFUNC);
    g_main_loop_quit(mainloop);
  }

  gnet_inetaddr_unref(server_990_addr);
  gnet_inetaddr_unref(server_5679_addr);
  gnet_inetaddr_unref(local_iface);

  g_debug("%s: listening for device", G_STRFUNC);

  if (rndis_device)
    synce_trigger_connection (device_ip);

  return FALSE;
}

gint
main(gint argc,
     gchar *argv[])
{
  GMainLoop *mainloop;
  GError *error = NULL;
  DBusError dbus_error;
  DBusGConnection *main_bus;
  LibHalContext *main_ctx;
  GIOChannel *signal_in = NULL;
  long fd_flags;
  struct sigaction *sigact = NULL;

  g_type_init ();
  gnet_init();

  openlog(g_get_prgname(), LOG_PID, LOG_LOCAL5);
  g_log_set_default_handler(log_to_syslog, &log_level);

  g_debug("%s: starting ...", G_STRFUNC);

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

  if (!device_ip) {
    g_critical("%s: device ip address required", G_STRFUNC);
    return EXIT_FAILURE;
  }
  if (!local_ip) {
    g_critical("%s: local ip address required", G_STRFUNC);
    return EXIT_FAILURE;
  }

  mainloop = g_main_loop_new (NULL, FALSE);

  /* catch the SIGTERM we get from hal on device disconnect */

  if(pipe(signal_pipe)) {
    g_critical("%s: failed to create signal processing pipe: %s", G_STRFUNC, g_strerror(errno));
    return EXIT_FAILURE;
  }

  /* set write end to nonblocking mode */
  fd_flags = fcntl(signal_pipe[1], F_GETFL);
  if(fd_flags == -1)
    {
      g_critical("%s: failed to read signal processing pipe flags: %s", G_STRFUNC, g_strerror(errno));
      return EXIT_FAILURE;
    }
  if(fcntl(signal_pipe[1], F_SETFL, fd_flags | O_NONBLOCK) == -1)
    {
      g_critical("%s: failed to write signal processing pipe flags: %s", G_STRFUNC, g_strerror(errno));
      return EXIT_FAILURE;
    }

  /* convert the reading end of the pipe into a GIOChannel */
  signal_in = g_io_channel_unix_new(signal_pipe[0]);

  if (g_io_channel_set_encoding(signal_in, NULL, &error) != G_IO_STATUS_NORMAL) {
    g_critical("%s: failed to set encoding on signal processing pipe: %s", G_STRFUNC, error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }

  /* put the reading end also into non-blocking mode */
  if (g_io_channel_set_flags(signal_in, g_io_channel_get_flags(signal_in) | G_IO_FLAG_NONBLOCK, &error) != G_IO_STATUS_NORMAL) {
    g_critical("%s: failed to set non blocking on signal processing pipe: %s", G_STRFUNC, error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }

  g_io_add_watch(signal_in, G_IO_IN | G_IO_PRI, deliver_posix_signal, mainloop);

  /* Install the unix signal handler pipe_signals for the signals of interest */

  sigact = g_new0(struct sigaction, 1);
  sigact->sa_handler = pipe_posix_signals;
  if (sigaction(SIGTERM, sigact, NULL) != 0) {
    g_critical("%s: failed to set SIGTERM signal handler", G_STRFUNC);
    return EXIT_FAILURE;
  }

  g_free(sigact);


  close (STDIN_FILENO);
  close (STDOUT_FILENO);
  close (STDERR_FILENO);

  g_debug("%s: called with device-ip=%s, local-ip=%s", G_STRFUNC, device_ip, local_ip);

  dbus_error_init(&dbus_error);

  if (!(main_bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error))) {
    g_critical("%s: Failed to connect to system bus: %s", G_STRFUNC, error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }
  if (!(main_ctx = libhal_ctx_new())) {
    g_critical("%s: failed to get libhal context", G_STRFUNC);
    return EXIT_FAILURE;
  }
  if (!(libhal_ctx_set_dbus_connection(main_ctx, dbus_g_connection_get_connection(main_bus)))) {
    g_critical("%s: failed to set hal context dbus connection", G_STRFUNC);
    return EXIT_FAILURE;
  }
  if (!(libhal_ctx_set_user_data(main_ctx, mainloop))) {
    g_critical("%s: failed to set hal context user data", G_STRFUNC);
    return EXIT_FAILURE;
  }
  if (!(libhal_ctx_init(main_ctx, &dbus_error))) {
    g_critical("%s: failed to initialise hal context: %s: %s", G_STRFUNC, dbus_error.name, dbus_error.message);
    return EXIT_FAILURE;
  }
  if (!(libhal_ctx_set_device_removed(main_ctx, hal_device_removed_callback))) {
    g_critical("%s: failed to set device removed cb", G_STRFUNC);
    return EXIT_FAILURE;
  }

  g_debug("%s: connected to hal, waiting for interface...", G_STRFUNC);

  g_timeout_add (100, check_interface_cb, mainloop);

  g_main_loop_run (mainloop);

  g_debug("%s: exiting normally", G_STRFUNC);

  closelog();

  return EXIT_SUCCESS;
}

