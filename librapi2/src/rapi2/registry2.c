/* $Id: registry.c 2355 2006-04-07 18:47:20Z voc $ */
#include "rapi2_api.h"
#include "rapi_context.h"
#include <stdlib.h>
#include <stdio.h>


LONG _CeRegCreateKeyEx2(
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
	LONG return_value = ERROR_GEN_FAILURE;
	HKEY result = 0;
	DWORD disposition = 0;

	rapi_context_begin_command(context, 0x31);
	rapi_buffer_write_uint32(context->send_buffer, hKey);
	rapi2_buffer_write_string(context->send_buffer, lpszSubKey);
	rapi2_buffer_write_string(context->send_buffer, lpszClass);

	if ( !rapi2_context_call(context) )
		return ERROR_GEN_FAILURE;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_int32(context->recv_buffer, &return_value);

	if (ERROR_SUCCESS == return_value)
	{
		rapi_buffer_read_int32(context->recv_buffer, &result);
		rapi_buffer_read_uint32(context->recv_buffer, &disposition);

		if (phkResult)
			*phkResult = result;

		if (lpdwDisposition)
			*lpdwDisposition = disposition;
	}

	return return_value;
}

LONG _CeRegOpenKeyEx2(
		HKEY hKey,
		LPCWSTR lpszSubKey,
		DWORD ulOptions,
		REGSAM samDesired,
		PHKEY phkResult)
{
	RapiContext* context = rapi_context_current();
	LONG return_value = ERROR_GEN_FAILURE;

	rapi_context_begin_command(context, 0x2f);
	rapi_buffer_write_uint32(context->send_buffer, hKey);
	rapi2_buffer_write_string(context->send_buffer, lpszSubKey);

	if ( !rapi2_context_call(context) )
		return ERROR_GEN_FAILURE;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_int32(context->recv_buffer, &return_value);

	if (ERROR_SUCCESS == return_value)
	{
		if (phkResult)
			rapi_buffer_read_int32(context->recv_buffer, phkResult);
	}

	return return_value;
}


LONG _CeRegQueryValueEx2(
		HKEY hKey,
		LPCWSTR lpValueName,
		LPDWORD lpReserved,
		LPDWORD lpType,
		LPBYTE lpData,
		LPDWORD lpcbData)
{
	RapiContext* context = rapi_context_current();
	LONG return_value = ERROR_GEN_FAILURE;
	rapi_context_begin_command(context, 0x37);
	rapi_buffer_write_uint32(context->send_buffer, hKey);
	rapi2_buffer_write_string(context->send_buffer, lpValueName);
    rapi_buffer_write_uint32(context->send_buffer, *lpcbData);

	if ( !rapi2_context_call(context) )
	{
		synce_trace("rapi2_context_call failed");
		return return_value;
	}

	if (!rapi_buffer_read_uint32(context->recv_buffer, &context->last_error))
	{
		synce_trace("rapi_buffer_read_uint32 failed");
		return return_value;
	}
	rapi_buffer_read_int32(context->recv_buffer, &return_value);

	if (ERROR_SUCCESS == return_value)
	{
        DWORD type = 0;

        rapi_buffer_read_uint32(context->recv_buffer, &type);
        rapi_buffer_read_uint32(context->recv_buffer, (uint32_t*)lpcbData);

        if (lpType)
            *lpType = type;

        if (lpData) {
            if (REG_DWORD == type) {
                rapi_buffer_read_uint32(context->recv_buffer, (uint32_t *) lpData);
            } else {
                rapi_buffer_read_data(context->recv_buffer, lpData, *lpcbData);
            }
        }
    }
#if 0
	else
	{
		synce_trace("CeRegQueryValueEx returning %i", return_value);
	}
#endif

	return return_value;
}


LONG _CeRegCloseKey2(
		HKEY hKey)
{
	RapiContext* context = rapi_context_current();
	LONG return_value = ERROR_GEN_FAILURE;

	rapi_context_begin_command(context, 0x32);
	rapi_buffer_write_uint32(context->send_buffer, hKey);

	if ( !rapi2_context_call(context) )
		return ERROR_GEN_FAILURE;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_int32(context->recv_buffer, &return_value);

	return return_value;
}


LONG _CeRegDeleteKey2(
        HKEY hKey,
        LPCWSTR lpszSubKey)
{
    RapiContext* context = rapi_context_current();
    LONG return_value = ERROR_GEN_FAILURE;

    rapi_context_begin_command(context, 0x33);
    rapi_buffer_write_uint32(context->send_buffer, hKey);
    rapi2_buffer_write_string(context->send_buffer, lpszSubKey);

    if ( !rapi2_context_call(context) )
      return ERROR_GEN_FAILURE;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_int32(context->recv_buffer, &return_value);

    return return_value;
}


LONG _CeRegDeleteValue2(
        HKEY hKey,
        LPCWSTR lpszValueName)
{
    RapiContext* context = rapi_context_current();
    LONG return_value = ERROR_GEN_FAILURE;

    rapi_context_begin_command(context, 0x35);
    rapi_buffer_write_uint32(context->send_buffer, hKey);
    rapi2_buffer_write_string(context->send_buffer, lpszValueName);

    if ( !rapi2_context_call(context) )
      return ERROR_GEN_FAILURE;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_int32(context->recv_buffer, &return_value);

    return return_value;
}



LONG _CeRegQueryInfoKey2(
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
	LONG return_value = ERROR_GEN_FAILURE;

	rapi_context_begin_command(context, 0x36);
	rapi_buffer_write_uint32        (context->send_buffer, hKey);
	rapi_buffer_write_uint32        (context->send_buffer, lpcbClass ? *lpcbClass : 0 );

	//lpftLastWriteTime, this should be put to 0
	rapi_buffer_write_uint32        (context->send_buffer, 0 );


	if ( !rapi2_context_call(context) )
		return ERROR_GEN_FAILURE;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_int32(context->recv_buffer, &return_value);

	
	
	if (ERROR_SUCCESS == return_value)
	{
		DWORD foo = 0 ; 
		rapi_buffer_read_uint32 (context->recv_buffer, &foo ) ; 
		if (lpClass)
		{
			*lpcbClass = foo ;
			rapi_buffer_read_string(context->recv_buffer, lpClass, lpcbClass );

		}

		
		rapi_buffer_read_uint32 (context->recv_buffer, &foo ) ; 
		if (lpcSubKeys)
			*lpcSubKeys = foo ; 
		rapi_buffer_read_uint32 (context->recv_buffer, &foo ) ; 
		if (lpcbMaxSubKeyLen)
			*lpcbMaxSubKeyLen = foo ; 
		rapi_buffer_read_uint32 (context->recv_buffer, &foo ) ; 
		if (lpcbMaxClassLen)
			*lpcbMaxClassLen = foo ; 
		rapi_buffer_read_uint32 (context->recv_buffer, &foo ) ; 
		if (lpcValues)
			*lpcValues = foo ; 
		rapi_buffer_read_uint32 (context->recv_buffer, &foo ) ; 
		if (lpcbMaxValueNameLen)
			*lpcbMaxValueNameLen = foo ; 
		rapi_buffer_read_uint32 (context->recv_buffer, &foo ) ; 
		if (lpcbMaxValueLen)
			*lpcbMaxValueLen = foo ; 
	}

	return return_value;
}


LONG _CeRegEnumValue2(
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
	LONG return_value = ERROR_GEN_FAILURE;

	rapi_context_begin_command(context, 0x34);
	//These two values are NOT allowed to be NULL, therefore, just send them
	//right away
	rapi_buffer_write_uint32         (context->send_buffer, hKey);
	rapi_buffer_write_uint32         (context->send_buffer, dwIndex);

	
	
	//After some testing, turned out that we need to tell the device the
	//size of the buffer we have on our side. BUT, you are not allowed to 
	//send 0, probably you tell the device how much total you expect for the
	//string to be in the reply of the device, which includes the actual 
	//length of the string
	//
	//The device reports |lenght|string
	//
	//If we send the value 00, this results in a problem, since the device
	//can't even send the legth of the string.
	//The length reported by the device should be 1 less then the value 
	
	//So if the user is not interested in the value 
	//(i.e. pcbData, or lpcbValueName is null) then send value 1
	
	//Furthermore, DON'T use the write_optional, for some reason 
	//that does not work, at writes too many things to the send buffer
	rapi_buffer_write_uint32(context->send_buffer, lpcbValueName ? *lpcbValueName : 1 );
	rapi_buffer_write_uint32(context->send_buffer, lpcbData ? *lpcbData : 1 );

	if ( !rapi2_context_call(context) )
		return false;


	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_int32(context->recv_buffer, &return_value);
	


	if (ERROR_SUCCESS == return_value)
	{
		DWORD foo ; 

		//If the user wants to know the valuename
		if (lpszValueName){
			//First read the valuename		
			rapi_buffer_read_string(context->recv_buffer, lpszValueName, lpcbValueName ) ; 
			//Just to be sure for the moment:
			lpszValueName[*lpcbValueName] = 0 ; 
		}
		else{
			//Other the value 0 will be printed to the buffer by the device, indicating
			//the length of the string sent
			rapi_buffer_read_uint32(context->recv_buffer, &foo) ; 
		}

	
		//Then read the type, this is always returned by the device
		//Just skip the value if it is not needed.
		foo = 0 ; 
        rapi_buffer_read_uint32(context->recv_buffer, &foo );
        if (lpType)
            *lpType = foo ;

		
		//Then read the data if needed
        rapi_buffer_read_uint32(context->recv_buffer, &foo );
        if (lpData) {
			*lpcbData = foo ; 

            if (REG_DWORD == *lpType) {
                rapi_buffer_read_uint32(context->recv_buffer, (uint32_t *) lpData);
            } else {
                rapi_buffer_read_data(context->recv_buffer, lpData, *lpcbData);
            }
        }
	}

	return return_value;
}


LONG _CeRegEnumKeyEx2(
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
	LONG return_value = ERROR_GEN_FAILURE;

	rapi_context_begin_command(context, 0x30);
	rapi_buffer_write_uint32         (context->send_buffer, hKey);
	rapi_buffer_write_uint32         (context->send_buffer, dwIndex);

    rapi_buffer_write_uint32(context->send_buffer, *lpcbName);
	rapi_buffer_write_uint32(context->send_buffer, lpcbClass ? *lpcbClass : 0 );
    rapi_buffer_write_uint32(context->send_buffer, 0);


	if ( !rapi2_context_call(context) )
		return ERROR_GEN_FAILURE;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_int32(context->recv_buffer, &return_value);

	if (ERROR_SUCCESS == return_value)
    {
		if (lpName)
		{
			rapi_buffer_read_string(context->recv_buffer, lpName, lpcbName );
		}
		if (lpClass)
		{
			rapi_buffer_read_string(context->recv_buffer, lpClass, lpcbClass );
		}
    }

	return return_value;
}

LONG _CeRegSetValueEx2(
		HKEY hKey,
		LPCWSTR lpValueName,
		DWORD Reserved,
		DWORD dwType,
		const BYTE *lpData,
		DWORD cbData)
{
	RapiContext* context = rapi_context_current();
	LONG return_value = ERROR_GEN_FAILURE;

	rapi_context_begin_command(context, 0x38);
	rapi_buffer_write_uint32(context->send_buffer, hKey);
	rapi2_buffer_write_string(context->send_buffer, lpValueName);
	rapi_buffer_write_uint32(context->send_buffer, dwType);
        rapi_buffer_write_uint32(context->send_buffer, cbData);
        rapi_buffer_write_data(context->send_buffer, lpData, cbData);

	if ( !rapi2_context_call(context) )
		return ERROR_GEN_FAILURE;

	if (!rapi_buffer_read_uint32(context->recv_buffer, &context->last_error))
		return ERROR_GEN_FAILURE;
	if (!rapi_buffer_read_int32(context->recv_buffer, &return_value))
		return ERROR_GEN_FAILURE;

	return return_value;
}

