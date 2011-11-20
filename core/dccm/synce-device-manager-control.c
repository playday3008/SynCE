#include <sys/types.h>
#include <unistd.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "synce-device-manager-control.h"
#include "synce-device-manager-control-glue.h"
#include "synce-device-manager-control-signals-marshal.h"

G_DEFINE_TYPE (SynceDeviceManagerControl, synce_device_manager_control, G_TYPE_OBJECT)

/* private stuff */
typedef struct _SynceDeviceManagerControlPrivate SynceDeviceManagerControlPrivate;

struct _SynceDeviceManagerControlPrivate
{
  gboolean dispose_has_run;
};

#define SYNCE_DEVICE_MANAGER_CONTROL_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), SYNCE_TYPE_DEVICE_MANAGER_CONTROL, SynceDeviceManagerControlPrivate))

static void
synce_device_manager_control_init (SynceDeviceManagerControl *self)
{
  SynceDeviceManagerControlPrivate *priv = SYNCE_DEVICE_MANAGER_CONTROL_GET_PRIVATE(self);

  GError *error = NULL;
  DBusGConnection *system_bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
  if (system_bus == NULL) {
    g_critical("%s: Failed to connect to system bus: %s", G_STRFUNC, error->message);
    g_error_free(error);
    return;
  }

  dbus_g_connection_register_g_object (system_bus,
                                       DEVICE_MANAGER_CONTROL_OBJECT_PATH,
				       G_OBJECT(self));
  dbus_g_connection_unref(system_bus);

  return;
}

static void
synce_device_manager_control_dispose (GObject *obj)
{
  SynceDeviceManagerControl *self = SYNCE_DEVICE_MANAGER_CONTROL (obj);
  SynceDeviceManagerControlPrivate *priv = SYNCE_DEVICE_MANAGER_CONTROL_GET_PRIVATE (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  if (G_OBJECT_CLASS (synce_device_manager_control_parent_class)->dispose)
    G_OBJECT_CLASS (synce_device_manager_control_parent_class)->dispose (obj);
}

static void
synce_device_manager_control_finalize (GObject *obj)
{
  SynceDeviceManagerControl *self = SYNCE_DEVICE_MANAGER_CONTROL (obj);
  SynceDeviceManagerControlPrivate *priv = SYNCE_DEVICE_MANAGER_CONTROL_GET_PRIVATE (self);

  G_OBJECT_CLASS (synce_device_manager_control_parent_class)->finalize (obj);
}

static void
synce_device_manager_control_class_init (SynceDeviceManagerControlClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (SynceDeviceManagerControlPrivate));

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

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                   &dbus_glib_synce_device_manager_control_object_info);
}

void
synce_device_manager_control_device_connected(SynceDeviceManagerControl *self, gchar *device_path, gchar *device_ip, gchar *local_ip, gboolean rndis, GError **error)
{
  SynceDeviceManagerControlPrivate *priv = SYNCE_DEVICE_MANAGER_CONTROL_GET_PRIVATE(self);

  g_signal_emit(self,
		SYNCE_DEVICE_MANAGER_CONTROL_GET_CLASS(SYNCE_DEVICE_MANAGER_CONTROL(self))->signals[SYNCE_DEVICE_MANAGER_CONTROL_DEVICE_CONNECTED],
		0,
		device_path, device_ip, local_ip, rndis);

  return;
}

