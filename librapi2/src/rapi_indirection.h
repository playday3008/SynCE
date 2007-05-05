/* $Id$ */
#undef __STRICT_ANSI__
#define _GNU_SOURCE
#include "rapi.h"
#include <stdlib.h>

struct rapi_ops_s
{
#ifndef SWIG

    BOOL ( *CeCloseHandle ) (
        HANDLE hObject );

    HANDLE ( *CeCreateFile ) (
            LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile );

    BOOL ( *CeReadFile ) (
            HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped );

    BOOL ( *CeWriteFile ) (
            HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped );

    DWORD ( *CeSetFilePointer ) (
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod );

    BOOL  ( *CeSetEndOfFile ) (
    HANDLE hObject);

    BOOL ( *CeGetFileTime ) (
    HANDLE hFile,
    LPFILETIME lpCreationTime,
    LPFILETIME lpLastAccessTime,
    LPFILETIME lpLastWriteTime );

    BOOL ( *CeSetFileTime ) (
    HANDLE hFile,
    LPFILETIME lpCreationTime,
    LPFILETIME lpLastAccessTime,
    LPFILETIME lpLastWriteTime );

#endif /* SWIG */


    /*
    * File management functions
    */

#ifndef SWIG

    BOOL ( *CeCopyFileA ) (
        LPCSTR lpExistingFileName,
        LPCSTR lpNewFileName,
        BOOL bFailIfExists );

    BOOL ( *CeCopyFile ) (
            LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists );

    BOOL ( *CeCreateDirectory ) (
            LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes );

    BOOL ( *CeDeleteFile ) (
            LPCWSTR lpFileName );

    BOOL ( *CeFindAllFiles ) (
            LPCWSTR szPath,
    DWORD dwFlags,
    LPDWORD lpdwFoundCount,
    LPLPCE_FIND_DATA ppFindDataArray );

    HANDLE ( *CeFindFirstFile ) (
            LPCWSTR lpFileName,
    LPCE_FIND_DATA lpFindFileData );

    BOOL ( *CeFindNextFile ) (
            HANDLE hFindFile,
    LPCE_FIND_DATA lpFindFileData );

    BOOL ( *CeFindClose ) (
            HANDLE hFindFile );

    DWORD ( *CeGetFileAttributes ) (
            LPCWSTR lpFileName );

    DWORD ( *CeGetFileSize ) (
            HANDLE hFile,
    LPDWORD lpFileSizeHigh );

    DWORD ( *CeGetSpecialFolderPath ) (
            int nFolder,
    DWORD nBufferLength,
    LPWSTR lpBuffer );

    BOOL ( *CeMoveFile ) (
            LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName );

    BOOL ( *CeRemoveDirectory ) (
            LPCWSTR lpPathName );

    BOOL ( *CeSetFileAttributes ) (
            LPCWSTR lpFileName,
    DWORD dwFileAttributes );

    BOOL ( *CeSHCreateShortcut ) (
            LPCWSTR lpszShortcut,
    LPCWSTR lpszTarget );

    BOOL ( *CeSyncTimeToPc ) ();

#endif /* SWIG */


    /*
    * Database functions
    */

#ifndef SWIG

    CEOID ( *CeCreateDatabase ) (
        LPWSTR lpszName,
        DWORD dwDbaseType,
        WORD wNumSortOrder,
        SORTORDERSPEC *rgSortSpecs );

    BOOL ( *CeDeleteDatabase ) (
            CEOID oid );

    BOOL ( *CeFindAllDatabases ) (
            DWORD dwDbaseType,
    WORD wFlags,
    LPWORD cFindData,
    LPLPCEDB_FIND_DATA ppFindData );

    HANDLE ( *CeFindFirstDatabase ) (
            DWORD dwDbaseType );

    CEOID ( *CeFindNextDatabase ) (
            HANDLE hEnum );

    HANDLE ( *CeOpenDatabase ) (
            PCEOID poid,
    LPWSTR lpszName,
    CEPROPID propid,
    DWORD dwFlags,
    HWND hwndNotify );

    CEOID ( *CeReadRecordProps ) (
            HANDLE hDbase,
    DWORD dwFlags,
    LPWORD lpcPropID,
    CEPROPID *rgPropID,
    LPBYTE *lplpBuffer,
    LPDWORD lpcbBuffer );

    CEOID ( *CeSeekDatabase ) (
            HANDLE hDatabase,
    DWORD dwSeekType,
    DWORD dwValue,
    LPDWORD lpdwIndex );

    CEOID ( *CeWriteRecordProps ) (
            HANDLE hDbase,
    CEOID oidRecord,
    WORD cPropID,
    CEPROPVAL *rgPropVal );

    BOOL ( *CeDeleteRecord ) (
            HANDLE hDatabase,
    CEOID oidRecord );

    BOOL ( *CeSetDatabaseInfo ) (
            CEOID oidDbase,
    CEDBASEINFO* pNewInfo );

#endif /* SWIG */


    /*
    * Registry
    */

#ifndef SWIG

    LONG ( *CeRegCreateKeyEx ) (
        HKEY hKey,
        LPCWSTR lpszSubKey,
        DWORD Reserved,
        LPWSTR lpszClass,
        DWORD ulOptions,
        REGSAM samDesired,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        PHKEY phkResult,
        LPDWORD lpdwDisposition );

    LONG ( *CeRegOpenKeyEx ) (
            HKEY hKey,
    LPCWSTR lpszSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult );

    LONG ( *CeRegCloseKey ) (
            HKEY hKey );

    LONG ( *CeRegDeleteKey ) (
            HKEY hKey,
            LPCWSTR lpszSubKey );

    LONG ( *CeRegDeleteValue ) (
            HKEY hKey,
            LPCWSTR lpszValueName );

    LONG ( *CeRegQueryInfoKey ) (
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
    PFILETIME lpftLastWriteTime );

    LONG ( *CeRegQueryValueEx ) (
            HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData );

    LONG ( *CeRegEnumValue ) (
            HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpszValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData );

    LONG ( *CeRegEnumKeyEx ) (
            HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime );

    LONG ( *CeRegSetValueEx ) (
            HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE *lpData,
    DWORD cbData );

#endif /* SWIG */


    /*
    * Misc functions
    */

#ifndef SWIG

    BOOL ( *CeCheckPassword ) (
        LPWSTR lpszPassword );

    BOOL ( *CeCreateProcess ) (
            LPCWSTR lpApplicationName,
    LPCWSTR lpCommandLine,
    void* lpProcessAttributes,
    void* lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPWSTR lpCurrentDirectory,
    void* lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation );

    BOOL ( *CeGetStoreInformation ) (
            LPSTORE_INFORMATION lpsi );

    void ( *CeGetSystemInfo ) (
            LPSYSTEM_INFO lpSystemInfo );

    BOOL ( *CeGetSystemPowerStatusEx ) ( PSYSTEM_POWER_STATUS_EX pSystemPowerStatus, BOOL refresh );

    BOOL ( *CeGetVersionEx ) (
            LPCEOSVERSIONINFO lpVersionInformation );

    BOOL ( *CeOidGetInfo ) (
            CEOID oid,
    CEOIDINFO *poidInfo );

    HRESULT ( *CeProcessConfig ) ( LPCWSTR config, DWORD flags, LPWSTR* reply );

    BOOL ( *CeStartReplication ) ( void );

    HRESULT ( *CeSyncStart ) ( LPCWSTR params );

    HRESULT ( *CeSyncResume ) ( void );

    HRESULT ( *CeSyncPause ) ( void );

    BOOL ( *CeGetSystemMemoryDivision ) (
            LPDWORD lpdwStoragePages,
    LPDWORD lpdwRamPages,
    LPDWORD lpdwPageSize );

    DWORD ( *CeSetSystemMemoryDivision ) (
            DWORD dwStoragePages );

    BOOL ( *CeRegCopyFile ) ( LPCWSTR filename );
    BOOL ( *CeRegRestoreFile ) ( LPCWSTR filename );

    BOOL ( *CeKillAllApps ) ();

#endif /* SWIG */


    /*
    * CeRapiInvoke stuff
    */

#ifndef SWIG

    ULONG ( *IRAPIStream_Release ) ( IRAPIStream* stream );

    HRESULT ( *IRAPIStream_Read ) (
            IRAPIStream* stream,
    void *pv,
    ULONG cb,
    ULONG *pcbRead );

    HRESULT ( *IRAPIStream_Write ) (
            IRAPIStream* stream,
    void const *pv,
    ULONG cb,
    ULONG *pcbWritten );
    /*
    int (*IRAPIStream_GetRawSocket)(IRAPIStream* stream);
    */

    HRESULT ( *CeRapiInvoke ) (
            LPCWSTR pDllPath,
    LPCWSTR pFunctionName,
    DWORD cbInput,
    const BYTE *pInput,
    DWORD *pcbOutput,
    BYTE **ppOutput,
    IRAPIStream **ppIRAPIStream,
    DWORD dwReserved );

    HRESULT ( *CeRapiInvokeA ) (
            LPCSTR pDllPath,
    LPCSTR pFunctionName,
    DWORD cbInput,
    const BYTE *pInput,
    DWORD *pcbOutput,
    BYTE **ppOutput,
    IRAPIStream **ppIRAPIStream,
    DWORD dwReserved );

#endif /* SWIG */
};
