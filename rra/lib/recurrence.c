/* $Id$ */
/*

   WARNING! This code is still under heavy development!

*/
#define _GNU_SOURCE 1
#include "recurrence.h"
#include "recurrence_pattern.h"
#include "generator.h"
#include "parser.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <string.h>

#define MINUTES_PER_DAY   (60*24)
#define SECONDS_PER_DAY   (60*MINUTES_PER_DAY)

static bool recurrence_generate_exceptions(
    Generator* g,
    RRA_Exceptions* exceptions)
{
  /* TODO: implement */
  return true;
}

static void recurrence_append_until_or_count(
    char* buffer, 
    size_t size, 
    RRA_RecurrencePattern* pattern)
{
  switch (pattern->flags & RecurrenceEndMask)
  {
    case RecurrenceEndsOnDate:
      /* TODO: use UNTIL and not COUNT here */
      /* fall through for now */
    case RecurrenceEndsAfterXOccurrences:
      snprintf(buffer, size, ";COUNT=%i", pattern->occurrences);
      break;
  }
}

typedef struct _DaysOfWeekMaskName
{
  RRA_DaysOfWeek mask;
  const char* name;
} DaysOfWeekMaskName;

static DaysOfWeekMaskName masks_and_names[] =
{
  {olSunday,    "SU"},
  {olMonday,    "MO"},
  {olTuesday,   "TU"},
  {olWednesday, "WE"},
  {olThursday,  "TH"},
  {olFriday,    "FR"},
  {olSaturday,  "SA"},
};

static void recurrence_append_byday(
    char* buffer, 
    size_t size,
    RRA_RecurrencePattern* pattern)
{
  int i;
  bool first = true;
  
 
  for (i = 0; i < 7; i++)
  {
    if (pattern->days_of_week_mask & masks_and_names[i].mask)
    {
      if (first)
      {
        snprintf(buffer, size, ";BYDAY=");
        first = false;
      }
      else
        snprintf(buffer, size, ",");

      size -= strlen(buffer);
      buffer += strlen(buffer);

#if 0
      switch (pattern->instance)
      {
        case 1:
        case 2:
        case 3:
        case 4:
          snprintf(buffer, size, "%i%s", pattern->instance, masks_and_names[i].name);
          break;
        case 5:
          snprintf(buffer, size, "-1%s", masks_and_names[i].name);
          break;
        case 0:
#endif
          snprintf(buffer, size, "%s", masks_and_names[i].name);
#if 0
          break;
        default:
          synce_error("Invalid instance: %08x", pattern->instance);
          break;
      }
#endif

      size -= strlen(buffer);
      buffer += strlen(buffer);
    }
  }
}

static bool recurrence_generate_daily_rrule(
    Generator* g,
    RRA_RecurrencePattern* pattern)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "FREQ=DAILY;INTERVAL=%i",
      pattern->interval / MINUTES_PER_DAY);

  recurrence_append_until_or_count(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), pattern);

  return generator_add_simple(g, "RRULE", buffer);
}

static bool recurrence_generate_weekly_rrule(
    Generator* g,
    RRA_RecurrencePattern* pattern)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "FREQ=WEEKLY;INTERVAL=%i",
      pattern->interval);

  recurrence_append_until_or_count(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), pattern);
  recurrence_append_byday         (buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), pattern);

  return generator_add_simple(g, "RRULE", buffer);
}

static bool recurrence_generate_monthly_rrule(
    Generator* g,
    RRA_RecurrencePattern* pattern)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "FREQ=MONTHLY;INTERVAL=%i;BYMONTHDAY=%i",
      pattern->interval, pattern->day_of_month);

  recurrence_append_until_or_count(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), pattern);

  return generator_add_simple(g, "RRULE", buffer);
}

static bool recurrence_generate_monthnth_rrule(
    Generator* g,
    RRA_RecurrencePattern* pattern)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "FREQ=MONTHLY;INTERVAL=%i;BYSETPOS=%i",
      pattern->interval,
      pattern->instance);

  recurrence_append_until_or_count(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), pattern);
  recurrence_append_byday         (buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), pattern);

  return generator_add_simple(g, "RRULE", buffer);
}

bool recurrence_generate_rrule(
    Generator* g, 
    CEPROPVAL* propval)
{
  bool success = false;
  RRA_RecurrencePattern* pattern = NULL;

  if ((propval->propid & 0xffff) != CEVT_BLOB)
  {
    synce_error("CEPROPVAL is not a BLOB");
    goto exit;
  }
  
  pattern = rra_recurrence_pattern_from_buffer(
    propval->val.blob.lpb, propval->val.blob.dwCount);

  if (!pattern)
  {
    synce_error("Failed to decode recurrence pattern");
    goto exit;
  }

  switch (pattern->recurrence_type)
  {
    case olRecursDaily:
      success = recurrence_generate_daily_rrule(g, pattern);
      break;
    case olRecursWeekly:
      success = recurrence_generate_weekly_rrule(g, pattern);
      break;
    case olRecursMonthly:
      success = recurrence_generate_monthly_rrule(g, pattern);
      break;
    case olRecursMonthNth:
      success = recurrence_generate_monthnth_rrule(g, pattern);
      break;
    default:
      goto exit;
  }

  if (!success)
  {
    synce_error("Failed to generate RRULE for recurrence type %i",
      pattern->recurrence_type);
    goto exit;
  }

  success = recurrence_generate_exceptions(g, pattern->exceptions);

exit:
  rra_recurrence_pattern_destroy(pattern);
  return success;
}

bool recurrence_parse_rrule(
    struct _Parser* p, 
    mdir_line* dtstart,
    mdir_line* dtend,
    mdir_line* rrule, 
    RRA_MdirLineVector* exdates)
{
  return false;
}

