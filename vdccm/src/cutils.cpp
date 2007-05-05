#include "cutils.h"
#include "utils.h"
#include "windowscedevicebase.h"

void
_vdccm_acquire_root_privileges ()
{
  Utils::acquireRootPrivileg ();
}

void
_vdccm_drop_root_privileges ()
{
  Utils::dropRootPrivileg ();
}

void
_vdccm_ce_device_base_disconnect (gpointer ce_device_base)
{
  WindowsCEDeviceBase *device = (WindowsCEDeviceBase *) ce_device_base;

  device->disconnect ();
}

gchar *
_vdccm_ce_device_base_get_name (gpointer ce_device_base)
{
  WindowsCEDeviceBase *device = (WindowsCEDeviceBase *) ce_device_base;

  return g_strdup (device->getDeviceName ().c_str ());
}

