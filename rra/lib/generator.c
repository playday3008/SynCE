/* $Id$ */
#include "generator.h"
#include "dbstream.h"
#include "strbuf.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdlib.h>

#define MAX_PROPVAL_COUNT         50

struct _Generator
{
  int flags;
  void* cookie;
  CEPROPVAL* propvals;
  size_t propval_count;
  StrBuf* buffer;
};

Generator* generator_new(int flags, void* cookie)/*{{{*/
{
  Generator* self = (Generator*)calloc(1, sizeof(Generator));

  if (self)
  {
    self->flags   = flags;
    self->cookie  = cookie;
    self->buffer  = strbuf_new(NULL);
  }

  return self;
}/*}}}*/

void generator_destroy(Generator* self)/*{{{*/
{
  if (self)
  {
    if (self->propvals)
      free(self->propvals);
    strbuf_destroy(self->buffer, true);
    free(self);
  }
}/*}}}*/

bool generator_set_data(Generator* self, uint8_t* data, size_t data_size)/*{{{*/
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

bool generator_run(Generator* self);
bool generator_get_result(Generator* self, char** result);

bool generator_add_simple(Generator* self, const char* name, const char* value);
bool generator_add_with_type(Generator* self, const char* name, const char* type, const char* value);


