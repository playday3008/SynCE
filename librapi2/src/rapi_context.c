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
					(context->input  = rapi_buffer_new()) &&
					(context->output = rapi_buffer_new()) &&
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
		rapi_buffer_free(context->input);
		rapi_buffer_free(context->output);
		rapi_socket_free(context->socket);
		free(context);
	}
}
	
bool rapi_context_call(RapiContext* context)
{
	if ( !rapi_socket_send(context->socket, context->output) )
	{
		/* TODO: set context->last_error */
		return false;
	}

	if ( !rapi_socket_recv(context->socket, context->input) )
	{
		/* TODO: set context->last_error */
		return false;
	}

	context->result     = rapi_buffer_read_uint32(context->output);
	context->last_error = rapi_buffer_read_uint32(context->output);

	return context->result != 0;
}

