/* $Id$ */
#define _GNU_SOURCE 1
#include "generator.h"
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

struct _Generator
{
  int flags;
  void* cookie;
  SHashTable* properties;
  StrBuf* buffer;
  CEPROPVAL* propvals;
  size_t propval_count;
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

bool generator_set_data(Generator* self, const uint8_t* data, size_t data_size)/*{{{*/
{
	bool success = false;

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

	self->propval_count = letoh32(*(uint32_t*)(data + 0));
	synce_trace("Field count: %i", self->propval_count);

	if (0 == self->propval_count)
	{
		synce_error("No fields in record!");
		goto exit;
	} 
	
	if (self->propval_count > MAX_PROPVAL_COUNT)
	{
		synce_error("Too many fields in record");
		goto exit;
	}

	self->propvals = (CEPROPVAL*)malloc(sizeof(CEPROPVAL) * self->propval_count);

	if (!dbstream_to_propvals(data + 8, self->propval_count, self->propvals))
	{
		synce_error("Failed to convert database stream");
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
  int i;
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
      synce_trace("Unhandled property id: %04x", id);
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
				strbuf_append_c(self->buffer, '\\');
				strbuf_append_c(self->buffer, *p);
				break;
				
#if 0
			case ';':
			case ',':
				if (flags & RRA_CONTACT_VERSION_3_0)
					strbuf_append_c(self->buffer, '\\');
				/* fall through */
#endif

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


bool generator_add_simple(Generator* self, const char* name, const char* value)
{
  strbuf_append(self->buffer, name);
  strbuf_append_c(self->buffer, ':');
  generator_append_escaped(self, value);
  strbuf_append_crlf(self->buffer);
  return true;
}

bool generator_add_with_type(Generator* self, const char* name, const char* type, const char* value)
{
  strbuf_append(self->buffer, name);
  strbuf_append(self->buffer, ";VALUE=");
  strbuf_append(self->buffer, type);
  strbuf_append_c(self->buffer, ':');
  generator_append_escaped(self, value);
  strbuf_append_crlf(self->buffer);
  return true;
}

bool generator_add_simple_propval(Generator* self, const char* name, struct _CEPROPVAL* propval)
{
  bool success = false;
  
  switch (propval->propid & 0xffff)
  {
    case CEVT_LPWSTR:
      strbuf_append(self->buffer, name);
      strbuf_append_c(self->buffer, ':');
      generator_append_escaped_wstr(self, propval->val.lpwstr);
      strbuf_append_crlf(self->buffer);
      success = true;
      break;

    default:
      synce_error("Data type not handled");
      break;
  }
  
  return success;
}


