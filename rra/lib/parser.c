/* $Id$ */
#define _GNU_SOURCE
#include "parser.h"
#include "dbstream.h"
#include <synce_log.h>
#include <synce_hash.h>
#include <rapi.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <synce_log.h>
#include <assert.h>

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

#define MAX_PROPVAL_COUNT         50

/* for parser_component_new() */
#define parser_component_HASH_SIZE   5
#define parser_property_HASH_SIZE    5

/* for parser_duration_to_seconds() */
#define SECONDS_PER_MINUTE  (60)
#define SECONDS_PER_HOUR    (SECONDS_PER_MINUTE * 60)
#define SECONDS_PER_DAY     (SECONDS_PER_HOUR   * 24)
#define SECONDS_PER_WEEK    (SECONDS_PER_DAY    * 7)

struct _ParserProperty
{
  char* name;
  ParserPropertyFunc func;
};

struct _ParserComponent
{
  char* name;
  SHashTable* parser_components;
  SHashTable* parser_properties;
};

struct _Parser
{
  ParserComponent* base_parser_component;
  int flags;
  void* cookie;
  mdir_line** mimedir;
  mdir_line** iterator;
  CEPROPVAL propvals[MAX_PROPVAL_COUNT];
	size_t propval_count;
};

bool parser_duration_to_seconds(const char* duration, int* result)/*{{{*/
{
  enum { dur_sign, dur_p, dur_any, dur_time, dur_end } state = dur_sign;
  const char *p;
  struct tm time_struct;
  int sign = 1;
  int value = 0;
  int seconds = 0;

  memset(&time_struct, 0, sizeof(time_struct));

  for (p = duration; *p; p++)
  {
    switch (state)
    {
      case dur_sign:/*{{{*/
        switch (*p)
        {
          case '+':
            /* default sign */
            state = dur_p;
            break;

          case '-':
            sign = -1;
            state = dur_p;
            break;

          case 'P':
            /* use default sign */
            state = dur_any;
            break;

          default:
            synce_error("Unexpected char '%c' at offset %i in duration '%s'",
                *p, p - duration, duration);
            return false;
        }
        break;/*}}}*/

      case dur_p:
        if ('P' == *p)
          state = dur_any;
        else
        {
          synce_error("Unexpected char '%c' at offset %i in duration '%s'",
              *p, p - duration, duration);
          return false;
        }
        break;

      case dur_any:/*{{{*/
        switch (*p)
        {
          case 'W':
            seconds += value * SECONDS_PER_WEEK;
            state = dur_end;
            break;
          case 'D':
            seconds += value * SECONDS_PER_DAY;
            value = 0;
            break;
          case 'T':
            state = dur_time;
            break;

          default:
            if (isdigit(*p))
            {
              value = (value * 10) + (*p - '0');
            }
            else 
            {
              synce_error("Unexpected char '%c' at offset %i in duration '%s'",
                  *p, p - duration, duration);
              return false;
            }
            break;
        }
        break;/*}}}*/

      case dur_time:/*{{{*/
        switch (*p)
        {
          case 'H':
            seconds += value * SECONDS_PER_HOUR;
            value = 0;
            break;
          case 'M':
            seconds += value * SECONDS_PER_MINUTE;
            value = 0;
            break;
          case 'S':
            seconds += value;
            state = dur_end;
            break;

          default:
            if (isdigit(*p))
            {
              value = (value * 10) + (*p - '0');
            }
            else 
            {
              synce_error("Unexpected char '%c' at offset %i in duration '%s'",
                  *p, p - duration, duration);
              return false;
            }
            break;
        }
        break;/*}}}*/

      case dur_end:
        synce_error("Unexpected char '%c' at offset %i in duration '%s'",
            *p, p - duration, duration);
        return false;
    }
  }

  *result = seconds * sign;
  return true;

}/*}}}*/

bool parser_datetime_to_unix_time(const char* datetime, time_t* unix_time)/*{{{*/
{
  int count;
  char suffix = 0;
  struct tm time_struct;
  memset(&time_struct, 0, sizeof(time_struct));

  count = sscanf(datetime, "%4d%2d%2dT%2d%2d%2d%1c", 
      &time_struct.tm_year,
      &time_struct.tm_mon,
      &time_struct.tm_mday,
      &time_struct.tm_hour,
      &time_struct.tm_min,
      &time_struct.tm_sec,
      &suffix);

  if (count != 3 && count != 6 && count != 7)
  {
    synce_error("Bad date-time: '%s'", datetime);
    return -1;
  }

  if (count >= 7)
  {
    if ('Z' != suffix)
      synce_warning("Unknown date-time suffix: '%c'", suffix);
  }

  time_struct.tm_year -= 1900;
  time_struct.tm_mon--;
  time_struct.tm_isdst = -1;

  *unix_time = mktime(&time_struct);

  return -1 != *unix_time;
}/*}}}*/

static CEPROPVAL* parser_get_next_propval(Parser* self)/*{{{*/
{
  if (MAX_PROPVAL_COUNT == self->propval_count)
  {
    synce_error("Too many propvals.");
    return NULL;
  }
  else
    return &self->propvals[self->propval_count++];
}/*}}}*/

bool parser_add_blob(Parser* self, uint16_t id, uint8_t* data, size_t data_size)/*{{{*/
{
  CEPROPVAL* propval = parser_get_next_propval(self);
  if (!propval)
    return false;

  assert(data);

  propval->propid = (id << 16) | CEVT_BLOB;
  propval->val.blob.dwCount = data_size;
  propval->val.blob.lpb     = malloc(data_size);
  assert(propval->val.blob.lpb);
  memcpy(propval->val.blob.lpb, data, data_size);

  return true;
}/*}}}*/

bool parser_add_int16(Parser* self, uint16_t id, int16_t value)/*{{{*/
{
  CEPROPVAL* propval = parser_get_next_propval(self);
  if (!propval)
    return false;

  propval->propid = (id << 16) | CEVT_I2;
  propval->val.iVal = value;

  return true;
}/*}}}*/

bool parser_add_int32 (Parser* self, uint16_t id, int32_t value)/*{{{*/
{
  CEPROPVAL* propval = parser_get_next_propval(self);
  if (!propval)
    return false;

  propval->propid = (id << 16) | CEVT_I4;
  propval->val.lVal = value;

  return true;
}/*}}}*/

bool parser_add_string(Parser* self, uint16_t id, const char* str)/*{{{*/
{
 CEPROPVAL* propval = parser_get_next_propval(self);
  if (!propval)
    return false;

  propval->propid = (id << 16) | CEVT_LPWSTR;

  if (self->flags & PARSER_UTF8)
    propval->val.lpwstr = wstr_from_utf8(str);
  else
    propval->val.lpwstr = wstr_from_ascii(str);

  return true;
}/*}}}*/

bool parser_add_time  (Parser* self, uint16_t id, time_t value)/*{{{*/
{
  CEPROPVAL* propval = parser_get_next_propval(self);
  if (!propval)
    return false;

  propval->propid = (id << 16) | CEVT_FILETIME;

  filetime_from_unix_time(value, &propval->val.filetime);

  return true;
}/*}}}*/

bool parser_add_string_from_line(Parser* self, uint16_t id, mdir_line* line)/*{{{*/
{
  return parser_add_string(self, id, line->values[0]);
}/*}}}*/

ParserTimeFormat parser_get_time_format(mdir_line* line)
{
  ParserTimeFormat format = PARSER_TIME_FORMAT_DATE_AND_TIME; /* default */
  char** types = mdir_get_param_values(line, "VALUE");
  
  if (types && types[0])
  {
    if (STR_EQUAL(types[0], "DATE"))
    {
      format = PARSER_TIME_FORMAT_ONLY_DATE;
    }
    else if (!STR_EQUAL(types[0], "DATE-TIME"))
    {
      synce_warning("Unknown data type: '%s'", types[0]);
      format = PARSER_TIME_FORMAT_UNKNOWN;
    }
  }

  return format;
}

bool parser_add_time_from_line  (Parser* self, uint16_t id, mdir_line* line)/*{{{*/
{
  bool success = false;
  time_t some_time;

  ParserTimeFormat format = parser_get_time_format(line);

  if (format == PARSER_TIME_FORMAT_DATE_AND_TIME ||
      format == PARSER_TIME_FORMAT_ONLY_DATE)
  {
    success = parser_datetime_to_unix_time(line->values[0], &some_time);
  }

  return success && parser_add_time(self, id, some_time);
}/*}}}*/

ParserProperty* parser_property_new(const char* name, ParserPropertyFunc func)/*{{{*/
{
  ParserProperty* self = (ParserProperty*)calloc(1, sizeof(ParserProperty));
  
  if (self)
  {
    self->name = name ? strdup(name) : NULL;
    self->func = func;
  }
  
  return self;
}/*}}}*/

void parser_property_destroy(ParserProperty* self)/*{{{*/
{
  if (self)
  {
    free(self->name);
    free(self);
  }
}/*}}}*/

ParserComponent* parser_component_new(const char* name)/*{{{*/
{
  ParserComponent* self = (ParserComponent*)calloc(1, sizeof(ParserComponent));
  
  if (self)
  {
    self->name = name ? strdup(name) : NULL;
    
    self->parser_components = s_hash_table_new(s_str_hash, s_str_equal_no_case,
        parser_component_HASH_SIZE);
    self->parser_properties  = s_hash_table_new(s_str_hash, s_str_equal_no_case, 
        parser_property_HASH_SIZE);
  }

  return self;
}/*}}}*/

void parser_component_destroy(ParserComponent* self)/*{{{*/
{
  if (self)
  {
    s_hash_table_destroy(self->parser_components, NULL); /*(void (*)(void *))parser_component_destroy); */
    s_hash_table_destroy(self->parser_properties,  (void (*)(void *))parser_property_destroy);
    if (self->name)
      free(self->name);
    free(self);
  }
}/*}}}*/

void parser_component_add_parser_component(ParserComponent* self, ParserComponent* ct)/*{{{*/
{
  s_hash_table_insert(self->parser_components, ct->name, ct);
}/*}}}*/

void parser_component_add_parser_property (ParserComponent* self, ParserProperty* pt)/*{{{*/
{
  s_hash_table_insert(self->parser_properties, pt->name, pt);
}/*}}}*/

ParserComponent* parser_component_get_parser_component(ParserComponent* self, const char* name)/*{{{*/
{
  if (self && name)
    return (ParserComponent*)s_hash_table_lookup(self->parser_components, name);
  else
    return NULL;
}/*}}}*/

ParserProperty* parser_component_get_parser_property(ParserComponent* self, const char* name)/*{{{*/
{
  if (self && name)
    return (ParserProperty*)s_hash_table_lookup(self->parser_properties, name);
  else
    return NULL;
}/*}}}*/

static bool parser_handle_component(Parser* p, ParserComponent* ct)/*{{{*/
{ 
  bool success = false;
  mdir_line* line = NULL;

  while ( (line = *p->iterator++) )
  {
    if (STR_EQUAL(line->name, "BEGIN"))
    {
      bool result;
      ParserComponent* other =
        parser_component_get_parser_component(ct, line->values[0]);

      if (other)
      {
        result = parser_handle_component(p, other);
      }
      else
      {
        /* create and use temporary component type */
        /*synce_trace("Handling unknown component '%s'", line->values[0]);*/
        other = parser_component_new(line->values[0]);
        result = parser_handle_component(p, other);
        parser_component_destroy(other);
      }

      if (!result)
      {
        synce_error("Failed to handle component '%s'", line->values[0]);
        break;
      }
    }
    else if (STR_EQUAL(line->name, "END"))
    {
      if (STR_EQUAL(line->values[0], ct->name))
      {
        success = true;
        break;
      }
      else
      {
        synce_error("Unexpected END: '%s'", line->values[0]);
        break;
      }
    }
    else
    {
      ParserProperty* pt = 
        parser_component_get_parser_property(ct, line->name);
      
      if (pt)
      {
        if (!pt->func(p, line, p->cookie))
        {
          synce_error("Failed to handle property '%s'", line->name);
          break;
        }
      }
      /*else
      {
        synce_trace("Property '%s' not handled", line->name);
      }*/
    }
  }

  /* no more lines, success! */
  if (!line)
    success = true;

  return success;
}/*}}}*/

Parser* parser_new(ParserComponent* base_parser_component, int flags, void* cookie)/*{{{*/
{
  Parser* self = (Parser*)calloc(1, sizeof(Parser));

  if (self)
  {
    self->base_parser_component = base_parser_component;
    self->flags = flags;
    self->cookie = cookie;
  }

  return self;
}/*}}}*/

bool parser_set_mimedir(Parser* self, const char* mimedir)/*{{{*/
{
  if (self->mimedir)
    return false;

  self->iterator = self->mimedir = mdir_parse((char*)mimedir);

  return NULL != self->mimedir;
}/*}}}*/

void parser_destroy(Parser* self)/*{{{*/
{
  if (self)
  {
    size_t i;

    /* free strings in propvals array */
    for (i = 0; i < self->propval_count; i++)
    {
      switch (self->propvals[i].propid & 0xffff)
      {
        case CEVT_BLOB:
          assert(self->propvals[i].val.blob.lpb);
          free(self->propvals[i].val.blob.lpb);
          break;

        case CEVT_LPWSTR:
          wstr_free_string(self->propvals[i].val.lpwstr);
          break;
      }
    }
    
    mdir_free(self->mimedir);
    free(self);
  }
}/*}}}*/

bool parser_run(Parser* self)/*{{{*/
{
  bool success = false;
  
  if (!self || !self->mimedir || self->propval_count)
  {
    synce_error("Invalid parser state");
    goto exit;
  }

  if (!parser_handle_component(self, self->base_parser_component))
  {
    synce_error("Failed to parse components");
    goto exit;
  }

  success = true;

exit:
  return success;
}/*}}}*/

bool parser_get_result(Parser* self, uint8_t** result, size_t* result_size)/*{{{*/
{
  if (self && self->propval_count && result && result_size)
    return dbstream_from_propvals(
        self->propvals,
        self->propval_count,
        result,
        result_size);
  else
    return false;
}/*}}}*/
