/* $Id$ */
#define _GNU_SOURCE 1
#include "librra.h"
#include "appointment_ids.h"
#include "generator.h"
#include "parser.h"
#include <rapi.h>
#include <synce_log.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "common_handlers.h"
#include "recurrence.h"
#include "timezone.h"
#include "../rra_config.h"

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

#define MINUTES_PER_DAY (24*60)
#define SECONDS_PER_DAY (MINUTES_PER_DAY*60)

typedef struct _EventGeneratorData
{
  CEPROPVAL* start;
  CEPROPVAL* duration;
  CEPROPVAL* type;
  CEPROPVAL* reminder_minutes;
  CEPROPVAL* reminder_enabled;
} EventGeneratorData;

/*
   Any on_propval_* functions not here are found in common_handlers.c
*/

static bool on_propval_busy_status(Generator* g, CEPROPVAL* propval, void* cookie)/*{{{*/
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

static bool on_propval_duration(Generator* g, CEPROPVAL* propval, void* cookie)
{
  EventGeneratorData* data = (EventGeneratorData*)cookie;
  data->duration = propval;
  return true;
}

static bool on_propval_type(Generator* g, CEPROPVAL* propval, void* cookie)
{
  EventGeneratorData* data = (EventGeneratorData*)cookie;
  data->type = propval;
  return true;
}

static bool on_propval_reminder_enabled(Generator* g, CEPROPVAL* propval, void* cookie)
{
  EventGeneratorData* data = (EventGeneratorData*)cookie;
  data->reminder_enabled = propval;
  return true;
}

static bool on_propval_reminder_minutes(Generator* g, CEPROPVAL* propval, void* cookie)
{
  EventGeneratorData* data = (EventGeneratorData*)cookie;
  data->reminder_minutes = propval;
  return true;
}

static bool on_propval_start(Generator* g, CEPROPVAL* propval, void* cookie)
{
  EventGeneratorData* data = (EventGeneratorData*)cookie;
  data->start = propval;
  return true;
}

bool rra_appointment_to_vevent(/*{{{*/
    uint32_t id,
    const uint8_t* data,
    size_t data_size,
    char** vevent,
    uint32_t flags,
    TimeZoneInformation* tzi)
{
	bool success = false;
  Generator* generator = NULL;
  unsigned generator_flags = 0;
  EventGeneratorData event_generator_data;
  memset(&event_generator_data, 0, sizeof(EventGeneratorData));

  switch (flags & RRA_APPOINTMENT_CHARSET_MASK)
  {
    case RRA_APPOINTMENT_UTF8:
      generator_flags = GENERATOR_UTF8;
      break;

    case RRA_APPOINTMENT_ISO8859_1:
    default:
      /* do nothing */
      break;
  }

  generator = generator_new(generator_flags, &event_generator_data);
  if (!generator)
    goto exit;

  generator_add_property(generator, ID_BUSY_STATUS, on_propval_busy_status);
  generator_add_property(generator, ID_DURATION,    on_propval_duration);
  generator_add_property(generator, ID_APPOINTMENT_TYPE, on_propval_type);
  generator_add_property(generator, ID_LOCATION,    on_propval_location);
  generator_add_property(generator, ID_NOTES,       on_propval_notes);
  generator_add_property(generator, ID_REMINDER_MINUTES_BEFORE_START, on_propval_reminder_minutes);
  generator_add_property(generator, ID_REMINDER_ENABLED, on_propval_reminder_enabled);
  generator_add_property(generator, ID_SENSITIVITY, on_propval_sensitivity);
  generator_add_property(generator, ID_APPOINTMENT_START,       on_propval_start);
  generator_add_property(generator, ID_SUBJECT,     on_propval_subject);

  if (!generator_set_data(generator, data, data_size))
    goto exit;

#if 0
  generator_add_simple(generator, "BEGIN", "VCALENDAR");
  generator_add_simple(generator, "PRODID", "-//SynCE//NONSGML SynCE RRA//EN");
 
  switch (flags & RRA_APPOINTMENT_VERSION_MASK)
  {
    case RRA_APPOINTMENT_VERSION_2_0:
      generator_add_simple(generator, "VERSION", "2.0");
      break;
  }
       
  generator_add_simple(generator, "METHOD", "PUBLISH");
#endif
 
  generator_add_simple(generator, "BEGIN", "VEVENT");

	if (id != RRA_APPOINTMENT_ID_UNKNOWN)
	{
		char id_str[32];
		snprintf(id_str, sizeof(id_str), "RRA-ID-%08x", id);
		generator_add_simple(generator, "UID", id_str);
	}
   if (!generator_run(generator))
    goto exit;

  if (event_generator_data.start && 
      event_generator_data.duration &&
      event_generator_data.type)
  {
    char buffer[32];
    time_t start_time = 
      filetime_to_unix_time(&event_generator_data.start->val.filetime);
    time_t end_time = 0;
    const char* type = NULL;
    const char* format = NULL;
    
    switch (event_generator_data.type->val.lVal)
    {
      case APPOINTMENT_TYPE_ALL_DAY:
        type   = "DATE";
        format = "%Y%m%d";

        /* days to seconds */
        end_time = start_time + 
          ((event_generator_data.duration->val.lVal / MINUTES_PER_DAY) + 1) *
          SECONDS_PER_DAY;
        break;


      case APPOINTMENT_TYPE_NORMAL:
        type   = "DATE-TIME";
        if (tzi)
          format = "%Y%m%dT%H%M%SZ";
        else
          format = "%Y%m%dT%H%M%S";

        /* minutes to seconds */
        end_time = start_time + 
          event_generator_data.duration->val.lVal * 60;

        if (tzi)
        {
          start_time = time_zone_convert_to_utc(tzi, start_time);
          end_time   = time_zone_convert_to_utc(tzi, end_time);
        }
      
        break;

      default:
        synce_warning("Unknown appintment type: %i", 
            event_generator_data.type->val.lVal);
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
#if 0
  generator_add_simple(generator, "END", "VCALENDAR");
#endif
  
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
  bool has_alarm;
  mdir_line* dtstart;
  mdir_line* dtend;
  mdir_line* rrule;
} EventParserData;

static bool on_alarm_trigger(Parser* p, mdir_line* line, void* cookie)/*{{{*/
{
  EventParserData* event_parser_data = (EventParserData*)cookie;

  char** data_type = mdir_get_param_values(line, "VALUE");
  char** related   = mdir_get_param_values(line, "RELATED");
  int duration = 0;

  if (event_parser_data->has_alarm)
    goto exit;

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
    parser_add_string(p, ID_REMINDER_SOUND_FILE, "Alarm1.wav");

    event_parser_data->has_alarm = true;
  }

exit:
  return true;
}/*}}}*/

static bool on_mdir_line_dtend(Parser* p, mdir_line* line, void* cookie)
{
  EventParserData* event_parser_data = (EventParserData*)cookie;
  event_parser_data->dtend = line;
  return true;
}

static bool on_mdir_line_dtstart(Parser* p, mdir_line* line, void* cookie)
{
  EventParserData* event_parser_data = (EventParserData*)cookie;
  event_parser_data->dtstart = line;
  return true;
}

static bool on_mdir_line_rrule(Parser* p, mdir_line* line, void* cookie)
{
  EventParserData* event_parser_data = (EventParserData*)cookie;
  event_parser_data->rrule = line;
  return true;
}

static bool on_mdir_line_transp(Parser* p, mdir_line* line, void* cookie)/*{{{*/
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
    uint32_t flags,
    TimeZoneInformation* tzi)
{
	bool success = false;
  Parser* parser = NULL;
  ParserComponent* base;
  ParserComponent* calendar;
  ParserComponent* event;
  ParserComponent* alarm;
  int parser_flags = 0;
  EventParserData event_parser_data;
  memset(&event_parser_data, 0, sizeof(EventParserData));

  switch (flags & RRA_APPOINTMENT_CHARSET_MASK)
  {
    case RRA_APPOINTMENT_UTF8:
      parser_flags = PARSER_UTF8;
      break;

    case RRA_APPOINTMENT_ISO8859_1:
    default:
      /* do nothing */
      break;
  }

  alarm = parser_component_new("vAlarm");

  parser_component_add_parser_property(alarm, 
      parser_property_new("trigger", on_alarm_trigger));

  event = parser_component_new("vEvent");
  parser_component_add_parser_component(event, alarm);

  parser_component_add_parser_property(event, 
      parser_property_new("Class", on_mdir_line_class));
  parser_component_add_parser_property(event, 
      parser_property_new("Description", on_mdir_line_description));
  parser_component_add_parser_property(event, 
      parser_property_new("dtEnd", on_mdir_line_dtend));
  parser_component_add_parser_property(event, 
      parser_property_new("dtStart", on_mdir_line_dtstart));
  parser_component_add_parser_property(event, 
      parser_property_new("Location", on_mdir_line_location));
  parser_component_add_parser_property(event, 
      parser_property_new("RRule", on_mdir_line_rrule));
  parser_component_add_parser_property(event, 
      parser_property_new("Summary", on_mdir_line_summary));
  parser_component_add_parser_property(event, 
      parser_property_new("Transp", on_mdir_line_transp));

  calendar = parser_component_new("vCalendar");
  parser_component_add_parser_component(calendar, event);

  /* allow parsing to start with either vCalendar or vEvent */
  base = parser_component_new(NULL);
  parser_component_add_parser_component(base, calendar);
  parser_component_add_parser_component(base, event);

  parser = parser_new(base, parser_flags, tzi, &event_parser_data);
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
    if (!parser_add_time_from_line(parser, ID_APPOINTMENT_START, event_parser_data.dtstart))
    {
      synce_error("Failed add time from line");
      goto exit;
    }
    
    if (event_parser_data.dtend)
    {
      time_t start = 0;
      time_t end = 0;
      int32_t minutes = 0;
      ParserTimeFormat format = parser_get_time_format(event_parser_data.dtstart);

      if (!parser_datetime_to_unix_time(event_parser_data.dtstart->values[0], &start, NULL))
        goto exit;
      if (!parser_datetime_to_unix_time(event_parser_data.dtend->values[0],   &end, NULL))
        goto exit;

      switch (format)
      {
        case PARSER_TIME_FORMAT_UNKNOWN:
          goto exit;

        case PARSER_TIME_FORMAT_DATE_AND_TIME:
          minutes = (end - start) / 60;
          parser_add_int32(parser, ID_APPOINTMENT_TYPE, APPOINTMENT_TYPE_NORMAL);
          break;

        case PARSER_TIME_FORMAT_ONLY_DATE:
          minutes = (end - start - SECONDS_PER_DAY) / 60 + 1;
          parser_add_int32(parser, ID_APPOINTMENT_TYPE, APPOINTMENT_TYPE_ALL_DAY);
          break;
      }

      parser_add_int32(parser, ID_DURATION, minutes);
    }

#if ENABLE_RECURRENCE
    if (!recurrence_parse_rrule(
          parser, 
          event_parser_data.rrule, 
          event_parser_data.dtstart,
          event_parser_data.dtend))
    {
      synce_error("Failed to parse recurrence rule");
      goto exit;
    }
#endif
  }

  if (!event_parser_data.has_alarm)
  {
    /* default stuff */
    parser_add_int16 (parser, ID_REMINDER_ENABLED, 0);
    parser_add_int32 (parser, ID_REMINDER_MINUTES_BEFORE_START, 15);
    parser_add_int32 (parser, ID_REMINDER_OPTIONS, REMINDER_LED|REMINDER_DIALOG|REMINDER_SOUND);
    parser_add_string(parser, ID_REMINDER_SOUND_FILE, "Alarm1.wav");
  }
  
  /* The calendar application on my HP 620LX just hangs without this! */
  parser_add_int32(parser, ID_UNKNOWN_0002, 0);

  if (!parser_get_result(parser, data, data_size))
  {
    synce_error("Failed to retrieve result");
    goto exit;
  }
  
 	success = true;

exit:
  /* destroy components (the order is important!) */
  parser_component_destroy(base);
  parser_component_destroy(calendar);
  parser_component_destroy(event);
  parser_component_destroy(alarm);
  parser_destroy(parser);
	return success;
}/*}}}*/


