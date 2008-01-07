#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
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

static GOptionEntry options[] =
  {
    { "log-level", 'l', 0, G_OPTION_ARG_INT, &log_level, "Set log level 0 (none) to 6 (debug), default 3", NULL },
    { "device-ip", 0, 0, G_OPTION_ARG_STRING, &device_ip, "Device's IP address in dotted quad format", NULL },
    { "local-ip", 0, 0, G_OPTION_ARG_STRING, &local_ip, "Interface's local IP address in dotted quad format", NULL },
    { "rndis", 0, 0, G_OPTION_ARG_NONE, &rndis_device, "Interface is rndis", NULL },
    { NULL }
  };


static void
device_disconnected_cb(SynceDevice *device,
		       gpointer user_data)
{
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

  GInetAddr *local_inet_addr = gnet_tcp_socket_get_local_inetaddr (conn->socket);
  gint local_port = gnet_inetaddr_get_port(local_inet_addr);

  g_debug("%s: have a connection to port %d", G_STRFUNC, local_port);

  if (local_port == 5679) {
    if (!synce_dev) {
      gnet_server_delete(server_990);
      synce_dev = g_object_new (SYNCE_TYPE_DEVICE_LEGACY, "connection", conn, NULL);
      g_signal_connect(synce_dev, "disconnected", G_CALLBACK(device_disconnected_cb), user_data);
    }
  } else {
    if (!synce_dev) {
      gnet_server_delete(server_5679);
      synce_dev = g_object_new (SYNCE_TYPE_DEVICE_RNDIS, "connection", conn, NULL);
      g_signal_connect(synce_dev, "disconnected", G_CALLBACK(device_disconnected_cb), user_data);
    } else {
      synce_device_rndis_client_connected (SYNCE_DEVICE_RNDIS(synce_dev), conn);
    }
  }

  gnet_inetaddr_unref(local_inet_addr);
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


gint
main(gint argc,
     gchar *argv[])
{
  GMainLoop *mainloop;
  GError *error = NULL;
  DBusError dbus_error;
  DBusGConnection *main_bus;
  LibHalContext *main_ctx;

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

  openlog(g_get_prgname(), LOG_PID, LOG_LOCAL5);
  g_log_set_default_handler(log_to_syslog, &log_level);

  close (STDIN_FILENO);
  close (STDOUT_FILENO);
  close (STDERR_FILENO);

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

  /* **************** */

  gchar *bytes = ip4_bytes_from_dotted_quad(device_ip);
  GInetAddr *device_inetaddr = gnet_inetaddr_new_bytes(bytes, 4);
  g_free(bytes);

  gint addr_length = gnet_inetaddr_get_length(device_inetaddr);
  bytes = g_malloc0(addr_length);
  gnet_inetaddr_get_bytes(device_inetaddr, bytes);
  g_debug("%s: ip from device_inetaddr: %u.%u.%u.%u", G_STRFUNC, (guint8)bytes[0], (guint8)bytes[1], (guint8)bytes[2], (guint8)bytes[3]);
  g_free(bytes);

  /* *************** 

  GInetAddr *local_from_device_inetaddr = gnet_inetaddr_get_interface_to(device_inetaddr);
  addr_length = gnet_inetaddr_get_length(local_from_device_inetaddr);
  bytes = g_malloc0(addr_length);
  gnet_inetaddr_get_bytes(local_from_device_inetaddr, bytes);
  g_debug("%s: ip from get_interface: %u.%u.%u.%u", G_STRFUNC, (guint8)bytes[0], (guint8)bytes[1], (guint8)bytes[2], (guint8)bytes[3]);
  g_free(bytes);
  gnet_inetaddr_unref(local_from_device_inetaddr);

   *************** */

  bytes = ip4_bytes_from_dotted_quad(local_ip);
  GInetAddr *local_inetaddr = gnet_inetaddr_new_bytes(bytes, 4);
  g_free(bytes);

  addr_length = gnet_inetaddr_get_length(local_inetaddr);
  bytes = g_malloc0(addr_length);
  gnet_inetaddr_get_bytes(local_inetaddr, bytes);
  g_debug("%s: ip from local_inetaddr: %u.%u.%u.%u", G_STRFUNC, (guint8)bytes[0], (guint8)bytes[1], (guint8)bytes[2], (guint8)bytes[3]);
  g_free(bytes);

  if (gnet_inetaddr_is_private(local_inetaddr))
    g_debug("%s: local ip works as private", G_STRFUNC);
  if (gnet_inetaddr_is_ipv4(local_inetaddr))
    g_debug("%s: local ip works as ipv4", G_STRFUNC);

  /* *************** */

  GList* addr_list = gnet_inetaddr_list_interfaces();

  GList *tmp = g_list_first(addr_list);
  bytes = ip4_bytes_from_dotted_quad(local_ip);

  while (tmp) {

    addr_length = gnet_inetaddr_get_length((GInetAddr*)tmp->data);
    bytes = g_malloc0(addr_length);
    gnet_inetaddr_get_bytes((GInetAddr*)tmp->data, bytes);
    g_debug("%s: list interfaces : %u.%u.%u.%u", G_STRFUNC, (guint8)bytes[0], (guint8)bytes[1], (guint8)bytes[2], (guint8)bytes[3]);
    g_free(bytes);

    gnet_inetaddr_unref((GInetAddr*)tmp->data);

    tmp = g_list_next(tmp);
  }

  g_list_free(addr_list);

  /* *************** */

  GInetAddr *server_990_addr = gnet_inetaddr_clone(local_inetaddr);

  gnet_inetaddr_set_port(server_990_addr, 990);
  /* gnet_inetaddr_set_port(local_inetaddr, 990); */
  /* server_990 = gnet_server_new (server_990_addr, 990, client_connected_cb, mainloop); */
  server_990 = gnet_server_new (NULL, 990, client_connected_cb, mainloop);
  if (!server_990) {
    g_error("%s: server_990 invalid", G_STRFUNC);
  }

  GInetAddr *server_5679_addr = gnet_inetaddr_clone(local_inetaddr);

  gnet_inetaddr_set_port(server_5679_addr, 5679);
  /* gnet_inetaddr_set_port(local_inetaddr, 5679); */
  /* server_5679 = gnet_server_new (server_5679_addr, 5679, client_connected_cb, mainloop); */
  server_5679 = gnet_server_new (NULL, 5679, client_connected_cb, mainloop);
  if (!server_5679)
    g_error("%s: server_5679 invalid", G_STRFUNC);


  gnet_inetaddr_unref(server_990_addr);
  gnet_inetaddr_unref(server_5679_addr);
  gnet_inetaddr_unref(local_inetaddr);
  gnet_inetaddr_unref(device_inetaddr);


  if (rndis_device)
    synce_trigger_connection (device_ip);

  /* ********************************* */

  g_main_loop_run (mainloop);

  closelog();

  g_debug("%s: exiting normally", G_STRFUNC);

  return EXIT_SUCCESS;
}

