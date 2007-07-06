#ifndef ODCCM_DEVICE_PRIVATE_H
#define ODCCM_DEVICE_PRIVATE_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "odccm-connection-broker.h"

G_BEGIN_DECLS

/* private stuff */
typedef enum {
    CTRL_STATE_HANDSHAKE,
    CTRL_STATE_GETTING_INFO,
    CTRL_STATE_GOT_INFO,
    CTRL_STATE_AUTH,
    CTRL_STATE_CONNECTED,
} ControlState;

typedef struct _OdccmDevicePrivate OdccmDevicePrivate;

struct _OdccmDevicePrivate
{
  gboolean dispose_has_run;

  GConn *conn;
  gchar *obj_path;

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

  /* state */
  ControlState state;
  gint32 info_buf_size;
  OdccmDevicePasswordFlags pw_flags;

  guint32 pw_key;
  DBusGMethodInvocation *pw_ctx;

  GHashTable *requests;
  guint req_id;
};

#define ODCCM_DEVICE_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), ODCCM_TYPE_DEVICE, OdccmDevicePrivate))

void
change_password_flags (OdccmDevice *self,
                       OdccmDevicePasswordFlags add,
                       OdccmDevicePasswordFlags remove);

void
conn_broker_done_cb (OdccmConnectionBroker *broker,
                     gpointer user_data);

void
odccm_device_provide_password (OdccmDevice *self,
                               const gchar *password,
                               DBusGMethodInvocation *ctx);

G_END_DECLS

#endif /* ODCCM_DEVICE_PRIVATE_H */
