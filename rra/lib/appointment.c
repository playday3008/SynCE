/* $Id$ */
#define _GNU_SOURCE 1
#include "librra.h"
#include "appointment_ids.h"
#include "parser.h"
#include <synce_log.h>
#include <string.h>

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

#define MINUTES_PER_DAY (24*60)

#if 0
#include "dbstream.h"
#include <rapi.h>
#include <libmimedir.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define MAX_FIELD_COUNT 50 /* just a guess */

#define SECONDS_PER_MINUTE  (60)
#define SECONDS_PER_HOUR    (SECONDS_PER_MINUTE * 60)
#define SECONDS_PER_DAY     (SECONDS_PER_HOUR   * 24)
#define SECONDS_PER_WEEK    (SECONDS_PER_DAY    * 7)

struct _AppointmentToVevent/*{{{*/
{
  uint32_t id;
	CEPROPVAL* fields;
	uint32_t field_count;
  char** vevent;
  uint32_t flags;
};/*}}}*/

typedef struct _AppointmentToVevent AppointmentToVevent;

bool rra_appointment_to_vevent(/*{{{*/
    uint32_t id,
    const uint8_t* data,
    size_t data_size,
    char** vevent,
    uint32_t flags)
{
	bool success = false;
	uint32_t field_count = 0;
	CEPROPVAL* fields = NULL;

	if (!data)
	{
		synce_error("Data is NULL");
		goto exit;
	}

	if (data_size < 8)
	{
		synce_error("Invalid data size");
		goto exit;
	}

	field_count = letoh32(*(uint32_t*)(data + 0));
	synce_trace("Field count: %i", field_count);

	if (0 == field_count)
	{
		synce_error("No fields!");
		goto exit;
	} 
	
	if (field_count > MAX_FIELD_COUNT)
	{
		synce_error("A contact does not have this many fields");
		goto exit;
	}

	fields = (CEPROPVAL*)malloc(sizeof(CEPROPVAL) * field_count);

	if (!dbstream_to_propvals(data + 8, field_count, fields))
	{
		fprintf(stderr, "Failed to convert database stream\n");
		goto exit;
	}

	if (!rra_contact_to_vcard2(
				id, 
				fields, 
				field_count, 
				vcard,
				flags))
	{
		fprintf(stderr, "Failed to create vCard\n");
		goto exit;
	}

	success = true;

exit:
	dbstream_free_propvals(fields);
	return success;
}/*}}}*/

#endif


/***********************************************************************

  Conversion from vEvent to Appointment

 ***********************************************************************/

typedef struct _AppointmentData
{
  bool have_alarm;
  mdir_line* dtstart;
  mdir_line* dtend;
} AppointmentData;

static bool on_alarm_trigger(Parser* p, mdir_line* line, void* cookie)/*{{{*/
{
  AppointmentData* appointment_data = (AppointmentData*)cookie;

  if (appointment_data->have_alarm)
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
    appointment_data->have_alarm = true;
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
  AppointmentData* appointment_data = (AppointmentData*)cookie;
  appointment_data->dtend = line;
  return true;
}

static bool on_event_dtstart(Parser* p, mdir_line* line, void* cookie)
{
  AppointmentData* appointment_data = (AppointmentData*)cookie;
  appointment_data->dtstart = line;
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
  ComponentType* base;
  ComponentType* calendar;
  ComponentType* event;
  ComponentType* alarm;
  int parser_flags = PARSER_UTF8; /* XXX */
  AppointmentData appointment_data;
  memset(&appointment_data, 0, sizeof(AppointmentData));

  alarm = component_type_new("vAlarm");

  component_type_add_property_type(alarm, 
      property_type_new("trigger", on_alarm_trigger));

  event = component_type_new("vEvent");
  component_type_add_component_type(event, alarm);

  component_type_add_property_type(event, 
      property_type_new("class", on_event_class));
  component_type_add_property_type(event, 
      property_type_new("dtEnd", on_event_dtend));
  component_type_add_property_type(event, 
      property_type_new("dtStart", on_event_dtstart));
  component_type_add_property_type(event, 
      property_type_new("Location", on_event_location));
  component_type_add_property_type(event, 
      property_type_new("Summary", on_event_summary));
  component_type_add_property_type(event, 
      property_type_new("Transp", on_event_transp));

  calendar = component_type_new("vCalendar");
  component_type_add_component_type(calendar, event);

  base = component_type_new(NULL);
  component_type_add_component_type(base, calendar);

  parser = parser_new(base, parser_flags, &appointment_data);
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

  if (appointment_data.dtstart)
  {
    parser_add_time_from_line(parser, ID_START, appointment_data.dtstart);
    
    if (appointment_data.dtend)
    {
      time_t start = 0;
      time_t end = 0;
      int32_t minutes;

      if (!parser_datetime_to_unix_time(appointment_data.dtstart->values[0], &start))
        goto exit;
      if (!parser_datetime_to_unix_time(appointment_data.dtend->values[0],   &end))
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
  component_type_destroy(calendar);
  parser_destroy(parser);
	return success;
}/*}}}*/


