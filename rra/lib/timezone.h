/* $Id$ */
#ifndef __timezone_h__
#define __timezone_h__

#include <synce.h>

typedef struct _RRA_Timezone
{
  int32_t Bias;                       /* 00 */
  WCHAR StandardName[32];             /* 04 */
  uint16_t unknown0;                  /* 44 */
  uint16_t StandardMonthOfYear;       /* 46 */
  uint16_t unknown1;                  /* 48 */
  uint16_t StandardInstance;          /* 4a */
  uint16_t StandardStartHour;         /* 4c */
  uint8_t unknown2[6];                /* 4e */
  int32_t StandardBias;               /* 54 */
  WCHAR DaylightName[32];             /* 58 */
  uint16_t unknown3;                  /* 98 */
  uint16_t DaylightMonthOfYear;       /* 9a */
  uint16_t unknown4;                  /* 9c */
  uint16_t DaylightInstance;          /* 9e */
  uint16_t DaylightStartHour;         /* a0 */
  uint8_t unknown5[6];                /* a2 */
  int32_t DaylightBias;               /* b0 */
} RRA_Timezone;

/** Get time zone information from connected device */
bool rra_timezone_get(RRA_Timezone* timezone);

#define RRA_TIMEZONE_INVALID_TIME   ((time_t)0xffffffff)

/** Convert a time in UTC to this timezone */
time_t rra_timezone_convert_from_utc(RRA_Timezone* tzi, time_t unix_time);

/** Convert a time in this timezone to UTC */
time_t rra_timezone_convert_to_utc  (RRA_Timezone* tzi, time_t unix_time);

/** Create a timezone ID for use in vCalendar objects */
void rra_timezone_create_id(RRA_Timezone* timezone, char** id);
#define rra_timezone_free_id(id)  if (id) free(id)

struct _Generator;

/** Write this timezone as a vTimezone object */
bool rra_timezone_generate_vtimezone(struct _Generator* generator, RRA_Timezone* tzi);

#endif
