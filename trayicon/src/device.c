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
#include <rapi2.h>
#include <glib/gi18n.h>
#include <string.h>

#include "device.h"
#include "utils.h"

G_DEFINE_TYPE (WmDevice, wm_device, G_TYPE_OBJECT)

typedef struct _WmDevicePrivate WmDevicePrivate;
struct _WmDevicePrivate {
  /* identifier from dccm */
  gchar *object_name;
  gchar *dccm_type;

  WmDeviceStatus connection_status;

  /* lowercase name from info file */
  gchar *name;

  uint16_t os_major;
  uint16_t os_minor;
  uint16_t build_number;
  uint16_t processor_type;
  uint32_t partner_id_1;
  uint32_t partner_id_2;
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

  IRAPIDevice *rapi_dev;
  IRAPISession *rapi_conn;

  gboolean disposed;
};

#define WM_DEVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), WM_DEVICE_TYPE, WmDevicePrivate))

/* properties */
enum
  {
    PROP_OBJECT_NAME = 1,
    PROP_DCCM_TYPE,
    PROP_CONNECTION_STATUS,
    PROP_NAME,
    PROP_OS_MAJOR,
    PROP_OS_MINOR,
    PROP_BUILD_NUMBER,
    PROP_PROCESSOR_TYPE,
    PROP_PARTNER_ID_1,
    PROP_PARTNER_ID_2,
    PROP_CLASS,
    PROP_HARDWARE,
    PROP_DEVICE_NAME,
    PROP_PASSWORD,
    PROP_KEY,
    PROP_DCCM_PID,
    PROP_IP,
    PROP_TRANSPORT,
    PROP_PORT,
    PROP_RAPI_CONN,

    LAST_PROPERTY
  };


     /* methods */

gboolean
wm_device_rapi_connect(WmDevice *self)
{
        if (!self) {
                g_warning("%s: Invalid object passed", G_STRFUNC);
                return FALSE;
        }
        WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

        if (priv->disposed) {
                g_warning("%s: Disposed object passed", G_STRFUNC);
                return FALSE;
        }

        if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
                g_warning("%s: device not fully connected", G_STRFUNC);
                return FALSE;
        }

        HRESULT hr;
	IRAPIDesktop *desktop = NULL;
	IRAPIEnumDevices *enumdev = NULL;
	IRAPIDevice *device = NULL;
	IRAPISession *session = NULL;
	RAPI_DEVICEINFO devinfo;

        if (priv->rapi_conn)
                return TRUE;

        g_debug("%s: Initialising device rapi connection", G_STRFUNC);

	if (FAILED(hr = IRAPIDesktop_Get(&desktop)))
	{
	  g_critical("%s: failed to initialise RAPI: %d: %s",
		     G_STRFUNC, hr, synce_strerror_from_hresult(hr));
	  goto error_exit;
	}

	if (FAILED(hr = IRAPIDesktop_EnumDevices(desktop, &enumdev)))
	{
	  g_critical("%s: failed to get connected devices: %d: %s",
		     G_STRFUNC, hr, synce_strerror_from_hresult(hr));
	  goto error_exit;
	}

	while (SUCCEEDED(hr = IRAPIEnumDevices_Next(enumdev, &device)))
	{
	  if (FAILED(IRAPIDevice_GetDeviceInfo(device, &devinfo)))
	  {
	    g_critical("%s: failure to get device info", G_STRFUNC);
	    goto error_exit;
	  }
	  if (strcmp(priv->name, devinfo.bstrName) == 0)
	    break;
	}

	if (FAILED(hr))
	{
	  g_critical("%s: Could not find device '%s' in RAPI: %08x: %s",
		     G_STRFUNC, priv->name, hr, synce_strerror_from_hresult(hr));
	  device = NULL;
	  goto error_exit;
	}

	IRAPIDevice_AddRef(device);
	IRAPIEnumDevices_Release(enumdev);
	enumdev = NULL;

	if (FAILED(hr = IRAPIDevice_CreateSession(device, &session)))
	{
	  g_critical("%s: Could not create a session to device '%s': %08x: %s",
		     G_STRFUNC, priv->name, hr, synce_strerror_from_hresult(hr));
	  goto error_exit;
	}

	if (FAILED(hr = IRAPISession_CeRapiInit(session)))
	{
	  g_critical("%s: Unable to initialize connection to device '%s': %08x: %s", 
		     G_STRFUNC, priv->name, hr, synce_strerror_from_hresult(hr));
	  goto error_exit;
	}

        priv->rapi_conn = session;
	priv->rapi_dev = device;

        priv->device_name = get_device_name_via_rapi(priv->rapi_conn);

        if (!(priv->device_name)) {
	  g_critical("%s: Unable to obtain device name for '%s' via RAPI", 
		     G_STRFUNC, priv->name);
	  goto error_exit;
        }

	if (desktop) IRAPIDesktop_Release(desktop);
        return TRUE;

 error_exit:
	if (session)
	{
	  IRAPISession_CeRapiUninit(session);
	  IRAPISession_Release(session);
	}

	if (device) IRAPIDevice_Release(device);
	if (enumdev) IRAPIEnumDevices_Release(enumdev);
	if (desktop) IRAPIDesktop_Release(desktop);
	return FALSE;
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

uint16_t
wm_device_get_os_version_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return 0;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return 0;
  }

  return priv->os_major;
}

uint16_t
wm_device_get_build_number_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return 0;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return 0;
  }

  return priv->build_number;
}

uint16_t
wm_device_get_processor_type_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return 0;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return 0;
  }

  return priv->processor_type;
}

uint32_t
wm_device_get_partner_id_1_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return 0;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return 0;
  }

  return priv->partner_id_1;
}

uint32_t
wm_device_get_partner_id_2_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return 0;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return 0;
  }

  return priv->partner_id_2;
}

gchar *
wm_device_get_name_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->name);
}

gchar *
wm_device_get_class_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->class);
}

gchar *
wm_device_get_hardware_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->hardware);
}

gchar *
wm_device_get_device_name_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->device_name);
}

gchar *
wm_device_get_password_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->password);
}

int
wm_device_get_key_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return 0;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return 0;
  }

  return priv->key;
}

pid_t
wm_device_get_dccm_pid_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return 0;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return 0;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return 0;
  }

  return priv->dccm_pid;
}

gchar *
wm_device_get_ip_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->ip);
}

gchar *
wm_device_get_transport_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return NULL;
  }

  return g_strdup(priv->transport);
}

gchar *
wm_device_get_port_impl(WmDevice *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
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
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }

  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return NULL;
  }

  memset(&power, 0, sizeof(SYSTEM_POWER_STATUS_EX));
  if (IRAPISession_CeGetSystemPowerStatusEx(priv->rapi_conn, &power, false) &&
      BATTERY_PERCENTAGE_UNKNOWN != power.BatteryLifePercent)
    {
      power_str = g_strdup_printf("%i%% (%s)", 
				  power.BatteryLifePercent, 
				  get_battery_flag_string(power.BatteryFlag));
    } else {
      power_str = g_strdup(_("Unknown"));
    }

  return power_str;
}
	 
gchar *
wm_device_get_store_status_impl(WmDevice *self)
{
  STORE_INFORMATION store;
  gchar* store_str = NULL;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return NULL;
  }

  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return NULL;
  }

  if (priv->connection_status != DEVICE_STATUS_CONNECTED) {
    g_warning("%s: device not fully connected", G_STRFUNC);
    return NULL;
  }

  memset(&store, 0, sizeof(store));
  if (IRAPISession_CeGetStoreInformation(priv->rapi_conn, &store) && store.dwStoreSize != 0)
    {
      store_str = g_strdup_printf(_("%i%% (%i megabytes)"), 
				  100 * store.dwFreeSize / store.dwStoreSize,
				  store.dwFreeSize >> 20);
    } else {
      store_str = g_strdup(_("Unknown"));
    }

  return store_str;
}



/* class & instance functions */

static void
wm_device_init(WmDevice *self)
{
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  priv->object_name = NULL;
  priv->dccm_type = NULL;
  priv->connection_status = DEVICE_STATUS_UNKNOWN;
  priv->os_major = 0;
  priv->os_minor = 0;
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
  priv->rapi_conn = NULL;
}


static void
wm_device_get_property (GObject    *obj,
			guint       property_id,
			GValue     *value,
			GParamSpec *pspec)
{
  WmDevice *self = WM_DEVICE (obj);
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  switch (property_id) {

  case PROP_OBJECT_NAME:
    g_value_set_string (value, priv->object_name);
    break;
  case PROP_DCCM_TYPE:
    g_value_set_string (value, priv->dccm_type);
    break;
  case PROP_CONNECTION_STATUS:
    g_value_set_uint (value, priv->connection_status);
    break;
  case PROP_NAME:
    g_value_set_string (value, priv->name);
    break;
  case PROP_OS_MAJOR:
    g_value_set_uint (value, priv->os_major);
    break;
  case PROP_OS_MINOR:
    g_value_set_uint (value, priv->os_minor);
    break;
  case PROP_BUILD_NUMBER:
    g_value_set_uint (value, priv->build_number);
    break;
  case PROP_PROCESSOR_TYPE:
    g_value_set_uint (value, priv->processor_type);
    break;
  case PROP_PARTNER_ID_1:
    g_value_set_uint (value, priv->partner_id_1);
    break;
  case PROP_PARTNER_ID_2:
    g_value_set_uint (value, priv->partner_id_2);
    break;
  case PROP_CLASS:
    g_value_set_string (value, priv->class);
    break;
  case PROP_HARDWARE:
    g_value_set_string (value, priv->hardware);
    break;
  case PROP_DEVICE_NAME:
    g_value_set_string (value, priv->device_name);
    break;
  case PROP_PASSWORD:
    g_value_set_string (value, priv->password);
    break;
  case PROP_KEY:
    g_value_set_uint (value, priv->key);
    break;
  case PROP_DCCM_PID:
    g_value_set_uint (value, priv->dccm_pid);
    break;
  case PROP_IP:
    g_value_set_string (value, priv->ip);
    break;
  case PROP_TRANSPORT:
    g_value_set_string (value, priv->transport);
    break;
  case PROP_PORT:
    g_value_set_string (value, priv->port);
    break;
  case PROP_RAPI_CONN:
    g_value_set_pointer (value, priv->rapi_conn);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
    break;
  }
}


static void
wm_device_set_property (GObject      *obj,
			guint         property_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  WmDevice *self = WM_DEVICE (obj);
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  switch (property_id) {
  case PROP_OBJECT_NAME:
    g_free (priv->object_name);
    priv->object_name = g_value_dup_string (value);
    break;
  case PROP_DCCM_TYPE:
    g_free (priv->dccm_type);
    priv->dccm_type = g_value_dup_string (value);
    break;
  case PROP_CONNECTION_STATUS:
    priv->connection_status = g_value_get_uint (value);
    break;
  case PROP_NAME:
    g_free (priv->name);
    priv->name = g_value_dup_string (value);
    break;
  case PROP_OS_MAJOR:
    priv->os_major = g_value_get_uint (value);
    break;
  case PROP_OS_MINOR:
    priv->os_minor = g_value_get_uint (value);
    break;
  case PROP_BUILD_NUMBER:
    priv->build_number = g_value_get_uint (value);
    break;
  case PROP_PROCESSOR_TYPE:
    priv->processor_type = g_value_get_uint (value);
    break;
  case PROP_PARTNER_ID_1:
    priv->partner_id_1 = g_value_get_uint (value);
    break;
  case PROP_PARTNER_ID_2:
    priv->partner_id_2 = g_value_get_uint (value);
    break;
  case PROP_CLASS:
    g_free (priv->class);
    priv->class = g_value_dup_string (value);
    break;
  case PROP_HARDWARE:
    g_free (priv->hardware);
    priv->hardware = g_value_dup_string (value);
    break;
  case PROP_DEVICE_NAME:
    g_free (priv->device_name);
    priv->device_name = g_value_dup_string (value);
    break;
  case PROP_PASSWORD:
    g_free (priv->password);
    priv->password = g_value_dup_string (value);
    break;
  case PROP_KEY:
    priv->key = g_value_get_uint (value);
    break;
  case PROP_DCCM_PID:
    priv->dccm_pid = g_value_get_uint (value);
    break;
  case PROP_IP:
    g_free (priv->ip);
    priv->ip = g_value_dup_string (value);
    break;
  case PROP_TRANSPORT:
    g_free (priv->transport);
    priv->transport = g_value_dup_string (value);
    break;
  case PROP_PORT:
    g_free (priv->port);
    priv->port = g_value_dup_string (value);
    break;
  case PROP_RAPI_CONN:
    if (priv->rapi_conn != NULL)
    {
      IRAPISession_CeRapiUninit(priv->rapi_conn);
      IRAPISession_Release(priv->rapi_conn);
    }
    priv->rapi_conn = g_value_get_pointer (value);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
    break;
  }
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

  if (priv->rapi_conn) {
    IRAPISession_CeRapiUninit(priv->rapi_conn);
    IRAPISession_Release(priv->rapi_conn);
  }

  if (G_OBJECT_CLASS (wm_device_parent_class)->dispose)
    G_OBJECT_CLASS (wm_device_parent_class)->dispose (obj);
}


static void
wm_device_finalize (GObject *obj)
{
  WmDevice *self = WM_DEVICE(obj);
  WmDevicePrivate *priv = WM_DEVICE_GET_PRIVATE (self);

  g_free(priv->object_name);
  g_free(priv->dccm_type);
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
  GParamSpec *param_spec;

  g_type_class_add_private (klass, sizeof (WmDevicePrivate));
  
  gobject_class->get_property = wm_device_get_property;
  gobject_class->set_property = wm_device_set_property;

  gobject_class->dispose = wm_device_dispose;
  gobject_class->finalize = wm_device_finalize;

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

  param_spec = g_param_spec_string ("object-name", "Object Name",
                                    "The device's dccm specific object name.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_OBJECT_NAME, param_spec);

  param_spec = g_param_spec_string ("dccm-type", "Dccm type",
                                    "Which dccm type the device is connected to.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_DCCM_TYPE, param_spec);

  param_spec = g_param_spec_uint ("connection-status", "Connection status",
                                  "The state of the connection to the device.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_CONNECTION_STATUS, param_spec);

  param_spec = g_param_spec_string ("name", "Device name",
                                    "The device's name.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_NAME, param_spec);

  param_spec = g_param_spec_uint ("os-major", "OS major version",
                                  "The device's OS major version.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_OS_MAJOR, param_spec);

  param_spec = g_param_spec_uint ("os-minor", "OS minor version",
                                  "The device's OS minor version.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_OS_MINOR, param_spec);

  param_spec = g_param_spec_uint ("build-number", "Device build number",
                                  "The device's build number.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_BUILD_NUMBER, param_spec);

  param_spec = g_param_spec_uint ("processor-type", "Device processor type",
                                  "The device's processor type.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_PROCESSOR_TYPE, param_spec);

  param_spec = g_param_spec_uint ("partner-id-1", "Partner id 1",
                                  "The device's first partner id.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_PARTNER_ID_1, param_spec);

  param_spec = g_param_spec_uint ("partner-id-2", "Partner id 2",
                                  "The device's second partner id.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_PARTNER_ID_2, param_spec);

  param_spec = g_param_spec_string ("class", "Class name",
                                    "The device's class.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_CLASS, param_spec);

  param_spec = g_param_spec_string ("hardware", "Hardware name",
                                    "The device's hardware name.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_HARDWARE, param_spec);

  param_spec = g_param_spec_string ("device-name", "Device real name",
                                    "The device's real name.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_DEVICE_NAME, param_spec);

  param_spec = g_param_spec_string ("password", "Device password",
                                    "The device's password.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_PASSWORD, param_spec);

  param_spec = g_param_spec_uint ("key", "Password key",
                                  "The device's password key.",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_KEY, param_spec);

  param_spec = g_param_spec_uint ("dccm-pid", "DCCM PID",
                                  "The device's connected DCCM PID .",
                                  0, G_MAXUINT32, 0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_DCCM_PID, param_spec);

  param_spec = g_param_spec_string ("ip", "Device ip",
                                    "The device's IP address.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_IP, param_spec);

  param_spec = g_param_spec_string ("transport", "Device transport",
                                    "The device's transport DCCM type.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_TRANSPORT, param_spec);

  param_spec = g_param_spec_string ("port", "Device port",
                                    "The device's ip port.",
                                    NULL,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_PORT, param_spec);

  param_spec = g_param_spec_pointer ("rapi-conn", "Rapi connection",
                                     "The device's rapi connection",
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_NICK |
                                     G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_RAPI_CONN, param_spec);
}
