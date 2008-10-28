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

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <gnet.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "odccm-connection-broker.h"

#include "util.h"

G_DEFINE_TYPE (OdccmConnectionBroker, odccm_connection_broker, G_TYPE_OBJECT)

/* properties */
enum
{
  PROP_ID = 1,
  PROP_CONTEXT,

  LAST_PROPERTY
};

/* signals */
enum
{
  DONE,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

/* private stuff */
typedef struct _OdccmConnectionBrokerPrivate OdccmConnectionBrokerPrivate;

struct _OdccmConnectionBrokerPrivate
{
  gboolean dispose_has_run;

  guint id;
  DBusGMethodInvocation *ctx;
  GConn *conn;
  gchar *filename;
  GUnixSocket *server;
};

#define ODCCM_CONNECTION_BROKER_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), ODCCM_TYPE_CONNECTION_BROKER, OdccmConnectionBrokerPrivate))

static void
odccm_connection_broker_init (OdccmConnectionBroker *self)
{
}

static void
odccm_connection_broker_get_property (GObject    *obj,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  OdccmConnectionBroker *self = ODCCM_CONNECTION_BROKER (obj);
  OdccmConnectionBrokerPrivate *priv = ODCCM_CONNECTION_BROKER_GET_PRIVATE (self);

  switch (property_id) {
    case PROP_ID:
      g_value_set_uint (value, priv->id);
      break;
    case PROP_CONTEXT:
      g_value_set_pointer (value, priv->ctx);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
  }
}

static void
odccm_connection_broker_set_property (GObject      *obj,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  OdccmConnectionBroker *self = ODCCM_CONNECTION_BROKER (obj);
  OdccmConnectionBrokerPrivate *priv = ODCCM_CONNECTION_BROKER_GET_PRIVATE (self);

  switch (property_id) {
    case PROP_ID:
      priv->id = g_value_get_uint (value);
      break;
    case PROP_CONTEXT:
      priv->ctx = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
  }
}

static GObject *
odccm_connection_broker_constructor (GType type, guint n_props,
                                     GObjectConstructParam *props)
{
  GObject *obj;
  OdccmConnectionBrokerPrivate *priv;

  obj = G_OBJECT_CLASS (odccm_connection_broker_parent_class)->
    constructor (type, n_props, props);

  priv = ODCCM_CONNECTION_BROKER_GET_PRIVATE (obj);

  g_assert (priv);

  return obj;
}

static void
odccm_connection_broker_dispose (GObject *obj)
{
  OdccmConnectionBroker *self = ODCCM_CONNECTION_BROKER (obj);
  OdccmConnectionBrokerPrivate *priv = ODCCM_CONNECTION_BROKER_GET_PRIVATE (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  if (priv->conn != NULL)
    gnet_conn_unref (priv->conn);

  if (priv->server != NULL)
    gnet_unix_socket_unref (priv->server);

  if (G_OBJECT_CLASS (odccm_connection_broker_parent_class)->dispose)
    G_OBJECT_CLASS (odccm_connection_broker_parent_class)->dispose (obj);
}

static void
odccm_connection_broker_finalize (GObject *obj)
{
  OdccmConnectionBroker *self = ODCCM_CONNECTION_BROKER (obj);
  OdccmConnectionBrokerPrivate *priv = ODCCM_CONNECTION_BROKER_GET_PRIVATE (self);

  g_free (priv->filename);

  G_OBJECT_CLASS (odccm_connection_broker_parent_class)->finalize (obj);
}

static void
odccm_connection_broker_class_init (OdccmConnectionBrokerClass *conn_broker_class)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (conn_broker_class);
  GParamSpec *param_spec;

  g_type_class_add_private (conn_broker_class,
                            sizeof (OdccmConnectionBrokerPrivate));

  obj_class->constructor = odccm_connection_broker_constructor;

  obj_class->get_property = odccm_connection_broker_get_property;
  obj_class->set_property = odccm_connection_broker_set_property;

  obj_class->dispose = odccm_connection_broker_dispose;
  obj_class->finalize = odccm_connection_broker_finalize;

  param_spec = g_param_spec_uint ("id", "Unique id",
                                  "Unique id.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_CONSTRUCT_ONLY |
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_ID, param_spec);

  param_spec = g_param_spec_pointer ("context", "D-Bus context",
                                     "D-Bus method invocation context.",
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_NICK |
                                     G_PARAM_STATIC_BLURB);
  g_object_class_install_property (obj_class, PROP_CONTEXT, param_spec);

  signals[DONE] =
    g_signal_new ("done",
                  G_OBJECT_CLASS_TYPE (conn_broker_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static gboolean
server_socket_readable_cb (GIOChannel *source,
                           GIOCondition condition,
                           gpointer user_data)
{
  OdccmConnectionBroker *self = user_data;
  OdccmConnectionBrokerPrivate *priv = ODCCM_CONNECTION_BROKER_GET_PRIVATE (self);
  GUnixSocket *sock;
  gint dev_fd, fd, ret;
  struct msghdr msg = { 0, };
  struct cmsghdr *cmsg;
  gchar cmsg_buf[CMSG_SPACE (sizeof (dev_fd))];
  struct iovec iov;
  guchar dummy_byte = 0x7f;

  sock = gnet_unix_socket_server_accept_nonblock (priv->server);
  if (sock == NULL)
    {
      g_warning ("%s: client disconnected?", G_STRFUNC);
      return TRUE;
    }

  dev_fd = g_io_channel_unix_get_fd (priv->conn->iochannel);
  fd = g_io_channel_unix_get_fd (gnet_unix_socket_get_io_channel (sock));

  msg.msg_control = cmsg_buf;
  msg.msg_controllen = sizeof (cmsg_buf);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  cmsg = CMSG_FIRSTHDR (&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN (sizeof (dev_fd));
  *((gint *) CMSG_DATA (cmsg)) = dev_fd;

  iov.iov_base = &dummy_byte;
  iov.iov_len = sizeof (dummy_byte);

  ret = sendmsg (fd, &msg, MSG_NOSIGNAL);
  if (ret != 1)
    {
      g_warning ("%s: sendmsg returned %d", G_STRFUNC, ret);
    }

  gnet_unix_socket_unref(sock);

  g_signal_emit (self, signals[DONE], 0);

  return FALSE;
}

void
_odccm_connection_broker_take_connection (OdccmConnectionBroker *self,
                                          GConn *conn)
{
  OdccmConnectionBrokerPrivate *priv = ODCCM_CONNECTION_BROKER_GET_PRIVATE (self);
  GRand *rnd;
  GIOChannel *chan;
  guint uid;

  g_assert (priv->conn == NULL);

  priv->conn = conn;

  rnd = g_rand_new ();
  priv->filename = g_strdup_printf ("/tmp/odccm-%08x%08x%08x%08x.sock",
      g_rand_int (rnd), g_rand_int (rnd), g_rand_int (rnd), g_rand_int (rnd));
  g_rand_free (rnd);

  priv->server = gnet_unix_socket_server_new (priv->filename);

  _odccm_get_dbus_sender_uid (dbus_g_method_get_sender (priv->ctx), &uid);

  chmod (priv->filename, S_IRUSR | S_IWUSR);
  chown (priv->filename, uid, -1);

  chan = gnet_unix_socket_get_io_channel (priv->server);
  g_io_add_watch (chan, G_IO_IN, server_socket_readable_cb, self);

  dbus_g_method_return (priv->ctx, priv->filename);
  priv->ctx = NULL;
}

