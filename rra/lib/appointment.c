/* $Id$ */
#define _GNU_SOURCE 1
#include "librra.h"
#include "appointment_ids.h"
#include "generator.h"
#include "parser.h"
#include <rapi.h>
#include <synce_log.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

#define MINUTES_PER_DAY (24*60)
#define SECONDS_PER_DAY (MINUTES_PER_DAY*60)

typedef struct _EventGeneratorData
{
  CEPROPVAL* start;
  CEPROPVAL* duration;
  CEPROPVAL* duration_unit;
  CEPROPVAL* reminder_minutes;
  CEPROPVAL* reminder_enabled;
} EventGeneratorData;

static bool on_property_busy_status(Generator* g, CEPROPVAL* propval, void* cookie)/*{{{*/
{
  switch (propval->val.iVal)
  {
    case BUSY_STATUS_FREE:
      generator_add_simple(g, "TRANSP", "TRANSPARENT");
      break;
      
    case BUSY_STATUS_TENTATIVE:
      synce_warning("Busy status 'tentative' not yet supported");
      break;
      
    case BUSY_STATUS_BUSY:
      generator_add_simple(g, "TRANSP", "OPAQUE");
      break;
      
    case BUSY_STATUS_OUT_OF_OFFICE:
      synce_warning("Busy status 'out of office' not yet supported");
      break;
      
    default:
      synce_warning("Unknown busy status: %04x", propval->val.iVal);
      break;
  }
  return true;
}/*}}}*/

static bool on_property_duration(Generator* g, CEPROPVAL* propval, void* cookie)
{
  EventGeneratorData* data = (EventGeneratorData*)cookie;
  data->duration = propval;
  return true;
}

static bool on_property_duration_unit(Generator* g, CEPROPVAL* propval, void* cookie)
{
  EventGeneratorData* data = (EventGeneratorData*)cookie;
  data->duration_unit = propval;
  return true;
}

static bool on_property_location(Generator* g, CEPROPVAL* propval, void* cookie)
{
  generator_add_simple_propval(g, "LOCATION", propval);
  return true;
}

static bool str_is_print(CEBLOB* blob)
{
  unsigned i;
  
  for (i = 0; i < blob->dwCount; i++)
  {
    switch (blob->lpb[i])
    {
      case 0x0a: /* LF */
      case 0x0d: /* CR */
        break;

      default:
        if (!isprint(blob->lpb[i]))
          return false;
    }
  }

  return true;
}

static bool on_property_notes(Generator* g, CEPROPVAL* propval, void* cookie)/*{{{*/
{
  assert(CEVT_BLOB == (propval->propid & 0xffff));

  if (str_is_print(&propval->val.blob))
  {
    char* tmp = strndup((const char*)
        propval->val.blob.lpb, 
        propval->val.blob.dwCount);
    generator_add_simple(g, "DESCRIPTION", tmp);
    free(tmp);
  }
  else
  {
    synce_warning("Note format not yet supported");
  }
  
  return true;
}/*}}}*/

static bool on_property_reminder_enabled(Generator* g, CEPROPVAL* propval, void* cookie)
{
  EventGeneratorData* data = (EventGeneratorData*)cookie;
  data->reminder_enabled = propval;
  return true;
}

static bool on_property_reminder_minutes(Generator* g, CEPROPVAL* propval, void* cookie)
{
  EventGeneratorData* data = (EventGeneratorData*)cookie;
  data->reminder_minutes = propval;
  return true;
}

static bool on_property_sensitivity(Generator* g, CEPROPVAL* propval, void* cookie)/*{{{*/
{
  switch (propval->val.iVal)
  {
    case SENSITIVITY_PUBLIC:
      generator_add_simple(g, "CLASS", "PUBLIC");
      break;
      
    case SENSITIVITY_PRIVATE:
      generator_add_simple(g, "CLASS", "PRIVATE");
      break;

    default:
      synce_warning("Unknown sensitivity: %04x", propval->val.iVal);
      break;
  }
  return true;
}/*}}}*/

static bool on_property_start(Generator* g, CEPROPVAL* propval, void* cookie)
{
  EventGeneratorData* data = (EventGeneratorData*)cookie;
  data->start = propval;
  return true;
}

static bool on_property_subject(Generator* g, CEPROPVAL* propval, void* cookie)
{
  generator_add_simple_propval(g, "SUMMARY", propval);
  return true;
}

bool rra_appointment_to_vevent(/*{{{*/
    uint32_t id,
    const uint8_t* data,
    size_t data_size,
    char** vevent,
    uint32_t flags)
{
	bool success = false;
  Generator* generator = NULL;
  EventGeneratorData event_generator_data;
  memset(&event_generator_data, 0, sizeof(EventGeneratorData));

  generator = generator_new(0, &event_generator_data);
  if (!generator)
    goto exit;

  generator_add_property(generator, ID_BUSY_STATUS, on_property_busy_status);
  generator_add_property(generator, ID_DURATION,    on_property_duration);
  generator_add_property(generator, ID_DURATION_UNIT, on_property_duration_unit);
  generator_add_property(generator, ID_LOCATION,    on_property_location);
  generator_add_property(generator, ID_NOTES,       on_property_notes);
  generator_add_property(generator, ID_REMINDER_MINUTES_BEFORE_START, on_property_reminder_minutes);
  generator_add_property(generator, ID_REMINDER_ENABLED, on_property_reminder_enabled);
  generator_add_property(generator, ID_SENSITIVITY, on_property_sensitivity);
  generator_add_property(generator, ID_START,       on_property_start);
  generator_add_property(generator, ID_SUBJECT,     on_property_subject);

  if (!generator_set_data(generator, data, data_size))
    goto exit;

  generator_add_simple(generator, "BEGIN", "VCALENDAR");
  
  switch (flags & RRA_VCALENDAR_VERSION_MASK)
  {
    case RRA_VCALENDAR_VERSION_2_0:
      generator_add_simple(generator, "VERSION", "2.0");
      break;
  }
  
  generator_add_simple(generator, "BEGIN", "VEVENT");

  if (!generator_run(generator))
    goto exit;

  if (event_generator_data.start && 
      event_generator_data.duration &&
      event_generator_data.duration_unit)
  {
    char buffer[32];
    time_t start_time = 
      filetime_to_unix_time(&event_generator_data.start->val.filetime);
    time_t end_time = 0;
    const char* type = NULL;
    const char* format = NULL;
    
    switch (event_generator_data.duration_unit->val.lVal)
    {
      case DURATION_UNIT_DAYS:
        type   = "DATE";
        format = "%Y%m%d";

        /* days to seconds */
        end_time = start_time + 
          event_generator_data.duration->val.lVal * SECONDS_PER_DAY;
        break;


      case DURATION_UNIT_MINUTES:
        type   = "DATE-TIME";
        format = "%Y%m%dT%H%M%S";

        /* minutes to seconds */
        end_time = start_time + 
          event_generator_data.duration->val.lVal * 60;
        break;

      default:
        synce_warning("Unknown duration unit: %i", 
            event_generator_data.duration_unit->val.lVal);
        break;
    }

    if (type && format)
    {
      strftime(buffer, sizeof(buffer), format, localtime(&start_time));
      generator_add_with_type(generator, "DTSTART", type, buffer);
      
      if (end_time)
      {
        strftime(buffer, sizeof(buffer), format, localtime(&end_time));
        generator_add_with_type(generator, "DTEND",   type, buffer);
      }
    }
  }
  else
    synce_warning("Missing start, duration or duration unit");


  if (event_generator_data.reminder_enabled &&
      event_generator_data.reminder_minutes &&
      event_generator_data.reminder_enabled->val.iVal)
  {
    char buffer[32];

    generator_add_simple(generator, "BEGIN", "VALARM");

    /* XXX: maybe this should correspond to ID_REMINDER_OPTIONS? */
    generator_add_simple(generator, "ACTION", "DISPLAY");

    /* XXX: what if minutes > 59 */
    snprintf(buffer, sizeof(buffer), "-PT%liM", 
        event_generator_data.reminder_minutes->val.lVal);

    generator_begin_line         (generator, "TRIGGER");
    
    generator_begin_parameter    (generator, "VALUE");
    generator_add_parameter_value(generator, "DURATION");
    generator_end_parameter      (generator);
    
    generator_begin_parameter    (generator, "RELATED");
    generator_add_parameter_value(generator, "START");
    generator_end_parameter      (generator);

    generator_add_value          (generator, buffer);
    generator_end_line           (generator);
    
    generator_add_simple(generator, "END", "VALARM");
  }

  generator_add_simple(generator, "END", "VEVENT");
  generator_add_simple(generator, "END", "VCALENDAR");
  
  if (!generator_get_result(generator, vevent))
    goto exit;

  success = true;

exit:
  generator_destroy(generator);
  return success;
}/*}}}*/



/***********************************************************************

  Conversion from vEvent to Appointment

 ***********************************************************************/

typedef struct _EventParserData
{
  bool have_alarm;
  mdir_line* dtstart;
  mdir_line* dtend;
} EventParserData;

static bool on_alarm_trigger(Parser* p, mdir_line* line, void* cookie)/*{{{*/
{
  EventParserData* event_parser_data = (EventParserData*)cookie;

  if (event_parser_data->have_alarm)
    goto exit;

  char** data_type = mdir_get_param_values(line, "VALUE");
  char** related   = mdir_get_param_values(line, "RELATED");
  int duration = 0;

  /* data type must be DURATION */
  if (data_type && data_type[0])
  {
    if (STR_EQUAL(data_type[0], "DATE-TIME"))
    {
      synce_warning("Absolute date/time for alarm is not supported");
      goto exit;
    }
    if (!STR_EQUAL(data_type[0], "DURATION"))
    {
      synce_warning("Unknown TRIGGER data type: '%s'", data_type[0]);
      goto exit;
    }
  }

  /* related must be START */
  if (related && related[0])
  {
    if (STR_EQUAL(related[0], "END"))
    {
      synce_warning("Alarms related to event end are not supported");
      goto exit;
    }
    if (!STR_EQUAL(related[0], "START"))
    {
      synce_warning("Unknown TRIGGER data type: '%s'", related[0]);
      goto exit;
    }
  }

  if (parser_duration_to_seconds(line->values[0], &duration) && duration <= 0)
  {
    /* XXX: use ACTION for this instead of defaults? */
    parser_add_int32(p, ID_REMINDER_OPTIONS, REMINDER_LED|REMINDER_DIALOG|REMINDER_SOUND);

    /* set alarm */
    parser_add_int32(p, ID_REMINDER_MINUTES_BEFORE_START, -duration / 60);
    parser_add_int16(p, ID_REMINDER_ENABLED, 1);
    event_parser_data->have_alarm = true;
  }

exit:
  return true;
}/*}}}*/

static bool on_event_class(Parser* p, mdir_line* line, void* cookie)/*{{{*/
{
  if (STR_EQUAL(line->values[0], "PUBLIC"))
    parser_add_int16(p, ID_SENSITIVITY, SENSITIVITY_PUBLIC);
  else if (
      STR_EQUAL(line->values[0], "PRIVATE") ||
      STR_EQUAL(line->values[0], "CONFIDENTIAL"))
    parser_add_int16(p, ID_SENSITIVITY, SENSITIVITY_PRIVATE);
  else
    synce_warning("Unknown value for CLASS: '%s'", line->values[0]);
  return true;
}/*}}}*/

static bool on_event_dtend(Parser* p, mdir_line* line, void* cookie)
{
  EventParserData* event_parser_data = (EventParserData*)cookie;
  event_parser_data->dtend = line;
  return true;
}

static bool on_event_dtstart(Parser* p, mdir_line* line, void* cookie)
{
  EventParserData* event_parser_data = (EventParserData*)cookie;
  event_parser_data->dtstart = line;
  return true;
}

static bool on_event_location(Parser* p, mdir_line* line, void* cookie)
{
  return parser_add_string_from_line(p, ID_LOCATION, line);
}

static bool on_event_summary(Parser* p, mdir_line* line, void* cookie)
{
  return parser_add_string_from_line(p, ID_SUBJECT, line);
}

static bool on_event_transp(Parser* p, mdir_line* line, void* cookie)/*{{{*/
{
  if (STR_EQUAL(line->values[0], "OPAQUE"))
    parser_add_int16(p, ID_BUSY_STATUS, BUSY_STATUS_BUSY);
  else if (STR_EQUAL(line->values[0], "TRANSPARENT"))
    parser_add_int16(p, ID_BUSY_STATUS, BUSY_STATUS_FREE);
  else
    synce_warning("Unknown value for TRANSP: '%s'", line->values[0]);
  return true;
}/*}}}*/

bool rra_appointment_from_vevent(/*{{{*/
    const char* vevent,
    uint32_t* id,
    uint8_t** data,
    size_t* data_size,
    uint32_t flags)
{
	bool success = false;
  Parser* parser = NULL;
  ParserComponent* base;
  ParserComponent* calendar;
  ParserComponent* event;
  ParserComponent* alarm;
  int parser_flags = PARSER_UTF8; /* XXX */
  EventParserData event_parser_data;
  memset(&event_parser_data, 0, sizeof(EventParserData));

  alarm = parser_component_new("vAlarm");

  parser_component_add_parser_property(alarm, 
      parser_property_new("trigger", on_alarm_trigger));

  event = parser_component_new("vEvent");
  parser_component_add_parser_component(event, alarm);

  parser_component_add_parser_property(event, 
      parser_property_new("class", on_event_class));
  parser_component_add_parser_property(event, 
      parser_property_new("dtEnd", on_event_dtend));
  parser_component_add_parser_property(event, 
      parser_property_new("dtStart", on_event_dtstart));
  parser_component_add_parser_property(event, 
      parser_property_new("Location", on_event_location));
  parser_component_add_parser_property(event, 
      parser_property_new("Summary", on_event_summary));
  parser_component_add_parser_property(event, 
      parser_property_new("Transp", on_event_transp));

  calendar = parser_component_new("vCalendar");
  parser_component_add_parser_component(calendar, event);

  base = parser_component_new(NULL);
  parser_component_add_parser_component(base, calendar);

  parser = parser_new(base, parser_flags, &event_parser_data);
  if (!parser)
  {
    synce_error("Failed to create parser");
    goto exit;
  }

  if (!parser_set_mimedir(parser, vevent))
  {
    synce_error("Failed to parse input data");
    goto exit;
  }

  if (!parser_run(parser))
  {
    synce_error("Failed to convert input data");
    goto exit;
  }

  if (event_parser_data.dtstart)
  {
    parser_add_time_from_line(parser, ID_START, event_parser_data.dtstart);
    
    if (event_parser_data.dtend)
    {
      time_t start = 0;
      time_t end = 0;
      int32_t minutes;

      if (!parser_datetime_to_unix_time(event_parser_data.dtstart->values[0], &start))
        goto exit;
      if (!parser_datetime_to_unix_time(event_parser_data.dtend->values[0],   &end))
        goto exit;

      minutes = (end - start) / 60;
    
      if (minutes % MINUTES_PER_DAY)
      {
        parser_add_int32(parser, ID_DURATION,      minutes);
        parser_add_int32(parser, ID_DURATION_UNIT, DURATION_UNIT_MINUTES);
      }
      else
      {
        parser_add_int32(parser, ID_DURATION,      minutes / MINUTES_PER_DAY);
        parser_add_int32(parser, ID_DURATION_UNIT, DURATION_UNIT_DAYS);
      }
    }
  }

  if (!parser_get_result(parser, data, data_size))
  {
    synce_error("Failed to retrieve result");
    goto exit;
  }
  
 	success = true;

exit:
  /* destroy top object */
  parser_component_destroy(calendar);
  parser_destroy(parser);
	return success;
}/*}}}*/


