/* $Id$ */
#ifndef __rapi_context_h__
#define __rapi_context_h__

#include "rapi_buffer.h"
#include "rapi_socket.h"

typedef struct _RapiContext
{
	RapiBuffer* input;
	RapiBuffer* output;
	RapiSocket* socket;
	unsigned last_error;
	unsigned result;
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
 * Send input buffer and receive output buffer on socket
 */
bool rapi_context_call(RapiContext* context);

#endif

