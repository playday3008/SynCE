/* $Id$ */
#include "rapi.h"
#include "rapi_context.h"
#include <stdlib.h>

struct _IRAPIStream
{
	RapiContext* context;
};

static IRAPIStream* rapi_stream_new()/*{{{*/
{
	IRAPIStream* stream = calloc(1, sizeof(IRAPIStream));

	if (stream)
	{
		stream->context = rapi_context_new();
	}

	return stream;
}/*}}}*/

static void rapi_stream_destroy(IRAPIStream* stream)/*{{{*/
{
	if (stream)
	{
		rapi_context_free(stream->context);
		free(stream);
	}
}/*}}}*/

ULONG IRAPIStream_Release(IRAPIStream* stream)/*{{{*/
{
	rapi_stream_destroy(stream);
	return 0;
}/*}}}*/

HRESULT IRAPIStream_Read( /*{{{*/
		IRAPIStream* stream,
		void *pv, 
		ULONG cb, 
		ULONG *pcbRead)
{
	HRESULT hr = E_FAIL;

	if (pv && synce_socket_read(stream->context->socket, pv, cb))
	{
		if (pcbRead)
			*pcbRead = cb;

		hr = S_OK;
	}
	
	return hr;
}/*}}}*/

HRESULT IRAPIStream_Write( /*{{{*/
		IRAPIStream* stream,
		void const *pv, 
		ULONG cb, 
		ULONG *pcbWritten)
{
	HRESULT hr = E_FAIL;

	if (pv && synce_socket_write(stream->context->socket, pv, cb))
	{
		if (pcbWritten)
			*pcbWritten = cb;

		hr = S_OK;
	}
	
	return hr;
}/*}}}*/

static HRESULT CeRapiInvokeStream( /*{{{*/
		LPCWSTR pDllPath, 
		LPCWSTR pFunctionName, 
		DWORD cbInput, 
		const BYTE *pInput, 
		DWORD *pcbOutput, 
		BYTE **ppOutput, 
		IRAPIStream **ppIRAPIStream, 
		DWORD dwReserved)
{
	HRESULT return_value = E_FAIL;
	RapiContext* context = NULL;

	if (!ppIRAPIStream)
		return E_FAIL;
	
	*ppIRAPIStream = rapi_stream_new();
	context = (**ppIRAPIStream).context;

	return_value = rapi_context_connect(context);
	if (FAILED(return_value))
		return return_value;

	rapi_context_begin_command(context, 0x45);
	rapi_buffer_write_uint32(context->send_buffer, dwReserved);
	rapi_buffer_write_string(context->send_buffer, pDllPath);
	rapi_buffer_write_string(context->send_buffer, pFunctionName);
	rapi_buffer_write_uint32(context->send_buffer, cbInput);

	if (cbInput && pInput)
		rapi_buffer_write_data(context->send_buffer, pInput, cbInput);
	
	rapi_buffer_write_uint32(context->send_buffer, 1);

	if ( !rapi_buffer_send(context->send_buffer, context->socket) )
	{
		synce_error("synce_socket_send failed");
		return E_FAIL;
	}

	return_value = IRAPIStream_Read(*ppIRAPIStream, &context->last_error, sizeof(HRESULT), NULL);
	
	return return_value;
}/*}}}*/

HRESULT CeRapiInvoke( /*{{{*/
		LPCWSTR pDllPath, 
		LPCWSTR pFunctionName, 
		DWORD cbInput, 
		const BYTE *pInput, 
		DWORD *pcbOutput, 
		BYTE **ppOutput, 
		IRAPIStream **ppIRAPIStream, 
		DWORD dwReserved)
{
	if (ppIRAPIStream)
		return CeRapiInvokeStream(pDllPath, pFunctionName, cbInput, pInput,
				pcbOutput, ppOutput, ppIRAPIStream, dwReserved);
	else
		return E_FAIL;
}/*}}}*/

HRESULT CeRapiInvokeA( /*{{{*/
		LPCSTR pDllPath, 
		LPCSTR pFunctionName, 
		DWORD cbInput, 
		const BYTE *pInput, 
		DWORD *pcbOutput, 
		BYTE **ppOutput, 
		IRAPIStream **ppIRAPIStream, 
		DWORD dwReserved)
{
	HRESULT hr;
	WCHAR* wide_dll_path       = wstr_from_current(pDllPath);
	WCHAR* wide_function_name  = wstr_from_current(pFunctionName);

	hr = CeRapiInvoke( wide_dll_path, wide_function_name, cbInput, pInput,
			pcbOutput, ppOutput, ppIRAPIStream, dwReserved);
	
	wstr_free_string(wide_dll_path);
	wstr_free_string(wide_function_name);

	return hr;
}

