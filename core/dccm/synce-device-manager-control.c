#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <unistd.h>
#include <glib-object.h>
#include <gio/gio.h>
#include "synce-device-manager-control.h"
#include "synce-device-manager-control-dbus.h"
#include "synce-device-manager-control-signals-marshal.h"


static void     synce_device_manager_control_initable_iface_init (GInitableIface  *iface);
static gboolean synce_device_manager_control_initable_init       (GInitable       *initable,
								  GCancellable    *cancellable,
								  GError         **error);
static void     synce_device_manager_control_async_initable_iface_init (GAsyncInitableIface  *iface);
static void     synce_device_manager_control_init_async (GAsyncInitable *initable, int io_priority, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
static gboolean synce_device_manager_control_init_finish (GAsyncInitable      *initable,
							  GAsyncResult        *res,
							  GError             **error);

enum {
  NOT_INITIALISED,
  INITIALISING,
  INITIALISED
};

/* private stuff */
typedef struct _SynceDeviceManagerControlPrivate SynceDeviceManagerControlPrivate;

struct _SynceDeviceManagerControlPrivate
{
  guint init_state;
  gboolean init_success;
  GList *init_results;
  GError *init_error;
  gboolean inited;
  gboolean dispose_has_run;
  SynceDbusDeviceManagerControl *interface;
};

G_DEFINE_TYPE_WITH_CODE (SynceDeviceManagerControl, synce_device_manager_control, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (SynceDeviceManagerControl)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, synce_device_manager_control_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, synce_device_manager_control_async_initable_iface_init))

#define SYNCE_DEVICE_MANAGER_CONTROL_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), SYNCE_TYPE_DEVICE_MANAGER_CONTROL, SynceDeviceManagerControlPrivate))


static gboolean
synce_device_manager_control_device_connected(SynceDbusDeviceManagerControl *interface,
					      GDBusMethodInvocation *invocation,
					      const gchar *device_path, const gchar *device_ip,
					      const gchar *local_ip, gboolean rndis,
					      gpointer userdata)
{
  SynceDeviceManagerControl *self = SYNCE_DEVICE_MANAGER_CONTROL(userdata);
  SynceDeviceManagerControlPrivate *priv = SYNCE_DEVICE_MANAGER_CONTROL_GET_PRIVATE(self);
  g_return_val_if_fail(priv->inited && !(priv->dispose_has_run), FALSE);

  g_signal_emit(self,
		SYNCE_DEVICE_MANAGER_CONTROL_GET_CLASS(SYNCE_DEVICE_MANAGER_CONTROL(self))->signals[SYNCE_DEVICE_MANAGER_CONTROL_DEVICE_CONNECTED],
		0,
		device_path, device_ip, local_ip, rndis);

  synce_dbus_device_manager_control_complete_device_connected(interface, invocation);

  return TRUE;
}

static gboolean
synce_device_manager_control_device_disconnected(SynceDbusDeviceManagerControl *interface,
                                                 GDBusMethodInvocation *invocation,
                                                 const gchar *device_path,
                                                 gpointer userdata)
{
  SynceDeviceManagerControl *self = SYNCE_DEVICE_MANAGER_CONTROL(userdata);
  SynceDeviceManagerControlPrivate *priv = SYNCE_DEVICE_MANAGER_CONTROL_GET_PRIVATE(self);
  g_return_val_if_fail(priv->inited && !(priv->dispose_has_run), FALSE);

  g_signal_emit(self,
		SYNCE_DEVICE_MANAGER_CONTROL_GET_CLASS(SYNCE_DEVICE_MANAGER_CONTROL(self))->signals[SYNCE_DEVICE_MANAGER_CONTROL_DEVICE_DISCONNECTED],
		0,
		device_path);

  synce_dbus_device_manager_control_complete_device_disconnected(interface, invocation);

  return TRUE;
}


static void
synce_device_manager_control_initable_iface_init (GInitableIface *iface)
{
  iface->init = synce_device_manager_control_initable_init;
}

static void
synce_device_manager_control_async_initable_iface_init (GAsyncInitableIface *iface)
{
  iface->init_async = synce_device_manager_control_init_async;
  iface->init_finish = synce_device_manager_control_init_finish;
}

static void
synce_device_manager_control_init (SynceDeviceManagerControl *self)
{
  SynceDeviceManagerControlPrivate *priv = SYNCE_DEVICE_MANAGER_CONTROL_GET_PRIVATE(self);

  priv->inited = FALSE;
  priv->init_state = NOT_INITIALISED;
  priv->init_success = FALSE;
  priv->init_results = NULL;
  priv->init_error = NULL;
  priv->dispose_has_run = FALSE;
  priv->interface = NULL;

  return;
}

static void
synce_device_manager_control_ready_cb (GObject *source_object,
				       GAsyncResult *res,
				       gpointer user_data)
{
  g_return_if_fail (SYNCE_IS_DEVICE_MANAGER_CONTROL(user_data));
  SynceDeviceManagerControl *self = SYNCE_DEVICE_MANAGER_CONTROL(user_data);
  SynceDeviceManagerControlPrivate *priv = SYNCE_DEVICE_MANAGER_CONTROL_GET_PRIVATE (self);

  GError *error = NULL;
  GDBusConnection *system_bus = NULL;

  system_bus = g_bus_get_finish (res, &error);

  if (system_bus == NULL) {
    g_critical("%s: Failed to connect to system bus: %s", G_STRFUNC, error->message);
    goto out;
  }

  if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(priv->interface),
					system_bus,
					DEVICE_MANAGER_CONTROL_OBJECT_PATH,
					&error)) {
    g_critical("%s: Failed to export interface on system bus: %s", G_STRFUNC, error->message);
    g_object_unref(system_bus);
    goto out;
  }
  g_object_unref(system_bus);

  priv->init_success = TRUE;
 out:

  priv->inited = TRUE;
  if (!priv->init_success)
    priv->init_error = error;

  GList *l;

  priv->init_state = INITIALISED;

  for (l = priv->init_results; l != NULL; l = l->next)
    {
      GTask *task = l->data;

      if (priv->init_success)
	g_task_return_boolean (task, TRUE);
      else
	g_task_return_error (task, g_error_copy(priv->init_error));
      g_object_unref (task);
    }

  g_list_free (priv->init_results);
  priv->init_results = NULL;

  return;
}

static void
synce_device_manager_control_init_async (GAsyncInitable *initable, int io_priority, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  g_return_if_fail (SYNCE_IS_DEVICE_MANAGER_CONTROL(initable));
  SynceDeviceManagerControl *self = SYNCE_DEVICE_MANAGER_CONTROL(initable);
  SynceDeviceManagerControlPrivate *priv = SYNCE_DEVICE_MANAGER_CONTROL_GET_PRIVATE (self);

  GTask *task = NULL;
  task = g_task_new (initable, cancellable, callback, user_data);

  switch (priv->init_state)
    {
    case NOT_INITIALISED:
      priv->init_state = INITIALISING;

      priv->interface = synce_dbus_device_manager_control_skeleton_new();
      g_signal_connect(priv->interface,
		       "handle-device-connected",
		       G_CALLBACK (synce_device_manager_control_device_connected),
		       self);
      g_signal_connect(priv->interface,
		       "handle-device-disconnected",
		       G_CALLBACK (synce_device_manager_control_device_disconnected),
		       self);

      /* should we ref self ? */
      g_bus_get (G_BUS_TYPE_SYSTEM,
		 cancellable,
		 synce_device_manager_control_ready_cb,
		 self);

      priv->init_results = g_list_append(priv->init_results, task);
      priv->init_state = INITIALISING;
      break;
    case INITIALISING:
      priv->init_results = g_list_append(priv->init_results, task);
      break;
    case INITIALISED:
      if (!priv->init_success)
	g_task_return_error (task, g_error_copy(priv->init_error));
      else
	g_task_return_boolean (task, TRUE);
      g_object_unref (task);
      break;
    }

  return;
}


static gboolean
synce_device_manager_control_init_finish (GAsyncInitable      *initable,
					  GAsyncResult        *res,
					  GError             **error)
{
  g_return_val_if_fail (g_task_is_valid (res, initable), FALSE);

  return g_task_propagate_boolean (G_TASK (res), error);
}

static gboolean
synce_device_manager_control_initable_init (GInitable *initable, GCancellable *cancellable, GError **error)
{
  g_return_val_if_fail (SYNCE_IS_DEVICE_MANAGER_CONTROL(initable), FALSE);
  SynceDeviceManagerControl *self = SYNCE_DEVICE_MANAGER_CONTROL(initable);
  SynceDeviceManagerControlPrivate *priv = SYNCE_DEVICE_MANAGER_CONTROL_GET_PRIVATE (self);

  switch (priv->init_state)
    {
    case NOT_INITIALISED:
      priv->init_state = INITIALISING;

      priv->interface = synce_dbus_device_manager_control_skeleton_new();
      g_signal_connect(priv->interface,
		       "handle-device-connected",
		       G_CALLBACK (synce_device_manager_control_device_connected),
		       self);
      g_signal_connect(priv->interface,
		       "handle-device-disconnected",
		       G_CALLBACK (synce_device_manager_control_device_disconnected),
		       self);

      GDBusConnection *system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &(priv->init_error));
      if (system_bus == NULL) {
	g_critical("%s: Failed to connect to system bus: %s", G_STRFUNC, priv->init_error->message);
	goto out;
      }
      if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(priv->interface),
					    system_bus,
					    DEVICE_MANAGER_CONTROL_OBJECT_PATH,
					    &(priv->init_error))) {
	g_critical("%s: Failed to export interface on system bus: %s", G_STRFUNC, priv->init_error->message);
	g_object_unref(system_bus);
	goto out;
      }
      g_object_unref(system_bus);

      priv->inited = TRUE;
      priv->init_success = TRUE;
      break;

    case INITIALISING:
      /* shouldn't ever have this unless initialised in 2 different threads ? */
      break;
    case INITIALISED:
      /* don't need to do anything here */
      break;
    }

 out:
  if (priv->init_success == FALSE)
    g_propagate_error (error, g_error_copy(priv->init_error));
  priv->init_state = INITIALISED;
  return priv->init_success;
}

static void
synce_device_manager_control_dispose (GObject *obj)
{
  SynceDeviceManagerControl *self = SYNCE_DEVICE_MANAGER_CONTROL (obj);
  SynceDeviceManagerControlPrivate *priv = SYNCE_DEVICE_MANAGER_CONTROL_GET_PRIVATE (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  if (priv->interface) {
    g_dbus_interface_skeleton_unexport(G_DBUS_INTERFACE_SKELETON(priv->interface));
    g_object_unref(priv->interface);
  }

  g_list_free_full(priv->init_results, g_object_unref);
  g_clear_error(&(priv->init_error));

  if (G_OBJECT_CLASS (synce_device_manager_control_parent_class)->dispose)
    G_OBJECT_CLASS (synce_device_manager_control_parent_class)->dispose (obj);
}

static void
synce_device_manager_control_finalize (GObject *obj)
{
  G_OBJECT_CLASS (synce_device_manager_control_parent_class)->finalize (obj);
}

static void
synce_device_manager_control_class_init (SynceDeviceManagerControlClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->dispose = synce_device_manager_control_dispose;
  obj_class->finalize = synce_device_manager_control_finalize;

  klass->signals[SYNCE_DEVICE_MANAGER_CONTROL_DEVICE_CONNECTED] =
    g_signal_new ("device-connected",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  synce_device_manager_control_marshal_VOID__STRING_STRING_STRING_BOOLEAN,
                  G_TYPE_NONE, 4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);

  klass->signals[SYNCE_DEVICE_MANAGER_CONTROL_DEVICE_DISCONNECTED] =
    g_signal_new ("device-disconnected",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

}

SynceDeviceManagerControl *
synce_device_manager_control_new (GCancellable *cancellable, GError **error)
{
  return SYNCE_DEVICE_MANAGER_CONTROL(g_initable_new(SYNCE_TYPE_DEVICE_MANAGER_CONTROL, cancellable, error, NULL));
}

void
synce_device_manager_control_new_async (GCancellable *cancellable,
					GAsyncReadyCallback callback,
					gpointer user_data)
{
  g_async_initable_new_async(SYNCE_TYPE_DEVICE_MANAGER_CONTROL, G_PRIORITY_DEFAULT, cancellable, callback, user_data, NULL);
}

SynceDeviceManagerControl *
synce_device_manager_control_new_finish (GAsyncResult *res,
					 GError **error)
{
  GObject *object = NULL;
  GObject *source_object = NULL;

  source_object = g_async_result_get_source_object (res);
  g_assert (source_object != NULL);
  object = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object),
					res,
					error);
  g_object_unref (source_object);
  if (object != NULL)
    return SYNCE_DEVICE_MANAGER_CONTROL(object);
  else
    return NULL;
}

