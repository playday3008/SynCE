/* $Id$ */
#include "rapi.h"
#include "rapi_context.h"
#include "rapi_endian.h"

LONG CeRegCreateKeyEx( 
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
}

LONG CeRegOpenKeyEx(
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
}

LONG CeRegCloseKey(
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
}

LONG CeRegQueryInfoKey( 
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
}

LONG CeRegEnumValue( 
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
}

LONG CeRegEnumKeyEx( 
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
		rapi_buffer_read_optional_filetime(context->recv_buffer, lpftLastWriteTime);
	}

	return return_value;
}

