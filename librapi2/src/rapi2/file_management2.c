/* $Id$ */
#include "rapi_internal.h"
#include "rapi2_api.h"
#include "rapi_buffer.h"
#include "rapi_context.h"
#include <string.h>
#include <stdlib.h>

DWORD _CeGetSpecialFolderPath2(
        int nFolder,
        DWORD nBufferLength,
        LPWSTR lpBuffer)
{
    RapiContext* context = rapi_context_current();
    size_t string_length = nBufferLength;

    rapi_context_begin_command(context, 0x4b);
    rapi_buffer_write_uint32(context->send_buffer, (uint32_t)nFolder);
    rapi_buffer_write_uint32(context->send_buffer, nBufferLength);

    if ( !rapi2_context_call(context) )
        return 0;

    if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
        return 0;
    synce_trace("last_error = %i", context->last_error);

    if ( !rapi_buffer_read_string(context->recv_buffer, lpBuffer, &string_length) )
        return 0;

    return string_length;
}


BOOL _CeFindAllFiles2(
        LPCWSTR szPath,
        DWORD dwFlags,
        LPDWORD lpdwFoundCount,
        LPLPCE_FIND_DATA ppFindDataArray)
{
    RapiContext* context = rapi_context_current();
    uint32_t count = 0;

    rapi_context_begin_command(context, 0x1a);
    rapi2_buffer_write_string(context->send_buffer, szPath);
    rapi_buffer_write_uint32(context->send_buffer, dwFlags);

    if ( !rapi2_context_call(context) )
        return false;

    /* TODO Don't know this two uint32_t fields */
    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    uint32_t unknown;
    rapi_buffer_read_uint32(context->recv_buffer, &unknown);

    rapi_buffer_read_uint32(context->recv_buffer, &count);

    synce_trace("found %i files", count);

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
                rapi_buffer_read_data(context->recv_buffer, array[i].cFileName, name_size );
                synce_trace_wstr(array[i].cFileName);
            }
        }

        if (ppFindDataArray)
            *ppFindDataArray = array;
    }

    if (lpdwFoundCount)
        *lpdwFoundCount = count;

    return true;
}


HANDLE _CeFindFirstFile2(
        LPCWSTR lpFileName,
        LPCE_FIND_DATA lpFindFileData)
{
    RapiContext* context = rapi_context_current();
    HANDLE handle = INVALID_HANDLE_VALUE;

    rapi_context_begin_command(context, 0x11);
    rapi2_buffer_write_string(context->send_buffer, lpFileName);

    if ( !rapi2_context_call(context) )
        return INVALID_HANDLE_VALUE;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, &handle);

    if ( !rapi_buffer_read_find_data(context->recv_buffer, lpFindFileData) )
        return INVALID_HANDLE_VALUE;

    return handle;
}


DWORD _CeGetFileAttributes2(
        LPCWSTR lpFileName)
{
    RapiContext* context = rapi_context_current();
    DWORD return_value = 0xFFFFFFFF;

    rapi_context_begin_command(context, 0x14);
    rapi2_buffer_write_string(context->send_buffer, lpFileName);

    if ( !rapi2_context_call(context) )
        goto exit;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, &return_value);

exit:
    return return_value;
}


BOOL _CeDeleteFile2(
        LPCWSTR lpFileName)
{
    RapiContext* context = rapi_context_current();
    BOOL return_value = 0;

    rapi_context_begin_command(context, 0x2d);
    /*    rapi_buffer_write_optional_string(context->send_buffer, lpFileName);*/
    rapi2_buffer_write_string(context->send_buffer, lpFileName);

    if ( !rapi2_context_call(context) )
        return 0;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, &return_value);

    return return_value;
}


BOOL _CeCreateDirectory2(
        LPCWSTR lpPathName,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    RapiContext* context = rapi_context_current();
    BOOL return_value = 0;

    rapi_context_begin_command(context, 0x28);
    rapi2_buffer_write_string(context->send_buffer, lpPathName);
    /*
    rapi_buffer_write_optional_string(context->send_buffer, lpPathName);
    rapi_buffer_write_optional_in(context->send_buffer, NULL, 0); */ /* lpSecurityAttributes */

    if ( !rapi2_context_call(context) )
        return 0;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, &return_value);

    return return_value;
}


BOOL _CeRemoveDirectory2(
        LPCWSTR lpPathName)
{
    RapiContext* context = rapi_context_current();
    BOOL return_value = 0;

    rapi_context_begin_command(context, 0x29);
    rapi2_buffer_write_string(context->send_buffer, lpPathName);
    /*    rapi_buffer_write_optional_string(context->send_buffer, lpPathName); */

    if ( !rapi2_context_call(context) )
        return 0;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, &return_value);

    return return_value;
}

#define BAD_FILE_SIZE 0xffffffff

DWORD _CeGetFileSize2(
        HANDLE hFile,
        LPDWORD lpFileSizeHigh)
{
    RapiContext* context = rapi_context_current();
    DWORD size = BAD_FILE_SIZE;

    rapi_context_begin_command(context, 0x2e);
    rapi_buffer_write_uint32(context->send_buffer, (uint32_t)hFile);
    rapi_buffer_write_optional_out(context->send_buffer,
    lpFileSizeHigh, sizeof(*lpFileSizeHigh));

    if ( !rapi2_context_call(context) )
        return BAD_FILE_SIZE;

    if ( !rapi_buffer_read_uint32(context->recv_buffer, &context->last_error) )
        return BAD_FILE_SIZE;
    synce_trace("last_error = %i", context->last_error);

    if ( !rapi_buffer_read_uint32(context->recv_buffer, &size) )
        return BAD_FILE_SIZE;

    if ( !rapi_buffer_read_optional_uint32(context->recv_buffer, lpFileSizeHigh) )
        return BAD_FILE_SIZE;

    return size;
}


/**
 * This function copies an existing file to a new file.
 *
 * Unicode version.
 */
BOOL _CeCopyFile2(
        LPCWSTR lpExistingFileName,
        LPCWSTR lpNewFileName,
        BOOL bFailIfExists)
{
    RapiContext* context = rapi_context_current();
    BOOL return_value = 0;

    rapi_context_begin_command(context, 0x2c);
    rapi2_buffer_write_string(context->send_buffer, lpExistingFileName);
    rapi2_buffer_write_string(context->send_buffer, lpNewFileName);
    rapi_buffer_write_uint32(context->send_buffer, bFailIfExists);

    if ( !rapi2_context_call(context) )
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
BOOL _CeCopyFileA2(
        LPCSTR lpExistingFileName,
        LPCSTR lpNewFileName,
        BOOL bFailIfExists)
{
    BOOL return_value = 0;
    LPWSTR lpExistingFileNameW = NULL;
    LPWSTR lpNewFileNameW = NULL;

    lpExistingFileNameW = wstr_from_current(lpExistingFileName);
    lpNewFileNameW      = wstr_from_current(lpNewFileName);

    if (lpExistingFileName && !lpExistingFileNameW)
        goto fail;

    if (lpNewFileName && !lpNewFileNameW)
        goto fail;

    return_value = CeCopyFile(lpExistingFileNameW, lpNewFileNameW, bFailIfExists);

fail:
    wstr_free_string(lpExistingFileNameW);
    wstr_free_string(lpNewFileNameW);

    return return_value;
}

BOOL _CeMoveFile2(
        LPCWSTR lpExistingFileName,
        LPCWSTR lpNewFileName)
{
    RapiContext* context = rapi_context_current();
    BOOL return_value = 0;

    rapi_context_begin_command(context, 0x2b);
    rapi2_buffer_write_string(context->send_buffer, lpExistingFileName);
    rapi2_buffer_write_string(context->send_buffer, lpNewFileName);

    if ( !rapi2_context_call(context) )
        return 0;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, &return_value);

    return return_value;
}
