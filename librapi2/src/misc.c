/* $Id$ */
#include "rapi.h"
#include "rapi_context.h"

DWORD CeGetLastError( void )
{
	RapiContext* context = rapi_context_current();
	return context->last_error;
}

BOOL CeGetVersionEx(/*{{{*/
		LPCEOSVERSIONINFO lpVersionInformation)
{
	RapiContext* context = rapi_context_current();
	BOOL result = false;
	uint32_t size = 0;
	
	rapi_context_begin_command(context, 0x3b);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	synce_trace("last_error = %i", context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &result);
	synce_trace("result = %i", result);
	
	rapi_buffer_read_uint32(context->recv_buffer, &size);

	if ( !rapi_buffer_read_data(context->recv_buffer, lpVersionInformation, size) )
		return false;

	return result;
}/*}}}*/

BOOL CeOidGetInfo(/*{{{*/
		CEOID oid, 
		CEOIDINFO *poidInfo)
{
	RapiContext* context = rapi_context_current();
	BOOL result = false;
	uint16_t size = 0;

	if (!poidInfo)
	{
		synce_error("poidInfo is NULL");
		goto fail;
	}
	
	rapi_context_begin_command(context, 0x0c);
	rapi_buffer_write_uint32(context->send_buffer, oid);

	if ( !rapi_context_call(context) )
		goto fail;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	synce_trace("last_error = %i", context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &result);
	synce_trace("result = %i", result);
	
	if ( !rapi_buffer_read_uint16(context->recv_buffer, &poidInfo->wObjType) )
		goto fail;
	synce_trace("object type = %i", poidInfo->wObjType);

	switch (poidInfo->wObjType)
	{
		case OBJTYPE_FILE:
			if ( !rapi_buffer_read_uint16(context->recv_buffer, &size) )
				goto fail;
			synce_trace("size = %i", size);
			break;

		case OBJTYPE_DIRECTORY:
			if ( !rapi_buffer_read_uint16(context->recv_buffer, &size) )
				goto fail;
			synce_trace("size = %i", size);
			break;

		case OBJTYPE_DATABASE:
			if ( !rapi_buffer_read_uint16(context->recv_buffer, &size) )
				goto fail;
			synce_trace("size = %i", size);
			break;

		case OBJTYPE_RECORD:
			if ( !rapi_buffer_read_uint32(context->recv_buffer, &poidInfo->u.infRecord.oidParent) )
				goto fail;
			break;

		case OBJTYPE_DELETED:
			synce_trace("This object is deleted");
			break;

		default:
			synce_error("unknown object type = %i, buffer size = %i", 
					poidInfo->wObjType, rapi_buffer_get_size(context->recv_buffer));
			goto fail;
	}

/*	if ( !rapi_buffer_read_data(context->recv_buffer, lpVersionInformation, size) )
		return false;*/

	return result;

fail:
	return false;
}/*}}}*/

/**
 * Undocumented function used by ActiveSync to begin synchronization
 *
 * See this mail for details:
 * 
 * http://sourceforge.net/mailarchive/forum.php?thread_id=844008&forum_id=1226
 */
DWORD CeStartReplication( void )/*{{{*/
{
	RapiContext* context = rapi_context_current();
	DWORD result = false;
	
	rapi_context_begin_command(context, 0x38);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &result);
	synce_trace("result = %i", result);

	return result;
}/*}}}*/

