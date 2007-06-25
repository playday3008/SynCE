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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <synce.h>
#include <rapi.h>
#include <glib/gi18n.h>
#include <string.h>

#include "device.h"

#ifdef G_THREADS_ENABLED
#define MUTEX_NEW()     g_mutex_new ()
#define MUTEX_FREE(a)   g_mutex_free (a)
#define MUTEX_LOCK(a)   if ((a) != NULL) g_mutex_lock (a)
#define MUTEX_UNLOCK(a) if ((a) != NULL) g_mutex_unlock (a)
#else
#define MUTEX_NEW()     NULL
#define MUTEX_FREE(a)
#define MUTEX_LOCK(a)
#define MUTEX_UNLOCK(a)
#endif

G_DEFINE_TYPE (WmDevice, wm_device, G_TYPE_OBJECT)

typedef struct _WmDevicePrivate WmDevicePrivate;
struct _WmDevicePrivate {
  uint16_t os_version;
  uint16_t build_number;
  uint16_t processor_type;
  uint32_t partner_id_1;
  uint32_t partner_id_2;
  /* lowercase name from info file */
  gchar *name;
  gchar *class;
  gchar *hardware;

  /* real name, get from registry */
  gchar *device_name;

  gchar *password;
  int key;
  pid_t dccm_pid;
  gchar *ip;
  gchar *transport;

  gchar *port;

  RapiConnection *rapi_conn;

  gboolean disposed;
};

#define WM_DEVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), WM_DEVICE_TYPE, WmDevicePrivate))

static GMutex * mutex = NULL;


     /* methods */

static void
wm_device_debug_device(WmDevice *self)
{
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  g_debug("Name (from dccm): %s", priv->name);
  g_debug("Device name: %s", priv->device_name);
  g_debug("OS Version: %d", priv->os_version);
  g_debug("Build number: %d", priv->build_number);
  g_debug("Processor type: %d", priv->processor_type);
  g_debug("Partder id 1: %d", priv->partner_id_1);
  g_debug("Partner id 2: %d", priv->partner_id_2);
  g_debug("Class: %s", priv->class);
  g_debug("Hardware: %s", priv->hardware);

  if (priv->password) {
    g_debug("Password: %s", priv->password);
    g_debug("Key: %d", priv->key);
  }

  g_debug("DCCM pid: %d", priv->dccm_pid);
  g_debug("IP: %s", priv->ip);
  g_debug("Transport: %s", priv->transport);
}

static gboolean
wm_device_rapi_connect(WmDevice *self)
{
  HRESULT hr;
  gboolean result = FALSE;
  RapiConnection *rapi_conn;

  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return FALSE;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return FALSE;
  }

  SynceInfo *info = g_malloc0(sizeof(SynceInfo));

  info->os_version = priv->os_version;
  info->build_number = priv->build_number;
  info->processor_type = priv->processor_type;
  info->partner_id_1 = priv->partner_id_1;
  info->partner_id_2 = priv->partner_id_2;
  info->name = g_strdup(priv->name);
  info->os_name = g_strdup(priv->class);
  info->model = g_strdup(priv->hardware);
  info->password = g_strdup(priv->password);
  info->key = priv->key;
  info->dccm_pid = priv->dccm_pid;
  info->ip = g_strdup(priv->ip);
  info->transport = g_strdup(priv->transport);
  info->os_name = g_strdup(priv->device_name);

  rapi_conn = rapi_connection_from_info(info);
  rapi_connection_select(rapi_conn);
  hr = CeRapiInit();
  if (FAILED(hr)) {
    g_critical("Rapi connection to %s failed: %d: %s: %s", priv->name, hr, synce_strerror(hr), G_STRFUNC);
    goto exit;
  }

  priv->rapi_conn = rapi_conn;
  result = TRUE;
exit:
  synce_info_destroy(info);
  return result;
}


static gboolean
wm_device_rapi_disconnect(WmDevice *self)
{
  gboolean result = FALSE;

  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return result;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return result;
  }

  rapi_connection_select(priv->rapi_conn);
  CeRapiUninit();
  rapi_connection_destroy(priv->rapi_conn);
  priv->rapi_conn = NULL;

  result = TRUE;

  return result;
}


WmDevice *
wm_device_from_synce_info(WmDevice *self, SynceInfo *info)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_from_synce_info(self, info);
}

uint16_t
wm_device_get_os_version(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_os_version(self);
}

uint16_t
wm_device_get_build_number(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_build_number(self);
}

uint16_t
wm_device_get_processor_type(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_processor_type(self);
}

uint32_t
wm_device_get_partner_id_1(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_partner_id_1(self);
}

uint32_t
wm_device_get_partner_id_2(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_partner_id_2(self);
}

gchar *
wm_device_get_name(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_name(self);
}

gchar *
wm_device_get_class(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_class(self);
}

gchar *
wm_device_get_hardware(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_hardware(self);
}

gchar *
wm_device_get_device_name(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_device_name(self);
}

gchar *
wm_device_get_password(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_password(self);
}

int
wm_device_get_key(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_key(self);
}

pid_t
wm_device_get_dccm_pid(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_dccm_pid(self);
}

gchar *
wm_device_get_ip(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_ip(self);
}

gchar *
wm_device_get_transport(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_transport(self);
}

gchar *
wm_device_get_port(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_port(self);
}

gchar *
wm_device_get_power_status(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_power_status(self);
}

gchar *
wm_device_get_store_status(WmDevice *self)
{
  return WM_DEVICE_GET_CLASS (self)->wm_device_get_store_status(self);
}


WmDevice *
wm_device_from_synce_info_impl(WmDevice *self, SynceInfo *info)
{
  LONG result;
  WCHAR* key_name = NULL;
  HKEY key_handle = 0;
  DWORD type = 0;
  DWORD size;
  WCHAR buffer[MAX_PATH];

  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return NULL;
  }

  priv->os_version = info->os_version;
  priv->build_number = info->build_number;
  priv->processor_type = info->processor_type;
  priv->partner_id_1 = info->partner_id_1;
  priv->partner_id_2 = info->partner_id_2;
  priv->name = g_strdup(info->name);
  priv->class = g_strdup(info->os_name);
  priv->hardware = g_strdup(info->model);
  priv->password = g_strdup(info->password);
  priv->key = info->key;
  priv->dccm_pid = info->dccm_pid;
  priv->ip = g_strdup(info->ip);
  priv->transport = g_strdup(info->transport);
  priv->port = NULL;

  g_free(priv->device_name);
  priv->device_name = NULL;
  MUTEX_LOCK(mutex);
  if (!(wm_device_rapi_connect(self))) {
    MUTEX_UNLOCK (mutex);
    goto exit;
  }

  key_name = wstr_from_ascii("Ident");
  result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, 0, &key_handle);
  wstr_free_string(key_name);

  if (result != ERROR_SUCCESS) {
    g_critical("CeRegOpenKeyEx failed getting device name: %s", G_STRFUNC);
    wm_device_rapi_disconnect(self);
    MUTEX_UNLOCK (mutex);
    goto exit;
  }

  key_name = wstr_from_ascii("Name");
  size = sizeof(buffer);

  result = CeRegQueryValueEx(key_handle, key_name, 0, &type, (LPBYTE)buffer, &size);
  wstr_free_string(key_name);

  if (result != ERROR_SUCCESS) {
    g_critical("CeRegQueryValueEx failed getting device name: %s", G_STRFUNC);
    wm_device_rapi_disconnect(self);
    MUTEX_UNLOCK (mutex);
    goto exit;
  }

  if (type != REG_SZ) {
    g_critical("Unexpected value type: 0x%08x = %i: %s", type, type, G_STRFUNC);
    wm_device_rapi_disconnect(self);
    MUTEX_UNLOCK (mutex);
    goto exit;
  }

  priv->device_name = wstr_to_ascii(buffer);

  wm_device_rapi_disconnect(self);
  MUTEX_UNLOCK (mutex);

exit:
  wm_device_debug_device(self);

  return self;
}


uint16_t
wm_device_get_os_version_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return 0;
  }

  return priv->os_version;
}

uint16_t
wm_device_get_build_number_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return 0;
  }

  return priv->build_number;
}

uint16_t
wm_device_get_processor_type_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return 0;
  }

  return priv->processor_type;
}

uint32_t
wm_device_get_partner_id_1_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return 0;
  }

  return priv->partner_id_1;
}

uint32_t
wm_device_get_partner_id_2_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return 0;
  }

  return priv->partner_id_2;
}

gchar *
wm_device_get_name_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->name);
}

gchar *
wm_device_get_class_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->class);
}

gchar *
wm_device_get_hardware_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->hardware);
}

gchar *
wm_device_get_device_name_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->device_name);
}

gchar *
wm_device_get_password_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->password);
}

int
wm_device_get_key_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return 0;
  }

  return priv->key;
}

pid_t
wm_device_get_dccm_pid_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return 0;
  }

  return priv->dccm_pid;
}

gchar *
wm_device_get_ip_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->ip);
}

gchar *
wm_device_get_transport_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->transport);
}

gchar *
wm_device_get_port_impl(WmDevice *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->port);
}

static const gchar*
get_battery_flag_string(guint flag)
{
  const gchar* name;
	
  switch (flag)
    {
    case BATTERY_FLAG_HIGH:        name = _("High");       break;
    case BATTERY_FLAG_LOW:         name = _("Low");        break;
    case BATTERY_FLAG_CRITICAL:    name = _("Critical");   break;
    case BATTERY_FLAG_CHARGING:    name = _("Charging");   break;
    case BATTERY_FLAG_NO_BATTERY:  name = _("NoBattery");  break;

    default: name = _("Unknown"); break;
    }

  return name;
}

gchar *
wm_device_get_power_status_impl(WmDevice *self)
{
  SYSTEM_POWER_STATUS_EX power;
  gchar* power_str = NULL;

  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return NULL;
  }

  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return NULL;
  }

  MUTEX_LOCK (mutex);
  if (!(wm_device_rapi_connect(self))) {
    MUTEX_UNLOCK (mutex);
    power_str = g_strdup(_("Unknown"));
    goto exit;
  }

  memset(&power, 0, sizeof(SYSTEM_POWER_STATUS_EX));
  if (CeGetSystemPowerStatusEx(&power, false) &&
      BATTERY_PERCENTAGE_UNKNOWN != power.BatteryLifePercent)
    {
      power_str = g_strdup_printf("%i%% (%s)", 
				  power.BatteryLifePercent, 
				  get_battery_flag_string(power.BatteryFlag));
    } else {
      power_str = g_strdup(_("Unknown"));
    }

  wm_device_rapi_disconnect(self);
  MUTEX_UNLOCK (mutex);
exit:
  return power_str;
}
	 
gchar *
wm_device_get_store_status_impl(WmDevice *self)
{
  STORE_INFORMATION store;
  gchar* store_str = NULL;

  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return NULL;
  }

  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return NULL;
  }

  MUTEX_LOCK (mutex);

  if (!(wm_device_rapi_connect(self))) {
    MUTEX_UNLOCK (mutex);
    store_str = g_strdup(_("Unknown"));
    goto exit;
  }

  memset(&store, 0, sizeof(store));
  if (CeGetStoreInformation(&store) && store.dwStoreSize != 0)
    {
      store_str = g_strdup_printf(_("%i%% (%i megabytes)"), 
				  100 * store.dwFreeSize / store.dwStoreSize,
				  store.dwFreeSize >> 20);
    } else {
      store_str = g_strdup(_("Unknown"));
    }

  wm_device_rapi_disconnect(self);
  MUTEX_UNLOCK (mutex);
exit:
  return store_str;
}



/* class & instance functions */

static void
wm_device_init(WmDevice *self)
{
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  priv->os_version = 0;
  priv->build_number = 0;
  priv->processor_type = 0;
  priv->partner_id_1 = 0;
  priv->partner_id_2 = 0;
  priv->name = NULL;
  priv->class = NULL;
  priv->hardware = NULL;
  priv->device_name = NULL;
  priv->password = NULL;
  priv->key = 0;
  priv->dccm_pid = 0;
  priv->ip = NULL;
  priv->transport = NULL;
  priv->port = NULL;
}

static void
wm_device_dispose (GObject *obj)
{
  WmDevice *self = WM_DEVICE(obj);
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    return;
  }
  priv->disposed = TRUE;

  /* unref other objects */

  if (G_OBJECT_CLASS (wm_device_parent_class)->dispose)
    G_OBJECT_CLASS (wm_device_parent_class)->dispose (obj);
}


static void
wm_device_finalize (GObject *obj)
{
  WmDevice *self = WM_DEVICE(obj);
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  g_free(priv->name);
  g_free(priv->class);
  g_free(priv->hardware);
  g_free(priv->device_name);
  g_free(priv->password);
  g_free(priv->ip);
  g_free(priv->transport);
  g_free(priv->port);

  if (G_OBJECT_CLASS (wm_device_parent_class)->finalize)
    G_OBJECT_CLASS (wm_device_parent_class)->finalize (obj);
}

static void
wm_device_class_init (WmDeviceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = wm_device_dispose;
  gobject_class->finalize = wm_device_finalize;

  g_type_class_add_private (klass, sizeof (WmDevicePrivate));
  
  klass->wm_device_from_synce_info = &wm_device_from_synce_info_impl;

  klass->wm_device_get_os_version = &wm_device_get_os_version_impl;
  klass->wm_device_get_build_number = &wm_device_get_build_number_impl;
  klass->wm_device_get_processor_type = &wm_device_get_processor_type_impl;
  klass->wm_device_get_partner_id_1 = &wm_device_get_partner_id_1_impl;
  klass->wm_device_get_partner_id_2 = &wm_device_get_partner_id_2_impl;
  klass->wm_device_get_name = &wm_device_get_name_impl;
  klass->wm_device_get_class = &wm_device_get_class_impl;
  klass->wm_device_get_hardware = &wm_device_get_hardware_impl;
  klass->wm_device_get_device_name = &wm_device_get_device_name_impl;
  klass->wm_device_get_password = &wm_device_get_password_impl;
  klass->wm_device_get_key = &wm_device_get_key_impl;
  klass->wm_device_get_dccm_pid = &wm_device_get_dccm_pid_impl;
  klass->wm_device_get_ip = &wm_device_get_ip_impl;
  klass->wm_device_get_transport = &wm_device_get_transport_impl;
  klass->wm_device_get_port = &wm_device_get_port_impl;
  klass->wm_device_get_power_status = &wm_device_get_power_status_impl;
  klass->wm_device_get_store_status = &wm_device_get_store_status_impl;
}
