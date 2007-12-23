/* $Id: misc.c 2362 2006-04-09 10:38:36Z voc $ */
#include "rapi_internal.h"
#include "rapi2_api.h"
#include "rapi_context.h"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

BOOL _CeCreateProcess2(/*{{{*/
        LPCWSTR lpApplicationName,
        LPCWSTR lpCommandLine,
        void* lpProcessAttributes,
        void* lpThreadAttributes,
        BOOL bInheritHandles,
        DWORD dwCreationFlags,
        LPVOID lpEnvironment,
        LPWSTR lpCurrentDirectory,
        void* lpStartupInfo,
        LPPROCESS_INFORMATION lpProcessInformation)
{
    RapiContext* context = rapi_context_current();
    BOOL result = false;

    rapi_context_begin_command(context, 0x2a);
    rapi2_buffer_write_string(context->send_buffer, lpApplicationName);
    rapi2_buffer_write_string(context->send_buffer, lpCommandLine);
    rapi_buffer_write_uint32(context->send_buffer, 0);
    rapi_buffer_write_uint32(context->send_buffer, 0);
    rapi_buffer_write_uint32(context->send_buffer, 0);
    rapi_buffer_write_uint32(context->send_buffer, dwCreationFlags);
    rapi_buffer_write_uint32(context->send_buffer, 0);
    rapi_buffer_write_uint32(context->send_buffer, 0);
    rapi_buffer_write_uint32(context->send_buffer, 0);
    /*    rapi_buffer_write_optional_out(context->send_buffer, lpProcessInformation, sizeof(PROCESS_INFORMATION)); */

    if ( !rapi2_context_call(context) )
        goto exit;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    synce_trace("last_error = %i", context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, &result);
    synce_trace("result = %i", result);

    if ( !rapi_buffer_read_optional(context->recv_buffer, lpProcessInformation, sizeof(PROCESS_INFORMATION)) )
        goto exit;

    if (lpProcessInformation)
    {
        lpProcessInformation->hProcess     = letoh32(lpProcessInformation->hProcess);
        lpProcessInformation->hThread      = letoh32(lpProcessInformation->hThread);
        lpProcessInformation->dwProcessId  = letoh32(lpProcessInformation->dwProcessId);
        lpProcessInformation->dwThreadId   = letoh32(lpProcessInformation->dwThreadId);
    }

exit:
        return result;
}/*}}}*/


void _CeGetSystemInfo2( /*{{{*/
        LPSYSTEM_INFO lpSystemInfo)
{
    RapiContext* context = rapi_context_current();

    rapi_context_begin_command(context, 0x3d);
    rapi_buffer_write_optional_out(context->send_buffer, lpSystemInfo, sizeof(SYSTEM_INFO));

    if ( !rapi2_context_call(context) )
        return;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    synce_trace("last_error = %i", context->last_error);

    if ( !rapi_buffer_read_data(context->recv_buffer, lpSystemInfo, sizeof(SYSTEM_INFO)) )
    {
        synce_error("Failed to read lpSystemInfo");
        return;
    }

    if (lpSystemInfo)
    {
        lpSystemInfo->wProcessorArchitecture       = letoh16(lpSystemInfo->wProcessorArchitecture);
        lpSystemInfo->wReserved                    = letoh16(lpSystemInfo->wReserved);
        lpSystemInfo->dwPageSize                   = letoh32(lpSystemInfo->dwPageSize);
        lpSystemInfo->lpMinimumApplicationAddress  = letoh32(lpSystemInfo->lpMinimumApplicationAddress);
        lpSystemInfo->lpMaximumApplicationAddress  = letoh32(lpSystemInfo->lpMaximumApplicationAddress);
        lpSystemInfo->dwActiveProcessorMask        = letoh32(lpSystemInfo->dwActiveProcessorMask);
        lpSystemInfo->dwNumberOfProcessors         = letoh32(lpSystemInfo->dwNumberOfProcessors);
        lpSystemInfo->dwProcessorType              = letoh32(lpSystemInfo->dwProcessorType);
        lpSystemInfo->dwAllocationGranularity      = letoh32(lpSystemInfo->dwAllocationGranularity);
        lpSystemInfo->wProcessorLevel              = letoh16(lpSystemInfo->wProcessorLevel);
        lpSystemInfo->wProcessorRevision           = letoh16(lpSystemInfo->wProcessorRevision);
    }

    return;
}/*}}}*/


BOOL _CeGetVersionEx2(/*{{{*/
        LPCEOSVERSIONINFO lpVersionInformation)
{
    RapiContext* context = rapi_context_current();
    BOOL result = false;
    uint32_t size = 0;

    rapi_context_begin_command(context, 0x43);

    if ( !rapi2_context_call(context) )
        return false;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    synce_trace("last_error = %i", context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, &result);
    synce_trace("result = %i", result);

        rapi_buffer_read_uint32(context->recv_buffer, &size);

    if ( !rapi_buffer_read_data(context->recv_buffer, lpVersionInformation, size) )
        return false;
    /*
    rapi_buffer_read_uint32(context->recv_buffer, &lpVersionInformation->dwOSVersionInfoSize);
    rapi_buffer_read_uint32(context->recv_buffer, &lpVersionInformation->dwMajorVersion);
    rapi_buffer_read_uint32(context->recv_buffer, &lpVersionInformation->dwMinorVersion);
    rapi_buffer_read_uint32(context->recv_buffer, &lpVersionInformation->dwBuildNumber);
    rapi_buffer_read_uint32(context->recv_buffer, &lpVersionInformation->dwPlatformId);
    */
    lpVersionInformation->dwOSVersionInfoSize  = letoh32(lpVersionInformation->dwOSVersionInfoSize);
    lpVersionInformation->dwMajorVersion       = letoh32(lpVersionInformation->dwMajorVersion);
    lpVersionInformation->dwMinorVersion       = letoh32(lpVersionInformation->dwMinorVersion);
    lpVersionInformation->dwBuildNumber        = letoh32(lpVersionInformation->dwBuildNumber);
    lpVersionInformation->dwPlatformId         = letoh32(lpVersionInformation->dwPlatformId);


    return result;
}/*}}}*/


BOOL _CeGetSystemPowerStatusEx2( /*{{{*/
        PSYSTEM_POWER_STATUS_EX pSystemPowerStatus,
        BOOL refresh)
{
    RapiContext* context = rapi_context_current();
    BOOL result = false;

    rapi_context_begin_command(context, 0x49);
    /*    rapi_buffer_write_optional_out(context->send_buffer, pSystemPowerStatus, sizeof(SYSTEM_POWER_STATUS_EX)); */
    rapi_buffer_write_uint32(context->send_buffer, refresh);

    if ( !rapi2_context_call(context) )
        goto exit;

    /*    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error); */
    rapi_buffer_read_uint32(context->recv_buffer, &result);

    if ( !rapi_buffer_read_data(context->recv_buffer, pSystemPowerStatus, sizeof(SYSTEM_POWER_STATUS_EX)) )
        goto exit;

    pSystemPowerStatus->BatteryLifeTime           = letoh32(pSystemPowerStatus->BatteryLifeTime);
    pSystemPowerStatus->BatteryFullLifeTime       = letoh32(pSystemPowerStatus->BatteryFullLifeTime);
    pSystemPowerStatus->BackupBatteryLifeTime     = letoh32(pSystemPowerStatus->BackupBatteryLifeTime);
    pSystemPowerStatus->BackupBatteryFullLifeTime = letoh32(pSystemPowerStatus->BackupBatteryFullLifeTime);

exit:
        /*        return true; */
        return result;
}/*}}}*/


BOOL _CeGetStoreInformation2( /*{{{*/
        LPSTORE_INFORMATION lpsi)
{
    RapiContext* context = rapi_context_current();
    BOOL result = false;

    rapi_context_begin_command(context, 0x39);
    /*    rapi_buffer_write_optional_out(context->send_buffer, lpsi, sizeof(STORE_INFORMATION)); */

    if ( !rapi2_context_call(context) )
        goto exit;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, &result);

    if ( !rapi_buffer_read_data(context->recv_buffer, lpsi, sizeof(STORE_INFORMATION)) )
    {
        synce_error("Failed to read lpsi");
        goto exit;
    }

    if (lpsi)
    {
        lpsi->dwStoreSize = letoh32(lpsi->dwStoreSize);
        lpsi->dwFreeSize = letoh32(lpsi->dwFreeSize);
    }

exit:
        return result;
}/*}}}*/


BOOL _CeGetSystemMemoryDivision2(
        LPDWORD lpdwStoragePages,
    LPDWORD lpdwRamPages,
    LPDWORD lpdwPageSize)
{
    /*    RapiContext* context = rapi_context_current(); */
    BOOL result = false;

    /* Do we have this call on WM5? */
    /*
    rapi_context_begin_command(context, 0x28);
    rapi_buffer_write_optional_uint32(context->send_buffer, lpdwStoragePages, false);
    rapi_buffer_write_optional_uint32(context->send_buffer, lpdwRamPages,     false);
    rapi_buffer_write_optional_uint32(context->send_buffer, lpdwPageSize,     false);

    if ( !rapi2_context_call(context) )
        goto exit;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, &result);

    rapi_buffer_read_uint32(context->recv_buffer, lpdwStoragePages);
    rapi_buffer_read_uint32(context->recv_buffer, lpdwRamPages);
    rapi_buffer_read_uint32(context->recv_buffer, lpdwPageSize);
    */
    /* exit: */
        return result;
}


/**
  Undocumented function used by the RapiConfig.exe tool
 */

HRESULT _CeProcessConfig2(LPCWSTR config, DWORD flags, LPWSTR* reply)
{
    RapiContext* context = rapi_context_current();
    HRESULT result = E_UNEXPECTED;
    DWORD size = 0;
    LPWSTR buffer = NULL;

    if (!config || !reply)
    {
        synce_error("Bad parameter(s)");
        goto exit;
    }

    rapi_context_begin_command(context, 0x0e);

    rapi2_buffer_write_string(context->send_buffer, config);
    rapi_buffer_write_uint32(context->send_buffer, flags);

    if ( !rapi2_context_call(context) )
        return false;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_int32(context->recv_buffer, &result);

    if (!rapi_buffer_read_uint32(context->recv_buffer, &size))
        goto exit;

    synce_trace("size = 0x%08x", size);

    buffer = (LPWSTR)malloc(size);
    if (!buffer)
    {
        synce_error("Failed to allocated %i bytes", size);
        goto exit;
    }

    if (!rapi_buffer_read_data(context->recv_buffer, buffer, size))
        goto exit;

    *reply = buffer;

exit:
    return result;
}


/**
 * Undocumented function used by ActiveSync to begin synchronization
 *
 * See this mail for details:
 *
 * http://sourceforge.net/mailarchive/forum.php?thread_id=844008&forum_id=1226
 */
BOOL _CeStartReplication2( void )/*{{{*/
{
    RapiContext* context = rapi_context_current();
    BOOL return_value = FALSE;

    rapi_context_begin_command(context, 0x02);

    if ( !rapi2_context_call(context) )
        return false;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, &return_value);

    return return_value;
}/*}}}*/


HRESULT _CeSyncStart2(LPCWSTR params)
{
    RapiContext* context = rapi_context_current();
    HRESULT return_value = -1;

    rapi_context_begin_command(context, 0x59);

    rapi2_buffer_write_string(context->send_buffer, params);

    if ( !rapi2_context_call(context) )
        return false;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, (DWORD *) &return_value);

    return return_value;
}


HRESULT _CeSyncResume2(void)
{
    RapiContext* context = rapi_context_current();
    HRESULT return_value = -1;

    rapi_context_begin_command(context, 0x10);

    if ( !rapi2_context_call(context) )
        return false;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, (DWORD *) &return_value);

    return return_value;
}


HRESULT _CeSyncPause2(void)
{
    RapiContext* context = rapi_context_current();
    HRESULT return_value = -1;

    rapi_context_begin_command(context, 0x0f);

    if ( !rapi2_context_call(context) )
        return false;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    rapi_buffer_read_uint32(context->recv_buffer, (DWORD *) &return_value);

    return return_value;
}


BOOL _CeSyncTimeToPc2()
{
    RapiContext* context = rapi_context_current();
    BOOL result = FALSE;
    FILETIME ftime_now;

    filetime_from_unix_time(time(NULL), &ftime_now);

    rapi_context_begin_command(context, 0x01);

    rapi_buffer_write_filetime(context->send_buffer, ftime_now);

    /* Not sure what these are.  Clock resolution?  */
    rapi_buffer_write_uint32(context->send_buffer, 0);
    rapi_buffer_write_uint32(context->send_buffer, 10000);

    if ( !rapi2_context_call(context) )
        goto exit;

    rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
    result = TRUE;

exit:
    return result;
}



DWORD _CeGetDiskFreeSpaceEx2(
		LPCTSTR _lpDirectoryName, 
		PULARGE_INTEGER lpFreeBytesAvailable, 
		PULARGE_INTEGER lpTotalNumberOfBytes, 
		PULARGE_INTEGER lpTotalNumberOfFreeBytes){
	
	RapiContext* context = rapi_context_current();
	LONG return_value = ERROR_GEN_FAILURE;

	rapi_context_begin_command(context, 0x5c);
	
	WCHAR* lpDirName_wide = wstr_from_current( _lpDirectoryName ) ; 
	rapi2_buffer_write_string(context->send_buffer,  lpDirName_wide );

	//NOTE: Some personat microsoft decided that CeGetDiskFreeSpaceEx should return
	//a zero value on success and a non-zero function on failure. This contradicts
	//with majority of all rapi functions!
	if ( !rapi2_context_call(context) )
		return 0 ;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_int32(context->recv_buffer, &return_value);
	
	
	uint32_t dword1 ; 
	uint32_t dword2 ; 

	//First read the two uint32s for the FreeBytesAvailable
	rapi_buffer_read_int32(context->recv_buffer, &dword1);
	rapi_buffer_read_int32(context->recv_buffer, &dword2);

	//The construct the uint64 out of this.
	*lpFreeBytesAvailable     = dword2 ; 
	(*lpFreeBytesAvailable) <<= 32 ;
	(*lpFreeBytesAvailable)  |= dword1 ;

	//Read the two uint32s for the TotalNumberOfBytes
	rapi_buffer_read_int32(context->recv_buffer, &dword1);
	rapi_buffer_read_int32(context->recv_buffer, &dword2);

	//The construct the uint64 out of this.
	*lpTotalNumberOfBytes     = dword2 ; 
	(*lpTotalNumberOfBytes) <<= 32 ;
	(*lpTotalNumberOfBytes)  |= dword1 ;

	//Finally read the two uint32s for the TotalNumberOfFreeBytes
	rapi_buffer_read_int32(context->recv_buffer, &dword1);
	rapi_buffer_read_int32(context->recv_buffer, &dword2);

	//The construct the uint64 out of this.
	*lpTotalNumberOfFreeBytes     = dword2 ; 
	(*lpTotalNumberOfFreeBytes) <<= 32 ;
	(*lpTotalNumberOfFreeBytes)  |= dword1 ;
			
	return return_value ;
}

