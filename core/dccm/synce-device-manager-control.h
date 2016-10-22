#ifndef SYNCE_DEVICE_MANAGER_CONTROL_H
#define SYNCE_DEVICE_MANAGER_CONTROL_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DEVICE_MANAGER_CONTROL_OBJECT_PATH "/org/synce/dccm/DeviceManagerControl"

/* signals */
enum
{
  SYNCE_DEVICE_MANAGER_CONTROL_DEVICE_CONNECTED,
  SYNCE_DEVICE_MANAGER_CONTROL_DEVICE_DISCONNECTED,
  SYNCE_DEVICE_MANAGER_CONTROL_LAST_SIGNAL
};

/* class definition */

typedef struct _SynceDeviceManagerControl SynceDeviceManagerControl;
struct _SynceDeviceManagerControl
{
  GObject parent;
};

typedef struct _SynceDeviceManagerControlClass SynceDeviceManagerControlClass;
struct _SynceDeviceManagerControlClass {
  GObjectClass parent;

  guint signals[SYNCE_DEVICE_MANAGER_CONTROL_LAST_SIGNAL];
};

GType synce_device_manager_control_get_type (void);

#define SYNCE_TYPE_DEVICE_MANAGER_CONTROL (synce_device_manager_control_get_type())
#define SYNCE_DEVICE_MANAGER_CONTROL(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SYNCE_TYPE_DEVICE_MANAGER_CONTROL, SynceDeviceManagerControl))
#define SYNCE_DEVICE_MANAGER_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), SYNCE_TYPE_DEVICE_MANAGER_CONTROL, SynceDeviceManagerControlClass))
#define SYNCE_IS_DEVICE_MANAGER_CONTROL(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), SYNCE_TYPE_DEVICE_MANAGER_CONTROL))
#define SYNCE_IS_DEVICE_MANAGER_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SYNCE_TYPE_DEVICE_MANAGER_CONTROL))
#define SYNCE_DEVICE_MANAGER_CONTROL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), SYNCE_TYPE_DEVICE_MANAGER_CONTROL, SynceDeviceManagerControlClass))

SynceDeviceManagerControl *
synce_device_manager_control_new (GCancellable *cancellable, GError **error);

void
synce_device_manager_control_new_async (GCancellable *cancellable,
					GAsyncReadyCallback callback,
					gpointer user_data);

SynceDeviceManagerControl *
synce_device_manager_control_new_finish (GAsyncResult *res,
					 GError **error);

G_END_DECLS

#endif /* SYNCE_DEVICE_MANAGER_CONTROL_H */
