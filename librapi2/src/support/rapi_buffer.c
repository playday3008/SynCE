/* $Id$ */
#include "rapi_internal.h"
#include "rapi_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#define RAPI_BUFFER_DEBUG 0

#if RAPI_BUFFER_DEBUG
#define rapi_buffer_trace(args...)    synce_trace(args)
#define rapi_buffer_warning(args...)  synce_warning(args)
#define rapi_buffer_error(args...)    synce_error(args)
#else
#define rapi_buffer_trace(args...)
#define rapi_buffer_warning(args...)
#define rapi_buffer_error(args...)
#endif

#if HAVE_DMALLOC_H
#include "dmalloc.h"
#endif

#define RAPI_BUFFER_INITIAL_SIZE 16

struct _RapiBuffer
{
	unsigned char* data;
	size_t max_size;
	unsigned bytes_used;
	unsigned read_index;
};

/**
 * Enlarge buffer to at least a specified size
 */
static bool rapi_buffer_enlarge(RapiBuffer* buffer, size_t bytes_needed)/*{{{*/
{
	size_t new_size = buffer->max_size;
	unsigned char* new_data = NULL;
	bool success = false;

	if (new_size == 0)
		new_size = RAPI_BUFFER_INITIAL_SIZE;
	
	while (new_size < bytes_needed)
		new_size <<= 1;
	
	rapi_buffer_trace("trying to realloc %i bytes, buffer->data=%p", new_size, buffer->data);
	
	new_data = realloc(buffer->data, new_size);
	if (new_data)
	{
		buffer->data = new_data;
		buffer->max_size = new_size;
		success = true;
	}
	else
	{
		rapi_buffer_error("realloc %i bytes failed", new_size);
	}
	
	return success;
}/*}}}*/

/**
 * See if the buffer is large enough, try to enlarge if it is too small
 */
static bool rapi_buffer_assure_size(RapiBuffer* buffer, size_t extra_size)/*{{{*/
{
	bool success = false;
	size_t bytes_needed = buffer->bytes_used + extra_size;

	if (bytes_needed > buffer->max_size)
	{
		success = rapi_buffer_enlarge(buffer, bytes_needed);
		if (!success)
		{
			rapi_buffer_error("failed to enlarge buffer, bytes_needed=%i\n", bytes_needed);
		}
	}
	else
	{
		success = true;
	}

	return success;
}/*}}}*/


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
		memset(buffer, 0, sizeof(RapiBuffer));
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
	rapi_buffer_trace("size=0x%08x", size);
	
	if (!buffer)
	{
		rapi_buffer_error("buffer is NULL");
		return false;
	}
	
	rapi_buffer_free_data(buffer);
	
	buffer->data = data;
	buffer->max_size = buffer->bytes_used = size;
	buffer->read_index = 0;

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

bool rapi_buffer_write_data(RapiBuffer* buffer, const void* data, size_t size)
{
	if (!rapi_buffer_assure_size(buffer, size))
	{
		rapi_buffer_error("rapi_buffer_assure_size failed, size=%i\n", size);
		return false;
	}

	memcpy(buffer->data + buffer->bytes_used, data, size);
	buffer->bytes_used += size;

	return true;
}

bool rapi_buffer_write_uint16(RapiBuffer* buffer, uint16_t value)
{
	uint16_t little_endian_value = htole16(value);
	return rapi_buffer_write_data(buffer, &little_endian_value, sizeof(uint16_t));
}

bool rapi_buffer_write_uint32(RapiBuffer* buffer, uint32_t value)
{
	uint32_t little_endian_value = htole32(value);
	return rapi_buffer_write_data(buffer, &little_endian_value, sizeof(uint32_t));
}

bool rapi_buffer_write_string(RapiBuffer* buffer, LPCWSTR unicode)
{
	/*
	 * This function writes a sequence like this:
	 *
	 * Offset  Value
	 *     00  0x00000001
	 *     04  String length in number of unicode chars + 1
	 *     08  String data 
	 */
	size_t length = wstr_strlen(unicode) + 1;
	
	rapi_buffer_trace("Writing string of length %i",length);
	
	return 
		rapi_buffer_write_uint32(buffer, 1) &&
		rapi_buffer_write_uint32(buffer, length) &&
		rapi_buffer_write_data(buffer, (void*)unicode, length * sizeof(WCHAR));
}

bool rapi_buffer_write_optional_string(RapiBuffer* buffer, LPCWSTR unicode)
{
	/*
	 * This function writes a sequence like this:
	 *
	 * Offset  Value
	 *     00  0x00000001
	 *     04  String length in number of bytes + 2
	 *     08  boolean specifying if string data is sent or not
	 *     0c  String data 
	 */
	size_t size;
	
	if (unicode)
	  size = (wstr_strlen(unicode) + 1) * sizeof(WCHAR);
	else
		size = 0;
	
	return rapi_buffer_write_optional(buffer, (void*)unicode, size, true);
}

bool rapi_buffer_write_optional_uint32(RapiBuffer* buffer, uint32_t* data, bool send_data)
{
	if (data && send_data)
		*data = htole32(*data);
	return rapi_buffer_write_optional(buffer, data, sizeof(uint32_t), send_data);
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

bool rapi_buffer_write_optional_in(RapiBuffer* buffer, const void* data, size_t size)
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

bool rapi_buffer_write_optional_no_size(RapiBuffer* buffer, const void* data, size_t size)
{
	if (data)
	{
		return
			rapi_buffer_write_uint32(buffer, 1) &&
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

bool rapi_buffer_read_data(RapiBuffer* buffer, void* data, size_t size)
{
	if (!data)
	{
		rapi_buffer_error("data is NULL");
		return false;
	}

	if (!buffer)
	{
		rapi_buffer_error("buffer is NULL");
		return false;
	}

	if ( (buffer->read_index + size) > buffer->bytes_used )
	{
		rapi_buffer_error("unable to read %i bytes. read_index=%i, bytes_used=%i", size,
				buffer->read_index, buffer->bytes_used);

		return false;
	}

	memcpy(data, buffer->data + buffer->read_index, size);
	buffer->read_index += size;

	return true;
}

bool rapi_buffer_read_uint16(RapiBuffer* buffer, uint16_t* value)
{
	if ( !rapi_buffer_read_data(buffer, value, sizeof(uint16_t)) )
		return false;

	*value = letoh16(*value);

	return true;
}

bool rapi_buffer_read_uint32(RapiBuffer* buffer, uint32_t* value)
{
	if ( !rapi_buffer_read_data(buffer, value, sizeof(uint32_t)) )
		return false;

	*value = letoh32(*value);

	return true;
}

bool rapi_buffer_read_string(RapiBuffer* buffer, LPWSTR unicode, size_t* size)
{
	uint32_t exact_size = 0;
	
	if (!buffer || !unicode || !size)
	{
		rapi_buffer_error("bad parameter");
		return false;
	}

	if ( !rapi_buffer_read_uint32(buffer, &exact_size) )
		return false;
	rapi_buffer_trace("exact_size = %i = 0x%x", exact_size, exact_size);

	if ( exact_size > *size )
	{
		rapi_buffer_error("buffer too small (have %i bytes, need %i bytes)", *size, exact_size);
		return false;
	}

	*size = exact_size;

	if ( !rapi_buffer_read_data(buffer, unicode, (exact_size+1) * sizeof(WCHAR)) )
	{
		rapi_buffer_error("failed to read buffer");
		return false;
	}
	
	return true;
}

bool rapi_buffer_read_optional(RapiBuffer* buffer, void* data, size_t max_size)
{
	/* See http://sourceforge.net/mailarchive/message.php?msg_id=64440 */
	uint32_t have_parameter = 0;
	
	if (!rapi_buffer_read_uint32(buffer, &have_parameter))
		return false;

	if (1 == have_parameter)
	{
		uint32_t size = 0;
		uint32_t have_value = 0;

		if (!rapi_buffer_read_uint32(buffer, &size))
			return false;

		if (!rapi_buffer_read_uint32(buffer, &have_value))
			return false;

		if (1 == have_value)
		{
			int overflow = 0;

			if (data)
			{
				if (!rapi_buffer_read_data(buffer, data, MIN(size, max_size)))
					return false;
				overflow = size - max_size;
			}
			else
			{
				overflow = size;
			}

			if (overflow > 0)
			{
				if (data)
				{
					rapi_buffer_warning("Overflow by %i bytes. Parameter size is %i bytes but only %i bytes was expected.",
							overflow, size, max_size);
				}

				/* skip overflowed bytes */
				buffer->read_index += overflow;
			}
		}
		else if (0 != have_value)
		{
			rapi_buffer_warning("have_value is not a boolean: %i=0x%08x", have_value);
		}
		
	}
	else if (0 != have_parameter)
	{
		rapi_buffer_warning("have_parameter is not a boolean: %i=0x%08x", have_parameter);
	}

	return true;
}

bool rapi_buffer_read_optional_uint32(RapiBuffer* buffer, uint32_t* value)
{
	bool success =  rapi_buffer_read_optional(buffer, value, sizeof(uint32_t));

	if (success && value)
	{
		*value = letoh32(*value);
	}

	return success;
}

bool rapi_buffer_read_optional_filetime(RapiBuffer* buffer, FILETIME* lpftLastWriteTime)
{
	bool success = rapi_buffer_read_optional(buffer, lpftLastWriteTime, sizeof(FILETIME));

	if (success && lpftLastWriteTime)
	{
		lpftLastWriteTime->dwLowDateTime  = letoh32(lpftLastWriteTime->dwLowDateTime);
		lpftLastWriteTime->dwHighDateTime = letoh32(lpftLastWriteTime->dwHighDateTime);
	}

	return success;
}

bool rapi_buffer_send(RapiBuffer* buffer, SynceSocket* socket)
{
	uint32_t size_le = htole32(rapi_buffer_get_size(buffer));

	if ( !synce_socket_write(socket, &size_le, sizeof(size_le)) )
		goto fail;

	if ( !synce_socket_write(socket, 
				rapi_buffer_get_raw(buffer), 
				rapi_buffer_get_size(buffer)) )
		goto fail;

	return true;

fail:
	/* XXX: is it wise to close the connection here? */
	synce_socket_close(socket);
	return false;
}

bool rapi_buffer_recv(RapiBuffer* buffer, SynceSocket* socket)
{
	uint32_t      size_le = 0;
	size_t         size    = 0;
	unsigned char* data    = NULL;
	
	if ( !synce_socket_read(socket, &size_le, sizeof(size_le)) )
	{
		rapi_buffer_error("Failed to read size");
		goto fail;
	}

	size = letoh32(size_le);

	rapi_buffer_trace("Size = 0x%08x", size);

	data = malloc(size);
	if (!data)
	{
		rapi_buffer_error("Failed to allocate 0x%08x bytes", size);
		goto fail;
	}

	if ( !synce_socket_read(socket, data, size) )
	{
		rapi_buffer_error("Failed to read 0x%08x bytes", size);
		goto fail;
	}

	if ( !rapi_buffer_reset(buffer, data, size) )
	{
		rapi_buffer_error("Failed to reset buffer with 0x%08x bytes", size);
		free(data);
		goto fail;
	}

	return true;
	
fail:
	/* XXX: is it wise to close the connection here? */
	synce_socket_close(socket);
	return false;	
}

