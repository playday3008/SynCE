/* $Id$ */
#include "rapi_buffer.h"
#include "rapi_endian.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define RAPI_BUFFER_INITIAL_SIZE 16

struct _RapiBuffer
{
	unsigned char* data;
	size_t max_size;
	unsigned bytes_used;
	unsigned read_index;
};

RapiBuffer* rapi_buffer_new()
{
	RapiBuffer* buffer = calloc(1, sizeof(RapiBuffer));
	
	return buffer;
}

void rapi_buffer_free_data(RapiBuffer* buffer)
{
	if (buffer && buffer->data)
	{
		free(buffer->data);
		buffer->data = NULL;
		buffer->max_size = 0;
		buffer->bytes_used = 0;
		buffer->read_index = 0;
	}
}

void rapi_buffer_free(RapiBuffer* buffer)
{
	if (buffer)
	{
		rapi_buffer_free_data(buffer);
		free(buffer);
	}
}

bool rapi_buffer_reset(RapiBuffer* buffer, unsigned char* data, size_t size)
{
	unsigned char* new_data = NULL;
	
	if (!buffer)
		return false;
	
	if (size)
	{
		new_data = malloc(size);
		
		if (!new_data)
			return false;
	}
		
	rapi_buffer_free_data(buffer);
	buffer->data = new_data;
	buffer->max_size = size;

	if (data)
		memcpy(new_data, data, size);

	return true;
}

size_t rapi_buffer_get_size(RapiBuffer* buffer)
{
	return buffer->bytes_used;
}

unsigned char* rapi_buffer_get_raw(RapiBuffer* buffer)
{
	return buffer->data;
}

/**
 * Enlarge buffer to at least a specified size
 */
static bool rapi_buffer_enlarge(RapiBuffer* buffer, size_t bytes_needed)
{
	size_t new_size = buffer->max_size;
	unsigned char* new_data = NULL;

	if (new_size == 0)
		new_size = RAPI_BUFFER_INITIAL_SIZE;
	
	while (new_size < bytes_needed)
		new_size <<= 1;

	new_data = realloc(buffer->data, buffer->max_size);
	if (new_data)
	{
		buffer->data = new_data;
		buffer->max_size = new_size;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * See if the buffer is large enough, try to enlarge if it is too small
 */
static bool rapi_buffer_assure_size(RapiBuffer* buffer, size_t extra_size)
{
	size_t bytes_needed = buffer->bytes_used + extra_size;

	if (bytes_needed > buffer->max_size)
		return rapi_buffer_enlarge(buffer, bytes_needed);

	return true;
}

bool rapi_buffer_write_data(RapiBuffer* buffer, void* data, size_t size)
{
	if (!rapi_buffer_assure_size(buffer, size))
		return false;

	memcpy(buffer->data + buffer->bytes_used, data, size);
	buffer->bytes_used += size;

	return true;
}

bool rapi_buffer_write_uint16(RapiBuffer* buffer, u_int16_t value)
{
	u_int16_t little_endian_value = htole16(value);
	return rapi_buffer_write_data(buffer, &little_endian_value, sizeof(u_int16_t));
}

bool rapi_buffer_write_uint32(RapiBuffer* buffer, u_int32_t value)
{
	u_int32_t little_endian_value = htole32(value);
	return rapi_buffer_write_data(buffer, &little_endian_value, sizeof(u_int32_t));
}

bool rapi_buffer_write_string(RapiBuffer* buffer, const uchar* unicode)
{
	size_t size;
	
	if (unicode)
	  size = (rapi_unicode_string_length(unicode) + 1) * sizeof(uchar);
	else
		size = 0;
	
	return rapi_buffer_write_optional(buffer, (void*)unicode, size, true);
}

bool rapi_buffer_write_optional(RapiBuffer* buffer, void* data, size_t size, bool send_data)
{
	/* See http://sourceforge.net/mailarchive/message.php?msg_id=64440 */
	if (data)
	{
		return 
			rapi_buffer_write_uint32(buffer, 1) &&
			rapi_buffer_write_uint32(buffer, size) &&
			rapi_buffer_write_uint32(buffer, send_data) &&
			(send_data ? rapi_buffer_write_data(buffer, data, size) : true);
	}
	else
	{
		return rapi_buffer_write_uint32(buffer, 0);
	}
}

bool rapi_buffer_write_optional_in(RapiBuffer* buffer, void* data, size_t size)
{
	if (data)
	{
		return
			rapi_buffer_write_uint32(buffer, 1) &&
			rapi_buffer_write_uint32(buffer, size) &&
			rapi_buffer_write_data(buffer, data, size);
	}
	else
	{
		return rapi_buffer_write_uint32(buffer, 0);
	}
}

bool rapi_buffer_write_optional_out(RapiBuffer* buffer, void* data, size_t size)
{
	/* See http://sourceforge.net/mailarchive/message.php?msg_id=64440 */
	if (data)
	{
		return
			rapi_buffer_write_uint32(buffer, 1) &&
			rapi_buffer_write_uint32(buffer, size) &&
			rapi_buffer_write_uint32(buffer, 0);
	}
	else
	{
		return rapi_buffer_write_uint32(buffer, 0);
	}
}

bool rapi_buffer_write_optional_inout(RapiBuffer* buffer, void* data, size_t size)
{
	/* See http://sourceforge.net/mailarchive/message.php?msg_id=64440 */
	if (data)
	{
		return 
			rapi_buffer_write_uint32(buffer, 1) &&
			rapi_buffer_write_uint32(buffer, size) &&
			rapi_buffer_write_uint32(buffer, 1) &&
			rapi_buffer_write_data(buffer, data, size);
	}
	else
	{
		return rapi_buffer_write_uint32(buffer, 0);
	}
}

bool rapi_buffer_read_data(RapiBuffer* buffer, void* data, size_t size);

u_int16_t rapi_buffer_read_uint16(RapiBuffer* buffer);

u_int32_t rapi_buffer_read_uint32(RapiBuffer* buffer);




