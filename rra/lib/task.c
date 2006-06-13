/* $Id$ */
#define _BSD_SOURCE 1
#include "task.h"
#include "task_ids.h"
#include "timezone.h"
#include "generator.h"
#include "parser.h"
#include "common_handlers.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <string.h>

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

#define DEFAULT_REMINDER_MINUTES_BEFORE_START 0
#define DEFAULT_REMINDER_SOUND_FILE   "Alarm1.wav"
#define DEFAULT_REMINDER_ENABLED      0
#define DEFAULT_REMINDER_OPTIONS      0

static uint8_t invalid_filetime_buffer[] = 
{0x00, 0x40, 0xdd, 0xa3, 0x57, 0x45, 0xb3, 0x0c};

/*
   Any on_propval_* functions not here are found in common_handlers.c
*/

typedef struct
{
  bool completed;
  FILETIME completed_time;
} TaskGeneratorData;

static bool on_propval_completed(Generator* g, CEPROPVAL* propval, void* cookie)
{
  bool success = false;
  TaskGeneratorData* data = (TaskGeneratorData*)cookie;

  switch (propval->propid & 0xffff)
  {
    case CEVT_FILETIME:
      data->completed_time = propval->val.filetime;
      success = true;
      break;

    case CEVT_I2:
      data->completed = propval->val.iVal;
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
  char date[9];

  parser_filetime_to_datetime(&propval->val.filetime , date, sizeof(date));

  generator_add_with_type(g, "DUE", "DATE", date);

  return true;
}

static bool on_propval_importance(Generator* g, CEPROPVAL* propval, void* cookie)
{
  switch(propval->val.iVal)
  {
  case IMPORTANCE_HIGH:
    generator_add_simple(g, "PRIORITY", "3");
    break;
  case IMPORTANCE_NORMAL:
    generator_add_simple(g, "PRIORITY", "5");
    break;
  case IMPORTANCE_LOW:
    generator_add_simple(g, "PRIORITY", "7");
    break;
  }
  return true;
}

static bool on_propval_start(Generator* g, CEPROPVAL* propval, void* cookie)
{
  char date[9];

  parser_filetime_to_datetime(&propval->val.filetime , date, sizeof(date));

  generator_add_with_type(g, "DTSTART", "DATE", date);

  return true;
}

bool rra_task_to_vtodo(
    uint32_t id,
    const uint8_t* data,
    size_t data_size,
    char** vtodo,
    uint32_t flags,
    RRA_Timezone* tzi)
{
  bool success = false;
  Generator* generator = NULL;
  unsigned generator_flags = 0;
  TaskGeneratorData task_generator_data;
  memset(&task_generator_data, 0, sizeof(TaskGeneratorData));

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

  generator = generator_new(generator_flags, &task_generator_data);
  if (!generator)
    goto exit;

  generator_add_property(generator, ID_TASK_CATEGORIES, on_propval_categories);
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

  if (task_generator_data.completed)
  {
    char date[9];

    generator_add_simple(generator, "PERCENT-COMPLETE", "100");
    generator_add_simple(generator, "STATUS",           "COMPLETED");

    parser_filetime_to_datetime(&task_generator_data.completed_time , date, sizeof(date));    

    generator_add_simple(generator, "COMPLETED", date);
  }

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

static bool on_mdir_line_completed(Parser* p, mdir_line* line, void* cookie)
{
  if (line)
  {
    return parser_add_time_from_line(p, ID_TASK_COMPLETED, line);
  }
  else
    return false; /*parser_add_time(p, ID_TASK_COMPLETED, time(NULL));*/
}

static bool on_mdir_line_due(Parser* p, mdir_line* line, void* cookie)
{
  if (line)
    return parser_add_time_from_line(p, ID_TASK_DUE, line);
  else
    return parser_add_filetime(p, ID_TASK_DUE, 
        (FILETIME*)invalid_filetime_buffer);
}

static bool on_mdir_line_dtstart(Parser* p, mdir_line* line, void* cookie)
{
  if (line)
    return parser_add_time_from_line(p, ID_TASK_START, line);
  else
    return parser_add_filetime(p, ID_TASK_START, 
        (FILETIME*)invalid_filetime_buffer);
}

static bool on_mdir_line_status(Parser* p, mdir_line* line, void* cookie)
{
  if (line && STR_EQUAL(line->values[0], "completed"))
    return parser_add_int16(p, ID_TASK_COMPLETED, 1);
  else
    return parser_add_int16(p, ID_TASK_COMPLETED, 0);
}

static bool on_mdir_line_importance(Parser* p, mdir_line* line, void* cookie)
{
  if (line)
  {
    if (STR_EQUAL(line->values[0], "1") ||
        STR_EQUAL(line->values[0], "2") ||
        STR_EQUAL(line->values[0], "3") ||
        STR_EQUAL(line->values[0], "4"))
      return parser_add_int32(p, ID_IMPORTANCE, IMPORTANCE_HIGH);
    else if (STR_EQUAL(line->values[0], "0") ||
             STR_EQUAL(line->values[0], "5"))
      return parser_add_int32(p, ID_IMPORTANCE, IMPORTANCE_NORMAL);
    else if (STR_EQUAL(line->values[0], "6") ||
             STR_EQUAL(line->values[0], "7") ||
             STR_EQUAL(line->values[0], "8") ||
             STR_EQUAL(line->values[0], "9"))
      return parser_add_int32(p, ID_IMPORTANCE, IMPORTANCE_LOW);
    else
      synce_warning("Unknown value for importance: '%s'", line->values[0]);
      return false;
  }
  else
    return parser_add_int32(p, ID_IMPORTANCE, IMPORTANCE_NORMAL);
}

bool rra_task_from_vtodo(
    const char* vtodo,
    uint32_t* id,
    uint8_t** data,
    size_t* data_size,
    uint32_t flags,
    RRA_Timezone* tzi)
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
      parser_property_new("Categories", on_mdir_line_categories));
  parser_component_add_parser_property(todo, 
      parser_property_new("Class", on_mdir_line_class));
  parser_component_add_parser_property(todo, 
      parser_property_new("Completed", on_mdir_line_completed));
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
  parser_component_add_parser_property(todo, 
      parser_property_new("Priority", on_mdir_line_importance));
/*  parser_component_add_parser_property(todo, 
      parser_property_new("Transp", on_mdir_line_transp));*/

  calendar = parser_component_new("vCalendar");
  parser_component_add_parser_component(calendar, todo);

  /* allow parsing to start with either a vCalendar or a vTodo */
  base = parser_component_new(NULL);
  parser_component_add_parser_component(base, calendar);
  parser_component_add_parser_component(base, todo);

  parser = parser_new(base, parser_flags, tzi, NULL /*&event_parser_data*/);
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
    
  /* This must always be included */
  parser_add_string(parser, 
      ID_REMINDER_SOUND_FILE, 
      DEFAULT_REMINDER_SOUND_FILE);
  
#if 0
  /* Default alarm stuff until we handle vAlarm */
  parser_add_int32(parser, 
      ID_REMINDER_MINUTES_BEFORE_START, 
      DEFAULT_REMINDER_MINUTES_BEFORE_START);
  parser_add_int16(parser, 
      ID_REMINDER_ENABLED, 
      DEFAULT_REMINDER_ENABLED);
  parser_add_int32(parser, 
      ID_REMINDER_OPTIONS, 
      DEFAULT_REMINDER_OPTIONS);

  /* Add these just for sure */
  parser_add_int32(parser, ID_UNKNOWN_0002, 0);
  parser_add_int16(parser, ID_UNKNOWN_0003, 0);
  parser_add_int16(parser, ID_UNKNOWN_0005, 0);
  parser_add_int32(parser, ID_IMPORTANCE, 0);
  parser_add_int16(parser, ID_UNKNOWN_4126, 0);
#endif

  parser_call_unused_properties(parser);
  
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


