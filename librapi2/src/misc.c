/* $Id$ */
#undef __STRICT_ANSI__
#define _GNU_SOURCE
#include "rapi_context.h"
#include <stdlib.h>


/* Standard rapi-calls valid for PocketPC 2002/2003 and Windows Mobile 5 */
HRESULT CeRapiFreeBuffer(
    LPVOID Buffer )
{
    free( Buffer );
    return S_OK;
}

HRESULT CeRapiInit( void ) /*{{{*/
{
    RapiContext * context = rapi_context_current();

    return rapi_context_connect( context );
} /*}}}*/

STDAPI CeRapiUninit( void ) /*{{{*/
{
    RapiContext * context = rapi_context_current();

    if ( context->is_initialized )
    {
        rapi_context_free( context );
        return S_OK;
    } else
    {
        return E_FAIL;
    }
} /*}}}*/

HRESULT CeRapiGetError( void ) /*{{{*/
{
    RapiContext * context = rapi_context_current();
    return context->rapi_error;
} /*}}}*/

DWORD CeRapiGetLastError()
{
    RapiContext * context = rapi_context_current();
    return context->last_error;
}

DWORD CeGetLastError( void )
{
    RapiContext* context = rapi_context_current();
    return context->last_error;
}
/* End Standard rapi-calls */

bool rapi_reg_create_key(/*{{{*/
        HKEY parent, const char* name, HKEY* key)
{
    WCHAR* name_wide = wstr_from_current(name);
    if (!name_wide)
            return FALSE;

    LONG result = CeRegCreateKeyEx(
            parent,
    name_wide,
    0,
    NULL,
    0,
    0,
    NULL,
    key,
    NULL
                                  );

    wstr_free_string(name_wide);

    return ERROR_SUCCESS == result;
}/*}}}*/

bool rapi_reg_open_key(/*{{{*/
        HKEY parent, const char* name, HKEY* key)
{
    WCHAR* name_wide = wstr_from_current(name);
    if (!name_wide)
            return FALSE;

    LONG result = CeRegOpenKeyEx(
            parent,
    name_wide,
    0,
    0,
    key
                                );

    wstr_free_string(name_wide);

    return ERROR_SUCCESS == result;
}/*}}}*/

bool rapi_reg_query_dword(/*{{{*/
        HKEY key, const char* name, DWORD* value)
{
    DWORD type;
    DWORD size = sizeof(DWORD);
    WCHAR* name_wide = wstr_from_current(name);
    if (!name_wide)
            return FALSE;

    LONG result = CeRegQueryValueEx(
            key,
    name_wide,
    NULL,
    &type,
    (LPBYTE)value,
    &size);

    wstr_free_string(name_wide);

    return
            ERROR_SUCCESS == result &&
            REG_DWORD == type &&
            sizeof(DWORD) == size;
}/*}}}*/

bool rapi_reg_query_string(/*{{{*/
        HKEY key, const char* name, char** value)
{
    bool success = false;
    DWORD type;
    DWORD size = 0;
    WCHAR* unicode = NULL;
    WCHAR* name_wide = wstr_from_current(name);
    if (!name_wide)
            return FALSE;

    LONG result = CeRegQueryValueEx(
            key,
    name_wide,
    NULL,
    &type,
    NULL,
    &size);

    if (ERROR_SUCCESS == result && REG_SZ == type)
    {
        unicode = calloc(1, size);

        result = CeRegQueryValueEx(
                key,
        name_wide,
        NULL,
        &type,
        (LPBYTE)unicode,
        &size);

    }

    if (ERROR_SUCCESS == result && REG_SZ == type)
    {
        *value = wstr_to_current(unicode);
        success = true;
    }

    free(unicode);
    wstr_free_string(name_wide);

    return success;
}/*}}}*/

bool rapi_reg_set_dword(/*{{{*/
        HKEY key, const char* name, DWORD value)
{
    WCHAR* name_wide = wstr_from_current(name);
    if (!name_wide)
            return FALSE;

    LONG result = CeRegSetValueEx(
            key,
    name_wide,
    0,
    REG_DWORD,
    (BYTE*)&value,
    sizeof(DWORD));

    wstr_free_string(name_wide);

    return ERROR_SUCCESS == result;
}/*}}}*/

bool rapi_reg_set_string(/*{{{*/
        HKEY key, const char* name, const char *value)
{
    WCHAR* name_wide = wstr_from_current(name);
    if (!name_wide)
            return FALSE;
    WCHAR* value_wide = wstr_from_current(value);
    if (!value_wide) {
            wstr_free_string(name_wide);
            return FALSE;
    }
    DWORD size = wstrlen(value_wide);

    LONG result = CeRegSetValueEx(
            key,
    name_wide,
    0,
    REG_SZ,
    (BYTE*)value_wide,
    (size * 2) + 2);

    wstr_free_string(name_wide);
    wstr_free_string(value_wide);

    return ERROR_SUCCESS == result;
}/*}}}*/
