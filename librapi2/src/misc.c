/* $Id$ */
#include "rapi.h"
#include "rapi_context.h"
#include "rapi_endian.h"
#include "rapi_log.h"

DWORD CeGetLastError( void )
{
	RapiContext* context = rapi_context_current();
	return context->last_error;
}

BOOL CeGetVersionEx(
		LPCEOSVERSIONINFO lpVersionInformation)
{
	RapiContext* context = rapi_context_current();
	BOOL result = false;
	uint32_t size = 0;
	
	rapi_context_begin_command(context, 0x3b);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_log("last_error = %i", context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &result);
	rapi_log("result = %i", result);
	
	rapi_buffer_read_uint32(context->recv_buffer, &size);

	if ( !rapi_buffer_read_data(context->recv_buffer, lpVersionInformation, size) )
		return false;

	return result;
}

/**
 * Undocumented function used by ActiveSync to begin synchronization
 *
 * See this mail for details:
 * 
 * http://sourceforge.net/mailarchive/forum.php?thread_id=844008&forum_id=1226
 */
DWORD CeStartReplication( void )
{
	RapiContext* context = rapi_context_current();
	DWORD result = false;
	
	rapi_context_begin_command(context, 0x38);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &result);
	rapi_log("result = %i", result);

	return result;
}

