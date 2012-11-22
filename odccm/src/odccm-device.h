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

#ifndef ODCCM_DEVICE_H
#define ODCCM_DEVICE_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "odccm-constants.h"

G_BEGIN_DECLS

#define DEVICE_BASE_OBJECT_PATH "/org/synce/odccm/Device"

typedef struct _OdccmDevice OdccmDevice;
typedef struct _OdccmDeviceClass OdccmDeviceClass;

struct _OdccmDevice
{
  GObject parent;
};

struct _OdccmDeviceClass
{
  GObjectClass parent_class;

  void (*conn_event_cb) (GConn *conn, GConnEvent *event, gpointer user_data);
  void (*odccm_device_request_connection) (OdccmDevice *self, DBusGMethodInvocation *ctx);
  void (*odccm_device_provide_password) (OdccmDevice *self, const gchar *password, DBusGMethodInvocation *ctx);
};

GType odccm_device_get_type (void);

#define ODCCM_TYPE_DEVICE \
    (odccm_device_get_type())
#define ODCCM_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), ODCCM_TYPE_DEVICE, OdccmDevice))
#define ODCCM_DEVICE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), ODCCM_TYPE_DEVICE, OdccmDeviceClass))
#define ODCCM_IS_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), ODCCM_TYPE_DEVICE))
#define ODCCM_IS_DEVICE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), ODCCM_TYPE_DEVICE))
#define ODCCM_DEVICE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), ODCCM_TYPE_DEVICE, OdccmDeviceClass))

gboolean odccm_device_get_ip_address (OdccmDevice *self, gchar **ip_address, GError **error);
gboolean odccm_device_get_iface_address (OdccmDevice *self, gchar **iface_address, GError **error);
gboolean odccm_device_get_guid (OdccmDevice *self, gchar **guid, GError **error);
gboolean odccm_device_get_os_version (OdccmDevice *self, guint *os_major, guint *os_minor, GError **error);
gboolean odccm_device_get_name (OdccmDevice *self, gchar **name, GError **error);
gboolean odccm_device_get_version (OdccmDevice *self, guint *version, GError **error);
gboolean odccm_device_get_cpu_type (OdccmDevice *self, guint *cpu_type, GError **error);
gboolean odccm_device_get_current_partner_id (OdccmDevice *self, guint *cur_partner_id, GError **error);
gboolean odccm_device_get_id (OdccmDevice *self, guint *id, GError **error);
gboolean odccm_device_get_platform_name (OdccmDevice *self, gchar **platform_name, GError **error);
gboolean odccm_device_get_model_name (OdccmDevice *self, gchar **model_name, GError **error);
gboolean odccm_device_get_password_flags (OdccmDevice *self, guint *pw_flags, GError **error);
void odccm_device_provide_password (OdccmDevice *self, const gchar *password, DBusGMethodInvocation *ctx);
void odccm_device_request_connection (OdccmDevice *self, DBusGMethodInvocation *ctx);

gboolean _odccm_device_is_identified (OdccmDevice *self);
void _odccm_device_client_connected (OdccmDevice *self, GConn *conn);

G_END_DECLS

#endif /* ODCCM_DEVICE_H */
