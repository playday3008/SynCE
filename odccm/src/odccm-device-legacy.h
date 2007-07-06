/*
 * Copyright (C) 2007 Mark Ellis <mark@mpellis.org.uk>
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

#ifndef ODCCM_DEVICE_LEGACY_H
#define ODCCM_DEVICE_LEGACY_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

typedef struct _OdccmDeviceLegacy OdccmDeviceLegacy;
typedef struct _OdccmDeviceLegacyClass OdccmDeviceLegacyClass;

struct _OdccmDeviceLegacy
{
  OdccmDevice parent;
};

struct _OdccmDeviceLegacyClass
{
  OdccmDeviceClass parent_class;
};

GType odccm_device_legacy_get_type (void);

#define ODCCM_TYPE_DEVICE_LEGACY \
    (odccm_device_legacy_get_type())
#define ODCCM_DEVICE_LEGACY(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), ODCCM_TYPE_DEVICE_LEGACY, OdccmDeviceLegacy))
#define ODCCM_DEVICE_LEGACY_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), ODCCM_TYPE_DEVICE_LEGACY, OdccmDeviceLegacyClass))
#define ODCCM_IS_DEVICE_LEGACY(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), ODCCM_TYPE_DEVICE_LEGACY))
#define ODCCM_IS_DEVICE_LEGACY_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), ODCCM_TYPE_DEVICE_LEGACY))
#define ODCCM_DEVICE_LEGACY_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), ODCCM_TYPE_DEVICE_LEGACY, OdccmDeviceLegacyClass))

G_END_DECLS

#endif /* ODCCM_DEVICE_LEGACY_H */
