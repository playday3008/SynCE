/* 
 * Copyright (c) 2005 Andrei Yurkevich <urruru@ru.ru>
 * Copyright (c) 2002 David Eriksson <twogood@users.sourceforge.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
 
#include <rapi.h>
#include "misc.h"
#include "info.h"

extern gboolean			hal_verbose;
static gboolean			rapi_initialized = FALSE;
static RapiConnection	*connection = NULL;

#define PROCESSOR_ARCHITECTURE_COUNT 8

#define INIT_RAPI_OR_RETURN(xx)  if (!rapi_initialized) { if (!initialize_rapi ()) return (xx); } 

static const char* architecture[] = {
  "Intel",
  "MIPS",
  "Alpha",
  "PPC",
  "SHX",
  "ARM",
  "IA64",
  "ALPHA64"
};

static gboolean
initialize_rapi ()
{
  gchar *path = NULL;
  HRESULT hr;
  
  connection = rapi_connection_from_path (path);
  rapi_connection_select (connection);
  hr = CeRapiInit ();
  
  if (FAILED (hr)) {
	  SHC_ERROR ("unable to initialize RAPI: %s", synce_strerror (hr));
	  return (FALSE);
  }
  
  rapi_initialized = TRUE;
  return (TRUE);
}

gchar *
get_device_os_version ()
{
	INIT_RAPI_OR_RETURN (NULL);
	  
	gchar *version_str = NULL;
  
	CEOSVERSIONINFO *version = g_new0 (CEOSVERSIONINFO, 1);
	version->dwOSVersionInfoSize = sizeof (CEOSVERSIONINFO);
  
	if (CeGetVersionEx (version))
		version_str = g_strdup_printf ("%i.%i.%i",
	                                   version->dwMajorVersion,
									   version->dwMinorVersion,
									   version->dwBuildNumber);
	else
		SHC_ERROR ("failed to get version information: %s", synce_strerror (CeGetLastError ()));
	  
	g_free (version);
  
	return version_str;
}

gchar *
get_device_arch ()
{
	INIT_RAPI_OR_RETURN (NULL);
		
	gchar *arch_str = NULL;
	
	SYSTEM_INFO *system = g_new0 (SYSTEM_INFO, 1);
	
	CeGetSystemInfo (system);
	arch_str = g_strdup_printf ((system->wProcessorArchitecture < PROCESSOR_ARCHITECTURE_COUNT) ?
                                architecture[system->wProcessorArchitecture] : "Unknown");

	g_free (system);
	return (arch_str);
}

#define BATT_IS_PRESENT(xx)  		((gboolean) !(xx->BatteryFlag & 0x80))
#define BATT_IS_RECHARGEABLE(xx)	(BATT_IS_PRESENT(xx))
#define BATT_IS_CHARGING(xx)		((gboolean) (xx->BatteryFlag & 0x08) && (BATT_LEVEL(xx) < 100))
#define BATT_IS_DISCHARGING(xx)		((gboolean) (xx->ACLineStatus == 0) && (!(xx->BatteryFlag & 0x08)))
#define BATT_LEVEL(xx)				((gint) (xx->BatteryLifePercent))
#define BATT_REMAINING(xx)			((gint) (xx->BatteryLifeTime))

BatteryStatus *
get_device_battery_status ()
{
	INIT_RAPI_OR_RETURN (FALSE);
	
	SYSTEM_POWER_STATUS_EX *power_status = g_new0 (SYSTEM_POWER_STATUS_EX, 1);
	
	if (CeGetSystemPowerStatusEx (power_status, TRUE)) {
		SHC_INFO ("ACLineStatus=0x%08x  BatteryFlag=0x%08x",
		          power_status->ACLineStatus,
				  power_status->BatteryFlag);
	
		BatteryStatus *status = g_new0 (BatteryStatus, 1);
		
		status->present = BATT_IS_PRESENT (power_status);
		status->rechargeable = BATT_IS_RECHARGEABLE (power_status);
		status->is_charging = BATT_IS_CHARGING (power_status);
		status->is_discharging = BATT_IS_DISCHARGING (power_status);
		status->level = BATT_LEVEL (power_status);
		status->remaining_time = BATT_REMAINING (power_status);
		
		g_free (power_status);
		return (status);
	}
	else {
		g_free (power_status);
		return (NULL);
	}
}
