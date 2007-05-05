/***************************************************************************
 * Copyright (c) 2006 Ole André Vadla Ravnås <oleavr@gmail.com>            *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/

/*#define INSANE_DEBUG*/

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libhal.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/ioctl.h>

#include "eventmanager.h"

#include "eventmanager-signals-marshal.h"
#include "eventmanager-glue.h"

#include "cutils.h"

G_DEFINE_TYPE (VDCCMEventManager, vdccm_event_manager, G_TYPE_OBJECT)
/* signal enum */
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
typedef struct _VDCCMEventManagerPrivate VDCCMEventManagerPrivate;

struct _VDCCMEventManagerPrivate
{
  gboolean dispose_has_run;

  DBusGConnection *bus;
  LibHalContext *hal_ctx;

  gchar *udi;
  gchar *ifname;
  gpointer ce_device_base;
};

#define VDCCM_EVENT_MANAGER_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), VDCCM_TYPE_EVENT_MANAGER, VDCCMEventManagerPrivate))

static gpointer
main_loop_thread_func (gpointer data)
{
  GMainLoop *main_loop = data;

  g_main_loop_run (main_loop);

  return NULL;
}

VDCCMEventManager *
_vdccm_get_event_manager ()
{
  static VDCCMEventManager *mgr = NULL;

  if (mgr == NULL)
    {
      GMainLoop *main_loop;

      g_thread_init (NULL);

      g_type_init ();

      main_loop = g_main_loop_new (NULL, FALSE);

      mgr = g_object_new (VDCCM_TYPE_EVENT_MANAGER, NULL);

      if (g_thread_create (main_loop_thread_func,
                           main_loop, TRUE, NULL) == NULL)
        {
          g_critical ("failed to create GMainLoop thread");
        }
    }

  return mgr;
}

static void
vdccm_event_manager_init (VDCCMEventManager *self)
{
}

static void init_hal (VDCCMEventManager *self);
static void shutdown_hal (VDCCMEventManager *self);

static GObject *
vdccm_event_manager_constructor (GType type, guint n_props,
                                 GObjectConstructParam *props)
{
  GObject *obj;
  VDCCMEventManagerPrivate *priv;
  GError *error = NULL;
  static DBusGProxy *bus_proxy = NULL;
  guint req_name_result;

  obj = G_OBJECT_CLASS (vdccm_event_manager_parent_class)->
    constructor (type, n_props, props);

  priv = VDCCM_EVENT_MANAGER_GET_PRIVATE (obj);

  priv->bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (priv->bus == NULL)
    {
      g_error ("Failed to connect to session bus: %s", error->message);
      exit (EXIT_FAILURE);
    }

  bus_proxy = dbus_g_proxy_new_for_name (priv->bus, "org.freedesktop.DBus",
                                         "/org/freedesktop/DBus",
                                         "org.freedesktop.DBus");
  if (bus_proxy == NULL)
    {
      g_error ("Failed to get bus proxy object");
      exit (EXIT_FAILURE);
    }

  if (!dbus_g_proxy_call (bus_proxy, "RequestName", &error,
                          G_TYPE_STRING, EVENT_MANAGER_BUS_NAME,
                          G_TYPE_UINT, DBUS_NAME_FLAG_DO_NOT_QUEUE,
                          G_TYPE_INVALID,
                          G_TYPE_UINT, &req_name_result, G_TYPE_INVALID))
    {
      g_error ("Failed to get bus name, already running?");
      exit (EXIT_FAILURE);
    }

  dbus_g_connection_register_g_object (priv->bus, EVENT_MANAGER_OBJECT_PATH, obj);

  init_hal (VDCCM_EVENT_MANAGER (obj));

  return obj;
}

static void
vdccm_event_manager_dispose (GObject *obj)
{
  VDCCMEventManager *self = VDCCM_EVENT_MANAGER (obj);
  VDCCMEventManagerPrivate *priv = VDCCM_EVENT_MANAGER_GET_PRIVATE (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  shutdown_hal (self);

  if (G_OBJECT_CLASS (vdccm_event_manager_parent_class)->dispose)
    G_OBJECT_CLASS (vdccm_event_manager_parent_class)->dispose (obj);
}

static void
vdccm_event_manager_finalize (GObject *obj)
{
  VDCCMEventManager *self = VDCCM_EVENT_MANAGER (obj);
  VDCCMEventManagerPrivate *priv = VDCCM_EVENT_MANAGER_GET_PRIVATE (self);

  g_free (priv->udi);
  g_free (priv->ifname);

  G_OBJECT_CLASS (vdccm_event_manager_parent_class)->finalize (obj);
}

static void
vdccm_event_manager_class_init (VDCCMEventManagerClass *event_manager_class)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (event_manager_class);

  g_type_class_add_private (event_manager_class,
                            sizeof (VDCCMEventManagerPrivate));

  obj_class->constructor = vdccm_event_manager_constructor;

  obj_class->dispose = vdccm_event_manager_dispose;
  obj_class->finalize = vdccm_event_manager_finalize;

  signals[DEVICE_ATTACHED] =
    g_signal_new ("device-attached",
                  G_OBJECT_CLASS_TYPE (event_manager_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  signals[DEVICE_DETACHED] =
    g_signal_new ("device-detached",
                  G_OBJECT_CLASS_TYPE (event_manager_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  signals[DEVICE_CONNECTED] =
    g_signal_new ("device-connected",
                  G_OBJECT_CLASS_TYPE (event_manager_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  signals[DEVICE_DISCONNECTED] =
    g_signal_new ("device-disconnected",
                  G_OBJECT_CLASS_TYPE (event_manager_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (event_manager_class),
                                   &dbus_glib_eventmanager_object_info);
}

static gboolean
configure_interface (const gchar *ifname,
                     const gchar *ip_addr,
                     const gchar *netmask,
                     const gchar *bcast_addr)
{
  gboolean result = FALSE;
  const gchar *op = "socket()";
  gint fd = -1;
  struct ifreq ifr;
  struct sockaddr_in *addr;

  if ((fd = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    goto ERROR;

  memset (&ifr, 0, sizeof (ifr));
  strcpy (ifr.ifr_name, ifname);

  addr = (struct sockaddr_in *) &ifr.ifr_addr;
  addr->sin_family = AF_INET;

  _vdccm_acquire_root_privileges ();

  inet_aton (ip_addr, &addr->sin_addr);
  if (ioctl (fd, SIOCSIFADDR, &ifr) < 0)
    {
      op = "ioctl(SIOCSIFADDR)";
      goto ERROR;
    }

  inet_aton (netmask, &addr->sin_addr);
  if (ioctl (fd, SIOCSIFNETMASK, &ifr) < 0)
    {
      op = "ioctl(SIOCSIFNETMASK)";
      goto ERROR;
    }

  inet_aton (bcast_addr, &addr->sin_addr);
  if (ioctl (fd, SIOCSIFBRDADDR, &ifr) < 0)
    {
      op = "ioctl(SIOCSIFBRDADDR)";
      goto ERROR;
    }

  if (ioctl (fd, SIOCGIFFLAGS, &ifr) < 0)
    {
      op = "ioctl(SIOCGIFFLAGS)";
      goto ERROR;
    }

  ifr.ifr_flags |= IFF_UP;

  if (ioctl (fd, SIOCSIFFLAGS, &ifr) < 0)
    {
      op = "ioctl(SIOCSIFFLAGS)";
      goto ERROR;
    }

  result = TRUE;
  goto OUT;

ERROR:
  g_warning ("%s: failed to configure %s. %s failed: %s",
             G_STRFUNC, ifname, op, strerror (errno));

OUT:
  _vdccm_drop_root_privileges ();

  if (fd != -1)
    close (fd);

  return result;
}

static gboolean
interface_is_configured (const gchar *ifname,
                         const gchar *expected_address)
{
  gboolean result = FALSE;
  const gchar *op = "socket()";
  gint fd = -1;
  struct ifreq ifr;
  struct sockaddr_in *addr;

  if ((fd = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    goto ERROR;

  memset (&ifr, 0, sizeof (ifr));
  strcpy (ifr.ifr_name, ifname);
  ifr.ifr_addr.sa_family = AF_INET;

  if (ioctl (fd, SIOCGIFADDR, &ifr) < 0)
    {
      op = "ioctl(SIOCGIFADDR)";
      goto ERROR;
    }

  addr = (struct sockaddr_in *) &ifr.ifr_addr;
  if (addr->sin_addr.s_addr != inet_addr (expected_address))
    goto OUT;

  if (ioctl (fd, SIOCGIFFLAGS, &ifr) < 0)
    {
      op = "ioctl(SIOCGIFFLAGS)";
      goto ERROR;
    }

  if ((ifr.ifr_flags & IFF_UP) == 0)
    goto OUT;

  result = TRUE;
  goto OUT;

ERROR:
  /*g_warning ("failed to get configuration for %s. %s failed: %s",
             ifname, op, strerror (errno));*/

OUT:
  if (fd != -1)
    close (fd);

  return result;
}

static gboolean
do_trigger_connection (VDCCMEventManager *self)
{
  gboolean result = FALSE;
  const gchar *op = "socket()";
  gint fd = -1;
  struct sockaddr_in sa;
  guchar b = 0x7f;

  if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
    goto ERROR;

  sa.sin_family = AF_INET;
  sa.sin_port = htons (5679);
  inet_aton ("169.254.2.1", &sa.sin_addr);

  if (sendto (fd, &b, sizeof (b), 0, (const struct sockaddr *) &sa,
              sizeof (sa)) != 1)
    {
      op = "sendto()";
      goto ERROR;
    }

  result = TRUE;
  goto OUT;

ERROR:
  g_warning ("failed to send trigger packet. %s failed: %s",
             op, strerror (errno));

OUT:
  return result;
}

static gboolean
interface_timed_cb (gpointer data)
{
  VDCCMEventManager *self = data;
  VDCCMEventManagerPrivate *priv = VDCCM_EVENT_MANAGER_GET_PRIVATE (self);

  /* Has the device disappeared for some reason? */
  if (priv->udi == NULL)
    return FALSE;

  /* Has it connected? */
  if (priv->ce_device_base != NULL)
    {
#ifdef INSANE_DEBUG
      g_debug ("%s: device connected, wootage!", G_STRFUNC);
#endif
      return FALSE;
    }

  /* Has the device been re-configured? */
  if (!interface_is_configured (priv->ifname, DEVICE_IP_ADDRESS))
    {
#ifdef INSANE_DEBUG
      g_debug ("%s: device has been re-configured, configuring again",
               G_STRFUNC);
#endif

      /* It has, reconfigure and try again on the next tick. */
      configure_interface (priv->ifname, DEVICE_IP_ADDRESS, DEVICE_NETMASK,
                           DEVICE_BROADCAST);

      return TRUE;
    }
#ifdef INSANE_DEBUG
  else
    {
      g_debug ("%s: device is still configured correctly", G_STRFUNC);
    }
#endif

  /* Send a trigger-packet to make the device connect. */
#ifdef INSANE_DEBUG
  g_debug ("%s: sending trigger-packet", G_STRFUNC);
#endif
  do_trigger_connection (self);

  return TRUE;
}

static void
hal_device_added_cb (LibHalContext *ctx, const gchar *udi)
{
  VDCCMEventManager *self = VDCCM_EVENT_MANAGER (
      libhal_ctx_get_user_data (ctx));
  VDCCMEventManagerPrivate *priv = VDCCM_EVENT_MANAGER_GET_PRIVATE (self);
  DBusError error;
  gchar *ifname;

  dbus_error_init (&error);

  ifname = libhal_device_get_property_string (ctx, udi, "net.interface", &error);
  if (ifname != NULL)
    {
      if (strncmp (ifname, "rndis", 5) == 0)
        {
#ifdef INSANE_DEBUG
          g_debug ("PDA network interface discovered! udi='%s', device='%s'",
                   udi, ifname);
#endif

          if (priv->udi != NULL)
            {
              g_warning ("only one device can be connected at the same time "
                         "for now");
              return;
            }

          priv->udi = g_strdup (udi);
          priv->ifname = g_strdup (ifname);

          configure_interface (ifname, DEVICE_IP_ADDRESS, DEVICE_NETMASK,
                               DEVICE_BROADCAST);

          g_timeout_add (50, interface_timed_cb, self);
        }

      libhal_free_string (ifname);
    }

  dbus_error_free (&error);
}

static void
hal_device_removed_cb (LibHalContext *ctx, const gchar *udi)
{
  VDCCMEventManager *self = VDCCM_EVENT_MANAGER (
      libhal_ctx_get_user_data (ctx));
  VDCCMEventManagerPrivate *priv = VDCCM_EVENT_MANAGER_GET_PRIVATE (self);

  if (priv->udi == NULL)
    return;

  if (strcmp (udi, priv->udi) != 0)
    return;

#ifdef INSANE_DEBUG
  g_debug ("PDA disconnected! udi='%s', device='%s'", priv->udi, priv->ifname);
#endif

  g_free (priv->udi);
  priv->udi = NULL;
  g_free (priv->ifname);
  priv->ifname = NULL;

  _vdccm_ce_device_base_disconnect (priv->ce_device_base);
}

static void
init_hal (VDCCMEventManager *self)
{
  VDCCMEventManagerPrivate *priv = VDCCM_EVENT_MANAGER_GET_PRIVATE (self);
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
shutdown_hal (VDCCMEventManager *self)
{
  VDCCMEventManagerPrivate *priv = VDCCM_EVENT_MANAGER_GET_PRIVATE (self);

  if (priv->hal_ctx)
    {
      DBusError error;

      dbus_error_init (&error);
      if (!libhal_ctx_shutdown (priv->hal_ctx, &error))
        dbus_error_free (&error);

      libhal_ctx_free (priv->hal_ctx);
      priv->hal_ctx = NULL;
    }
}

typedef struct
{
  VDCCMEventManager *mgr;
  guint signal_id;
  gchar *name;
  gpointer ce_device_base;
} ConnectivityEmitContext;

static ConnectivityEmitContext *
connectivity_emit_context_new (VDCCMEventManager *mgr, guint signal_id,
                               gpointer ce_device_base)
{
  ConnectivityEmitContext *ctx = g_new (ConnectivityEmitContext, 1);

  ctx->mgr = mgr;
  ctx->signal_id = signal_id;
  ctx->name = _vdccm_ce_device_base_get_name (ce_device_base);
  ctx->ce_device_base = ce_device_base;

  return ctx;
}

static void
connectivity_emit_context_free (ConnectivityEmitContext *ctx)
{
  g_free (ctx->name);
  g_free (ctx);
}

static gboolean
do_emit_connectivity_signal (gpointer data)
{
  ConnectivityEmitContext *ctx = data;
  VDCCMEventManager *self = VDCCM_EVENT_MANAGER (ctx->mgr);
  VDCCMEventManagerPrivate *priv = VDCCM_EVENT_MANAGER_GET_PRIVATE (self);

  priv->ce_device_base = (ctx->signal_id == DEVICE_CONNECTED)
    ? ctx->ce_device_base : NULL;

  g_signal_emit (ctx->mgr, signals[ctx->signal_id], 0, ctx->name);

  connectivity_emit_context_free (ctx);

  return FALSE;
}

void
_vdccm_event_manager_device_connected (VDCCMEventManager *self,
                                       gpointer ce_device_base)
{
  ConnectivityEmitContext *ctx = connectivity_emit_context_new (self,
                                                                DEVICE_CONNECTED,
                                                                ce_device_base);
  g_idle_add (do_emit_connectivity_signal, ctx);
}

void
_vdccm_event_manager_device_disconnected (VDCCMEventManager *self,
                                          gpointer ce_device_base)
{
  ConnectivityEmitContext *ctx = connectivity_emit_context_new (self,
                                                                DEVICE_DISCONNECTED,
                                                                ce_device_base);
  g_idle_add (do_emit_connectivity_signal, ctx);
}

