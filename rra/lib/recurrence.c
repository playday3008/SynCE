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

static uint8_t blob_0001[104] =
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
static uint8_t blob_0067[52] = 
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

    if (STR_EQUAL(name, "freq"))
    {
      rrule->properties |= PROPERTY_FREQ;

      if (STR_EQUAL(value, "yearly"))
        rrule->freq = FREQ_YEARLY;
      else
      {
        synce_warning("Frequency '%s' not yet supported", value);
        continue;
      }
    }
    else if (STR_EQUAL(name, "count"))
    {
      rrule->properties |= PROPERTY_COUNT;
      rrule->count = atoi(value);
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

static bool recurrence_parse_yearly(/*{{{*/
    RecurrenceRule* rrule, 
    RecurrencePattern* pattern,
    mdir_line* dtstart)
{
  /* use monthly recursion with interval 12 */
  pattern->recurrence_type = olRecursMonthly;
  pattern->details.monthly.interval     = 12 * rrule->interval;
  pattern->details.monthly.day_of_month = 27; /* XXX: set correct */
  pattern->details.monthly.flags = DEFAULT_FLAGS;

  if (rrule->properties & PROPERTY_UNTIL)
  {
    pattern->details.monthly.flags |= FLAG_ENDS_ON_DATE;
    synce_error("UNTIL not yet supported");
    return false;
  }
  else if (rrule->properties & PROPERTY_COUNT)
  {
    pattern->details.monthly.flags |= FLAG_ENDS_AFTER_X_OCCURENCES;
    pattern->details.monthly.occurrences = rrule->count;
  }
  else
  {
    pattern->details.monthly.flags |= FLAG_DOES_NOT_END;
  }

  return true;
}/*}}}*/

bool recurrence_parse_rrule(Parser* p, mdir_line* line, mdir_line* dtstart)
{
  bool success = false;
  RecurrenceRule rrule;
  RecurrencePattern pattern;

  /* verify the structures are packed correctly */
  assert(sizeof(RecurringMonthly)  == 0x20);
  assert(sizeof(RecurrencePattern) == 0x68);

  memset(&pattern, 0, sizeof(RecurrencePattern));
 
  /* always the same? */
  pattern.unknown1[0] = 0x04; 
  pattern.unknown1[1] = 0x30; 
  pattern.unknown1[2] = 0x04;
  pattern.unknown1[3] = 0x30;

  /* variable: 0x0b, 0x0d */
  pattern.unknown1[4] = 0x0d;

  /* always the same? */
  pattern.unknown1[5] = 0x20;

  /* variable */
  pattern.unknown2    = 0x0002a300; /* 2*24*60*60 */

  /* variable? */
  pattern.unknown3[0x00] = 0x20; 
  pattern.unknown3[0x01] = 0x63;
  
  /* always the same? */
  pattern.unknown3[0x02] = 0x9d; 
  pattern.unknown3[0x03] = 0x0c;
  pattern.unknown3[0x04] = 0xdf;
  pattern.unknown3[0x05] = 0x80;
  pattern.unknown3[0x06] = 0xe9;
  pattern.unknown3[0x07] = 0x5a;
  pattern.unknown3[0x08] = 0x05;
  pattern.unknown3[0x09] = 0x30;

  pattern.unknown3[0x0c] = 0x05;
  pattern.unknown3[0x0d] = 0x30;
  
  pattern.unknown3[0x14] = 0x01;

  recurrence_line_to_rrule(line, &rrule);

  switch (rrule.freq)
  {
    case FREQ_YEARLY:
      success = recurrence_parse_yearly(&rrule, &pattern, dtstart);
      break;

    default:
      synce_warning("Frequency %i not yet supported", rrule.freq);
      break;
  }
  
  parser_add_blob(p, ID_UNKNOWN_0067, blob_0067, sizeof(blob_0067));

  if (success)
    success = parser_add_blob(p, ID_UNKNOWN_4015, 
        (uint8_t*)&pattern, sizeof(RecurrencePattern));

  parser_add_blob(p, ID_UNKNOWN_0001, blob_0001, sizeof(blob_0001));

  if (success)
    success = parser_add_int16(p, ID_OCCURANCE, OCCURANCE_REPEATED);

  return success;
}

