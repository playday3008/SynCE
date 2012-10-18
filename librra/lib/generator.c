/* $Id$ */
#define _GNU_SOURCE 1
#include "generator.h"
#include "parser.h"
#include "dbstream.h"
#include "strbuf.h"
#include <rapi.h>
#include <synce_log.h>
#include <synce_hash.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MAX_PROPVAL_COUNT         50

struct _GeneratorProperty
{
  GeneratorPropertyFunc func;
  uint16_t id;
};

typedef enum _LineState
{
  STATE_IDLE,
  STATE_PARAM_OR_VALUE,
  STATE_FIRST_PARAM,
  STATE_NEXT_PARAM,
  STATE_VALUE
} LineState;

struct _Generator
{
  int flags;
  void* cookie;
  SHashTable* properties;
  StrBuf* buffer;
  CEPROPVAL* propvals;
  size_t propval_count;
  LineState state;
};

static unsigned id_hash(const void* id)
{
  return *(const uint16_t*)id;
}

static int id_equal(const void* a, const void* b)
{
  return *(const uint16_t*)a == *(const uint16_t*)b;
}

Generator* generator_new(int flags, void* cookie)/*{{{*/
{
  Generator* self = (Generator*)calloc(1, sizeof(Generator));

  if (self)
  {
    self->flags       = flags;
    self->cookie      = cookie;
    self->properties  = s_hash_table_new(id_hash, id_equal, 20);
    self->buffer      = strbuf_new(NULL);
    self->state       = STATE_IDLE;
  }

  return self;
}/*}}}*/

void generator_destroy(Generator* self)/*{{{*/
{
  if (self)
  {
    s_hash_table_destroy(self->properties, free);
    strbuf_destroy(self->buffer, true);
    if (self->propvals)
      free(self->propvals);
    free(self);
  }
}/*}}}*/

bool generator_utf8(Generator* self)
{
  if (self)
    return self->flags & GENERATOR_UTF8;
  else
    return false;
}

bool generator_set_data(Generator* self, const uint8_t* data, size_t data_size)/*{{{*/
{
  bool success = false;

  if (!data)
  {
    synce_error("RRA Calendar data is NULL");
    goto exit;
  }

  if (data_size < 8)
  {
    synce_error("Invalid data size for RRA calendar data");
    goto exit;
  }

  self->propval_count = letoh32(*(uint32_t*)(data + 0));
  synce_trace("RRA calendar data field count: %i", self->propval_count);

  if (0 == self->propval_count)
  {
    synce_error("No fields in RRA calendar record!");
    goto exit;
  } 
	
  if (self->propval_count > MAX_PROPVAL_COUNT)
  {
    synce_error("Too many fields in RRA calendar record");
    goto exit;
  }

  self->propvals = (CEPROPVAL*)malloc(sizeof(CEPROPVAL) * self->propval_count);

  if (!dbstream_to_propvals(data + 8, self->propval_count, self->propvals))
  {
    synce_error("Failed to convert RRA calendar database stream");
    goto exit;
  }

  success = true;

exit:
  return success;
}/*}}}*/

void generator_add_property(Generator* self, uint16_t id, GeneratorPropertyFunc func)/*{{{*/
{
  GeneratorProperty* gp = (GeneratorProperty*)calloc(1, sizeof(GeneratorProperty));
  if (gp)
  {
    gp->id = id;
    gp->func = func;
    s_hash_table_insert(self->properties, &gp->id, gp);
  }
}/*}}}*/

bool generator_run(Generator* self)
{
  unsigned i;
  bool success = false;

  for (i = 0; i < self->propval_count; i++)
  {
    uint16_t id = self->propvals[i].propid >> 16;
    GeneratorProperty* gp = 
      (GeneratorProperty*)s_hash_table_lookup(self->properties, &id);

    if (gp)
    {
      if (!gp->func(self, &self->propvals[i], self->cookie))
        goto exit;
    }
    else
    {
      char *tmp_str;
      switch (self->propvals[i].propid & 0xffff)
      {
      case CEVT_BLOB:
        synce_trace("Generator: Unhandled property, id: %04x, type: BLOB", id);
        break;
      case CEVT_BOOL:
        if (self->propvals[i].val.boolVal == FALSE)
          synce_trace("Generator: Unhandled property, id: %04x, type: bool:FALSE", id);
        else
          synce_trace("Generator: Unhandled property, id: %04x, type: bool:TRUE", id);
        break;
      case CEVT_FILETIME:
        if ((0 == self->propvals[i].val.filetime.dwLowDateTime) && (0 == self->propvals[i].val.filetime.dwHighDateTime))
          synce_trace("Generator: Unhandled property, id: %04x, type: filetime:NULL", id);
        else
        {
          time_t start_time;
          char buffer[32];
          parser_filetime_to_unix_time(&self->propvals[i].val.filetime, &start_time);
          strftime(buffer, sizeof(buffer), "%Y%m%dT%H%M%SZ", gmtime(&start_time));
          synce_trace("Generator: Unhandled property, id: %04x, type: filetime:%08x %08x=%s", id, self->propvals[i].val.filetime.dwHighDateTime,
                      self->propvals[i].val.filetime.dwLowDateTime,
                      buffer);
        }
        break;
      case CEVT_I2:
        synce_trace("Generator: Unhandled property, id: %04x, type: I2:%d", id, self->propvals[i].val.iVal);
        break;
      case CEVT_I4:
        synce_trace("Generator: Unhandled property, id: %04x, type: I4:%d", id, self->propvals[i].val.iVal);
        break;
      case CEVT_LPWSTR:
        tmp_str = wstr_to_current(self->propvals[i].val.lpwstr);
        synce_trace("Generator: Unhandled property, id: %04x, type: WSTR:%s", id, tmp_str);
        free(tmp_str);
        break;
      case CEVT_R8:
        synce_trace("Generator: Unhandled property, id: %04x, type: R8", id);
        break;
      case CEVT_UI2:
        synce_trace("Generator: Unhandled property, id: %04x, type: UI2:%u", id, self->propvals[i].val.uiVal);
        break;
      case CEVT_UI4:
        synce_trace("Generator: Unhandled property, id: %04x, type: UI4:%u", id, self->propvals[i].val.uiVal);
        break;
      default:
        synce_trace("Generator: Unhandled property, id: %04x, unknown type: %u", id, (self->propvals[i].propid & 0xffff));
        break;
      }
    }
  }

  success = true;

exit:
  return success;
}

bool generator_get_result(Generator* self, char** result)
{
  *result = strdup(self->buffer->buffer);
  return NULL != *result;
}

/* 
   ESCAPED-CHAR = "\\" / "\;" / "\," / "\n" / "\N")
        ; \\ encodes \, \n or \N encodes newline
        ; \; encodes ;, \, encodes ,
*/
static void generator_append_escaped(Generator* self, const char* str)/*{{{*/
{
  const char* p;

  assert(self);
  assert(self->buffer);
  if (!str)
    return;

  for (p = str; *p; p++)
  {
    switch (*p)
    {
    case '\r': 				/* CR */
      /* ignore */
      break;		

    case '\n':				/* LF */
      strbuf_append_c(self->buffer, '\\');
      strbuf_append_c(self->buffer, 'n');
      break;

    case '\\':
    case ';':
    case ',':
      strbuf_append_c(self->buffer, '\\');
      /* fall through */

    default:
      strbuf_append_c(self->buffer, *p);
      break;
    }
  }
}/*}}}*/

void generator_append_escaped_wstr(Generator* self, const WCHAR* wstr)/*{{{*/
{
  assert(self);
  if (wstr)
  {
    char* str = NULL;

    if (self->flags & GENERATOR_UTF8)
      str = wstr_to_utf8(wstr);
    else
      str = wstr_to_ascii(wstr);

    generator_append_escaped(self, str);
    wstr_free_string(str);
  }
}/*}}}*/

bool generator_add_simple(Generator* self, const char* name, const char* value)/*{{{*/
{
  if (STATE_IDLE != self->state)
  {
    synce_error("Missing call to generator_end_line()");
    return false;
  }

  strbuf_append(self->buffer, name);
  strbuf_append_c(self->buffer, ':');
  generator_append_escaped(self, value);
  strbuf_append_crlf(self->buffer);
  return true;
}/*}}}*/

bool generator_add_simple_unescaped(Generator* self, const char* name, const char* value)/*{{{*/
{
  if (STATE_IDLE != self->state)
  {
    synce_error("Missing call to generator_end_line()");
    return false;
  }

  strbuf_append(self->buffer, name);
  strbuf_append_c(self->buffer, ':');
  strbuf_append(self->buffer, value);
  strbuf_append_crlf(self->buffer);
  return true;
}/*}}}*/

bool generator_add_with_type(Generator* self, const char* name, /*{{{*/
    const char* type, const char* value)
{
  if (STATE_IDLE != self->state)
  {
    synce_error("Missing call to generator_end_line()");
    return false;
  }

  strbuf_append(self->buffer, name);
  strbuf_append(self->buffer, ";VALUE=");
  strbuf_append(self->buffer, type);
  strbuf_append_c(self->buffer, ':');
  generator_append_escaped(self, value);
  strbuf_append_crlf(self->buffer);
  return true;
}/*}}}*/

bool generator_add_simple_propval(Generator* self, const char* name, CEPROPVAL* propval)/*{{{*/
{
  bool success = false;
  
  if (STATE_IDLE != self->state)
  {
    synce_error("Missing call to generator_end_line()");
    return false;
  }

  switch (propval->propid & 0xffff)
  {
    case CEVT_LPWSTR:
      /* do not add empty strings */
      if (propval->val.lpwstr[0])
      {
        strbuf_append(self->buffer, name);
        strbuf_append_c(self->buffer, ':');
        generator_append_escaped_wstr(self, propval->val.lpwstr);
        strbuf_append_crlf(self->buffer);
      }
      success = true;
      break;

    default:
      synce_error("Data type not handled");
      break;
  }
  
  return success;
}/*}}}*/

bool generator_begin_line(Generator* self, const char* name)/*{{{*/
{
  if (STATE_IDLE != self->state)
  {
    synce_error("Missing call to generator_end_line()");
    return false;
  }

  self->state = STATE_PARAM_OR_VALUE;
  
  strbuf_append(self->buffer, name);
  return true;
}/*}}}*/

bool generator_begin_parameter(Generator* self, const char* name)/*{{{*/
{
  if (STATE_PARAM_OR_VALUE != self->state)
  {
    synce_error("Invalid state: %i", self->state);
    return false;
  }

  self->state = STATE_FIRST_PARAM;

  strbuf_append_c(self->buffer, ';');
  strbuf_append(self->buffer, name);
  strbuf_append_c(self->buffer, '=');
  
  return true;
}/*}}}*/

bool generator_add_parameter_value(Generator* self, const char* value)/*{{{*/
{
  if (STATE_NEXT_PARAM == self->state)
  {
    strbuf_append_c(self->buffer, ',');
  }
  else if (STATE_FIRST_PARAM != self->state)
  {
    synce_error("Invalid state: %i", self->state);
    return false;
  }

  self->state = STATE_NEXT_PARAM;

  /* XXX: escape string?  */
  strbuf_append(self->buffer, value);

  return true;
}/*}}}*/

bool generator_end_parameter(Generator* self)/*{{{*/
{
  if (STATE_FIRST_PARAM == self->state)
  {
    synce_warning("Empty parameter");
  }
  else if (STATE_NEXT_PARAM != self->state)
  {
    synce_error("Invalid state: %i", self->state);
    return false;
  }

  self->state = STATE_PARAM_OR_VALUE;

  return true;
}/*}}}*/

bool generator_add_value(Generator* self, const char* value)/*{{{*/
{
  if (STATE_PARAM_OR_VALUE == self->state)
  {
    strbuf_append_c(self->buffer, ':');
  }
  else if (STATE_VALUE == self->state)
  {
    strbuf_append_c(self->buffer, ',');
  }
  else
  {
    synce_error("Invalid state: %i", self->state);
    return false;
  }

  self->state = STATE_VALUE;

  generator_append_escaped(self, value);
  return true;
}/*}}}*/

bool generator_end_line(Generator* self)/*{{{*/
{
  if (STATE_VALUE != self->state)
  {
    synce_error("Invalid state: %i", self->state);
    return false;
  }

  self->state = STATE_IDLE;

  strbuf_append_crlf(self->buffer);
  return true;
}/*}}}*/


