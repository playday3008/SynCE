/* $Id$ */
#include "rapi_internal.h"
#include "rapi.h"
#include "rapi_buffer.h"
#include "rapi_context.h"
#include "rapi_wstr.h"
#include "rapi_log.h"

BOOL CeDeleteFile(
		LPCWSTR lpFileName)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = 0;
	
	rapi_context_begin_command(context, 0x1c);
	rapi_buffer_write_optional_string(context->send_buffer, lpFileName);

	if ( !rapi_context_call(context) )
		return 0;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value);

	return return_value;
}


DWORD CeGetSpecialFolderPath( 
		int nFolder, 
		DWORD nBufferLength, 
		LPWSTR lpBuffer)
{
	RapiContext* context = rapi_context_current();
	DWORD string_length = nBufferLength;
	
	rapi_context_begin_command(context, 0x44);
	rapi_buffer_write_uint32(context->send_buffer, (u_int32_t)nFolder);
	rapi_buffer_write_uint32(context->send_buffer, nBufferLength);

	if ( !rapi_context_call(context) )
		return 0;

	if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
		return 0;
	rapi_log("last_error = %i", context->last_error);

	if ( !rapi_buffer_read_string(context->recv_buffer, lpBuffer, &string_length) )
		return 0;

	return string_length;
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
	rapi_buffer_write_optional_string(context->send_buffer, lpExistingFileName);
	rapi_buffer_write_optional_string(context->send_buffer, lpNewFileName);
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


