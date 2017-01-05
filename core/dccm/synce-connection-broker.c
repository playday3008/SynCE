#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>
#include <gio/gunixconnection.h>

#include "synce-connection-broker.h"

#include "synce-device-dbus.h"

#include "utils.h"

G_DEFINE_TYPE (SynceConnectionBroker, synce_connection_broker, G_TYPE_OBJECT)

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
typedef struct _SynceConnectionBrokerPrivate SynceConnectionBrokerPrivate;

struct _SynceConnectionBrokerPrivate
{
  gboolean dispose_has_run;

  guint id;
  GDBusMethodInvocation *ctx;
  GSocketConnection *conn;
  gchar *filename;
  GSocketService *server;
};

#define SYNCE_CONNECTION_BROKER_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), SYNCE_TYPE_CONNECTION_BROKER, SynceConnectionBrokerPrivate))

static void
synce_connection_broker_init (SynceConnectionBroker *self)
{
  SynceConnectionBrokerPrivate *priv = SYNCE_CONNECTION_BROKER_GET_PRIVATE(self);

  priv->ctx = NULL;
  priv->conn = NULL;
  priv->filename = NULL;
  priv->server = NULL;
}

static void
synce_connection_broker_get_property (GObject    *obj,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  SynceConnectionBroker *self = SYNCE_CONNECTION_BROKER (obj);
  SynceConnectionBrokerPrivate *priv = SYNCE_CONNECTION_BROKER_GET_PRIVATE (self);

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
synce_connection_broker_set_property (GObject      *obj,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  SynceConnectionBroker *self = SYNCE_CONNECTION_BROKER (obj);
  SynceConnectionBrokerPrivate *priv = SYNCE_CONNECTION_BROKER_GET_PRIVATE (self);

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

static void
synce_connection_broker_dispose (GObject *obj)
{
  SynceConnectionBroker *self = SYNCE_CONNECTION_BROKER (obj);
  SynceConnectionBrokerPrivate *priv = SYNCE_CONNECTION_BROKER_GET_PRIVATE (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  if (priv->conn != NULL)
    g_object_unref (priv->conn);

  if (priv->server != NULL) {
    g_socket_service_stop(priv->server);
    g_object_unref(priv->server);
  }

  if (G_OBJECT_CLASS (synce_connection_broker_parent_class)->dispose)
    G_OBJECT_CLASS (synce_connection_broker_parent_class)->dispose (obj);
}

static void
synce_connection_broker_finalize (GObject *obj)
{
  SynceConnectionBroker *self = SYNCE_CONNECTION_BROKER (obj);
  SynceConnectionBrokerPrivate *priv = SYNCE_CONNECTION_BROKER_GET_PRIVATE (self);

  if (priv->filename != NULL) {
    unlink (priv->filename);
    g_free (priv->filename);
  }

  G_OBJECT_CLASS (synce_connection_broker_parent_class)->finalize (obj);
}

static void
synce_connection_broker_class_init (SynceConnectionBrokerClass *conn_broker_class)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (conn_broker_class);
  GParamSpec *param_spec;

  g_type_class_add_private (conn_broker_class,
                            sizeof (SynceConnectionBrokerPrivate));

  obj_class->get_property = synce_connection_broker_get_property;
  obj_class->set_property = synce_connection_broker_set_property;

  obj_class->dispose = synce_connection_broker_dispose;
  obj_class->finalize = synce_connection_broker_finalize;

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
server_socket_readable_cb(G_GNUC_UNUSED GSocketService *source,
			  GSocketConnection *client_connection,
			  G_GNUC_UNUSED GObject *source_object,
			  gpointer user_data)
{
  SynceConnectionBroker *self = user_data;
  SynceConnectionBrokerPrivate *priv = SYNCE_CONNECTION_BROKER_GET_PRIVATE (self);
  gint device_fd = -1;
  GError *error = NULL;

  GSocket *device_socket = g_socket_connection_get_socket(priv->conn);
  if (device_socket == NULL) {
    g_warning ("%s: device disconnected ?", G_STRFUNC);
    return TRUE;
  }

  device_fd = g_socket_get_fd(device_socket);

  if (!g_unix_connection_send_fd (G_UNIX_CONNECTION(client_connection), device_fd, NULL, &error)) {
    g_warning("%s: failed to send device file descriptor: %s", G_STRFUNC, error->message);
    g_clear_error(&error);
  }

  g_signal_emit (self, signals[DONE], 0);

  return TRUE;
}

void
_synce_connection_broker_take_connection (SynceConnectionBroker *self,
                                          GSocketConnection *conn)
{
  SynceConnectionBrokerPrivate *priv = SYNCE_CONNECTION_BROKER_GET_PRIVATE (self);
  GRand *rnd;
  guint uid = 0;
  GError *error = NULL;
  GSocketAddress *sock_address = NULL;

  g_assert (priv->conn == NULL);

  priv->conn = conn;

  rnd = g_rand_new ();
  priv->filename = g_strdup_printf ("%s/run/synce-%08x%08x%08x%08x.sock", LOCALSTATEDIR,
      g_rand_int (rnd), g_rand_int (rnd), g_rand_int (rnd), g_rand_int (rnd));
  g_rand_free (rnd);
  sock_address = g_unix_socket_address_new(priv->filename);

  priv->server = g_socket_service_new();
  if (!(priv->server)) {
    g_critical("%s: failed to create socket service", G_STRFUNC);
    goto error_exit;
  }

  if (!(g_socket_listener_add_address(G_SOCKET_LISTENER(priv->server), sock_address, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, NULL, NULL, &error))) {
    g_critical("%s: failed to add address %s to socket service: %s", G_STRFUNC, priv->filename, error->message);
    goto error_exit;
  }
  g_object_unref(sock_address);

  g_signal_connect(priv->server, "incoming", G_CALLBACK(server_socket_readable_cb), self);

  synce_get_dbus_sender_uid(g_dbus_method_invocation_get_sender (priv->ctx), &uid);

  if (chmod(priv->filename, S_IRUSR | S_IWUSR) < 0)
    g_warning("%s: failed to set permissions on socket: %d: %s", G_STRFUNC, errno, g_strerror(errno));
  if (chown(priv->filename, uid, -1) < 0)
    g_warning("%s: failed to set ownership on socket: %d: %s", G_STRFUNC, errno, g_strerror(errno));

  g_socket_service_start(priv->server);

  /* we don't need the object for the first argument, it doesn't go anywhere */
  synce_dbus_device_complete_request_connection(NULL, priv->ctx, priv->filename);
  priv->ctx = NULL;

  return;

 error_exit:
  if (error) g_error_free(error);
  if (sock_address) g_object_unref(sock_address);

  error = g_error_new (G_FILE_ERROR, G_FILE_ERROR_FAILED,
		       "Failed to create socket to pass connection");
  g_dbus_method_invocation_return_gerror(priv->ctx, error);
  g_error_free(error);
  priv->ctx = NULL;
  g_free(priv->filename);
  priv->filename = NULL;

  return;
}

SynceConnectionBroker *
synce_connection_broker_new (guint id, GDBusMethodInvocation *context)
{
  return SYNCE_CONNECTION_BROKER(g_object_new(SYNCE_TYPE_CONNECTION_BROKER, "id", id, "context", context, NULL));
}

