/* $Id$ */
#define _GNU_SOURCE 1
#include "librra.h"
#include "appointment_ids.h"
#include "generator.h"
#include "parser.h"
#include <rapi.h>
#include <synce_log.h>
#include <string.h>

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

#define MINUTES_PER_DAY (24*60)

typedef struct _EventGeneratorData
{
  CEPROPVAL* dummy;
} EventGeneratorData;

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

  if (!generator_set_data(generator, data, data_size))
    goto exit;

  if (!generator_run(generator))
    goto exit;


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


