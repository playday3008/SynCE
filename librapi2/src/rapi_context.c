/* $Id$ */
#include "rapi_context.h"
#include <stdlib.h>

#define RAPI_CONTEXT_DEBUG 1

#if RAPI_CONTEXT_DEBUG
#define rapi_context_trace(args...)    rapi_trace(args)
#define rapi_context_warning(args...)  rapi_warning(args)
#define rapi_context_error(args...)    rapi_error(args)
#else
#define rapi_context_trace(args...)
#define rapi_context_warning(args...)
#define rapi_context_error(args...)
#endif

static RapiContext* current_context;

RapiContext* rapi_context_current()
{
	/* TODO: make thread-safe version of this with thread private variables */

	if (!current_context)
	{
		current_context = rapi_context_new();
	}

	return current_context;
}

RapiContext* rapi_context_new()
{
	RapiContext* context = calloc(sizeof(RapiContext), 1);

	if (context)
	{
		if (!(
					(context->send_buffer  = rapi_buffer_new()) &&
					(context->recv_buffer = rapi_buffer_new()) &&
					(context->socket = rapi_socket_new())
				 ))
		{
			rapi_context_free(context);
			return NULL;
		}
	}

	return context;
}

void rapi_context_free(RapiContext* context)
{
	if (context)
	{
		rapi_buffer_free(context->send_buffer);
		rapi_buffer_free(context->recv_buffer);
		rapi_socket_free(context->socket);
		free(context);
	}
}

bool rapi_context_begin_command(RapiContext* context, uint32_t command)
{
	rapi_context_trace("command=0x%02x", command);
	
	rapi_buffer_free_data(context->send_buffer);
	
	if ( !rapi_buffer_write_uint32(context->send_buffer, command) )
		return false;

	return true;
}
	
bool rapi_context_call(RapiContext* context)
{
	if ( !rapi_socket_send(context->socket, context->send_buffer) )
	{
		rapi_context_error("rapi_socket_send failed");
		/* TODO: set context->last_error */
		return false;
	}

	if ( !rapi_socket_recv(context->socket, context->recv_buffer) )
	{
		rapi_context_error("rapi_socket_recv failed");
		/* TODO: set context->last_error */
		return false;
	}

	/* this is a boolean? */
	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->result_1) )
	{
		rapi_context_error("reading result_1 failed");
		return false;
	}

	rapi_context_trace("result 1 = 0x%08x", context->result_1);

	if (1 == context->result_1)
	{
		/* this is a HRESULT */
		if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->result_2) )
		{
			rapi_context_error("reading result_2 failed");
			return false;
		}

		rapi_context_trace("result 2 = 0x%08x", context->result_2);

		/*	if (context->result_2 != 0)
				return false;*/
	}

	return true;
}

