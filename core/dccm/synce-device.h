#ifndef SYNCE_DEVICE_H
#define SYNCE_DEVICE_H

#include <glib-object.h>
#include <gio/gio.h>
#include "synce-device-dbus.h"

G_BEGIN_DECLS

#define DEVICE_BASE_OBJECT_PATH "/org/synce/dccm/Device"

typedef enum _SynceDeviceSignals SynceDeviceSignals;
enum _SynceDeviceSignals
{
  SYNCE_DEVICE_SIGNAL_PASSWORD_FLAGS_CHANGED,
  SYNCE_DEVICE_SIGNAL_DISCONNECTED,
  SYNCE_DEVICE_LAST_SIGNAL
};

/* class definition */

typedef struct _SynceDevice SynceDevice;
struct _SynceDevice
{
  GObject parent;
};

typedef struct _SynceDeviceClass SynceDeviceClass;
struct _SynceDeviceClass {
  GObjectClass parent;

  guint signals[SYNCE_DEVICE_LAST_SIGNAL];

  void (*synce_device_conn_event_cb) (GObject *istream, GAsyncResult *res, gpointer user_data);
  gboolean (*synce_device_request_connection) (SynceDbusDevice *interface, GDBusMethodInvocation *invocation, gpointer userdata);
  gboolean (*synce_device_provide_password) (SynceDbusDevice *interface, GDBusMethodInvocation *invocation, const gchar *password, gpointer userdata);
};

GType synce_device_get_type (void);

#define SYNCE_TYPE_DEVICE (synce_device_get_type())
#define SYNCE_DEVICE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SYNCE_TYPE_DEVICE, SynceDevice))
#define SYNCE_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), SYNCE_TYPE_DEVICE, SynceDeviceClass))
#define SYNCE_IS_DEVICE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), SYNCE_TYPE_DEVICE))
#define SYNCE_IS_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SYNCE_TYPE_DEVICE))
#define SYNCE_DEVICE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), SYNCE_TYPE_DEVICE, SynceDeviceClass))

G_END_DECLS

#endif /* SYNCE_DEVICE_H */
