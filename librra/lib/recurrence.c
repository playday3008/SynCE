/* $Id$ */
/*

   WARNING! This code is still under heavy development!

*/
#define _GNU_SOURCE 1
#include "appointment_ids.h"
#include "recurrence.h"
#include "recurrence_pattern.h"
#include "generator.h"
#include "parser.h"
#include "strv.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <string.h>

#define VERBOSE 1

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

#define MINUTES_PER_DAY   (60*24)
#define SECONDS_PER_DAY   (60*MINUTES_PER_DAY)

typedef struct
{
  char* byday;
  int   bymonthday;
  int   bysetpos;
  int   count;
  char* freq;
  int   interval;
  char* until;
} RRule;

static bool recurrence_generate_exceptions(
    Generator* g,
    RRA_Exceptions* exceptions)
{
  int i;

  for (i = 0; i < rra_exceptions_count(exceptions); i++)
  {
    RRA_Exception* e = rra_exceptions_item(exceptions, i);

    if (e)
    {
      struct tm date = rra_minutes_to_struct(e->date);
      char buffer[64];

      strftime(buffer, sizeof(buffer), "%Y%m%d", &date);
      generator_add_with_type(g, "EXDATE", "DATE", buffer);
    }
  }
  
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
      {
        struct tm date = rra_minutes_to_struct(pattern->pattern_end_date + pattern->start_minute);
        strftime(buffer, size, ";UNTIL=%Y%m%dT%H%M%SZ", &date);
        synce_trace("UNTIL: %s", buffer);
      }
      break;

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

  return generator_add_simple_unescaped(g, "RRULE", buffer);
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

  return generator_add_simple_unescaped(g, "RRULE", buffer);
}

static bool recurrence_generate_monthly_rrule(
    Generator* g,
    RRA_RecurrencePattern* pattern)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "FREQ=MONTHLY;INTERVAL=%i;BYMONTHDAY=%i",
      pattern->interval, pattern->day_of_month);

  recurrence_append_until_or_count(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), pattern);

  return generator_add_simple_unescaped(g, "RRULE", buffer);
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

  return generator_add_simple_unescaped(g, "RRULE", buffer);
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

static void replace_string_with_copy(char** str, const char* value)
{
  if (*str)
    free(*str);
  *str = strdup(value);
}

static bool recurrence_initialize_rrule(const char* str, RRule* rrule)
{
  int i;
  char** strv = strsplit(str, ';');

  rrule->interval = 1;

  for (i = 0; strv[i]; i++)
  {
    char** pair = strsplit(strv[i], '=');
    if (!pair[0] || !pair[1])
    {
      synce_warning("Invalid rrule part: '%s'", strv[i]);
      continue;
    }

    synce_trace("RRULE part: key=%s, value=%s", 
        pair[0], pair[1]);

    if (STR_EQUAL(pair[0], "BYDAY"))
      replace_string_with_copy(&rrule->byday, pair[1]);
    else if (STR_EQUAL(pair[0], "BYMONTHDAY"))
      rrule->bymonthday = atoi(pair[1]);
    else if (STR_EQUAL(pair[0], "BYSETPOS"))
      rrule->bysetpos = atoi(pair[1]);
    else if (STR_EQUAL(pair[0], "COUNT"))
      rrule->count = atoi(pair[1]);
    else if (STR_EQUAL(pair[0], "FREQ"))
      replace_string_with_copy(&rrule->freq, pair[1]);
    else if (STR_EQUAL(pair[0], "INTERVAL"))
      rrule->interval = atoi(pair[1]);
    else if (STR_EQUAL(pair[0], "UNTIL"))
      replace_string_with_copy(&rrule->until, pair[1]);
    else
      synce_warning("Unhandled part of RRULE: '%s'", strv[i]);

    strv_free(pair);
  }
  
  strv_free(strv);
  return true;
}

static void recurrence_set_days_of_week_mask(
    RRA_RecurrencePattern* pattern, 
    RRule* rrule)
{
  int i, j;
  char** days = strsplit(rrule->byday, ',');

  if (days)
  {
    for (i = 0; i < 7; i ++)
      for (j = 0; days[j]; j++)
      {
        if (STR_EQUAL(masks_and_names[i].name, days[j]))
          pattern->days_of_week_mask |= masks_and_names[i].mask;
      }

    strv_free(days);
  }

  if (pattern->days_of_week_mask == 0)
  {
    /* get day from start date */
    struct tm start_date = rra_minutes_to_struct(pattern->pattern_start_date);
    synce_warning("BYDAY is missing or empty, assumimg BYDAY=%s", masks_and_names[start_date.tm_wday].name);
    pattern->days_of_week_mask = masks_and_names[start_date.tm_wday].mask;
  }
}

static bool recurrence_set_dates(
    RRA_RecurrencePattern* pattern, 
    mdir_line* mdir_dtstart,
    mdir_line* mdir_dtend)
{
  bool success = false;
  struct tm start_struct;
  struct tm tmp_struct;
  time_t start;
  time_t end;
  int32_t minutes = 0;
  ParserTimeFormat format = parser_get_time_format(mdir_dtstart);
  bool start_is_utc = false;
  bool end_is_utc = false;

  /* XXX timezone handling? */

  if (!parser_datetime_to_struct(mdir_dtstart->values[0], &start_struct, NULL))
    goto exit;
  if (!parser_datetime_to_unix_time(mdir_dtstart->values[0], &start, &start_is_utc))
    goto exit;
  if (!parser_datetime_to_unix_time(mdir_dtend->values[0], &end, &end_is_utc))
    goto exit;

#if VERBOSE
  synce_trace("start is %s", asctime(&start_struct));
  synce_trace("start is utc: %i, end is utc: %i", start_is_utc, end_is_utc);
#endif

  tmp_struct = start_struct;
  tmp_struct.tm_sec = 0;
  tmp_struct.tm_min = 0;
  tmp_struct.tm_hour = 0;
  pattern->pattern_start_date = rra_minutes_from_struct(&tmp_struct);

  pattern->start_minute = start_struct.tm_hour * 60 + start_struct.tm_min;

  switch (format)
  {
    case PARSER_TIME_FORMAT_UNKNOWN:
      goto exit;

    case PARSER_TIME_FORMAT_DATE_AND_TIME:
      minutes = (end - start) / 60;
      break;

    case PARSER_TIME_FORMAT_ONLY_DATE:
      minutes = (end - start - SECONDS_PER_DAY) / 60 + 1;
      break;
  }
  
  pattern->end_minute = pattern->start_minute + minutes;

  synce_trace("pattern->start_minute/60: %u", pattern->start_minute / 60);
  synce_trace("pattern->end_minute  /60: %u", pattern->end_minute   / 60);

  success = true;

exit:
  return success;
}

static bool recurrence_set_exceptions(RRA_RecurrencePattern* pattern, RRA_MdirLineVector* exdates)
{
  unsigned i;
  bool success = false;
  RRA_Exceptions* exceptions = pattern->exceptions;

  /* TODO: support more than one exception per EXDATE line */

  rra_exceptions_make_reservation(exceptions, exdates->used);

  for (i = 0; i < exdates->used; i++)
  {
    RRA_Exception* exception = rra_exceptions_item(exceptions, i);
    struct tm exdate;

    if (!parser_datetime_to_struct(exdates->items[i]->values[0], &exdate, NULL))
      goto exit;
    
    exception->deleted = true;

    /* Only date */
    exception->date = rra_minutes_from_struct(&exdate);

    /* Date and time */
    exdate.tm_min = pattern->start_minute;
    exception->original_time = rra_minutes_from_struct(&exdate);
    synce_trace("exception->original_time: %s", asctime(&exdate));
  }

  success = true;

exit:
  return success;
}

bool recurrence_parse_rrule(
    struct _Parser* p, 
    mdir_line* mdir_dtstart,
    mdir_line* mdir_dtend,
    mdir_line* mdir_rrule, 
    RRA_MdirLineVector* exdates)
{
  bool success = false;
  RRule rrule;
  RRA_RecurrencePattern* pattern = rra_recurrence_pattern_new();

  if (!recurrence_set_dates(pattern, mdir_dtstart, mdir_dtend))
  {
    synce_error("Failed to set dates");
    goto exit;
  }

  memset(&rrule, 0, sizeof(RRule));
  if (!recurrence_initialize_rrule(mdir_rrule->values[0], &rrule))
  {
    synce_error("Failed to parse RRULE '%s'", mdir_rrule->values[0]);
    goto exit;
  }

  if (!rrule.freq)
  {
    synce_error("No FREQ part in RRULE '%s'", mdir_rrule->values[0]);
    goto exit;
  }

  if (STR_EQUAL(rrule.freq, "DAILY"))
  {
    pattern->recurrence_type = olRecursDaily;
    /* Convert to Daily with 24*60 times the interval (days->minutes) */
    synce_trace("Converting Interval to minutes");
    rrule.interval *= MINUTES_PER_DAY;
  }
  else if (STR_EQUAL(rrule.freq, "WEEKLY"))
  {
    pattern->recurrence_type = olRecursWeekly;
    recurrence_set_days_of_week_mask(pattern, &rrule);
  }
  else if (STR_EQUAL(rrule.freq, "MONTHLY"))
  {
    if (rrule.bymonthday)
    {
      pattern->recurrence_type = olRecursMonthly;
      pattern->day_of_month = rrule.bymonthday;
    }
    else if (rrule.bysetpos)
    {
      pattern->recurrence_type = olRecursMonthNth;
      pattern->instance = rrule.bysetpos;
      recurrence_set_days_of_week_mask(pattern, &rrule);
    }
    else
    {
      synce_error("Missing information for monthly recurrence in RRULE '%s'", 
          mdir_rrule->values[0]);
      goto exit;
    }
  }
  else if (STR_EQUAL(rrule.freq, "YEARLY"))
  {
    /* Convert to Monthly with 12 times the interval */
    pattern->recurrence_type = olRecursMonthly;
    rrule.interval *= 12;

    if (rrule.bymonthday)
    {
      pattern->day_of_month = rrule.bymonthday;
    }
    else if (rrule.bysetpos)
    {
      synce_error("Don't know how to handle BYSETPOS in RRULE '%s'", 
          mdir_rrule->values[0]);
      goto exit;
    }
    else
    {
      /* Get BYMONTHDAY from start date */
      struct tm start_date = rra_minutes_to_struct(pattern->pattern_start_date);
      pattern->day_of_month = start_date.tm_mday;
    }
  }
  else
  {
    synce_error("Unexpected frequencey in RRULE '%s'", mdir_rrule->values[0]);
    goto exit;
  }

  pattern->interval = rrule.interval;

  if (rrule.count)
  {
    pattern->occurrences = rrule.count;
    pattern->flags |= RecurrenceEndsAfterXOccurrences;

    /* XXX calculate pattern->pattern_end_date */
    /* CE requires an explicit end date */
    /* XXX Could be problematic with leap years */
    switch (pattern->recurrence_type)
    {
      case olRecursDaily:
        synce_trace("Calculating Pattern end date for daily recursion");
        pattern->pattern_end_date = pattern->pattern_start_date + (rrule.count-1) * rrule.interval;
        break;
      case olRecursWeekly:
        /* XXX Only works for interval=1 at the moment */
        synce_trace("Calculating Pattern end date for weekly recursion");
        uint32_t count_in_mask=0;
        int i;
        for (i=0; i<7; i++)
	  if (pattern->days_of_week_mask &  (1 << i)) count_in_mask++;

        uint32_t days_to_add=((rrule.count-1)/count_in_mask)*7;
        uint32_t rest=(rrule.count-1)%count_in_mask;
	
        struct tm start_date = rra_minutes_to_struct(pattern->pattern_start_date);
        /* rotate the bitmask */
        uint32_t biased_mask = (pattern->days_of_week_mask | ((pattern->days_of_week_mask & ((1<<start_date.tm_wday)-1)) << 7))>>(start_date.tm_wday+1);
        while (rest)
	{
	  rest--;
          while (biased_mask%2==0)
	  {
            if (biased_mask==0) {synce_error("Calculation of Pattern end date failed"); goto failed;}
	    days_to_add++;
	    biased_mask>>=1;
	  }
          days_to_add++;
          biased_mask>>=1;
	}
	
	pattern->pattern_end_date = pattern->pattern_start_date + days_to_add * MINUTES_PER_DAY;

        break;
      default:
      failed:
        synce_trace("FIXME: Have to calculate Pattern end date");
    pattern->pattern_end_date = RRA_DoesNotEndDate;
  }
  }
  else if (rrule.until)
  {
    struct tm until;
    bool is_utc;
    RRA_Timezone tzi;

    CeRapiInit();
    rra_timezone_get(&tzi);

    if (!parser_datetime_to_struct(rrule.until, &until, &is_utc))
      goto exit;

    pattern->flags |= RecurrenceEndsOnDate;
    pattern->pattern_end_date = (rra_minutes_from_struct(&until)/MINUTES_PER_DAY)*MINUTES_PER_DAY;
    /* XXX calculate pattern->occurrences */
  }
  else
  {
    pattern->flags |= RecurrenceDoesNotEnd;
    pattern->pattern_end_date = RRA_DoesNotEndDate;
  }

  if (!recurrence_set_exceptions(pattern, exdates))
  {
    synce_error("Failed to store recurrence exceptions");
    goto exit;
  }

  {
    uint8_t* buffer = NULL;
    size_t size = 0;
    if (!rra_recurrence_pattern_to_buffer(pattern, &buffer, &size))
    {
      synce_error("Failed to convert recurrence pattern to buffer for RRULE '%s'", mdir_rrule->values[0]);
      goto exit;
    }

    if (!parser_add_blob(p, ID_RECURRENCE_PATTERN, buffer, size))
    {
      synce_error("Failed to set recurrcence pattern in output");
      goto exit;
    }

    if (!parser_add_int16(p, ID_OCCURENCE, OCCURENCE_REPEATED))
    {
      synce_error("Failed to sett occurence in output");
      goto exit;
    }
  }

  success = true;

exit:
  rra_recurrence_pattern_destroy(pattern);
  return success;
}

