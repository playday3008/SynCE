/* $Id$ */
#define _BSD_SOURCE 1
#include "timezone.h"
#include "generator.h"
#include <rapi.h>
#include <synce.h>
#include <synce_log.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define REGISTRY_KEY_NAME     "Time"
#define REGISTRY_VALUE_NAME   "TimeZoneInformation"

#define LETOH16(x)  x = letoh16(x)
#define LETOH32(x)  x = letoh32(x)

static const uint8_t empty[6] = {0,0,0,0,0,0};

/*
   See this page for info:

   http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcemain4/html/_wcepb_time_zone_registry_settings.asp
 */
static const uint8_t default_timezone[172]={
  0xe0,0x01,0x00,0x00,0x4d,0x00,0x54,0x00,
  0x2d,0x00,0x38,0x00,0x20,0x00,0x50,0x00,
  0x61,0x00,0x63,0x00,0x69,0x00,0x66,0x00,
  0x69,0x00,0x63,0x00,0x20,0x00,0x55,0x00,
  0x53,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x00,
  0x00,0x00,0x05,0x00,0x02,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x54,0x00,0x2d,0x00,0x38,0x00,0x20,0x00,
  0x50,0x00,0x61,0x00,0x63,0x00,0x69,0x00,
  0x66,0x00,0x69,0x00,0x63,0x00,0x20,0x00,
  0x55,0x00,0x53,0x00,0x22,0x00,0x50,0x00,
  0x61,0x00,0x63,0x00,0x69,0x00,0x66,0x00,
  0x69,0x00,0x63,0x00,0x20,0x00,0x53,0x00,
  0x74,0x00,0x61,0x00,0x6e,0x00,0x64,0x00,
  0x61,0x00,0x72,0x00,0x64,0x00,0x00,0x00,
  0x00,0x00,0x04,0x00,0x00,0x00,0x01,0x00,
  0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0xc4,0xff,0xff,0xff};

bool rra_timezone_get(RRA_Timezone* tzi, RapiConnection *connection)/*{{{*/
{
  bool success = false;
  LONG error;
  HKEY key = 0;
  WCHAR* wide_value_name = NULL;
  DWORD size = sizeof(RRA_Timezone);
  bool registry_success = false;
  
  assert(sizeof(RRA_Timezone) == 172);

  if (!tzi)
  {
    synce_error("Invalid parameter");
    goto exit;
  }

  rapi_connection_select(connection);
  if (rapi_reg_open_key(HKEY_LOCAL_MACHINE, REGISTRY_KEY_NAME, &key))
  {
    wide_value_name = wstr_from_ascii(REGISTRY_VALUE_NAME);

    error = CeRegQueryValueEx(key, wide_value_name, NULL, NULL, (void*)tzi, &size);
    if (ERROR_SUCCESS == error)
    {
      if (sizeof(RRA_Timezone) == size)
        registry_success = true;
      else
      {
        synce_warning("Expected value size %i but got %i", sizeof(RRA_Timezone), size);
        goto exit;
      }
    }
    else
      synce_warning("Failed to get registry value: %s", synce_strerror(error));
  }
  else
    synce_warning("Failed to open registry key 'HKEY_LOCAL_MACHINE\\%s'", 
        REGISTRY_KEY_NAME);

  if (!registry_success)
  {
    synce_warning("Fallback to default time zone enabled");
    memcpy(tzi, default_timezone, sizeof(RRA_Timezone));
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
}/*}}}*/

/* XXX: improve ID generation */
void rra_timezone_create_id(RRA_Timezone* tzi, char** id)/*{{{*/
{
  char* name = wstr_to_ascii(tzi->StandardName);
  char* p;
  char buffer[128];

  if (!id)
    return;

  for (p = name; *p != '\0'; p++)
  {
    if (!isalnum(*p))
      *p = '_';
  }

  snprintf(buffer, sizeof(buffer), "/synce.sourceforge.net/SynCE/%s", name);
  
  *id = strdup(buffer);

  wstr_free_string(name);
}/*}}}*/

static const unsigned days_of_month[12] =/*{{{*/
{
  31,  /* jan */
  28,
  31,  /* mar */
  30,
  31,  /* may */
  30,
  31,  /* jul */
  31,
  30,  /* sep */
  31,
  30,  /* nov */
  31
};/*}}}*/

/* http://www.visi.com/~pmk/new-dayofweek.html */
static const unsigned month_skew[12] =/*{{{*/
{
  0,  /* jan */
  3,
  3,  /* mar */
  6,
  1,  /* may */
  4,
  6,  /* jul */
  2,
  5,  /* sep */
  0,
  3,  /* nov */
  5
};/*}}}*/

static unsigned day_from_month_and_week(unsigned month, unsigned week)/*{{{*/
{
  /* don't ask... */
  unsigned first_sunday = (8 - ((4 + month_skew[month-1]) % 7)) % 7;
  unsigned result;

  if (week < 1 || week > 5)
  {
    synce_error("Invalid week number %i", week);
    return 0;
  }

  for (;;)
  {
    result = first_sunday + (week - 1) * 7;
    if (result > days_of_month[month-1])
      week--;
    else
      break;
  }

  return result;
}/*}}}*/

static bool using_daylight_saving(RRA_Timezone* tzi, struct tm* time_struct)
{
  int month = time_struct->tm_mon + 1;

  if (tzi->StandardMonthOfYear > tzi->DaylightMonthOfYear)
  {
    if (month < tzi->DaylightMonthOfYear || month > tzi->StandardMonthOfYear)
      return false;
    else if (month > tzi->DaylightMonthOfYear && month < tzi->StandardMonthOfYear)
      return true;
    
    if (month == tzi->StandardMonthOfYear)
    {
      int day = day_from_month_and_week(tzi->StandardMonthOfYear, tzi->StandardInstance);
      if (time_struct->tm_mday < day)
        return !false;
      else if (time_struct->tm_mday > day)
        return !true;
      else /* Standard start day */
      {
        if (time_struct->tm_hour >= tzi->StandardStartHour)
          return !true;
        else
          return !false;
      }
    }
    
    if (month == tzi->DaylightMonthOfYear)
    {
      int day = day_from_month_and_week(tzi->DaylightMonthOfYear, tzi->DaylightInstance);
      if (time_struct->tm_mday < day)
        return false;
      else if (time_struct->tm_mday > day)
        return true;
      else /* daylight saving start day */
      {
        if (time_struct->tm_hour >= tzi->DaylightStartHour)
          return true;
        else
          return false;
      }
    }

    synce_error("Month is %i", month);
    assert(0);  /* should not be reached */
  }
  else
  {
    synce_error("Cannot handle this time zone");
  }
    
  return false;
}

time_t rra_timezone_convert_from_utc(RRA_Timezone* tzi, time_t unix_time)
{
  time_t result = RRA_TIMEZONE_INVALID_TIME;
  struct tm time_struct;
  
  if (tzi && gmtime_r(&unix_time, &time_struct))
  {
    unix_time -= tzi->Bias * 60;

    if (using_daylight_saving(tzi, &time_struct))
      unix_time -= tzi->DaylightBias * 60;
    else
      unix_time -= tzi->StandardBias * 60;

    result = unix_time;
  }

  return result;
}

time_t rra_timezone_convert_to_utc(RRA_Timezone* tzi, time_t unix_time)
{
  time_t result = RRA_TIMEZONE_INVALID_TIME;
  struct tm time_struct;
  
  if (tzi && gmtime_r(&unix_time, &time_struct))
  {
    unix_time += tzi->Bias * 60;
    
    if (using_daylight_saving(tzi, &time_struct))
      unix_time += tzi->DaylightBias * 60;
    else
      unix_time += tzi->StandardBias * 60;

    result = unix_time;
  }

  return result;
}

static void offset_string(char* buffer, size_t size, int default_bias, int extra_bias)/*{{{*/
{
  int bias = default_bias + extra_bias;
  snprintf(buffer, size, "%+03i%02i", -bias / 60, abs(bias) % 60);
}/*}}}*/

static bool time_string(char* buffer, size_t size, /*{{{*/
    unsigned month, unsigned week, unsigned hour)
{
  if (week <= 5 || month <= 12)
  {
    unsigned day = day_from_month_and_week(month, week);

    if (!day)
    {
      synce_error("Unknown month/week combination: week=%i, month=%i - report to SynCE developers!",
          week, month);
    }
    
    snprintf(buffer, size, "1970%02i%02iT%02i0000",
        month, day, hour);
    return true;
  }
  else
  {
    synce_error("Bad time zone information: week=%i, month=%i", week, month);
    return false;
  }
}/*}}}*/

static void add_rrule(Generator* generator, unsigned instance, unsigned month)/*{{{*/
{
  char rrule[128];
  
  snprintf(rrule, sizeof(rrule), 
      "FREQ=YEARLY;INTERVAL=1;BYDAY=%iSU;BYMONTH=%i",
      (5 == instance) ? (unsigned)-1 : instance, month);
  
  generator_add_simple(generator, "RRULE", rrule);
}     /*}}}*/

static void add_tzid(Generator* generator, RRA_Timezone* tzi)/*{{{*/
{
  char* id = NULL;
  rra_timezone_create_id(tzi, &id);
  generator_add_simple(generator, "TZID", id);
  rra_timezone_free_id(id);
}/*}}}*/

bool rra_timezone_generate_vtimezone(Generator* generator, RRA_Timezone* tzi)/*{{{*/
{
  bool success = false;
  char standard_offset[10];
  char daylight_offset[10];
  char dtstart[20];

  /* UTC = local time + bias   -->   UTC - bias = local time */

  offset_string(standard_offset, sizeof(standard_offset), tzi->Bias, tzi->StandardBias);
  offset_string(daylight_offset, sizeof(daylight_offset), tzi->Bias, tzi->DaylightBias);

  generator_add_simple(generator, "BEGIN", "VTIMEZONE");
  add_tzid(generator, tzi);

  /* 
     Daylight time 
   */

  generator_add_simple(generator, "BEGIN", "DAYLIGHT");
  /* required: dtstart / tzoffsetto / tzoffsetfrom */
  generator_add_simple(generator, "TZOFFSETFROM", standard_offset);
  generator_add_simple(generator, "TZOFFSETTO", daylight_offset);

  if (!time_string(dtstart, sizeof(dtstart), tzi->DaylightMonthOfYear, tzi->DaylightInstance, tzi->DaylightStartHour))
    goto exit;

  generator_add_simple(generator, "DTSTART", dtstart);
  add_rrule(generator, tzi->DaylightInstance, tzi->DaylightMonthOfYear);
  generator_add_simple(generator, "END", "DAYLIGHT");

  /* 
     Standard time 
   */

  generator_add_simple(generator, "BEGIN", "STANDARD");
  /* required: dtstart / tzoffsetto / tzoffsetfrom */
  generator_add_simple(generator, "TZOFFSETFROM", daylight_offset);
  generator_add_simple(generator, "TZOFFSETTO", standard_offset);

  if (!time_string(dtstart, sizeof(dtstart), tzi->StandardMonthOfYear, tzi->StandardInstance, tzi->StandardStartHour))
    goto exit;
  
  generator_add_simple(generator, "DTSTART", dtstart);
  add_rrule(generator, tzi->StandardInstance, tzi->StandardMonthOfYear);
  generator_add_simple(generator, "END", "STANDARD");

  generator_add_simple(generator, "END", "VTIMEZONE");

  success = true;

exit:
  return success;
}/*}}}*/
