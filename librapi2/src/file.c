/* $Id$ */
#include "rapi_internal.h"
#include "rapi.h"
#include "rapi_buffer.h"
#include "rapi_context.h"
#include "rapi_wstr.h"

BOOL CeCloseHandle( 
		HANDLE hObject)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = 0;
	
	rapi_context_begin_command(context, 0x08);
	rapi_buffer_write_uint32(context->send_buffer, hObject);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value);

	return return_value;
}

/**
 * This function copies an existing file to a new file.
 *
 * Unicode version.
 */
BOOL CeCopyFile(
		LPCWSTR lpExistingFileName, 
		LPCWSTR lpNewFileName, 
		BOOL bFailIfExists)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = 0;
	
	rapi_context_begin_command(context, 0x1b);
	rapi_buffer_write_string(context->send_buffer, lpExistingFileName);
	rapi_buffer_write_string(context->send_buffer, lpNewFileName);
	rapi_buffer_write_uint32(context->send_buffer, bFailIfExists);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value);

	return return_value;
}

/**
 * This function copies an existing file to a new file.
 *
 * Ascii version.
 */
BOOL CeCopyFileA(
		LPCSTR lpExistingFileName, 
		LPCSTR lpNewFileName, 
		BOOL bFailIfExists)
{
	BOOL return_value = 0;
	LPWSTR lpExistingFileNameW = NULL;
	LPWSTR lpNewFileNameW = NULL;

	lpExistingFileNameW = rapi_wstr_from_ascii(lpExistingFileName);
	lpNewFileNameW      = rapi_wstr_from_ascii(lpNewFileName);

	if (lpExistingFileName && !lpExistingFileNameW)
		goto fail;

	if (lpNewFileName && !lpNewFileNameW)
		goto fail;
	
	return_value = CeCopyFile(lpExistingFileNameW, lpNewFileNameW, bFailIfExists);

fail:
	rapi_wstr_free_string(lpExistingFileNameW);
	rapi_wstr_free_string(lpNewFileNameW);
	
	return return_value;
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
	u_int32_t bytes_read = 0;

	rapi_context_begin_command(context, 0x06);
	rapi_buffer_write_uint32(context->send_buffer, hFile);
	rapi_buffer_write_optional_out(context->send_buffer, lpBuffer, nNumberOfBytesToRead);
	rapi_buffer_write_optional_in(context->send_buffer, lpOverlapped, 0);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value);

	rapi_buffer_read_uint32(context->recv_buffer, &bytes_read);
	if (lpNumberOfBytesRead)
		*lpNumberOfBytesRead = bytes_read;

	if (lpBuffer)
		rapi_buffer_read_data(context->recv_buffer, lpBuffer, bytes_read);

	return return_value;
}

