/* $Id$ */
#include "timezone.h"
#include <rapi.h>
#include <synce.h>
#include <synce_log.h>
#include <assert.h>
#include <string.h>

#define REGISTRY_KEY_NAME     "Time"
#define REGISTRY_VALUE_NAME   "TimeZoneInformation"

#define LETOH16(x)  x = letoh16(x)
#define LETOH32(x)  x = letoh32(x)

static const uint8_t empty[6] = {0,0,0,0,0,0};

bool rra_get_time_zone_information(TimeZoneInformation* tzi)
{
  bool success = false;
  LONG error;
  HKEY key = 0;
  WCHAR* wide_value_name = NULL;
  DWORD size = sizeof(TimeZoneInformation);
  
  assert(sizeof(TimeZoneInformation) == 172);
  /*assert(6 == sizeof(tzi->unknown2));
  assert(6 == sizeof(tzi->unknown5));*/

  if (!rapi_reg_open_key(HKEY_LOCAL_MACHINE, REGISTRY_KEY_NAME, &key))
  {
    synce_error("Failed to open registry key 'HKEY_LOCAL_MACHINE\\%s'", 
        REGISTRY_KEY_NAME);
    goto exit;
  }
  
  wide_value_name = wstr_from_ascii(REGISTRY_VALUE_NAME);

  error = CeRegQueryValueEx(key, wide_value_name, NULL, NULL, (void*)tzi, &size);
  if (ERROR_SUCCESS != error)
  {
    synce_error("Failed to get registry value: %s", synce_strerror(error));
    goto exit;
  }

  if (sizeof(TimeZoneInformation) != size)
  {
    synce_error("Expected value size %i but got %i", sizeof(TimeZoneInformation), size);
    goto exit;
  }

  LETOH32(tzi->Bias);
  
  LETOH16(tzi->StandardMonthOfYear);
  LETOH16(tzi->StandardInstance);
  LETOH16(tzi->StandardStartHour);
  LETOH32(tzi->StandardBias);
  
  LETOH16(tzi->DaylightMonthOfYear);
  LETOH16(tzi->DaylightInstance);
  LETOH16(tzi->DaylightStartHour);
  LETOH32(tzi->DaylightBias);

  if (tzi->unknown0 || tzi->unknown1 || tzi->unknown3 || tzi->unknown4 ||
      0 != memcmp(tzi->unknown2, empty, sizeof(tzi->unknown2)) || 
      0 != memcmp(tzi->unknown5, empty, sizeof(tzi->unknown5)))
  {
    synce_warning("Unknown value used in time zone information");
  }

  success = true;
  
exit:
  if (key)
    CeRegCloseKey(key);
  wstr_free_string(wide_value_name);
  return success;
}

