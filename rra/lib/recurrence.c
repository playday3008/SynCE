/* $Id$ */
#define _GNU_SOURCE 1
#include "recurrence.h"
#include "generator.h"
#include "parser.h"
#include "appointment_ids.h"
#include "recurrence_internal.h"
#include <synce_log.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

#define MINUTES_FROM_1601_TO_1970   194074560

#define MINUTES_PER_DAY   (60*24)
#define SECONDS_PER_DAY   (60*MINUTES_PER_DAY)

static const int DAYS_TO_MONTH[12] =
{
  0,                                  /* jan */
  31,                                 /* feb */
  31+28,                              /* mar */
  31+28+31,                           /* apr */
  31+28+31+30,                        /* may */
  31+28+31+30+31,                     /* jun */
  31+28+31+30+31+30,                  /* jul */
  31+28+31+30+31+30+31,               /* aug */
  31+28+31+30+31+30+31+31,            /* sep */
  31+28+31+30+31+30+31+31+30,         /* oct */
  31+28+31+30+31+30+31+31+30+31,      /* nov */
  31+28+31+30+31+30+31+31+30+31+30,   /* dec */
};

static const uint8_t blob_0001[104] =
{
  0xc4,0xff,0xff,0xff,0x00,0x00,0x00,0x00,
  0xc4,0xff,0xff,0xff,0x00,0x00,0x00,0x00,
  0x0a,0x00,0x00,0x00,0x05,0x00,0x03,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x03,0x00,0x00,0x00,0x05,0x00,
  0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/* Last 0x10 bytes are variable */
static const uint8_t blob_0067[52] = 
{
  0x04,0x00,0x00,0x00,0x82,0x00,0xe0,0x00,
  0x74,0xc5,0xb7,0x10,0x1a,0x82,0xe0,0x08,
  0x00,0x00,0x00,0x00,0xe6,0x0d,0xd4,0x01,
  0x9b,0x36,0xe8,0x6f,0x0d,0x4a,0x3f,0x0c,
  0x11,0x2e,0x03,0x10,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00
};

/* XXX: the string vector functions were just copied from contact.c - not good! */

static char** strsplit(const char* source, int separator)/*{{{*/
{
	int i;
	int count = 0;
	const char* p = NULL;
	char** result = NULL;
	size_t length = 0;

	for (p = source; *p; p++)
		if (separator == *p)
			count++;

	result = malloc((count + 2) * sizeof(char*));

	for (p = source, i = 0; i < count; i++)
	{
		length = strchr(p, separator) - p;
		result[i] = strndup(p, length);
		p += length + 1;
	}

	result[i++] = strdup(p);

	result[i] = NULL;
	return result;
}/*}}}*/

#if 0
static void strv_dump(char** strv)/*{{{*/
{
	char** pp;

	for (pp = strv; *pp; pp++)
		synce_trace("'%s'", *pp);
}/*}}}*/
#endif

static void strv_free(char** strv)/*{{{*/
{
	char** pp;

	for (pp = strv; *pp; pp++)
		free(*pp);

	free(strv);
}/*}}}*/

static void split_name_equals_value(char* name, char** value)/*{{{*/
{
  char* equals = strchr(name, '=');
  if (equals)
  {
    *equals = '\0';
    *value = equals+1;
  }
  else
  {
    *value = NULL;
  }
}/*}}}*/

static void recurrence_line_to_rrule(mdir_line* line, RecurrenceRule* rrule)/*{{{*/
{
  char** spec_vector = strsplit(line->values[0], ';');
  char** pp;

  /* default values */
  memset(rrule, 0, sizeof(RecurrenceRule));
  rrule->interval = 1;
 
	for (pp = spec_vector; *pp; pp++)
  {
    char* name = *pp;
    char* value = NULL;

		synce_trace("'%s'", *pp);
    split_name_equals_value(name, &value);

    if (!value)
    {
      synce_warning("Specification without value: '%s'", name);
      continue;
    }

    if (STR_EQUAL(name, "byday"))
    {
      rrule->properties |= PROPERTY_BYDAY;
      rrule->byday = strdup(value);
    }
    else if (STR_EQUAL(name, "bysetpos"))
    {
      rrule->properties |= PROPERTY_BYSETPOS;
      rrule->bysetpos = strdup(value);
    }
    else if (STR_EQUAL(name, "count"))
    {
      rrule->properties |= PROPERTY_COUNT;
      rrule->count = atoi(value);
    }
    else if (STR_EQUAL(name, "freq"))
    {
      rrule->properties |= PROPERTY_FREQ;

      if (STR_EQUAL(value, "weekly"))
        rrule->freq = FREQ_WEEKLY;
      else if (STR_EQUAL(value, "monthly"))
        rrule->freq = FREQ_MONTHLY;
      else if (STR_EQUAL(value, "yearly"))
        rrule->freq = FREQ_YEARLY;
      else
      {
        synce_warning("Frequency '%s' not yet supported", value);
        continue;
      }
    }
    else if (STR_EQUAL(name, "interval"))
    {
      rrule->properties |= PROPERTY_INTERVAL;
      rrule->interval = atoi(value);
    }
    else
    {
      synce_warning("Unknown specification: '%s'", name);
      continue;
    }
  }

  strv_free(spec_vector);
}/*}}}*/

static bool recurrence_set_flags(/*{{{*/
    RecurrenceRule* rrule,
    uint32_t* flags,
    uint32_t* occurrences)
{
  assert(flags);

  *flags = DEFAULT_FLAGS;
  
  if (rrule->properties & PROPERTY_UNTIL)
  {
    *flags |= FLAG_ENDS_ON_DATE;
    synce_error("UNTIL not yet supported");
    return false;
  }
  else if (rrule->properties & PROPERTY_COUNT)
  {
    *flags |= FLAG_ENDS_AFTER_X_OCCURENCES;
    if (occurrences)
      *occurrences = rrule->count;
  }
  else
  {
    *flags |= FLAG_DOES_NOT_END;
  }

  return true;
}/*}}}*/

static bool recurrence_set_days_of_week_mask(/*{{{*/
    RecurrenceRule* rrule, 
    uint32_t* days_of_week_mask)
{
  if (rrule->properties & PROPERTY_BYDAY)
  {
    char** days = strsplit(rrule->byday, ',');
    char** pp;

    for (pp = days; *pp; pp++)
    {
      /* TODO: make a table and loop */
      if (STR_EQUAL(*pp, "mo"))
        *days_of_week_mask |= olMonday;
      else if (STR_EQUAL(*pp, "tu"))
        *days_of_week_mask |= olThursday;
      else if (STR_EQUAL(*pp, "we"))
        *days_of_week_mask |= olWednesday;
      else if (STR_EQUAL(*pp, "th"))
        *days_of_week_mask |= olThursday;
      else if (STR_EQUAL(*pp, "fr"))
        *days_of_week_mask |= olFriday;
      else if (STR_EQUAL(*pp, "sa"))
        *days_of_week_mask |= olSaturday;
      else if (STR_EQUAL(*pp, "su"))
        *days_of_week_mask |= olSunday;
      else
        synce_warning("Bad weekday: '%s'", *pp);
    }

    strv_free(days);
    return true;
  }
  else
  {
    synce_error("BYDAY missing");
    return false;
  }
}/*}}}*/

static void recurrence_set_unknown3(/*{{{*/
    uint8_t* unknown3)
{
  unknown3[0x00] = 0xdf;
  unknown3[0x01] = 0x80;
  unknown3[0x02] = 0xe9;
  unknown3[0x03] = 0x5a;
  unknown3[0x04] = 0x05;
  unknown3[0x05] = 0x30;

  unknown3[0x08] = 0x05;
  unknown3[0x09] = 0x30;
  
  /*unknown3[0x14] = 0x01;*/
}/*}}}*/

static bool recurrence_set_date_time(/*{{{*/
    mdir_line* dtstart,
    mdir_line* dtend,
    uint32_t* date,
    uint32_t* start_minute,
    uint32_t* end_minute)
{
  bool success = false;
  struct tm start;
  struct tm end;

  if (!parser_datetime_to_struct(dtstart->values[0], &start))
    goto exit;
  
  if (!parser_datetime_to_struct(dtend->values[0], &end))
    goto exit;

  if (!date || !start_minute || !end_minute)
  {
    synce_error("Invalid parameters");
    goto exit;
  }

  *date         = (mktime(&start) / SECONDS_PER_DAY) * MINUTES_PER_DAY + MINUTES_FROM_1601_TO_1970;
  *start_minute = start.tm_hour * 60 + start.tm_min;
  *end_minute   = end  .tm_hour * 60 + end  .tm_min;

  success = true;

exit:
  return success;
}/*}}}*/

static bool recurrence_parse_weekly(/*{{{*/
    RecurrenceRule* rrule, 
    RecurrenceBlob* pattern,
    mdir_line* dtstart,
    mdir_line* dtend)
{
  bool success = false;
  pattern->recurrence_type          = olRecursWeekly;
  pattern->details.weekly.interval  = rrule->interval;

  if (!recurrence_set_flags(rrule, 
      &pattern->details.weekly.flags,
      &pattern->details.weekly.occurrences))
    goto exit;

  if (!recurrence_set_days_of_week_mask(rrule, 
      &pattern->details.weekly.days_of_week_mask))
    goto exit;

  /* some more info */
  recurrence_set_date_time(
      dtstart, 
      dtend, 
      &pattern->details.weekly.date,
      &pattern->details.weekly.start_minute,
      &pattern->details.weekly.end_minute);
    
  /* always 0x0b for olRecursWeekly? */
  pattern->unknown1[4] = 0x0b;

  /* always 6 days? */
  pattern->day = 6 * MINUTES_PER_DAY;

  recurrence_set_unknown3(pattern->details.weekly.unknown3);

  success = true;

exit:
  return success;
}/*}}}*/

static bool recurrence_parse_monthly(/*{{{*/
    RecurrenceRule* rrule, 
    RecurrenceBlob* pattern,
    mdir_line* dtstart,
    mdir_line* dtend)
{
  bool success = false;

  pattern->recurrence_type            = olRecursMonthNth;
  pattern->details.month_nth.interval = rrule->interval;

  if (!recurrence_set_flags(rrule, 
      &pattern->details.month_nth.flags,
      &pattern->details.month_nth.occurrences))
    goto exit;

  if (!recurrence_set_days_of_week_mask(rrule, 
      &pattern->details.month_nth.days_of_week_mask))
    goto exit;

  if (rrule->properties & PROPERTY_BYSETPOS)
  {
    char** strv = strsplit(rrule->bysetpos, ',');

    if (strv[0]) 
    {
      int week = atoi(strv[0]);

      if (week < 1 || week > 4)
      {
        if (week != -1)
          synce_warning("Can't handle week number %i in BYSETPOS, assuming last week of month", week);
        week = 5;
      }
      
      pattern->details.month_nth.instance = (uint32_t)week;
    }

    if (strv[1])
      synce_warning("Can't handle more than one week in BYSETPOS");

    strv_free(strv);
  }
  else
    synce_warning("BYSETPOS missing");

  /* some more info */
  recurrence_set_date_time(
      dtstart, 
      dtend, 
      &pattern->details.month_nth.date,
      &pattern->details.month_nth.start_minute,
      &pattern->details.month_nth.end_minute);

   /* always 0x0c for olRecursMonthNth? */
  pattern->unknown1[4] = 0x0c;

#if 0
  /* variable */
  pattern->day = 0x0002a300; /* 2*24*60*60 */
#endif

  recurrence_set_unknown3(pattern->details.month_nth.unknown3);

  success = true;

exit:
  return success;
}/*}}}*/

static bool recurrence_parse_yearly(/*{{{*/
    RecurrenceRule* rrule, 
    RecurrenceBlob* pattern,
    mdir_line* dtstart,
    mdir_line* dtend)
{
  bool success = false;
  struct tm start;

  if (!parser_datetime_to_struct(dtstart->values[0], &start))
    goto exit;
  
   /* use monthly recursion with interval 12 */
  pattern->recurrence_type              = olRecursMonthly;
  pattern->details.monthly.interval     = 12 * rrule->interval;
  pattern->details.monthly.day_of_month = start.tm_mday;

  if (!recurrence_set_flags(rrule, 
      &pattern->details.monthly.flags,
      &pattern->details.monthly.occurrences))
    goto exit;
  
  /* some more info */
  recurrence_set_date_time(
      dtstart, 
      dtend, 
      &pattern->details.monthly.date,
      &pattern->details.monthly.start_minute,
      &pattern->details.monthly.end_minute);

  /* always 0x0d for olRecursMonthly? */
  pattern->unknown1[4] = 0x0d;

  /* I think this is the pattern... */
  {
    struct tm start;

    if (!parser_datetime_to_struct(dtstart->values[0], &start))
      goto exit;

    pattern->day = DAYS_TO_MONTH[start.tm_mon] * MINUTES_PER_DAY;
  }

  recurrence_set_unknown3(pattern->details.monthly.unknown3);

  success = true;
  
exit:
  return success;
}/*}}}*/

bool recurrence_parse_rrule(/*{{{*/
    Parser* p, 
    mdir_line* line, 
    mdir_line* dtstart,
    mdir_line* dtend)
{
  bool success = false;
  RecurrenceRule rrule;
  RecurrenceBlob pattern;

  if (!line)
  {
    /* no recurrence */
    return parser_add_int16(p, ID_OCCURANCE, OCCURANCE_ONCE);
  }

  /* verify the structures are packed correctly */
  assert(sizeof(RecurringWeekly)   == 0x3c);
  assert(sizeof(RecurringMonthly)  == 0x3c);
  assert(sizeof(RecurringMonthNth) == 0x3c);
  assert(sizeof(RecurrenceBlob) == 0x68);

  memset(&pattern, 0, sizeof(RecurrenceBlob));
 
  /* always the same? */
  pattern.unknown1[0] = 0x04; 
  pattern.unknown1[1] = 0x30; 
  pattern.unknown1[2] = 0x04;
  pattern.unknown1[3] = 0x30;

  /* variable: 0x0a-0x0d */
  /* pattern.unknown1[4] = 0x0d; */

  /* always the same? */
  pattern.unknown1[5] = 0x20;

  recurrence_line_to_rrule(line, &rrule);

  switch (rrule.freq)
  {
    case FREQ_WEEKLY:
      success = recurrence_parse_weekly(&rrule, &pattern, dtstart, dtend);
      break;

    case FREQ_MONTHLY:
      success = recurrence_parse_monthly(&rrule, &pattern, dtstart, dtend);
      break;

    case FREQ_YEARLY:
      success = recurrence_parse_yearly(&rrule, &pattern, dtstart, dtend);
      break;

    default:
      synce_warning("Frequency %i not yet supported", rrule.freq);
      break;
  }
  
  parser_add_blob(p, ID_UNKNOWN_0067, blob_0067, sizeof(blob_0067));

  if (success)
    success = parser_add_blob(p, ID_UNKNOWN_4015, 
        (uint8_t*)&pattern, sizeof(RecurrenceBlob));

  parser_add_blob(p, ID_UNKNOWN_0001, blob_0001, sizeof(blob_0001));

  if (success)
    success = parser_add_int16(p, ID_OCCURANCE, OCCURANCE_REPEATED);

  if (rrule.byday)
    free(rrule.byday);
  if (rrule.bysetpos)
    free(rrule.bysetpos);
  return success;
}/*}}}*/

