/* $Id$ */
#undef __STRICT_ANSI__
#define _GNU_SOURCE
#include "rapi_internal.h"
#include "rapi.h"
#include "rapi_buffer.h"
#include "rapi_context.h"
#include <stdlib.h>

HRESULT CeRapiFreeBuffer( 
		LPVOID Buffer)
{
	free(Buffer);
	return S_OK;
}

HRESULT CeRapiInit(void)/*{{{*/
{
	RapiContext* context = rapi_context_current();

	return rapi_context_connect(context);
}/*}}}*/

STDAPI CeRapiUninit(void)/*{{{*/
{
	RapiContext* context = rapi_context_current();
	
	if (context->is_initialized)
	{
    rapi_context_free(context);
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}/*}}}*/

BOOL CeCheckPassword( /*{{{*/
		LPWSTR lpszPassword)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = 0;
	
	rapi_context_begin_command(context, 0x34);
	rapi_buffer_write_optional_string(context->send_buffer, lpszPassword);

	if ( !rapi_context_call(context) )
		return false;
	
	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value);

	return return_value;
}/*}}}*/

