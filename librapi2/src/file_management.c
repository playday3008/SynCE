/* $Id$ */
#include "rapi_internal.h"
#include "rapi.h"
#include "rapi_buffer.h"
#include "rapi_context.h"
#include "rapi_wstr.h"
#include "rapi_log.h"
#include "rapi_endian.h"

BOOL CeCreateDirectory(
		LPCWSTR lpPathName, 
		LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = 0;
	
	rapi_context_begin_command(context, 0x17);
	rapi_buffer_write_optional_string(context->send_buffer, lpPathName);
	rapi_buffer_write_optional_in(context->send_buffer, NULL, 0); /* lpSecurityAttributes */

	if ( !rapi_context_call(context) )
		return 0;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value);

	return return_value;
}

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

BOOL CeFindAllFiles(
		LPCWSTR szPath, 
		DWORD dwFlags, 
		LPDWORD lpdwFoundCount, 
		LPLPCE_FIND_DATA ppFindDataArray)
{
	RapiContext* context = rapi_context_current();
	uint32_t count = 0;
	
	rapi_context_begin_command(context, 0x09);
	rapi_buffer_write_string(context->send_buffer, szPath);
	rapi_buffer_write_uint32(context->send_buffer, dwFlags);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &count);

	rapi_trace("found %i files", count);

	if (count)
	{
		unsigned i;
		uint32_t name_size;
		CE_FIND_DATA* array = calloc(count, sizeof(CE_FIND_DATA));
			
		if (!array)
			return false;

		for (i = 0; i < count; i++)
		{
			if (dwFlags & FAF_NAME)
				rapi_buffer_read_uint32(context->recv_buffer, &name_size);

			if (dwFlags & FAF_ATTRIBUTES)
				rapi_buffer_read_uint32(context->recv_buffer, &array[i].dwFileAttributes);

			if (dwFlags & FAF_CREATION_TIME)
			{
				rapi_buffer_read_uint32(context->recv_buffer, &array[i].ftCreationTime.dwLowDateTime);
				rapi_buffer_read_uint32(context->recv_buffer, &array[i].ftCreationTime.dwHighDateTime);
			}

			if (dwFlags & FAF_LASTACCESS_TIME)
			{
				rapi_buffer_read_uint32(context->recv_buffer, &array[i].ftLastAccessTime.dwLowDateTime);
				rapi_buffer_read_uint32(context->recv_buffer, &array[i].ftLastAccessTime.dwHighDateTime);
			}

			if (dwFlags & FAF_LASTWRITE_TIME)
			{
				rapi_buffer_read_uint32(context->recv_buffer, &array[i].ftLastWriteTime.dwLowDateTime);
				rapi_buffer_read_uint32(context->recv_buffer, &array[i].ftLastWriteTime.dwHighDateTime);
			}

			if (dwFlags & FAF_SIZE_HIGH)
				rapi_buffer_read_uint32(context->recv_buffer, &array[i].nFileSizeHigh);

			if (dwFlags & FAF_SIZE_LOW)
				rapi_buffer_read_uint32(context->recv_buffer, &array[i].nFileSizeLow);

			if (dwFlags & FAF_OID)
				rapi_buffer_read_uint32(context->recv_buffer, &array[i].dwOID);

			if (dwFlags & FAF_NAME)
			{
				rapi_buffer_read_data(context->recv_buffer, array[i].cFileName, name_size * sizeof(WCHAR) );
				rapi_trace_wstr(array[i].cFileName);
			}
		}

		if (ppFindDataArray)
			*ppFindDataArray = array;

	}

	if (lpdwFoundCount)
		*lpdwFoundCount = count;

	return count;
}

static bool rapi_read_find_data(
		RapiContext* context,
		LPCE_FIND_DATA lpFindFileData)
{
	if (lpFindFileData)
	{
		size_t size = 0;
		rapi_buffer_read_uint32(context->recv_buffer, &size);
		
		memset(&lpFindFileData->cFileName, 0, sizeof(lpFindFileData->cFileName));
		rapi_buffer_read_data(context->recv_buffer, lpFindFileData, size);

		/* convert fields from little endian */
		lpFindFileData->dwFileAttributes                = letoh32(lpFindFileData->dwFileAttributes);
		lpFindFileData->ftCreationTime.  dwLowDateTime  = letoh32(lpFindFileData->ftCreationTime.  dwLowDateTime);
		lpFindFileData->ftCreationTime.  dwHighDateTime = letoh32(lpFindFileData->ftCreationTime.  dwHighDateTime);
		lpFindFileData->ftLastAccessTime.dwLowDateTime  = letoh32(lpFindFileData->ftLastAccessTime.dwLowDateTime);
		lpFindFileData->ftLastAccessTime.dwHighDateTime = letoh32(lpFindFileData->ftLastAccessTime.dwHighDateTime);
		lpFindFileData->ftLastWriteTime. dwLowDateTime  = letoh32(lpFindFileData->ftLastWriteTime. dwLowDateTime);
		lpFindFileData->ftLastWriteTime. dwHighDateTime = letoh32(lpFindFileData->ftLastWriteTime. dwHighDateTime);
		lpFindFileData->nFileSizeHigh                   = letoh32(lpFindFileData->nFileSizeHigh);
		lpFindFileData->nFileSizeLow                    = letoh32(lpFindFileData->nFileSizeLow);
		lpFindFileData->dwOID                           = letoh32(lpFindFileData->dwOID);
		
		rapi_trace("dwFileAttributes=0x%08x", lpFindFileData->dwFileAttributes);
		rapi_trace("nFileSizeLow=0x%08x",     lpFindFileData->nFileSizeLow);
		rapi_trace("dwOID=0x%08x",            lpFindFileData->dwOID);

	}

	return true;
}

HANDLE CeFindFirstFile(
		LPCWSTR lpFileName, 
		LPCE_FIND_DATA lpFindFileData)
{
	RapiContext* context = rapi_context_current();
	HANDLE handle = INVALID_HANDLE_VALUE;
	
	rapi_context_begin_command(context, 0x00);
	rapi_buffer_write_string(context->send_buffer, lpFileName);

	if ( !rapi_context_call(context) )
		return INVALID_HANDLE_VALUE;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &handle);

	if (!rapi_read_find_data(context, lpFindFileData) )
		return INVALID_HANDLE_VALUE;

	return handle;
}

BOOL CeFindNextFile( 
		HANDLE hFindFile, 
		LPCE_FIND_DATA lpFindFileData)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = false;
	
	rapi_context_begin_command(context, 0x01);
	rapi_buffer_write_uint32(context->send_buffer, hFindFile);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value);
	
	if ( !rapi_read_find_data(context, lpFindFileData) )
		return false;

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
	rapi_buffer_write_uint32(context->send_buffer, (uint32_t)nFolder);
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

BOOL CeMoveFile(
		LPCWSTR lpExistingFileName, 
		LPCWSTR lpNewFileName)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = 0;
	
	rapi_context_begin_command(context, 0x1a);
	rapi_buffer_write_optional_string(context->send_buffer, lpExistingFileName);
	rapi_buffer_write_optional_string(context->send_buffer, lpNewFileName);

	if ( !rapi_context_call(context) )
		return 0;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value);

	return return_value;
}

BOOL CeRemoveDirectory(
		LPCWSTR lpPathName)
{
	RapiContext* context = rapi_context_current();
	BOOL return_value = 0;
	
	rapi_context_begin_command(context, 0x18);
	rapi_buffer_write_optional_string(context->send_buffer, lpPathName);

	if ( !rapi_context_call(context) )
		return 0;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value);

	return return_value;
}

