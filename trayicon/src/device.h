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

#ifndef WM_DEVICE_H
#define WM_DEVICE_H

#include <glib-object.h>
#include <stdint.h>
#include <sys/types.h>

G_BEGIN_DECLS

typedef struct _WmDevice WmDevice;
struct _WmDevice {
  GObject parent;
};

typedef struct _WmDeviceClass WmDeviceClass;
struct _WmDeviceClass {
  GObjectClass parent_class;

  uint16_t (*wm_device_get_os_version) (WmDevice *self);
  uint16_t (*wm_device_get_build_number) (WmDevice *self);
  uint16_t (*wm_device_get_processor_type) (WmDevice *self);
  uint32_t (*wm_device_get_partner_id_1) (WmDevice *self);
  uint32_t (*wm_device_get_partner_id_2) (WmDevice *self);
  gchar *(*wm_device_get_name) (WmDevice *self);
  gchar *(*wm_device_get_class) (WmDevice *self);
  gchar *(*wm_device_get_hardware) (WmDevice *self);
  gchar *(*wm_device_get_device_name) (WmDevice *self);
  gchar *(*wm_device_get_password) (WmDevice *self);
  int (*wm_device_get_key) (WmDevice *self);
  pid_t (*wm_device_get_dccm_pid) (WmDevice *self);
  gchar *(*wm_device_get_ip) (WmDevice *self);
  gchar *(*wm_device_get_transport) (WmDevice *self);
  gchar *(*wm_device_get_port) (WmDevice *self);

  gchar *(*wm_device_get_power_status) (WmDevice *self);
  gchar *(*wm_device_get_store_status) (WmDevice *self);
};

GType wm_device_get_type (void);

#define WM_DEVICE_TYPE (wm_device_get_type())
#define WM_DEVICE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WM_DEVICE_TYPE, WmDevice))
#define WM_DEVICE_CLASS(c) (G_TYPE_CHECK_CLASS_CAST ((c), WM_DEVICE_TYPE, WmDeviceClass))

#ifdef G_TYPE_CHECK_TYPE
#define WM_IS_DEVICE(obj) (G_TYPE_CHECK_TYPE ((obj), WM_DEVICE_TYPE))
#else
#define WM_IS_DEVICE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WM_DEVICE_TYPE))
#endif

#define WM_IS_DEVICE_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), WM_DEVICE_TYPE))
#define WM_DEVICE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WM_DEVICE_TYPE, WmDeviceClass))

typedef enum _WmDeviceStatus WmDeviceStatus;
enum _WmDeviceStatus {
        DEVICE_STATUS_CONNECTED = 0,
        DEVICE_STATUS_PASSWORD_REQUIRED,
        DEVICE_STATUS_PASSWORD_REQUIRED_ON_DEVICE,
        DEVICE_STATUS_UNKNOWN
};

uint16_t wm_device_get_os_version(WmDevice *self);
uint16_t wm_device_get_build_number(WmDevice *self);
uint16_t wm_device_get_processor_type(WmDevice *self);
uint32_t wm_device_get_partner_id_1(WmDevice *self);
uint32_t wm_device_get_partner_id_2(WmDevice *self);
gchar *wm_device_get_name(WmDevice *self);
gchar *wm_device_get_class(WmDevice *self);
gchar *wm_device_get_hardware(WmDevice *self);
gchar *wm_device_get_device_name(WmDevice *self);
gchar *wm_device_get_password(WmDevice *self);
int wm_device_get_key(WmDevice *self);
pid_t wm_device_get_dccm_pid(WmDevice *self);
gchar *wm_device_get_ip(WmDevice *self);
gchar *wm_device_get_transport(WmDevice *self);
gchar *wm_device_get_port(WmDevice *self);
gchar *wm_device_get_power_status(WmDevice *self);
gchar *wm_device_get_store_status(WmDevice *self);

gboolean wm_device_rapi_connect(WmDevice *device);
void wm_device_rapi_select(WmDevice *device);

G_END_DECLS

#endif /* WM_DEVICE_H */
