/* $Id$ */
#define _BSD_SOURCE 1
#include "librra.h"
#include "generator.h"
#include "parser.h"
#include "task_ids.h"
#include "common_handlers.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <string.h>

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

/*
   Any on_propval_* functions not here are found in common_handlers.c
*/

static bool on_propval_completed(Generator* g, CEPROPVAL* propval, void* cookie)
{
  bool success = false;
  
  switch (propval->propid & 0xffff)
  {
    case CEVT_FILETIME:
      success = true;
      break;

    case CEVT_I2:
      if (propval->val.iVal)
      {
        generator_add_simple(g, "PERCENT-COMPLETE", "100");
        generator_add_simple(g, "STATUS",           "COMPLETED");
      }
      success = true;
      break;

    default:
      synce_error("Unexpected data type: %08x", propval->propid);
      break;
  }

  return success;
}

static bool on_propval_due(Generator* g, CEPROPVAL* propval, void* cookie)
{
  time_t due_time = 
    filetime_to_unix_time(&propval->val.filetime);

  if (due_time > 0)
  {
    char date[16];
    strftime(date, sizeof(date), "%Y%m%d", gmtime(&due_time));
    generator_add_with_type(g, "DUE", "DATE", date);
  }
  return true;
}

static bool on_propval_importance(Generator* g, CEPROPVAL* propval, void* cookie)
{
  /* TODO: set PRIORITY */
  return true;
}

static bool on_propval_start(Generator* g, CEPROPVAL* propval, void* cookie)
{
  time_t start_time = 
    filetime_to_unix_time(&propval->val.filetime);

  if (start_time > 0)
  {
    char date[16];
    strftime(date, sizeof(date), "%Y%m%d", gmtime(&start_time));
    generator_add_with_type(g, "DTSTART", "DATE", date);
  }
  return true;
}

bool rra_task_to_vtodo(
    uint32_t id,
    const uint8_t* data,
    size_t data_size,
    char** vtodo,
    uint32_t flags)
{
  bool success = false;
  Generator* generator = NULL;
  unsigned generator_flags = 0;
  /*EventGeneratorData event_generator_data;
    memset(&event_generator_data, 0, sizeof(EventGeneratorData));*/

  switch (flags & RRA_TASK_CHARSET_MASK)
  {
    case RRA_TASK_UTF8:
      generator_flags = GENERATOR_UTF8;
      break;

    case RRA_TASK_ISO8859_1:
    default:
      /* do nothing */
      break;
  }

  generator = generator_new(generator_flags, NULL/*&event_generator_data*/);
  if (!generator)
    goto exit;

  generator_add_property(generator, ID_TASK_DUE,        on_propval_due);
  generator_add_property(generator, ID_IMPORTANCE,      on_propval_importance);
  generator_add_property(generator, ID_NOTES,           on_propval_notes);
  generator_add_property(generator, ID_SENSITIVITY,     on_propval_sensitivity);
  generator_add_property(generator, ID_TASK_COMPLETED,  on_propval_completed);
  generator_add_property(generator, ID_TASK_START,      on_propval_start);
  generator_add_property(generator, ID_SUBJECT,         on_propval_subject);

  if (!generator_set_data(generator, data, data_size))
    goto exit;

  generator_add_simple(generator, "BEGIN", "VTODO");

  if (id != RRA_TASK_ID_UNKNOWN)
  {
    char id_str[32];
    snprintf(id_str, sizeof(id_str), "RRA-ID-%08x", id);
    generator_add_simple(generator, "UID", id_str);
  }
  if (!generator_run(generator))
    goto exit;

  generator_add_simple(generator, "END", "VTODO");

  if (!generator_get_result(generator, vtodo))
    goto exit;

  success = true;

exit:
  generator_destroy(generator);
  return success;
}

/*
   Any on_mdir_line_* functions not here are found in common_handlers.c
*/

static bool on_mdir_line_due(Parser* p, mdir_line* line, void* cookie)
{
  return parser_add_time_from_line(p, ID_TASK_DUE, line);
}

static bool on_mdir_line_dtstart(Parser* p, mdir_line* line, void* cookie)
{
  return parser_add_time_from_line(p, ID_TASK_START, line);
}

static bool on_mdir_line_status(Parser* p, mdir_line* line, void* cookie)
{
  if (STR_EQUAL(line->values[0], "completed"))
    return parser_add_int16(p, ID_TASK_COMPLETED, 1);
  else
    return parser_add_int16(p, ID_TASK_COMPLETED, 0);
}


bool rra_task_from_vtodo(
    const char* vtodo,
    uint32_t* id,
    uint8_t** data,
    size_t* data_size,
    uint32_t flags)
{
	bool success = false;
  Parser* parser = NULL;
  ParserComponent* base;
  ParserComponent* calendar;
  ParserComponent* todo;
/*  ParserComponent* alarm;*/
  int parser_flags = 0;
  /*EventParserData event_parser_data;
  memset(&event_parser_data, 0, sizeof(EventParserData));*/

  switch (flags & RRA_TASK_CHARSET_MASK)
  {
    case RRA_TASK_UTF8:
      parser_flags = PARSER_UTF8;
      break;

    case RRA_TASK_ISO8859_1:
    default:
      /* do nothing */
      break;
  }

/*  alarm = parser_component_new("vAlarm");

  parser_component_add_parser_property(alarm, 
      parser_property_new("trigger", on_alarm_trigger));*/

  todo = parser_component_new("vTodo");
/*  parser_component_add_parser_component(todo, alarm);*/

  parser_component_add_parser_property(todo, 
      parser_property_new("Class", on_mdir_line_class));
  parser_component_add_parser_property(todo, 
      parser_property_new("dtStart", on_mdir_line_dtstart));
  parser_component_add_parser_property(todo, 
      parser_property_new("Due", on_mdir_line_due));
  parser_component_add_parser_property(todo, 
      parser_property_new("Location", on_mdir_line_location));
  parser_component_add_parser_property(todo, 
      parser_property_new("Description", on_mdir_line_description));
  parser_component_add_parser_property(todo, 
      parser_property_new("Status", on_mdir_line_status));
  parser_component_add_parser_property(todo, 
      parser_property_new("Summary", on_mdir_line_summary));
/*  parser_component_add_parser_property(todo, 
      parser_property_new("Transp", on_mdir_line_transp));*/

  calendar = parser_component_new("vCalendar");
  parser_component_add_parser_component(calendar, todo);

  /* allow parsing to start with either a vCalendar or a vTodo */
  base = parser_component_new(NULL);
  parser_component_add_parser_component(base, calendar);
  parser_component_add_parser_component(base, todo);

  parser = parser_new(base, parser_flags, NULL /*&event_parser_data*/);
  if (!parser)
  {
    synce_error("Failed to create parser");
    goto exit;
  }

  if (!parser_set_mimedir(parser, vtodo))
  {
    synce_error("Failed to parse input data");
    goto exit;
  }

  if (!parser_run(parser))
  {
    synce_error("Failed to convert input data");
    goto exit;
  }

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
  parser_component_destroy(todo);
  parser_destroy(parser);
	return success;
}


