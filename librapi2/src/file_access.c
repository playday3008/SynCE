/* $Id$ */
#include "rapi_internal.h"
#include "rapi.h"
#include "rapi_buffer.h"
#include "rapi_context.h"
#include "rapi_wstr.h"
#include "rapi_log.h"


BOOL CeCloseHandle( 
		HANDLE hObject)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = 0;
	
	rapi_trace("begin");
	
	rapi_context_begin_command(context, 0x08);
	rapi_buffer_write_uint32(context->send_buffer, hObject);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value);

	return return_value;
}

HANDLE CeCreateFile(
		LPCWSTR lpFileName, 
		DWORD dwDesiredAccess, 
		DWORD dwShareMode, 
		LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
		DWORD dwCreationDisposition, 
		DWORD dwFlagsAndAttributes, 
		HANDLE hTemplateFile)
{
	RapiContext* context = rapi_context_current();
	HANDLE handle = INVALID_HANDLE_VALUE;

	rapi_trace("begin");
	
	rapi_context_begin_command(context, 0x05);
	rapi_buffer_write_uint32(context->send_buffer, dwDesiredAccess);
	rapi_buffer_write_uint32(context->send_buffer, dwShareMode);
	rapi_buffer_write_uint32(context->send_buffer, dwCreationDisposition);
	rapi_buffer_write_uint32(context->send_buffer, dwFlagsAndAttributes);
	rapi_buffer_write_uint32(context->send_buffer, hTemplateFile);
	rapi_buffer_write_string(context->send_buffer, lpFileName);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &handle);

	return handle;
}

BOOL CeReadFile( 
		HANDLE hFile, 
		LPVOID lpBuffer, 
		DWORD nNumberOfBytesToRead, 
		LPDWORD lpNumberOfBytesRead, 
		LPOVERLAPPED lpOverlapped)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = 0;
	uint32_t bytes_read = 0;

	rapi_trace("begin");
	
	rapi_context_begin_command(context, 0x06);
	rapi_buffer_write_uint32(context->send_buffer, hFile);
	rapi_buffer_write_optional_out(context->send_buffer, lpBuffer, nNumberOfBytesToRead);
	rapi_buffer_write_optional_in(context->send_buffer, NULL, 0); /* lpOverlapped */

	if ( !rapi_context_call(context) )
	{
		rapi_error("rapi_context_call failed");
		return false;
	}

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		return false;
	rapi_trace("context->last_error=0x%08x", context->last_error);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &return_value) )
		return false;
	rapi_trace("return_value=0x%08x", return_value);

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &bytes_read) )
		return false;

	if (lpNumberOfBytesRead)
		*lpNumberOfBytesRead = bytes_read;

	if (lpBuffer)
		if ( !rapi_buffer_read_data(context->recv_buffer, lpBuffer, bytes_read) )
			return false;

	return return_value;
}

BOOL CeWriteFile( 
		HANDLE hFile, 
		LPCVOID lpBuffer, 
		DWORD nNumberOfBytesToWrite, 
		LPDWORD lpNumberOfBytesWritten, 
		LPOVERLAPPED lpOverlapped)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = 0;
	uint32_t bytes_written = 0;

	rapi_trace("begin");
	
	rapi_context_begin_command(context, 0x07);
	rapi_buffer_write_uint32(context->send_buffer, hFile);
	rapi_buffer_write_optional_in(context->send_buffer, lpBuffer, nNumberOfBytesToWrite);
	rapi_buffer_write_optional_in(context->send_buffer, NULL, 0); /* lpOverlapped */

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value);

	rapi_buffer_read_uint32(context->recv_buffer, &bytes_written);
	if (lpNumberOfBytesWritten)
		*lpNumberOfBytesWritten = bytes_written;

	return return_value;
	
}


