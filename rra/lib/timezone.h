/* $Id$ */

#include <stdint.h>
#include <synce.h>

#define P __attribute__((packed))

typedef struct 
{
  int32_t Bias;                       /* 00 */
  WCHAR Name[32];                     /* 04 */
  uint16_t unknown0;                  /* 44 */
  uint16_t StandardMonthOfYear;       /* 46 */
  uint16_t unknown1;                  /* 48 */
  uint16_t StandardInstance;          /* 4a */
  uint16_t StandardStartHour;         /* 4c */
  uint8_t unknown2[6];                /* 4e */
  int32_t StandardBias;               /* 54 */
  WCHAR Description[32];              /* 58 */
  uint16_t unknown3;                  /* 98 */
  uint16_t DaylightMonthOfYear;       /* 9a */
  uint16_t unknown4;                  /* 9c */
  uint16_t DaylightInstance;          /* 9e */
  uint16_t DaylightStartHour;         /* a0 */
  uint8_t unknown5[6];                /* a2 */
  int32_t DaylightBias;               /* b0 */
} TimeZoneInformation;

#undef P

bool rra_get_time_zone_information(TimeZoneInformation* tzi);

