/* $Id$ */
#undef __STRICT_ANSI__
#define _GNU_SOURCE
#include "rapi_indirection.h"
#include "rapi_context.h"
#if HAVE_CONFIG_H
#include "rapi_config.h"
#endif


/* Indirection to the particular rapi-calls */

#ifndef SWIG
BOOL CeCloseHandle(
    HANDLE hObject )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeCloseHandle ) ( hObject );
}

HANDLE CeCreateFile(
        LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return INVALID_HANDLE_VALUE;
    }

    return ( *context->rapi_ops->CeCreateFile ) (
            lpFileName,
    dwDesiredAccess,
    dwShareMode,
    lpSecurityAttributes,
    dwCreationDisposition,
    dwFlagsAndAttributes,
    hTemplateFile );
}


BOOL CeReadFile(
        HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeReadFile ) (
            hFile,
    lpBuffer,
    nNumberOfBytesToRead,
    lpNumberOfBytesRead,
    lpOverlapped );
}


BOOL CeWriteFile(
        HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeWriteFile ) (
            hFile,
    lpBuffer,
    nNumberOfBytesToWrite,
    lpNumberOfBytesWritten,
    lpOverlapped );
}


DWORD CeSetFilePointer(
        HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return 0xFFFFFFFF;
    }

    return ( *context->rapi_ops->CeSetFilePointer ) (
            hFile,
    lDistanceToMove,
    lpDistanceToMoveHigh,
    dwMoveMethod );
}

BOOL CeSetEndOfFile(
        HANDLE hFile)
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeSetEndOfFile ) ( hFile );
}

BOOL CeGetFileTime(
        HANDLE hFile,
    LPFILETIME lpCreationTime,
    LPFILETIME lpLastAccessTime,
    LPFILETIME lpLastWriteTime )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeGetFileTime ) (
            hFile,
    lpCreationTime,
    lpLastAccessTime,
    lpLastWriteTime );
}

BOOL CeSetFileTime(
        HANDLE hFile,
    LPFILETIME lpCreationTime,
    LPFILETIME lpLastAccessTime,
    LPFILETIME lpLastWriteTime )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeSetFileTime ) (
            hFile,
    lpCreationTime,
    lpLastAccessTime,
    lpLastWriteTime );
}

#endif /* SWIG */


/*
* File management functions
*/

#ifndef SWIG

BOOL CeCopyFileA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    BOOL bFailIfExists )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeCopyFileA ) (
            lpExistingFileName,
    lpNewFileName,
    bFailIfExists );
}


BOOL CeCopyFile(
        LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeCopyFile ) (
            lpExistingFileName,
    lpNewFileName,
    bFailIfExists );
}


BOOL CeCreateDirectory(
        LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeCreateDirectory ) (
            lpPathName,
    lpSecurityAttributes );
}


BOOL CeDeleteFile(
        LPCWSTR lpFileName )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeDeleteFile ) (
            lpFileName );
}


BOOL CeFindAllFiles(
        LPCWSTR szPath,
    DWORD dwFlags,
    LPDWORD lpdwFoundCount,
    LPLPCE_FIND_DATA ppFindDataArray )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeFindAllFiles ) (
            szPath,
    dwFlags,
    lpdwFoundCount,
    ppFindDataArray );
}


HANDLE CeFindFirstFile(
        LPCWSTR lpFileName,
    LPCE_FIND_DATA lpFindFileData )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return INVALID_HANDLE_VALUE;
    }

    return ( *context->rapi_ops->CeFindFirstFile ) (
            lpFileName,
    lpFindFileData );
}


BOOL CeFindNextFile(
        HANDLE hFindFile,
    LPCE_FIND_DATA lpFindFileData )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeFindNextFile ) (
            hFindFile,
    lpFindFileData );
}


BOOL CeFindClose(
        HANDLE hFindFile )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeFindClose ) ( hFindFile );
}


DWORD CeGetFileAttributes(
        LPCWSTR lpFileName )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return 0xFFFFFFFF;
    }

    return ( *context->rapi_ops->CeGetFileAttributes ) ( lpFileName );
}


DWORD CeGetFileSize(
        HANDLE hFile,
    LPDWORD lpFileSizeHigh )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return INVALID_FILE_SIZE;
    }

    return ( *context->rapi_ops->CeGetFileSize ) (
            hFile,
    lpFileSizeHigh );
}


DWORD CeGetSpecialFolderPath(
        int nFolder,
    DWORD nBufferLength,
    LPWSTR lpBuffer )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return 0;
    }

    return ( *context->rapi_ops->CeGetSpecialFolderPath ) (
            nFolder,
    nBufferLength,
    lpBuffer );
}


BOOL CeMoveFile(
        LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeMoveFile ) (
            lpExistingFileName,
    lpNewFileName );
}


BOOL CeRemoveDirectory(
        LPCWSTR lpPathName )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeRemoveDirectory ) ( lpPathName );
}


BOOL CeSetFileAttributes(
        LPCWSTR lpFileName,
    DWORD dwFileAttributes )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeSetFileAttributes ) (
            lpFileName,
    dwFileAttributes );
}


BOOL CeSHCreateShortcut(
        LPCWSTR lpszShortcut,
    LPCWSTR lpszTarget )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeSHCreateShortcut ) (
            lpszShortcut,
    lpszTarget );
}


BOOL CeSyncTimeToPc()
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeSyncTimeToPc ) ();
}


#endif /* SWIG */


/*
* Database functions
*/

#ifndef SWIG

CEOID CeCreateDatabase(
    LPWSTR lpszName,
    DWORD dwDbaseType,
    WORD wNumSortOrder,
    SORTORDERSPEC *rgSortSpecs )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return 0;
    }

    return ( *context->rapi_ops->CeCreateDatabase ) (
            lpszName,
    dwDbaseType,
    wNumSortOrder,
    rgSortSpecs );
}


BOOL CeDeleteDatabase( CEOID oid )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeDeleteDatabase ) ( oid );
}


BOOL CeFindAllDatabases(
        DWORD dwDbaseType,
    WORD wFlags,
    LPWORD cFindData,
    LPLPCEDB_FIND_DATA ppFindData )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeFindAllDatabases ) (
            dwDbaseType,
    wFlags,
    cFindData,
    ppFindData );
}


HANDLE CeFindFirstDatabase(
        DWORD dwDbaseType )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return INVALID_HANDLE_VALUE;
    }

    return ( *context->rapi_ops->CeFindFirstDatabase ) (
            dwDbaseType );
}


CEOID CeFindNextDatabase(
        HANDLE hEnum )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return 0;
    }

    return ( *context->rapi_ops->CeFindNextDatabase ) (
            hEnum );
}


HANDLE CeOpenDatabase(
        PCEOID poid,
    LPWSTR lpszName,
    CEPROPID propid,
    DWORD dwFlags,
    HWND hwndNotify )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return INVALID_HANDLE_VALUE;
    }

    return ( *context->rapi_ops->CeOpenDatabase ) (
            poid,
    lpszName,
    propid,
    dwFlags,
    hwndNotify );
}


CEOID CeReadRecordProps(
        HANDLE hDbase,
    DWORD dwFlags,
    LPWORD lpcPropID,
    CEPROPID *rgPropID,
    LPBYTE *lplpBuffer,
    LPDWORD lpcbBuffer )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return 0;
    }

    return ( *context->rapi_ops->CeReadRecordProps ) (
            hDbase,
    dwFlags,
    lpcPropID,
    rgPropID,
    lplpBuffer,
    lpcbBuffer );
}


CEOID CeSeekDatabase(
        HANDLE hDatabase,
    DWORD dwSeekType,
    DWORD dwValue,
    LPDWORD lpdwIndex )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return 0;
    }

    return ( *context->rapi_ops->CeSeekDatabase ) (
            hDatabase,
    dwSeekType,
    dwValue,
    lpdwIndex );
}


CEOID CeWriteRecordProps(
        HANDLE hDbase,
    CEOID oidRecord,
    WORD cPropID,
    CEPROPVAL *rgPropVal )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return 0;
    }

    return ( *context->rapi_ops->CeWriteRecordProps ) (
            hDbase,
    oidRecord,
    cPropID,
    rgPropVal );
}


BOOL CeDeleteRecord(
        HANDLE hDatabase,
    CEOID oidRecord )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeDeleteRecord ) (
            hDatabase, oidRecord );
}


BOOL CeSetDatabaseInfo(
        CEOID oidDbase,
    CEDBASEINFO* pNewInfo )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeSetDatabaseInfo ) (
            oidDbase,
    pNewInfo );
}


#endif /* SWIG */

/*
* Registry
*/

#ifndef SWIG

LONG CeRegCreateKeyEx(
    HKEY hKey,
    LPCWSTR lpszSubKey,
    DWORD Reserved,
    LPWSTR lpszClass,
    DWORD ulOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return ERROR_NOT_CONNECTED;
    }

    return ( *context->rapi_ops->CeRegCreateKeyEx ) (
            hKey,
    lpszSubKey,
    Reserved,
    lpszClass,
    ulOptions,
    samDesired,
    lpSecurityAttributes,
    phkResult,
    lpdwDisposition );
}


LONG CeRegOpenKeyEx(
        HKEY hKey,
    LPCWSTR lpszSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return ERROR_NOT_CONNECTED;
    }

    return ( *context->rapi_ops->CeRegOpenKeyEx ) (
            hKey,
    lpszSubKey,
    ulOptions,
    samDesired,
    phkResult );
}


LONG CeRegCloseKey(
        HKEY hKey )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return ERROR_NOT_CONNECTED;
    }

    return ( *context->rapi_ops->CeRegCloseKey ) (
            hKey );
}


LONG CeRegDeleteKey(
        HKEY hKey,
    LPCWSTR lpszSubKey )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return ERROR_NOT_CONNECTED;
    }

    return ( *context->rapi_ops->CeRegDeleteKey ) (
            hKey,
    lpszSubKey
    );
}


LONG CeRegDeleteValue(
        HKEY hKey,
    LPCWSTR lpszValueName )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return ERROR_NOT_CONNECTED;
    }

    return ( *context->rapi_ops->CeRegDeleteValue ) (
            hKey,
    lpszValueName
    );
}



DWORD CeGetDiskFreeSpaceEx(
		LPCTSTR _lpDirectoryName, 
		PULARGE_INTEGER lpFreeBytesAvailable, 
		PULARGE_INTEGER lpTotalNumberOfBytes, 
		PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeGetDiskFreeSpaceEx ) (
			_lpDirectoryName,
			lpFreeBytesAvailable,
			lpTotalNumberOfBytes,
			lpTotalNumberOfFreeBytes
    );
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
    PFILETIME lpftLastWriteTime )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return ERROR_NOT_CONNECTED;
    }

    return ( *context->rapi_ops->CeRegQueryInfoKey ) (
            hKey,
    lpClass,
    lpcbClass,
    lpReserved,
    lpcSubKeys,
    lpcbMaxSubKeyLen,
    lpcbMaxClassLen,
    lpcValues,
    lpcbMaxValueNameLen,
    lpcbMaxValueLen,
    lpcbSecurityDescriptor,
    lpftLastWriteTime );
}


LONG CeRegQueryValueEx(
        HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return ERROR_NOT_CONNECTED;
    }

    return ( *context->rapi_ops->CeRegQueryValueEx ) (
            hKey,
    lpValueName,
    lpReserved,
    lpType,
    lpData,
    lpcbData );
}


LONG CeRegEnumValue(
        HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpszValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return ERROR_NOT_CONNECTED;
    }

    return ( *context->rapi_ops->CeRegEnumValue ) (
            hKey,
    dwIndex,
    lpszValueName,
    lpcbValueName,
    lpReserved,
    lpType,
    lpData,
    lpcbData );
}


LONG CeRegEnumKeyEx(
        HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return ERROR_NOT_CONNECTED;
    }

    return ( *context->rapi_ops->CeRegEnumKeyEx ) (
            hKey,
    dwIndex,
    lpName,
    lpcbName,
    lpReserved,
    lpClass,
    lpcbClass,
    lpftLastWriteTime );
}


LONG CeRegSetValueEx(
        HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE *lpData,
    DWORD cbData )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return ERROR_NOT_CONNECTED;
    }

    return ( *context->rapi_ops->CeRegSetValueEx ) (
            hKey,
    lpValueName,
    Reserved,
    dwType,
    lpData,
    cbData );
}

#endif /* SWIG */

/*
* Misc functions
*/

#ifndef SWIG

BOOL CeCheckPassword(
    LPWSTR lpszPassword )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeCheckPassword ) ( lpszPassword );
}


BOOL CeCreateProcess(
        LPCWSTR lpApplicationName,
    LPCWSTR lpCommandLine,
    void* lpProcessAttributes,
    void* lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPWSTR lpCurrentDirectory,
    void* lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeCreateProcess ) (
            lpApplicationName,
    lpCommandLine,
    lpProcessAttributes,
    lpThreadAttributes,
    bInheritHandles,
    dwCreationFlags,
    lpEnvironment,
    lpCurrentDirectory,
    lpStartupInfo,
    lpProcessInformation );
}


BOOL CeGetStoreInformation(
        LPSTORE_INFORMATION lpsi )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeGetStoreInformation ) ( lpsi );
}


void CeGetSystemInfo(
        LPSYSTEM_INFO lpSystemInfo )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return;
    }

    return ( *context->rapi_ops->CeGetSystemInfo ) (
            lpSystemInfo );
}


BOOL CeGetSystemPowerStatusEx( PSYSTEM_POWER_STATUS_EX pSystemPowerStatus, BOOL refresh )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeGetSystemPowerStatusEx ) ( pSystemPowerStatus, refresh );
}


BOOL CeGetVersionEx(
        LPCEOSVERSIONINFO lpVersionInformation )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeGetVersionEx ) (
            lpVersionInformation );
}


BOOL CeOidGetInfo(
        CEOID oid,
    CEOIDINFO *poidInfo )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeOidGetInfo ) (
            oid,
    poidInfo );
}


HRESULT CeProcessConfig( LPCWSTR config, DWORD flags, LPWSTR* reply )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return E_UNEXPECTED;
    }

    return ( *context->rapi_ops->CeProcessConfig ) ( config, flags, reply );
}


BOOL CeStartReplication( void )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeStartReplication ) ();
}


HRESULT CeSyncStart( LPCWSTR params )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return E_UNEXPECTED;
    }

    return ( *context->rapi_ops->CeSyncStart ) ( params );
}


HRESULT CeSyncResume( void )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return E_UNEXPECTED;
    }

    return ( *context->rapi_ops->CeSyncResume ) ();
}


HRESULT CeSyncPause( void )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return E_UNEXPECTED;
    }

    return ( *context->rapi_ops->CeSyncPause ) ();
}


BOOL CeGetSystemMemoryDivision(
        LPDWORD lpdwStoragePages,
    LPDWORD lpdwRamPages,
    LPDWORD lpdwPageSize )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeGetSystemMemoryDivision ) (
            lpdwStoragePages,
    lpdwRamPages,
    lpdwPageSize );
}


DWORD CeSetSystemMemoryDivision(
        DWORD dwStoragePages )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return 3;
    }

    return ( *context->rapi_ops->CeSetSystemMemoryDivision ) ( dwStoragePages );
}


BOOL CeRegCopyFile( LPCWSTR filename )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeRegCopyFile ) ( filename );
}


BOOL CeRegRestoreFile( LPCWSTR filename )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeRegRestoreFile ) ( filename );
}


BOOL CeKillAllApps()
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return false;
    }

    return ( *context->rapi_ops->CeKillAllApps ) ();
}


#endif /* SWIG */

/*
* CeRapiInvoke stuff
*/

#ifndef SWIG

ULONG IRAPIStream_Release( IRAPIStream* stream )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        context->rapi_error = S_OK;
        context->last_error = ERROR_NOT_CONNECTED;
        return 0;
    }

    return ( *context->rapi_ops->IRAPIStream_Release ) ( stream );
}


HRESULT IRAPIStream_Read(
        IRAPIStream* stream,
    void *pv,
    ULONG cb,
    ULONG *pcbRead )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return E_UNEXPECTED;
    }

    return ( *context->rapi_ops->IRAPIStream_Read ) (
            stream,
    pv,
    cb,
    pcbRead );
}


HRESULT IRAPIStream_Write(
        IRAPIStream* stream,
    void const *pv,
    ULONG cb,
    ULONG *pcbWritten )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return E_UNEXPECTED;
    }

    return ( *context->rapi_ops->IRAPIStream_Write ) (
            stream,
    pv,
    cb,
    pcbWritten );
}

/*
int (*IRAPIStream_GetRawSocket)(IRAPIStream* stream);
*/

HRESULT CeRapiInvoke(
        LPCWSTR pDllPath,
    LPCWSTR pFunctionName,
    DWORD cbInput,
    const BYTE *pInput,
    DWORD *pcbOutput,
    BYTE **ppOutput,
    IRAPIStream **ppIRAPIStream,
    DWORD dwReserved )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return E_UNEXPECTED;
    }

    return ( *context->rapi_ops->CeRapiInvoke ) (
            pDllPath,
    pFunctionName,
    cbInput,
    pInput,
    pcbOutput,
    ppOutput,
    ppIRAPIStream,
    dwReserved );
}


HRESULT CeRapiInvokeA(
        LPCSTR pDllPath,
    LPCSTR pFunctionName,
    DWORD cbInput,
    const BYTE *pInput,
    DWORD *pcbOutput,
    BYTE **ppOutput,
    IRAPIStream **ppIRAPIStream,
    DWORD dwReserved )
{
    RapiContext * context = rapi_context_current();
    if (!context->is_initialized)
    {
        return E_UNEXPECTED;
    }

    return ( *context->rapi_ops->CeRapiInvokeA ) (
            pDllPath,
    pFunctionName,
    cbInput,
    pInput,
    pcbOutput,
    ppOutput,
    ppIRAPIStream,
    dwReserved );
}

#endif /* SWIG */
