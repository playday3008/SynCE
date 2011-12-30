#ifndef SYNCE_DEVICE_INTERNAL_H
#define SYNCE_DEVICE_INTERNAL_H

#include <glib-object.h>
#include <gio/gio.h>
#include <dbus/dbus-glib.h>
#ifdef USE_HAL
#include <libhal.h>
#else
#if HAVE_GUDEV
#include <gudev/gudev.h>
#endif
#endif

#include "synce-connection-broker.h"

G_BEGIN_DECLS

typedef enum {
  CTRL_STATE_HANDSHAKE,
  CTRL_STATE_GETTING_INFO,
  CTRL_STATE_GOT_INFO,
  CTRL_STATE_AUTH,
  CTRL_STATE_CONNECTED,
} ControlState;

typedef enum {
  SYNCE_DEVICE_PASSWORD_FLAG_UNSET             = 0,
  SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE           = 1,
  SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE = 2,
  SYNCE_DEVICE_PASSWORD_FLAG_CHECKING          = 3,
  SYNCE_DEVICE_PASSWORD_FLAG_UNLOCKED          = 4,
} SynceDevicePasswordFlags;

typedef struct _SynceDevicePrivate SynceDevicePrivate;
struct _SynceDevicePrivate
{
  gboolean dispose_has_run;

  GSocketConnection *conn;
  gchar *iobuf;

  /* the hal udi or sysfs path */
  gchar *device_path;

  gchar *guid;
  guint os_major;
  guint os_minor;
  gchar *name;
  guint version;
  guint cpu_type;
  guint cur_partner_id;
  guint id;
  gchar *platform_name;
  gchar *model_name;
  gchar *ip_address;
  gchar *iface_address;

  /* state */
  ControlState state;
  gint32 info_buf_size;
  SynceDevicePasswordFlags pw_flags;

  guint32 pw_key;

#ifdef USE_HAL
  LibHalContext *hal_ctx;
  DBusGConnection *hal_bus;
#else
  /* the dbus object path */
  gchar *obj_path;
#if HAVE_GUDEV
  GUdevClient *gudev_client;
#endif
#endif

  DBusGMethodInvocation *pw_ctx;

  GHashTable *requests;
  guint req_id;
};

#define SYNCE_DEVICE_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), SYNCE_TYPE_DEVICE, SynceDevicePrivate))

/* header information */

void synce_device_provide_password (SynceDevice *self, const gchar *password, DBusGMethodInvocation *ctx);
void synce_device_request_connection (SynceDevice *self, DBusGMethodInvocation *ctx);
void synce_device_change_password_flags (SynceDevice *self, SynceDevicePasswordFlags new_flag);
void synce_device_conn_broker_done_cb (SynceConnectionBroker *broker, gpointer user_data);
#ifdef USE_HAL
void synce_device_set_hal_props (SynceDevice *device);
#else
void synce_device_dbus_init(SynceDevice *self);
#endif
void synce_device_conn_event_cb(GObject *istream, GAsyncResult *res, gpointer user_data);

G_END_DECLS

#endif /* SYNCE_DEVICE_INTERNAL_H */
