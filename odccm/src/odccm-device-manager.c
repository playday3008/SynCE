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

#include <gnet.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libhal.h>
#include <linux/netlink.h>
#include <sys/types.h>
#include <unistd.h>

#include "odccm-device-manager.h"

#include "odccm-device-manager-glue.h"

#include "odccm-device.h"
#ifdef ENABLE_LEGACY_SUPPORT
#include "odccm-device-legacy.h"
#endif
#include "util.h"

/* FIXME: make these configurable */
#define DEVICE_IP_ADDRESS "169.254.2.1"
#define LOCAL_IP_ADDRESS "169.254.2.2"
#define LOCAL_NETMASK    "255.255.255.0"
#define LOCAL_BROADCAST  "169.254.2.255"

G_DEFINE_TYPE (OdccmDeviceManager, odccm_device_manager, G_TYPE_OBJECT)

/* signals */
enum
{
  DEVICE_ATTACHED,
  DEVICE_DETACHED,
  DEVICE_CONNECTED,
  DEVICE_DISCONNECTED,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

/* private stuff */
typedef struct _OdccmDeviceManagerPrivate OdccmDeviceManagerPrivate;

struct _OdccmDeviceManagerPrivate
{
  gboolean dispose_has_run;

  GServer *server;
#ifdef ENABLE_LEGACY_SUPPORT
  GServer *legacy_server;
#endif

  GSList *devices;

  LibHalContext *hal_ctx;
  gchar *udi;
  gchar *ifname;
  GIOChannel *udev_chann;
};

#define ODCCM_DEVICE_MANAGER_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), ODCCM_TYPE_DEVICE_MANAGER, OdccmDeviceManagerPrivate))

static void
odccm_device_manager_init (OdccmDeviceManager *self)
{
}

static void client_connected_cb (GServer *server, GConn *conn, gpointer user_data);
static void init_hal (OdccmDeviceManager *self);
static void init_udev (OdccmDeviceManager *self);

static GObject *
odccm_device_manager_constructor (GType type, guint n_props,
                                  GObjectConstructParam *props)
{
  GObject *obj;
  OdccmDeviceManagerPrivate *priv;

  obj = G_OBJECT_CLASS (odccm_device_manager_parent_class)->
    constructor (type, n_props, props);

  priv = ODCCM_DEVICE_MANAGER_GET_PRIVATE (obj);

  priv->server = gnet_server_new (NULL, 990, client_connected_cb, obj);
#ifdef ENABLE_LEGACY_SUPPORT
  priv->legacy_server = gnet_server_new (NULL, 5679, client_connected_cb, obj);
#endif

  dbus_g_connection_register_g_object (_odccm_get_dbus_conn (),
                                       DEVICE_MANAGER_OBJECT_PATH, obj);

  init_hal (ODCCM_DEVICE_MANAGER (obj));
  init_udev (ODCCM_DEVICE_MANAGER (obj));

  return obj;
}

static void
odccm_device_manager_dispose (GObject *obj)
{
  OdccmDeviceManager *self = ODCCM_DEVICE_MANAGER (obj);
  OdccmDeviceManagerPrivate *priv = ODCCM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  if (G_OBJECT_CLASS (odccm_device_manager_parent_class)->dispose)
    G_OBJECT_CLASS (odccm_device_manager_parent_class)->dispose (obj);
}

static void
odccm_device_manager_finalize (GObject *obj)
{
  OdccmDeviceManager *self = ODCCM_DEVICE_MANAGER (obj);
  OdccmDeviceManagerPrivate *priv = ODCCM_DEVICE_MANAGER_GET_PRIVATE (self);

  g_free (priv->udi);
  g_free (priv->ifname);

  G_OBJECT_CLASS (odccm_device_manager_parent_class)->finalize (obj);
}

static void
odccm_device_manager_class_init (OdccmDeviceManagerClass *dev_mgr_class)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (dev_mgr_class);

  g_type_class_add_private (dev_mgr_class,
                            sizeof (OdccmDeviceManagerPrivate));

  obj_class->constructor = odccm_device_manager_constructor;

  obj_class->dispose = odccm_device_manager_dispose;
  obj_class->finalize = odccm_device_manager_finalize;

  signals[DEVICE_ATTACHED] =
    g_signal_new ("device-attached",
                  G_OBJECT_CLASS_TYPE (dev_mgr_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  signals[DEVICE_DETACHED] =
    g_signal_new ("device-detached",
                  G_OBJECT_CLASS_TYPE (dev_mgr_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  signals[DEVICE_CONNECTED] =
    g_signal_new ("device-connected",
                  G_OBJECT_CLASS_TYPE (dev_mgr_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  signals[DEVICE_DISCONNECTED] =
    g_signal_new ("device-disconnected",
                  G_OBJECT_CLASS_TYPE (dev_mgr_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (dev_mgr_class),
                                   &dbus_glib_odccm_device_manager_object_info);
}

static void device_obj_path_changed_cb (GObject *obj, GParamSpec *param, gpointer user_data);

static void
client_connected_cb (GServer *server,
                     GConn *conn,
                     gpointer user_data)
{
  OdccmDeviceManager *self = ODCCM_DEVICE_MANAGER (user_data);
  OdccmDeviceManagerPrivate *priv = ODCCM_DEVICE_MANAGER_GET_PRIVATE (self);
  struct in_addr addr;
  OdccmDevice *dev = NULL;
  GSList *cur;

  if (conn == NULL)
    {
      g_warning ("%s: an error occurred", G_STRFUNC);
      return;
    }

  gnet_inetaddr_get_bytes (gnet_tcp_socket_get_remote_inetaddr (conn->socket),
                           (gchar *) &(addr.s_addr));

  for (cur = priv->devices; cur != NULL && dev == NULL; cur = cur->next)
    {
      guint32 cur_addr;

      g_object_get (cur->data, "ip-address", &cur_addr, NULL);

      if (cur_addr == addr.s_addr)
        {
          dev = cur->data;
        }
    }

  if (dev != NULL)
    {
      _odccm_device_client_connected (dev, conn);
    }
  else
    {
#ifdef ENABLE_LEGACY_SUPPORT
      GInetAddr *local_inet_addr = gnet_tcp_socket_get_local_inetaddr (conn->socket);
      if (gnet_inetaddr_get_port(local_inet_addr) == 5679)
	dev = g_object_new (ODCCM_TYPE_DEVICE_LEGACY, "connection", conn, NULL);
      else
	dev = g_object_new (ODCCM_TYPE_DEVICE, "connection", conn, NULL);
      gnet_inetaddr_unref(local_inet_addr);
#else
      dev = g_object_new (ODCCM_TYPE_DEVICE, "connection", conn, NULL);
#endif

      priv->devices = g_slist_append (priv->devices, dev);

      g_signal_connect (dev, "notify::object-path",
                        (GCallback) device_obj_path_changed_cb,
                        self);

      gnet_conn_unref (conn);
    }
}

static void
device_obj_path_changed_cb (GObject    *obj,
                            GParamSpec *param,
                            gpointer    user_data)
{
  OdccmDeviceManager *self = ODCCM_DEVICE_MANAGER (user_data);
  OdccmDevice *dev = ODCCM_DEVICE (obj);

  if (_odccm_device_is_identified (dev))
    {
      gchar *obj_path;

      g_object_get (dev, "object-path", &obj_path, NULL);

      g_signal_emit (self, signals[DEVICE_CONNECTED], 0, obj_path);

      g_free (obj_path);
    }
}

gboolean
odccm_device_manager_get_connected_devices (OdccmDeviceManager *self,
                                            GPtrArray **ret,
                                            GError **error)
{
  OdccmDeviceManagerPrivate *priv = ODCCM_DEVICE_MANAGER_GET_PRIVATE (self);
  GSList *cur;

  *ret = g_ptr_array_new ();
  for (cur = priv->devices; cur != NULL; cur = cur->next)
    {
      OdccmDevice *dev = cur->data;

      if (_odccm_device_is_identified (dev))
        {
          gchar *obj_path;

          g_object_get (dev, "object-path", &obj_path, NULL);
          g_ptr_array_add (*ret, obj_path);
        }
    }

  return TRUE;
}

static gboolean
interface_timed_cb (gpointer data)
{
  OdccmDeviceManager *self = ODCCM_DEVICE_MANAGER (data);
  OdccmDeviceManagerPrivate *priv = ODCCM_DEVICE_MANAGER_GET_PRIVATE (self);

  /* Has the device disappeared for some reason? */
  if (priv->udi == NULL)
    return FALSE;

  /* Has it connected? */
  if (priv->devices != NULL)
    return FALSE;

  /* Has the device been re-configured? */
  if (!_odccm_interface_is_configured (priv->ifname, LOCAL_IP_ADDRESS))
    {
      /* It has, reconfigure and try again on the next tick. */
      _odccm_configure_interface (priv->ifname, LOCAL_IP_ADDRESS, LOCAL_NETMASK,
                                  LOCAL_BROADCAST);

      return TRUE;
    }

  /* Send a trigger-packet to make the device connect. */
  _odccm_trigger_connection (self);

  return TRUE;
}

static int
hal_device_is_pda (LibHalContext *ctx, const char *udi, gchar **ret_ifname)
{
  int result = 0;

  DBusError error;
  dbus_error_init (&error);

  /* Be sure it is a network interface */
  gchar *ifname = libhal_device_get_property_string (ctx, udi, "net.interface",
      &error);
  if (ifname == NULL) goto DONE;
  if (ret_ifname != NULL) *ret_ifname = g_strdup(ifname);
  libhal_free_string (ifname);

  /* We'll then check some properties of its parent */
  gchar *parentname = libhal_device_get_property_string (ctx, udi,
      "info.parent", &error);
  if (parentname == NULL) goto DONE;

  /* Check the parent's device driver name (for usb-rndis-lite) */
  gchar *drvname = libhal_device_get_property_string (ctx, parentname,
      "info.linux.driver", &error);
  if (drvname != NULL)
    {
      if (strncmp ("rndis_host", drvname, 11) == 0) result = 1;
      libhal_free_string (drvname);
    }

  /* Check pda.platform property (for usb-rndis-ng) */
  gchar *pdaplatform = libhal_device_get_property_string (ctx, parentname,
      "pda.platform", &error);
  if (pdaplatform != NULL)
    {
      if (strncmp ("pocketpc", pdaplatform, 8) == 0) result = 1;
      libhal_free_string (pdaplatform);
    }

  libhal_free_string (parentname);

DONE:
  dbus_error_free (&error);
  /*g_debug ("%s: udi='%s', result=%d", G_STRFUNC, udi, result);*/
  return result;
}

static void
hal_device_added_cb (LibHalContext *ctx, const gchar *udi)
{
  OdccmDeviceManager *self = ODCCM_DEVICE_MANAGER (
      libhal_ctx_get_user_data (ctx));
  OdccmDeviceManagerPrivate *priv = ODCCM_DEVICE_MANAGER_GET_PRIVATE (self);
  gchar *ifname;

  if (hal_device_is_pda (ctx, udi, &ifname))
    {
      g_debug ("PDA network interface discovered! udi='%s'", udi);

      if (priv->udi != NULL)
        {
          g_warning ("Only the first device discovered will be auto-"
                     "configured for now");
          return;
        }

      priv->udi = g_strdup (udi);
      priv->ifname = g_strdup (ifname);

      _odccm_configure_interface (ifname, LOCAL_IP_ADDRESS, LOCAL_NETMASK,
                                  LOCAL_BROADCAST);

      g_timeout_add (50, interface_timed_cb, self);
    }
}

static void
hal_device_removed_cb (LibHalContext *ctx, const gchar *udi)
{
  OdccmDeviceManager *self = ODCCM_DEVICE_MANAGER (
      libhal_ctx_get_user_data (ctx));
  OdccmDeviceManagerPrivate *priv = ODCCM_DEVICE_MANAGER_GET_PRIVATE (self);
  GSList *cur;

  if (priv->udi == NULL)
    return;

  if (strcmp (udi, priv->udi) != 0)
    return;

  g_debug ("PDA disconnected! udi='%s', device='%s'", priv->udi, priv->ifname);

  /* FIXME: Hack.  We should have a hashtable mapping UDIs to IP-addresses and
   *        use that instead of this ugly approach... */
  for (cur = priv->devices; cur != NULL; cur = cur->next)
    {
      OdccmDevice *dev = cur->data;
      guint32 addr;

      g_object_get (dev, "ip-address", &addr, NULL);

      if (addr == inet_addr (DEVICE_IP_ADDRESS))
        {
          gchar *obj_path;

          g_object_get (dev, "object-path", &obj_path, NULL);
          g_signal_emit (self, signals[DEVICE_DISCONNECTED], 0, obj_path);
          g_free (obj_path);

          priv->devices = g_slist_delete_link (priv->devices, cur);
          g_object_unref (dev);

          break;
        }
    }

  g_free (priv->udi);
  priv->udi = NULL;
  g_free (priv->ifname);
  priv->ifname = NULL;
}

static void
init_hal (OdccmDeviceManager *self)
{
  OdccmDeviceManagerPrivate *priv = ODCCM_DEVICE_MANAGER_GET_PRIVATE (self);
  gboolean success = FALSE, initialized = FALSE;
  DBusConnection *conn;
  const gchar *func_name = "";
  DBusError error;

  dbus_error_init (&error);

  if ((conn = dbus_bus_get (DBUS_BUS_SYSTEM, &error)) == NULL)
    {
      func_name = "dbus_bus_get";
      goto DBUS_ERROR;
    }

  dbus_connection_setup_with_g_main (conn, NULL);

  priv->hal_ctx = libhal_ctx_new ();
  if (priv->hal_ctx == NULL)
    goto OUT;

  if (!libhal_ctx_set_dbus_connection (priv->hal_ctx, conn))
    goto OUT;

  if (!libhal_ctx_init (priv->hal_ctx, &error))
    {
      func_name = "libhal_ctx_init";
      goto DBUS_ERROR;
    }

  initialized = TRUE;

  if (!libhal_ctx_set_user_data (priv->hal_ctx, self))
    goto OUT;

  if (!libhal_ctx_set_device_added (priv->hal_ctx, hal_device_added_cb))
    goto OUT;

  if (!libhal_ctx_set_device_removed (priv->hal_ctx, hal_device_removed_cb))
    goto OUT;

  success = TRUE;
  goto OUT;

DBUS_ERROR:
  g_warning ("%s failed with D-Bus error %s: %s\n",
             func_name, error.name, error.message);

  dbus_error_free (&error);

OUT:
  if (!success)
    {
      if (!initialized)
        {
          libhal_ctx_free (priv->hal_ctx);
          priv->hal_ctx = NULL;
        }

      g_warning ("%s: failed", G_STRFUNC);
    }
}

static void
udev_device_added (gpointer self, gchar *ifname, gchar *udi)
{
  OdccmDeviceManagerPrivate *priv = ODCCM_DEVICE_MANAGER_GET_PRIVATE (self);

  // Check if this is "our" interface
  if (!_odccm_interface_address (ifname, LOCAL_IP_ADDRESS)) 
     return;

  g_debug ("PDA network interface discovered! udi='%s'", udi);

  priv->udi = g_strdup (udi);
  priv->ifname = g_strdup (ifname);
 
  g_timeout_add (50, interface_timed_cb, self);
}

static void
udev_device_removed (gpointer self, gchar *ifname, gchar *udi)
{
  OdccmDeviceManagerPrivate *priv = ODCCM_DEVICE_MANAGER_GET_PRIVATE (self);
  GSList *cur;

  if (priv->udi == NULL)
    return;

  if (strcmp (udi, priv->udi) != 0)
    return;

  g_debug ("PDA disconnected! udi='%s', device='%s'", priv->udi, priv->ifname);

  /* FIXME: Hack.  We should have a hashtable mapping UDIs to IP-addresses and
   *        use that instead of this ugly approach... */
  for (cur = priv->devices; cur != NULL; cur = cur->next)
    {
      OdccmDevice *dev = cur->data;
      guint32 addr;

      g_object_get (dev, "ip-address", &addr, NULL);

      if (addr == inet_addr (DEVICE_IP_ADDRESS))
        {
          gchar *obj_path;

          g_object_get (dev, "object-path", &obj_path, NULL);
          g_signal_emit (self, signals[DEVICE_DISCONNECTED], 0, obj_path);
          g_free (obj_path);

          priv->devices = g_slist_delete_link (priv->devices, cur);
          g_object_unref (dev);

          break;
        }
    }

  g_free (priv->udi);
  priv->udi = NULL;
  g_free (priv->ifname);
  priv->ifname = NULL;
}

static gboolean
udev_read_cb (GIOChannel *source,
              GIOCondition condition,
              gpointer user_data)
{
  OdccmDeviceManagerPrivate *priv = ODCCM_DEVICE_MANAGER_GET_PRIVATE (user_data);
#define UEVENT_BUFFER_SIZE 2048
  gchar buf[UEVENT_BUFFER_SIZE*2];
  GIOError ioError;
  guint buflen = 0;
  size_t bufpos;
  gboolean is_net = FALSE, is_ppp = FALSE, added = FALSE;
  gchar iface[10];
  gchar udi[UEVENT_BUFFER_SIZE];
  
  ioError = g_io_channel_read(priv->udev_chann, &buf, sizeof(buf), &buflen);

  if (ioError != G_IO_ERROR_NONE) {
    g_warning("%s: error reading event",G_STRFUNC);
  }  

  bufpos = strlen(buf) + 1;
  while (bufpos < (size_t)buflen) {
    int keylen;
    char *key;
    char *tmpptr;
    
    key = &buf[bufpos];
    keylen = strlen(key);
    if (keylen == 0) break;
    
    if (strncmp("SUBSYSTEM=net",key,13)==0) is_net = TRUE;
    if (strncmp("INTERFACE=ppp",key,13)==0){
      is_ppp = TRUE;
      tmpptr = strchr(key,'=');
      strncpy(iface,tmpptr+1,10);
    }
    if (strncmp("ACTION=remove",key,13)==0) added = FALSE;
    if (strncmp("ACTION=add",key,13)==0) added = TRUE;
    if (strncmp("DEVPATH",key,7)==0) {
      tmpptr = strchr(key,'=');
      strncpy(udi,tmpptr+1,UEVENT_BUFFER_SIZE);
    }
    bufpos += keylen + 1;
  }

  if (is_net == TRUE && is_ppp == TRUE) {
    if (added == TRUE) {
      udev_device_added(user_data,iface,udi);
    } else {
      udev_device_removed(user_data,iface,udi);
    }
  }

  return TRUE;
}

static void
init_udev (OdccmDeviceManager *self)
{
  OdccmDeviceManagerPrivate *priv = ODCCM_DEVICE_MANAGER_GET_PRIVATE (self);
  gboolean success = FALSE;
  const gchar *func_name = "";
  struct sockaddr_nl snl;
  int retval;
  int udev_sock;

  udev_sock = -1;

  memset(&snl,0, sizeof(struct sockaddr_nl));
  snl.nl_family = AF_NETLINK;
  snl.nl_pid = getpid();
  snl.nl_groups = 1;

  udev_sock = socket(PF_NETLINK,SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);

  if (udev_sock == -1) {
    func_name = "socket";
    goto ERROR;
  }

  retval = bind(udev_sock, (struct sockaddr *) &snl,
                sizeof(struct sockaddr_nl));

  if (retval < 0) {
    func_name = "bind";
    close(udev_sock);
    goto ERROR;
  }
  priv->udev_chann = g_io_channel_unix_new(udev_sock);
  g_io_add_watch (priv->udev_chann, G_IO_IN, udev_read_cb, self);

  success = TRUE;
  goto OUT;

ERROR:
  g_warning ("%s: %s() failed", G_STRFUNC, func_name);

OUT:
  if (!success)
    {
      g_warning ("%s: failed", G_STRFUNC);
    }
}
