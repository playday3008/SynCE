/* $Id$ */
#ifndef __rapi_context_h__
#define __rapi_context_h__

#include "rapi_buffer.h"
#include "rapi_socket.h"

typedef struct _RapiContext
{
	RapiBuffer* send_buffer;
	RapiBuffer* recv_buffer;
	RapiSocket* socket;
	uint32_t result_1;
	uint32_t result_2;
	uint32_t last_error;
	bool is_initialized;
} RapiContext;

/**
 * Get current RapiContext
 */
RapiContext* rapi_context_current();

/**
 * Create new context
 */
RapiContext* rapi_context_new();

/**
 * Free context 
 */
void rapi_context_free(RapiContext* context);

/**
 * Begin a command
 */
bool rapi_context_begin_command(RapiContext* context, uint32_t command);

/**
 * Send send_buffer and receive recv_buffer on socket
 */
bool rapi_context_call(RapiContext* context);

#endif

