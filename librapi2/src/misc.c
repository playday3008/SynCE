/* $Id$ */
#include "rapi.h"
#include "rapi_context.h"
#include "rapi_endian.h"
#include "rapi_log.h"

BOOL CeGetVersionEx(
		LPCEOSVERSIONINFO lpVersionInformation)
{
	RapiContext* context = rapi_context_current();
	u_int32_t size = 0;
	
	rapi_context_begin_command(context, 0x3b);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_log("last_error = %i", context->last_error);
	
	rapi_buffer_read_uint32(context->recv_buffer, &size);

	if ( !rapi_buffer_read_data(context->recv_buffer, lpVersionInformation, size) )
		return false;

	return true;
}


