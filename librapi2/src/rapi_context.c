/* $Id$ */
#include "rapi_context.h"
#include <stdlib.h>

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

bool rapi_context_begin_command(RapiContext* context, u_int32_t command)
{
	rapi_buffer_free_data(context->send_buffer);
	
	if ( !rapi_buffer_write_uint32(context->send_buffer, command) )
		return false;

	return true;
}
	
bool rapi_context_call(RapiContext* context)
{
	if ( !rapi_socket_send(context->socket, context->send_buffer) )
	{
		/* TODO: set context->last_error */
		return false;
	}

	if ( !rapi_socket_recv(context->socket, context->recv_buffer) )
	{
		/* TODO: set context->last_error */
		return false;
	}

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->result_1) )
		return false;

	if (context->result_1 != 1)
		return false;
	
	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->result_2) )
		return false;

	return context->result_2;
}

