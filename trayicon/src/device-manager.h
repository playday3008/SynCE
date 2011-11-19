/*
Copyright (c) 2007-2008 Mark Ellis <mark@mpellis.org.uk>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/

#ifndef WM_DEVICE_MANAGER_H
#define WM_DEVICE_MANAGER_H

#include <glib-object.h>
#include "device.h"

G_BEGIN_DECLS

typedef enum _WmDeviceManagerSignals WmDeviceManagerSignals;
enum _WmDeviceManagerSignals
{
  DEVICE_ADDED,
  DEVICE_REMOVED,
  DEVICE_MANAGER_NUM_SIGNALS
};

typedef struct _WmDeviceManager WmDeviceManager;
struct _WmDeviceManager {
  GObject parent;
};

typedef struct _WmDeviceManagerClass WmDeviceManagerClass;
struct _WmDeviceManagerClass {
  GObjectClass parent_class;

  guint signals[DEVICE_MANAGER_NUM_SIGNALS];

  gint (*wm_device_manager_device_all_count) (WmDeviceManager *self);
  gint (*wm_device_manager_device_connected_count) (WmDeviceManager *self);
  gint (*wm_device_manager_device_passwordreq_count) (WmDeviceManager *self);
  gint (*wm_device_manager_device_passwordreqondevice_count) (WmDeviceManager *self);
  WmDevice * (*wm_device_manager_find_by_name) (WmDeviceManager *self, const gchar *name);
  WmDevice * (*wm_device_manager_find_by_index) (WmDeviceManager *self, gint index);
  void (*wm_device_manager_remove_by_name) (WmDeviceManager *self, const gchar *name);
  void (*wm_device_manager_remove_by_prop) (WmDeviceManager *self, const gchar *prop_name, const gchar *prop_val);
  void (*wm_device_manager_remove_all) (WmDeviceManager *self);
  GList * (*wm_device_manager_get_all_names) (WmDeviceManager *self);
  GList * (*wm_device_manager_get_connected_names) (WmDeviceManager *self);
  GList * (*wm_device_manager_get_passwordreq_names) (WmDeviceManager *self);
  GList * (*wm_device_manager_get_passwordreqondevice_names) (WmDeviceManager *self);
  gboolean (*wm_device_manager_add) (WmDeviceManager *self, WmDevice *device);
  void (*wm_device_manager_unlocked) (WmDeviceManager *self, const gchar *name);

};

GType wm_device_manager_get_type (void);

#define WM_DEVICE_MANAGER_TYPE (wm_device_manager_get_type())
#define WM_DEVICE_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WM_DEVICE_MANAGER_TYPE, WmDeviceManager))
#define WM_DEVICE_MANAGER_CLASS(c) (G_TYPE_CHECK_CLASS_CAST ((c), WM_DEVICE_MANAGER_TYPE, WmDeviceManagerClass))

#ifdef G_TYPE_CHECK_TYPE
#define WM_IS_DEVICE_MANAGER(obj) (G_TYPE_CHECK_TYPE ((obj), WM_DEVICE_MANAGER_TYPE))
#else
#define WM_IS_DEVICE_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WM_DEVICE_MANAGER_TYPE))
#endif

#define WM_IS_DEVICE_MANAGER_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), WM_DEVICE_MANAGER_TYPE))
#define WM_DEVICE_MANAGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WM_DEVICE_MANAGER_TYPE, WmDeviceManagerClass))

gint wm_device_manager_device_all_count(WmDeviceManager *self);
gint wm_device_manager_device_connected_count(WmDeviceManager *self);
gint wm_device_manager_device_passwordreq_count(WmDeviceManager *self);
gint wm_device_manager_device_passwordreqondevice_count(WmDeviceManager *self);
WmDevice *wm_device_manager_find_by_name(WmDeviceManager *self, const gchar *name);
WmDevice *wm_device_manager_find_by_index(WmDeviceManager *self, gint index);
void wm_device_manager_remove_by_name(WmDeviceManager *self, const gchar *name);
void wm_device_manager_remove_by_prop(WmDeviceManager *self, const gchar *prop_name, const gchar *prop_val);
void wm_device_manager_remove_all(WmDeviceManager *self);
GList *wm_device_manager_get_all_names(WmDeviceManager *self);
GList *wm_device_manager_get_connected_names(WmDeviceManager *self);
GList *wm_device_manager_get_passwordreq_names(WmDeviceManager *self);
GList *wm_device_manager_get_passwordreqondevice_names(WmDeviceManager *self);
gboolean wm_device_manager_add(WmDeviceManager *self, WmDevice *device);
void wm_device_manager_unlocked(WmDeviceManager *self, const gchar *name);

G_END_DECLS

#endif /* WM_DEVICE_MANAGER_H */
