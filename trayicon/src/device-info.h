/*
Copyright (c) 2007 Mark Ellis <mark@mpellis.org.uk>

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

#ifndef WM_DEVICE_INFO_H
#define WM_DEVICE_INFO_H

#include <glib-object.h>

#include "synce-trayicon.h"
#include "device.h"

G_BEGIN_DECLS

typedef enum _WmDeviceInfoSignals WmDeviceInfoSignals;
enum _WmDeviceInfoSignals
{
  DEVICE_INFO_CLOSED,
  DEVICE_INFO_NUM_SIGNALS
};

typedef struct _WmDeviceInfo WmDeviceInfo;
struct _WmDeviceInfo {
  GObject parent;
};

typedef struct _WmDeviceInfoClass WmDeviceInfoClass;
struct _WmDeviceInfoClass {
  GObjectClass parent_class;

  guint signals[DEVICE_INFO_NUM_SIGNALS];
};

GType wm_device_info_get_type (void);

#define WM_DEVICE_INFO_TYPE (wm_device_info_get_type())
#define WM_DEVICE_INFO(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WM_DEVICE_INFO_TYPE, WmDeviceInfo))
#define WM_DEVICE_INFO_CLASS(c) (G_TYPE_CHECK_CLASS_CAST ((c), WM_DEVICE_INFO_TYPE, WmDeviceInfoClass))
#define WM_IS_DEVICE_INFO(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WM_DEVICE_INFO_TYPE))
#define WM_IS_DEVICE_INFO_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), WM_DEVICE_INFO_TYPE))
#define WM_DEVICE_INFO_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WM_DEVICE_INFO_TYPE, WmDeviceInfoClass))

G_END_DECLS

#endif /* WM_DEVICE_INFO_H */
