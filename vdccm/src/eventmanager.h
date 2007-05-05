/***************************************************************************
 * Copyright (c) 2006 Ole André Vadla Ravnås <oleavr@gmail.com>            *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/

#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define EVENT_MANAGER_BUS_NAME    "org.synce.vdccm.EventManager"
#define EVENT_MANAGER_OBJECT_PATH "/org/synce/vdccm/EventManager"

/* FIXME: make these configurable */
#define DEVICE_IP_ADDRESS "169.254.2.2"
#define DEVICE_NETMASK    "255.255.255.0"
#define DEVICE_BROADCAST  "169.254.2.255"

typedef struct _VDCCMEventManager VDCCMEventManager;
typedef struct _VDCCMEventManagerClass VDCCMEventManagerClass;

struct _VDCCMEventManager
{
  GObject parent;
};

struct _VDCCMEventManagerClass
{
  GObjectClass parent_class;
};

GType vdccm_event_manager_get_type (void);

VDCCMEventManager *_vdccm_get_event_manager ();

void _vdccm_event_manager_device_connected (VDCCMEventManager *self,
                                            gpointer ce_device_base);
void _vdccm_event_manager_device_disconnected (VDCCMEventManager *self,
                                               gpointer ce_device_base);

#define VDCCM_TYPE_EVENT_MANAGER \
    (vdccm_event_manager_get_type())
#define VDCCM_EVENT_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), VDCCM_TYPE_EVENT_MANAGER, VDCCMEventManager))
#define VDCCM_EVENT_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), VDCCM_TYPE_EVENT_MANAGER, VDCCMEventManagerClass))
#define VDCCM_IS_EVENT_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), VDCCM_TYPE_EVENT_MANAGER))
#define VDCCM_IS_EVENT_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), VDCCM_TYPE_EVENT_MANAGER))
#define VDCCM_EVENT_MANAGER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), VDCCM_TYPE_EVENT_MANAGER, VDCCMEventManagerClass))

G_END_DECLS

#endif /* EVENTMANAGER_H */
