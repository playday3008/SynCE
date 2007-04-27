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

#ifndef ODCCM_DEVICE_MANAGER_H
#define ODCCM_DEVICE_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DEVICE_MANAGER_OBJECT_PATH "/org/synce/odccm/DeviceManager"

typedef struct _OdccmDeviceManager OdccmDeviceManager;
typedef struct _OdccmDeviceManagerClass OdccmDeviceManagerClass;

struct _OdccmDeviceManager
{
  GObject parent;
};

struct _OdccmDeviceManagerClass
{
  GObjectClass parent_class;
};

GType odccm_device_manager_get_type (void);

#define ODCCM_TYPE_DEVICE_MANAGER \
    (odccm_device_manager_get_type())
#define ODCCM_DEVICE_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), ODCCM_TYPE_DEVICE_MANAGER, OdccmDeviceManager))
#define ODCCM_DEVICE_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), ODCCM_TYPE_DEVICE_MANAGER, OdccmDeviceManagerClass))
#define ODCCM_IS_DEVICE_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), ODCCM_TYPE_DEVICE_MANAGER))
#define ODCCM_IS_DEVICE_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), ODCCM_TYPE_DEVICE_MANAGER))
#define ODCCM_DEVICE_MANAGER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), ODCCM_TYPE_DEVICE_MANAGER, OdccmDeviceManagerClass))

gboolean odccm_device_manager_get_connected_devices (OdccmDeviceManager *self, GPtrArray **ret, GError **error);

G_END_DECLS

#endif /* ODCCM_DEVICE_MANAGER_H */
