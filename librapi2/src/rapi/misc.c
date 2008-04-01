/* $Id$ */
#include "rapi_api.h"
#include "rapi_context.h"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

BOOL _CeCreateProcess(/*{{{*/
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

	rapi_context_begin_command(context, 0x19);
	rapi_buffer_write_optional_string(context->send_buffer, lpApplicationName);
	rapi_buffer_write_optional_string(context->send_buffer, lpCommandLine);
	rapi_buffer_write_uint32(context->send_buffer, 0);
	rapi_buffer_write_uint32(context->send_buffer, 0);
	rapi_buffer_write_uint32(context->send_buffer, 0);
	rapi_buffer_write_uint32(context->send_buffer, 0);
	rapi_buffer_write_uint32(context->send_buffer, 0);
	rapi_buffer_write_uint32(context->send_buffer, 0);
	rapi_buffer_write_uint32(context->send_buffer, 0);
	rapi_buffer_write_optional_out(context->send_buffer, lpProcessInformation, sizeof(PROCESS_INFORMATION));

	if ( !rapi_context_call(context) )
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

BOOL _CeGetStoreInformation( /*{{{*/
		LPSTORE_INFORMATION lpsi)
{
	RapiContext* context = rapi_context_current();
	BOOL result = false;

	rapi_context_begin_command(context, 0x29);
	rapi_buffer_write_optional_out(context->send_buffer, lpsi, sizeof(STORE_INFORMATION));

	if ( !rapi_context_call(context) )
		goto exit;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &result);

	if ( !rapi_buffer_read_optional(context->recv_buffer, lpsi, sizeof(STORE_INFORMATION)) )
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

void _CeGetSystemInfo( /*{{{*/
		LPSYSTEM_INFO lpSystemInfo)
{
	RapiContext* context = rapi_context_current();

	rapi_context_begin_command(context, 0x2f);
	rapi_buffer_write_optional_out(context->send_buffer, lpSystemInfo, sizeof(SYSTEM_INFO));

	if ( !rapi_context_call(context) )
		return;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	synce_trace("last_error = %i", context->last_error);
/*	rapi_buffer_read_uint32(context->recv_buffer, &result);
	synce_trace("result = %i", result);*/

	if ( !rapi_buffer_read_optional(context->recv_buffer, lpSystemInfo, sizeof(SYSTEM_INFO)) )
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

BOOL _CeGetSystemPowerStatusEx( /*{{{*/
		PSYSTEM_POWER_STATUS_EX pSystemPowerStatus,
		BOOL refresh)
{
	RapiContext* context = rapi_context_current();
	BOOL result = false;

	rapi_context_begin_command(context, 0x41);
	rapi_buffer_write_optional_out(context->send_buffer, pSystemPowerStatus, sizeof(SYSTEM_POWER_STATUS_EX));
	rapi_buffer_write_uint32(context->send_buffer, refresh);

	if ( !rapi_context_call(context) )
		goto exit;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &result);

	if ( !rapi_buffer_read_optional(context->recv_buffer, pSystemPowerStatus, sizeof(SYSTEM_POWER_STATUS_EX)) )
		goto exit;

	pSystemPowerStatus->BatteryLifeTime           = letoh32(pSystemPowerStatus->BatteryLifeTime);
	pSystemPowerStatus->BatteryFullLifeTime       = letoh32(pSystemPowerStatus->BatteryFullLifeTime);
	pSystemPowerStatus->BackupBatteryLifeTime     = letoh32(pSystemPowerStatus->BackupBatteryLifeTime);
	pSystemPowerStatus->BackupBatteryFullLifeTime = letoh32(pSystemPowerStatus->BackupBatteryFullLifeTime);

exit:
	return result;
}/*}}}*/

BOOL _CeGetVersionEx(/*{{{*/
		LPCEOSVERSIONINFO lpVersionInformation)
{
	RapiContext* context = rapi_context_current();
	BOOL result = false;
	uint32_t size = 0;

	rapi_context_begin_command(context, 0x3b);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	synce_trace("last_error = %i", context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &result);
	synce_trace("result = %i", result);

	rapi_buffer_read_uint32(context->recv_buffer, &size);

	if ( !rapi_buffer_read_data(context->recv_buffer, lpVersionInformation, size) )
		return false;

	lpVersionInformation->dwOSVersionInfoSize  = letoh32(lpVersionInformation->dwOSVersionInfoSize);
	lpVersionInformation->dwMajorVersion       = letoh32(lpVersionInformation->dwMajorVersion);
	lpVersionInformation->dwMinorVersion       = letoh32(lpVersionInformation->dwMinorVersion);
	lpVersionInformation->dwBuildNumber        = letoh32(lpVersionInformation->dwBuildNumber);
	lpVersionInformation->dwPlatformId         = letoh32(lpVersionInformation->dwPlatformId);

	return result;
}/*}}}*/

BOOL _CeOidGetInfo(/*{{{*/
		CEOID oid,
		CEOIDINFO *poidInfo)
{
	RapiContext* context = rapi_context_current();
	BOOL result = false;
	uint16_t size = 0;

	if (!poidInfo)
	{
		synce_error("poidInfo is NULL");
		goto fail;
	}

	rapi_context_begin_command(context, 0x0c);
	rapi_buffer_write_uint32(context->send_buffer, oid);

	if ( !rapi_context_call(context) )
		goto fail;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
	synce_trace("last_error = %i", context->last_error);
	rapi_buffer_read_uint32(context->recv_buffer, &result);
	synce_trace("result = %i", result);

	if ( !rapi_buffer_read_uint16(context->recv_buffer, &poidInfo->wObjType) )
		goto fail;
	synce_trace("object type = %i", poidInfo->wObjType);

	switch (poidInfo->wObjType)
	{
		case OBJTYPE_FILE:
			if ( !rapi_buffer_read_uint16(context->recv_buffer, &size) )
				goto fail;
			synce_trace("size = %i", size);

			/* XXX: not portable to big-endian CPUs! */
			if ( !rapi_buffer_read_data(context->recv_buffer, 4 + (char*)poidInfo, size) )
				return false;
			break;

		case OBJTYPE_DIRECTORY:
			if ( !rapi_buffer_read_uint16(context->recv_buffer, &size) )
				goto fail;
			synce_trace("size = %i", size);

			/* XXX: not portable to big-endian CPUs! */
			if ( !rapi_buffer_read_data(context->recv_buffer, 4 + (char*)poidInfo, size) )
				return false;
			break;

		case OBJTYPE_DATABASE:
			if ( !rapi_buffer_read_uint16(context->recv_buffer, &size) )
				goto fail;
			synce_trace("size = %i", size);

			/* XXX: not portable to big-endian CPUs! */
			if ( !rapi_buffer_read_data(context->recv_buffer, 4 + (char*)poidInfo, size) )
				return false;
			break;

		case OBJTYPE_RECORD:
			if ( !rapi_buffer_read_uint32(context->recv_buffer, &poidInfo->u.infRecord.oidParent) )
				goto fail;
			break;

		case OBJTYPE_DELETED:
			synce_trace("This object is deleted");
			break;

		default:
			synce_error("unknown object type = %i, buffer size = %i",
					poidInfo->wObjType, rapi_buffer_get_size(context->recv_buffer));
			goto fail;
	}

	return result;

fail:
	return false;
}/*}}}*/

/**
  Undocumented function used by the RapiConfig.exe tool
*/

HRESULT _CeProcessConfig(LPCWSTR config, DWORD flags, LPWSTR* reply)
{
  RapiContext* context = rapi_context_current();
  HRESULT result = E_UNEXPECTED;
  BOOL has_reply = FALSE;
  DWORD size = 0;
  DWORD dummy = 0;
  LPWSTR buffer = NULL;

  if (!config || !reply)
  {
    synce_error("Bad parameter(s)");
    goto exit;
  }

  rapi_context_begin_command(context, 0x56);

  rapi_buffer_write_optional_string(context->send_buffer, config);
  rapi_buffer_write_uint32(context->send_buffer, flags);

  if ( !rapi_context_call(context) )
    return false;

  if (!rapi_buffer_read_uint32(context->recv_buffer, (uint32_t*)&result))
    goto exit;
  synce_trace("result = 0x%08x", result);

  /*
    should use rapi_buffer_read_optional(), but this needs an allocated
    buffer of a known size, which we don't know. Is there a system
    wide max size ?
  */

  if (!rapi_buffer_read_uint32(context->recv_buffer, &has_reply))
    goto exit;

  if (!has_reply)
    goto exit;

  if (!rapi_buffer_read_uint32(context->recv_buffer, &size))
    goto exit;
  synce_trace("size = 0x%08x", size);

  buffer = (LPWSTR)malloc(size);
  if (!buffer)
  {
    synce_error("Failed to allocated %i bytes", size);
    goto exit;
  }

  if (!rapi_buffer_read_uint32(context->recv_buffer, &dummy))
    goto exit;

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
BOOL _CeStartReplication( void )/*{{{*/
{
	RapiContext* context = rapi_context_current();
	DWORD result = false;

	rapi_context_begin_command(context, 0x38);

	if ( !rapi_context_call(context) )
		return false;

	rapi_buffer_read_uint32(context->recv_buffer, &result);
	/* synce_trace("result = 0x%08x", result); */

	return result;
}/*}}}*/

/*
   SetSystemMemoryDivision and GetSystemMemoryDivision are not documented
   as RAPI functions, but they are there. Docs for the functions:

   http://www.pocketpcdn.com/qa/memorydivision.html
*/

BOOL _CeGetSystemMemoryDivision(
    LPDWORD lpdwStoragePages,
    LPDWORD lpdwRamPages,
    LPDWORD lpdwPageSize)
{
  RapiContext* context = rapi_context_current();
  BOOL result = false;

  rapi_context_begin_command(context, 0x28);
  rapi_buffer_write_optional_uint32(context->send_buffer, lpdwStoragePages, false);
  rapi_buffer_write_optional_uint32(context->send_buffer, lpdwRamPages,     false);
  rapi_buffer_write_optional_uint32(context->send_buffer, lpdwPageSize,     false);

  if ( !rapi_context_call(context) )
    goto exit;

  rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
  rapi_buffer_read_uint32(context->recv_buffer, &result);

  rapi_buffer_read_optional_uint32(context->recv_buffer, lpdwStoragePages);
  rapi_buffer_read_optional_uint32(context->recv_buffer, lpdwRamPages);
  rapi_buffer_read_optional_uint32(context->recv_buffer, lpdwPageSize);

exit:
  return result;
}

DWORD _CeSetSystemMemoryDivision(
    DWORD dwStoragePages)
{
  RapiContext* context = rapi_context_current();
  DWORD result = 3;

  rapi_context_begin_command(context, 0x42);
  rapi_buffer_write_uint32(context->send_buffer, dwStoragePages);

  if ( !rapi_context_call(context) )
    goto exit;

  rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
  synce_trace("last_error = %i", context->last_error);
  rapi_buffer_read_uint32(context->recv_buffer, &result);
  synce_trace("result = %i", result);

exit:
  return result;
}

BOOL _CeRegCopyFile(LPCWSTR filename)
{
  RapiContext* context = rapi_context_current();
  BOOL result = FALSE;

  if (!filename)
  {
    synce_error("Bad parameter(s)");
    goto exit;
  }

  rapi_context_begin_command(context, 0x2d);

  rapi_buffer_write_optional_string(context->send_buffer, filename);

  if ( !rapi_context_call(context) )
    return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
  if (!rapi_buffer_read_uint32(context->recv_buffer, &result))
    goto exit;
  synce_trace("result = 0x%08x", result);

exit:
  return result;
}

BOOL _CeRegRestoreFile(LPCWSTR filename)
{
  RapiContext* context = rapi_context_current();
  BOOL result = FALSE;

  if (!filename)
  {
    synce_error("Bad parameter(s)");
    goto exit;
  }

  rapi_context_begin_command(context, 0x2e);

  rapi_buffer_write_optional_string(context->send_buffer, filename);

  if ( !rapi_context_call(context) )
    return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
  if (!rapi_buffer_read_uint32(context->recv_buffer, &result))
    goto exit;
  synce_trace("result = 0x%08x", result);

exit:
  return result;
}

BOOL _CeKillAllApps()
{
  RapiContext* context = rapi_context_current();
  BOOL result = FALSE;

  rapi_context_begin_command(context, 0x49);

  if ( !rapi_context_call(context) )
    return false;

	rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
  if (!rapi_buffer_read_uint32(context->recv_buffer, &result))
    goto exit;
  synce_trace("result = 0x%08x", result);

exit:
  return result;
}

BOOL _CeSyncTimeToPc()
{
  RapiContext* context = rapi_context_current();
  BOOL result = FALSE;
  FILETIME ftime_now;

  filetime_from_unix_time(time(NULL), &ftime_now);

  rapi_context_begin_command(context, 0x37);

  rapi_buffer_write_filetime(context->send_buffer, ftime_now);

  /* Not sure what these are.  Clock resolution?  */
  rapi_buffer_write_uint32(context->send_buffer, 0);
  rapi_buffer_write_uint32(context->send_buffer, 10000);

  if ( !rapi_context_call(context) )
    goto exit;

  rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
  result = TRUE;

exit:
  return result;


}


/*
 *
 * dummy function, not yet implemented
 *
 */
DWORD _CeGetDiskFreeSpaceEx(
		LPCTSTR _lpDirectoryName, 
		PULARGE_INTEGER lpFreeBytesAvailable, 
		PULARGE_INTEGER lpTotalNumberOfBytes, 
		PULARGE_INTEGER lpTotalNumberOfFreeBytes){
	
	LONG return_value = ERROR_GEN_FAILURE;
			
	return return_value ;
}

