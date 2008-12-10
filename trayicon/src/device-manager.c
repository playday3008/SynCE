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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "device-manager.h"

G_DEFINE_TYPE (WmDeviceManager, wm_device_manager, G_TYPE_OBJECT)

typedef struct _WmDeviceManagerPrivate WmDeviceManagerPrivate;
struct _WmDeviceManagerPrivate {
  GPtrArray *devices;

  gboolean disposed;
};

#define WM_DEVICE_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), WM_DEVICE_MANAGER_TYPE, WmDeviceManagerPrivate))

     /* methods */

gint
wm_device_manager_device_all_count(WmDeviceManager *self)
{
  return WM_DEVICE_MANAGER_GET_CLASS (self)->wm_device_manager_device_all_count(self);
}

gint
wm_device_manager_device_connected_count(WmDeviceManager *self)
{
  return WM_DEVICE_MANAGER_GET_CLASS (self)->wm_device_manager_device_connected_count(self);
}

gint
wm_device_manager_device_passwordreq_count(WmDeviceManager *self)
{
  return WM_DEVICE_MANAGER_GET_CLASS (self)->wm_device_manager_device_passwordreq_count(self);
}

WmDevice *
wm_device_manager_find_by_name(WmDeviceManager *self, const gchar *name)
{
  return WM_DEVICE_MANAGER_GET_CLASS (self)->wm_device_manager_find_by_name(self, name);
}

WmDevice *
wm_device_manager_find_by_index(WmDeviceManager *self, gint index)
{
  return WM_DEVICE_MANAGER_GET_CLASS (self)->wm_device_manager_find_by_index(self, index);
}

void
wm_device_manager_remove_by_name(WmDeviceManager *self, const gchar *name)
{
  return WM_DEVICE_MANAGER_GET_CLASS (self)->wm_device_manager_remove_by_name(self, name);
}

void
wm_device_manager_remove_by_prop(WmDeviceManager *self, const gchar *prop_name, const gchar *prop_val)
{
  return WM_DEVICE_MANAGER_GET_CLASS (self)->wm_device_manager_remove_by_prop(self, prop_name, prop_val);
}

void
wm_device_manager_remove_all(WmDeviceManager *self)
{
  return WM_DEVICE_MANAGER_GET_CLASS (self)->wm_device_manager_remove_all(self);
}

GList *
wm_device_manager_get_all_names(WmDeviceManager *self)
{
  return WM_DEVICE_MANAGER_GET_CLASS (self)->wm_device_manager_get_all_names(self);
}

GList *
wm_device_manager_get_connected_names(WmDeviceManager *self)
{
  return WM_DEVICE_MANAGER_GET_CLASS (self)->wm_device_manager_get_connected_names(self);
}

GList *
wm_device_manager_get_passwordreq_names(WmDeviceManager *self)
{
  return WM_DEVICE_MANAGER_GET_CLASS (self)->wm_device_manager_get_passwordreq_names(self);
}

void
wm_device_manager_unlocked(WmDeviceManager *self, const gchar *name)
{
  return WM_DEVICE_MANAGER_GET_CLASS (self)->wm_device_manager_unlocked(self, name);
}

gboolean
wm_device_manager_add(WmDeviceManager *self, WmDevice *device)
{
  return WM_DEVICE_MANAGER_GET_CLASS (self)->wm_device_manager_add(self, device);
}


gint
wm_device_manager_device_all_count_impl(WmDeviceManager *self)
{

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return 0;
  }
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return 0;
  }

  return priv->devices->len;
}

gint
wm_device_manager_device_connected_count_impl(WmDeviceManager *self)
{

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return 0;
  }
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return 0;
  }

  guint connection_status;
  guint count = 0;
  gint i;
  WmDevice *device = NULL;

  for (i = 0; i < priv->devices->len; i++) {
    device = g_ptr_array_index(priv->devices, i);
    g_object_get(device, "connection-status", &connection_status, NULL);

    if (connection_status == DEVICE_STATUS_CONNECTED)
            count++;
  }

  return count;
}

gint
wm_device_manager_device_passwordreq_count_impl(WmDeviceManager *self)
{

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return 0;
  }
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return 0;
  }

  guint connection_status;
  guint count = 0;
  gint i;
  WmDevice *device = NULL;

  for (i = 0; i < priv->devices->len; i++) {
    device = g_ptr_array_index(priv->devices, i);
    g_object_get(device, "connection-status", &connection_status, NULL);

    if (connection_status == DEVICE_STATUS_PASSWORD_REQUIRED)
            count++;
  }

  return count;
}

WmDevice *
wm_device_manager_find_by_name_impl(WmDeviceManager *self, const gchar *name)
{
  WmDevice *device = NULL;
  int i;
  gchar *tmpname;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  for (i = 0; i < priv->devices->len; i++) {
    device = g_ptr_array_index(priv->devices, i);
    tmpname = wm_device_get_name(device);
    if (!(g_ascii_strcasecmp(name,tmpname))) {
      g_free(tmpname);
      return device;
    }
    g_free(tmpname);
  }

  return NULL;
}

WmDevice *
wm_device_manager_find_by_index_impl(WmDeviceManager *self, gint index)
{
  WmDevice *device = NULL;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }
  if (index >= priv->devices->len) {
    g_critical("%s: Attempt to find index %d, only %d present", G_STRFUNC, index, priv->devices->len);
    return NULL;
  }
  device = g_ptr_array_index(priv->devices, index);

  return device;
}

void
wm_device_manager_remove_by_name_impl(WmDeviceManager *self, const gchar *name)
{
  WmDevice *device = NULL;
  int i;
  gchar *tmpname;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  for (i = 0; i < priv->devices->len; i++) {
    device = g_ptr_array_index(priv->devices, i);
    tmpname = wm_device_get_name(device);
    if (!(g_ascii_strcasecmp(name,tmpname))) {
      g_ptr_array_remove_index_fast(priv->devices, i);
      g_signal_emit (self, WM_DEVICE_MANAGER_GET_CLASS (self)->signals[DEVICE_REMOVED], 0, tmpname);
      g_free(tmpname);
      g_object_unref(device);
      return;
    }
    g_free(tmpname);
  }
  g_warning("%s: Device '%s' not removed, not found", G_STRFUNC, name);
  return;
}

void
wm_device_manager_remove_by_prop_impl(WmDeviceManager *self, const gchar *prop_name, const gchar *prop_val)
{
  WmDevice *device = NULL;
  gchar *tmpval = NULL;
  guint i;
  gchar *name = NULL;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  for (i = 0; i < priv->devices->len; i++) {
    device = g_ptr_array_index(priv->devices, i);

    g_object_get(device, prop_name, &tmpval, NULL);

    if (!(g_ascii_strcasecmp(prop_val, tmpval))) {
      g_ptr_array_remove_index_fast(priv->devices, i);
      g_object_get(device, "name", &name, NULL);
      g_signal_emit (self, WM_DEVICE_MANAGER_GET_CLASS (self)->signals[DEVICE_REMOVED], 0, name);
      g_free(name);
      g_object_unref(device);
    }
    g_free(tmpval);
  }

  return;
}

void
wm_device_manager_remove_all_impl(WmDeviceManager *self)
{
  WmDevice *device = NULL;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  while(priv->devices->len > 0) {
    device = g_ptr_array_index(priv->devices, 0);
    g_object_unref(device);
    g_ptr_array_remove_index_fast(priv->devices, 0);
  }
  return;
}


GList *
wm_device_manager_get_all_names_impl(WmDeviceManager *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  WmDevice *device = NULL;
  GList *device_names = NULL;
  gchar *name = NULL;
  gint i;

  for (i = 0; i < priv->devices->len; i++) {
    device = g_ptr_array_index(priv->devices, i);
    name = wm_device_get_name(device);

    device_names = g_list_append(device_names, name);
  }

  return device_names;
}


GList *
wm_device_manager_get_connected_names_impl(WmDeviceManager *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  WmDevice *device = NULL;
  GList *device_names = NULL;
  gchar *name = NULL;
  guint connection_status;
  gint i;

  for (i = 0; i < priv->devices->len; i++) {
    device = g_ptr_array_index(priv->devices, i);
    g_object_get(device, "connection-status", &connection_status, NULL);

    if (connection_status == DEVICE_STATUS_CONNECTED) {
            name = wm_device_get_name(device);
            device_names = g_list_append(device_names, name);
    }
  }

  return device_names;
}

GList *
wm_device_manager_get_passwordreq_names_impl(WmDeviceManager *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  WmDevice *device = NULL;
  GList *device_names = NULL;
  gchar *name = NULL;
  guint connection_status;
  gint i;

  for (i = 0; i < priv->devices->len; i++) {
    device = g_ptr_array_index(priv->devices, i);
    g_object_get(device, "connection-status", &connection_status, NULL);

    if (connection_status == DEVICE_STATUS_PASSWORD_REQUIRED) {
            name = wm_device_get_name(device);
            device_names = g_list_append(device_names, name);
    }
  }

  return device_names;
}

gboolean
wm_device_manager_add_impl(WmDeviceManager *self, WmDevice *device)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return FALSE;
  }
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return FALSE;
  }

  guint connection_status;
  gchar *name = NULL;

  g_ptr_array_add(priv->devices, device);

  g_object_get(device, "connection-status", &connection_status, NULL);
  if (connection_status == DEVICE_STATUS_CONNECTED) {
          wm_device_rapi_connect(device);
          g_object_get(device, "name", &name, NULL);
          g_signal_emit (self, WM_DEVICE_MANAGER_GET_CLASS (self)->signals[DEVICE_ADDED], 0, name);
          g_free(name);
  }

  return TRUE;
}

void
wm_device_manager_unlocked_impl(WmDeviceManager *self, const gchar *name)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  WmDevice *device = NULL;
  gchar *tmpname = NULL;
  guint conn_stat;
  gint i;

  for (i = 0; i < priv->devices->len; i++) {
          device = g_ptr_array_index(priv->devices, i);

          tmpname = wm_device_get_name(device);
          if (!(g_ascii_strcasecmp(name,tmpname))) {
                  g_free(tmpname);
                  g_object_get(device, "connection-status", &conn_stat, NULL);
                  if (conn_stat == DEVICE_STATUS_CONNECTED) {
                          g_debug("%s: ignoring unlocked message for \"%s\", already connected", G_STRFUNC, name);
                          return;
                  }

                  g_object_set(device, "connection-status", DEVICE_STATUS_CONNECTED, NULL);
                  wm_device_rapi_connect(device);
                  g_signal_emit (self, WM_DEVICE_MANAGER_GET_CLASS (self)->signals[DEVICE_ADDED], 0, name);
                  return;
          }
          g_free(tmpname);
  }

  g_debug("%s: ignoring unlocked message for \"%s\", not connected", G_STRFUNC, name);
  return;
}


/* class & instance functions */

static void
wm_device_manager_init(WmDeviceManager *self)
{
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  priv->devices = g_ptr_array_new();
}

static void
wm_device_manager_dispose (GObject *obj)
{
  WmDeviceManager *self = WM_DEVICE_MANAGER(obj);
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);
  WmDevice *device;
  int i;

  if (priv->disposed) {
    return;
  }
  priv->disposed = TRUE;

  /* unref other objects */
  for (i = 0; i < priv->devices->len; i++) {
    device = g_ptr_array_index(priv->devices, i);
    g_object_unref(device);
  }

  if (G_OBJECT_CLASS (wm_device_manager_parent_class)->dispose)
    G_OBJECT_CLASS (wm_device_manager_parent_class)->dispose (obj);
}

static void
wm_device_manager_finalize (GObject *obj)
{
  WmDeviceManager *self = WM_DEVICE_MANAGER(obj);
  WmDeviceManagerPrivate *priv = WM_DEVICE_MANAGER_GET_PRIVATE (self);

  g_ptr_array_free(priv->devices, TRUE);

  if (G_OBJECT_CLASS (wm_device_manager_parent_class)->finalize)
    G_OBJECT_CLASS (wm_device_manager_parent_class)->finalize (obj);
}

static void
wm_device_manager_class_init (WmDeviceManagerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (WmDeviceManagerPrivate));

  gobject_class->dispose = wm_device_manager_dispose;
  gobject_class->finalize = wm_device_manager_finalize;

  klass->signals[DEVICE_ADDED] = g_signal_new ("device-added",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  klass->signals[DEVICE_REMOVED] = g_signal_new ("device-removed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  klass->signals[DEVICE_MANAGER_LAST_SIGNAL] = 0;

  klass->wm_device_manager_device_all_count = &wm_device_manager_device_all_count_impl;
  klass->wm_device_manager_device_connected_count = &wm_device_manager_device_connected_count_impl;
  klass->wm_device_manager_device_passwordreq_count = &wm_device_manager_device_passwordreq_count_impl;
  klass->wm_device_manager_find_by_name = &wm_device_manager_find_by_name_impl;
  klass->wm_device_manager_find_by_index = &wm_device_manager_find_by_index_impl;
  klass->wm_device_manager_remove_by_name = &wm_device_manager_remove_by_name_impl;
  klass->wm_device_manager_remove_by_prop = &wm_device_manager_remove_by_prop_impl;
  klass->wm_device_manager_remove_all = &wm_device_manager_remove_all_impl;
  klass->wm_device_manager_get_all_names = &wm_device_manager_get_all_names_impl;
  klass->wm_device_manager_get_connected_names = &wm_device_manager_get_connected_names_impl;
  klass->wm_device_manager_get_passwordreq_names = &wm_device_manager_get_passwordreq_names_impl;
  klass->wm_device_manager_add = &wm_device_manager_add_impl;
  klass->wm_device_manager_unlocked = &wm_device_manager_unlocked_impl;
}
