/* $Id$ */
#include "rapi.h"
#include "rapi_context.h"
#include <stdlib.h>

LONG CeRegCreateKeyEx( /*{{{*/
		HKEY hKey, 
		LPCWSTR lpszSubKey, 
		DWORD Reserved, 
		LPWSTR lpszClass, 
		DWORD ulOptions, 
		REGSAM samDesired, 
		LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
		PHKEY phkResult, 
		LPDWORD lpdwDisposition)
{
	RapiContext* context = rapi_context_current();
	LONG return_value = 0;
	HKEY result = 0;
	DWORD disposition = 0;
	
	rapi_context_begin_command(context, 0x20);
	rapi_buffer_write_uint32(context->send_buffer, hKey);
	rapi_buffer_write_string(context->send_buffer, lpszSubKey);
	rapi_buffer_write_string(context->send_buffer, lpszClass);
	/* remaining parameters are ignored */

	if ( !rapi_context_call(context) )
		return false;
	
	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value);

	if (ERROR_SUCCESS == return_value)
	{
		rapi_buffer_read_uint32(context->recv_buffer, &result);
		rapi_buffer_read_uint32(context->recv_buffer, &disposition);

		if (phkResult)
			*phkResult = result;

		if (lpdwDisposition)
			*lpdwDisposition = disposition;
	}

	return return_value;
}/*}}}*/

LONG CeRegOpenKeyEx(/*{{{*/
		HKEY hKey, 
		LPCWSTR lpszSubKey, 
		DWORD ulOptions, 
		REGSAM samDesired, 
		PHKEY phkResult)
{
	RapiContext* context = rapi_context_current();
	LONG return_value = 0;
	
	rapi_context_begin_command(context, 0x1e);
	rapi_buffer_write_uint32(context->send_buffer, hKey);
	rapi_buffer_write_string(context->send_buffer, lpszSubKey);
	/* remaining parameters are ignored */

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value); 

	if (ERROR_SUCCESS == return_value)
	{
		if (phkResult)
			rapi_buffer_read_uint32(context->recv_buffer, phkResult);
	}

	return return_value;
}/*}}}*/

LONG CeRegQueryValueEx( /*{{{*/
		HKEY hKey, 
		LPCWSTR lpValueName, 
		LPDWORD lpReserved, 
		LPDWORD lpType, 
		LPBYTE lpData, 
		LPDWORD lpcbData)
{
	RapiContext* context = rapi_context_current();
	LONG return_value = -1;
	
	rapi_context_begin_command(context, 0x26);
	rapi_buffer_write_uint32(context->send_buffer, hKey);
	rapi_buffer_write_optional_string(context->send_buffer, lpValueName);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpReserved, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpType, false);
	rapi_buffer_write_optional       (context->send_buffer, lpData, lpcbData ? *lpcbData : 0, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpcbData, true);

	if ( !rapi_context_call(context) )
	{
		synce_trace("rapi_context_call failed");
		return return_value;
	}

	if (!rapi_buffer_read_uint32(context->recv_buffer, &context->last_error))
	{
		synce_trace("rapi_buffer_read_uint32 failed");
		return return_value;
	}
	rapi_buffer_read_uint32(context->recv_buffer, &return_value); 

	if (ERROR_SUCCESS == return_value)
	{
    DWORD type = 0;

    rapi_buffer_read_optional_uint32(context->recv_buffer, &type);
    if (lpType)
      *lpType = type;
      
		if (REG_DWORD == type)
			rapi_buffer_read_optional_uint32(context->recv_buffer, (uint32_t*)lpData);
		else
      rapi_buffer_read_optional(context->recv_buffer, lpData, lpcbData ? *lpcbData * sizeof(WCHAR) : 0);
    
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpcbData);
	}
	else
	{
		synce_trace("CeRegQueryValueEx returning %i", return_value);
	}

	return return_value;
}/*}}}*/

LONG CeRegCloseKey(/*{{{*/
		HKEY hKey)
{
	RapiContext* context = rapi_context_current();
	LONG return_value = 0;
	
	rapi_context_begin_command(context, 0x21);
	rapi_buffer_write_uint32(context->send_buffer, hKey);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value); 

	return return_value;
}/*}}}*/

LONG CeRegQueryInfoKey( /*{{{*/
		HKEY hKey, 
		LPWSTR lpClass, 
		LPDWORD lpcbClass, 
		LPDWORD lpReserved, 
		LPDWORD lpcSubKeys, 
		LPDWORD lpcbMaxSubKeyLen, 
		LPDWORD lpcbMaxClassLen, 
		LPDWORD lpcValues, 
		LPDWORD lpcbMaxValueNameLen, 
		LPDWORD lpcbMaxValueLen, 
		LPDWORD lpcbSecurityDescriptor, 
		PFILETIME lpftLastWriteTime)
{
	RapiContext* context = rapi_context_current();
	LONG return_value = 0;
	
	rapi_context_begin_command(context, 0x25);
	rapi_buffer_write_uint32         (context->send_buffer, hKey);
	rapi_buffer_write_optional       (context->send_buffer, lpClass, lpcbClass ? *lpcbClass * sizeof(WCHAR) : 0, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpcbClass, true);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpReserved, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpcSubKeys, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpcbMaxSubKeyLen, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpcbMaxClassLen, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpcValues, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpcbMaxValueNameLen, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpcbMaxValueLen, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpcbSecurityDescriptor, false);
	rapi_buffer_write_optional       (context->send_buffer, lpftLastWriteTime, sizeof(FILETIME), false);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value); 

	if (ERROR_SUCCESS == return_value)
	{
		rapi_buffer_read_optional       (context->recv_buffer, lpClass, lpcbClass ? *lpcbClass * sizeof(WCHAR) : 0);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpcbClass);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpReserved);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpcSubKeys);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpcbMaxSubKeyLen);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpcbMaxClassLen);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpcValues);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpcbMaxValueNameLen);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpcbMaxValueLen);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpcbSecurityDescriptor);
		rapi_buffer_read_optional_filetime(context->recv_buffer, lpftLastWriteTime);
	}

	return return_value;
}/*}}}*/

LONG CeRegEnumValue( /*{{{*/
		HKEY hKey, 
		DWORD dwIndex, 
		LPWSTR lpszValueName, 
		LPDWORD lpcbValueName, 
		LPDWORD lpReserved, 
		LPDWORD lpType, 
		LPBYTE lpData, 
		LPDWORD lpcbData)
{
	RapiContext* context = rapi_context_current();
	LONG return_value = 0;
	
	rapi_context_begin_command(context, 0x23);
	rapi_buffer_write_uint32         (context->send_buffer, hKey);
	rapi_buffer_write_uint32         (context->send_buffer, dwIndex);
	rapi_buffer_write_optional       (context->send_buffer, lpszValueName, lpcbValueName ? *lpcbValueName * sizeof(WCHAR) : 0, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpcbValueName, true);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpReserved, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpType, false);
	rapi_buffer_write_optional       (context->send_buffer, lpData, lpcbData ? *lpcbData : 0, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpcbData, true);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value); 

	if (ERROR_SUCCESS == return_value)
	{
		rapi_buffer_read_optional       (context->recv_buffer, lpszValueName, lpcbValueName ? *lpcbValueName * sizeof(WCHAR) : 0);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpcbValueName);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpReserved);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpType);
		rapi_buffer_read_optional       (context->recv_buffer, lpData, lpcbData ? *lpcbData : 0);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpcbData);	
	}

	return return_value;
}/*}}}*/

LONG CeRegEnumKeyEx( /*{{{*/
		HKEY hKey, 
		DWORD dwIndex, 
		LPWSTR lpName, 
		LPDWORD lpcbName, 
		LPDWORD lpReserved, 
		LPWSTR lpClass, 
		LPDWORD lpcbClass, 
		PFILETIME lpftLastWriteTime)
{
	RapiContext* context = rapi_context_current();
	LONG return_value = 0;
	
	rapi_context_begin_command(context, 0x1f);
	rapi_buffer_write_uint32         (context->send_buffer, hKey);
	rapi_buffer_write_uint32         (context->send_buffer, dwIndex);
	rapi_buffer_write_optional       (context->send_buffer, lpName, lpcbName ? *lpcbName * sizeof(WCHAR) : 0, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpcbName, true);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpReserved, false);
	rapi_buffer_write_optional       (context->send_buffer, lpClass, lpcbClass ? *lpcbClass * sizeof(WCHAR) : 0, false);
	rapi_buffer_write_optional_uint32(context->send_buffer, lpcbClass, true);
	rapi_buffer_write_optional       (context->send_buffer, lpftLastWriteTime, sizeof(FILETIME), false);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &return_value); 

	if (ERROR_SUCCESS == return_value)
	{
		rapi_buffer_read_optional       (context->recv_buffer, lpName, lpcbName ? *lpcbName * sizeof(WCHAR) : 0);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpcbName);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpReserved);
		rapi_buffer_read_optional       (context->recv_buffer, lpClass, lpcbClass ? *lpcbClass * sizeof(WCHAR) : 0);
		rapi_buffer_read_optional_uint32(context->recv_buffer, lpcbClass);

    if (lpftLastWriteTime)
      rapi_buffer_read_optional_filetime(context->recv_buffer, lpftLastWriteTime);
  }

	return return_value;
}/*}}}*/

LONG CeRegSetValueEx( /*{{{*/
		HKEY hKey, 
		LPCWSTR lpValueName, 
		DWORD Reserved, 
		DWORD dwType, 
		const BYTE *lpData, 
		DWORD cbData)
{
	RapiContext* context = rapi_context_current();
	LONG return_value = 0;
	
	rapi_context_begin_command(context, 0x27);
	rapi_buffer_write_uint32(context->send_buffer, hKey);
	rapi_buffer_write_optional_string(context->send_buffer, lpValueName);
	rapi_buffer_write_uint32(context->send_buffer, dwType);
	if (dwType == REG_DWORD)
	{
		rapi_buffer_write_optional_uint32 (context->send_buffer, (uint32_t*)lpData, true);
	}
	else
	{
	rapi_buffer_write_optional(context->send_buffer, lpData, cbData, true);
	}
	rapi_buffer_write_uint32(context->send_buffer, cbData);

	if ( !rapi_context_call(context) )
		return false;
	
	if (!rapi_buffer_read_uint32(context->recv_buffer, &context->last_error))
		return false;
	if (!rapi_buffer_read_uint32(context->recv_buffer, &return_value))
		return false;

	return return_value;
}/*}}}*/

bool rapi_reg_create_key(/*{{{*/
		HKEY parent, const char* name, HKEY* key)
{
	WCHAR* name_wide = wstr_from_current(name);

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
	WCHAR* value_wide = wstr_from_current(value);
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
