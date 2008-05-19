/* $Id$ */
#ifndef __rapi_api_h__
#define __rapi_api_h__
#include "rapi.h"

#ifndef SWIG

BOOL _CeCloseHandle(
        HANDLE hObject);

HANDLE _CeCreateFile(
        LPCWSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile);

BOOL _CeReadFile(
        HANDLE hFile,
        LPVOID lpBuffer,
        DWORD nNumberOfBytesToRead,
        LPDWORD lpNumberOfBytesRead,
        LPOVERLAPPED lpOverlapped);

BOOL _CeWriteFile(
        HANDLE hFile,
        LPCVOID lpBuffer,
        DWORD nNumberOfBytesToWrite,
        LPDWORD lpNumberOfBytesWritten,
        LPOVERLAPPED lpOverlapped);

BOOL _CeSetEndOfFile(
        HANDLE hFile);

DWORD _CeSetFilePointer(
        HANDLE hFile,
        LONG lDistanceToMove,
        LONG *lpDistanceToMoveHigh,
        DWORD dwMoveMethod);

BOOL _NotImplementedCeGetFileTime(
        HANDLE hFile,
        LPFILETIME lpCreationTime,
        LPFILETIME lpLastAccessTime,
        LPFILETIME lpLastWriteTime);

BOOL _NotImplementedCeSetFileTime(
        HANDLE hFile,
        LPFILETIME lpCreationTime,
        LPFILETIME lpLastAccessTime,
        LPFILETIME lpLastWriteTime);

#endif /* SWIG */


/*
 * File management functions
 */

#ifndef SWIG

BOOL _CeCopyFileA(
        LPCSTR lpExistingFileName,
        LPCSTR lpNewFileName,
        BOOL bFailIfExists);

BOOL _CeCopyFile(
        LPCWSTR lpExistingFileName,
        LPCWSTR lpNewFileName,
        BOOL bFailIfExists);

BOOL _CeCreateDirectory(
        LPCWSTR lpPathName,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes);

BOOL _CeDeleteFile(
        LPCWSTR lpFileName);

BOOL _CeFindAllFiles(
        LPCWSTR szPath,
        DWORD dwFlags,
        LPDWORD lpdwFoundCount,
        LPLPCE_FIND_DATA ppFindDataArray);

HANDLE _CeFindFirstFile(
        LPCWSTR lpFileName,
        LPCE_FIND_DATA lpFindFileData);

BOOL _CeFindNextFile(
        HANDLE hFindFile,
        LPCE_FIND_DATA lpFindFileData);

BOOL _CeFindClose(
        HANDLE hFindFile);

DWORD _CeGetFileAttributes(
        LPCWSTR lpFileName);

DWORD _CeGetFileSize(
        HANDLE hFile,
        LPDWORD lpFileSizeHigh);

DWORD _CeGetSpecialFolderPath(
        int nFolder,
        DWORD nBufferLength,
        LPWSTR lpBuffer);

BOOL _CeMoveFile(
        LPCWSTR lpExistingFileName,
        LPCWSTR lpNewFileName);

BOOL _CeRemoveDirectory(
        LPCWSTR lpPathName);

BOOL _CeSetFileAttributes(
        LPCWSTR lpFileName,
        DWORD dwFileAttributes);

BOOL _CeSHCreateShortcut(
        LPCWSTR lpszShortcut,
        LPCWSTR lpszTarget);

BOOL _CeSyncTimeToPc();

#endif /* SWIG */


/*
 * Database functions
 */

#ifndef SWIG

CEOID _CeCreateDatabase(
        LPWSTR lpszName,
        DWORD dwDbaseType,
        WORD wNumSortOrder,
        SORTORDERSPEC *rgSortSpecs);

BOOL _CeDeleteDatabase(
        CEOID oid);

BOOL _CeFindAllDatabases(
        DWORD dwDbaseType,
        WORD wFlags,
        LPWORD cFindData,
        LPLPCEDB_FIND_DATA ppFindData);

HANDLE _CeFindFirstDatabase(
        DWORD dwDbaseType);

CEOID _CeFindNextDatabase(
        HANDLE hEnum);

HANDLE _CeOpenDatabase(
        PCEOID poid,
        LPWSTR lpszName,
        CEPROPID propid,
        DWORD dwFlags,
        HWND hwndNotify);

CEOID _CeReadRecordProps(
        HANDLE hDbase,
        DWORD dwFlags,
        LPWORD lpcPropID,
        CEPROPID *rgPropID,
        LPBYTE *lplpBuffer,
        LPDWORD lpcbBuffer);

CEOID _CeSeekDatabase(
        HANDLE hDatabase,
        DWORD dwSeekType,
        DWORD dwValue,
        LPDWORD lpdwIndex);

CEOID _CeWriteRecordProps(
        HANDLE hDbase,
        CEOID oidRecord,
        WORD cPropID,
        CEPROPVAL *rgPropVal);

BOOL _CeDeleteRecord(
        HANDLE hDatabase,
        CEOID oidRecord);

BOOL _CeSetDatabaseInfo(
        CEOID oidDbase,
        CEDBASEINFO* pNewInfo);

#endif /* SWIG */

/*
 * Registry
 */

#ifndef SWIG

LONG _CeRegCreateKeyEx(
        HKEY hKey,
        LPCWSTR lpszSubKey,
        DWORD Reserved,
        LPWSTR lpszClass,
        DWORD ulOptions,
        REGSAM samDesired,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        PHKEY phkResult,
        LPDWORD lpdwDisposition);

LONG _CeRegOpenKeyEx(
        HKEY hKey,
        LPCWSTR lpszSubKey,
        DWORD ulOptions,
        REGSAM samDesired,
        PHKEY phkResult);

LONG _CeRegCloseKey(
        HKEY hKey);

LONG _CeRegQueryInfoKey(
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
        PFILETIME lpftLastWriteTime);

LONG _CeRegQueryValueEx(
        HKEY hKey,
        LPCWSTR lpValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE lpData,
        LPDWORD lpcbData);

LONG _CeRegEnumValue(
        HKEY hKey,
        DWORD dwIndex,
        LPWSTR lpszValueName,
        LPDWORD lpcbValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE lpData,
        LPDWORD lpcbData);

LONG _CeRegEnumKeyEx(
        HKEY hKey,
        DWORD dwIndex,
        LPWSTR lpName,
        LPDWORD lpcbName,
        LPDWORD lpReserved,
        LPWSTR lpClass,
        LPDWORD lpcbClass,
        PFILETIME lpftLastWriteTime);

LONG _CeRegSetValueEx(
        HKEY hKey,
        LPCWSTR lpValueName,
        DWORD Reserved,
        DWORD dwType,
        const BYTE *lpData,
        DWORD cbData);

LONG _CeRegDeleteValue(
	HKEY hKey,
	LPCWSTR lpszValueName);

LONG _CeRegDeleteKey(
	HKEY hKey,
	LPCWSTR lpszSubKey);

/*
 * Convenience functions for easy registry access
 */

bool _rapi_reg_create_key(
        HKEY parent,
        const char* name,
        HKEY* key);

bool _rapi_reg_open_key(
        HKEY parent, const char* name, HKEY* key);

bool _rapi_reg_query_dword(
        HKEY key,
        const char* name,
        DWORD* value);

bool _rapi_reg_query_string(
        HKEY key,
        const char* name,
        char** value);

bool _rapi_reg_set_dword(
        HKEY key,
        const char* name,
        DWORD value);

bool _rapi_reg_set_string(
        HKEY key,
        const char* name,
        const char *value);

#endif /* SWIG */

/*
 * Misc functions
 */

#ifndef SWIG

BOOL _CeCheckPassword(
        LPWSTR lpszPassword);

BOOL _CeCreateProcess(
        LPCWSTR lpApplicationName,
        LPCWSTR lpCommandLine,
        void* lpProcessAttributes,
        void* lpThreadAttributes,
        BOOL bInheritHandles,
        DWORD dwCreationFlags,
        LPVOID lpEnvironment,
        LPWSTR lpCurrentDirectory,
        void* lpStartupInfo,
        LPPROCESS_INFORMATION lpProcessInformation);

DWORD _CeGetLastError( void );

BOOL _CeGetStoreInformation(
        LPSTORE_INFORMATION lpsi);

void _CeGetSystemInfo(
        LPSYSTEM_INFO lpSystemInfo);

BOOL _CeGetSystemPowerStatusEx(PSYSTEM_POWER_STATUS_EX pSystemPowerStatus, BOOL refresh);

BOOL _CeGetVersionEx(
        LPCEOSVERSIONINFO lpVersionInformation);

BOOL _CeOidGetInfo(
        CEOID oid,
        CEOIDINFO *poidInfo);

HRESULT _CeProcessConfig(LPCWSTR config, DWORD flags, LPWSTR* reply);

BOOL _CeStartReplication( void );

HRESULT _NotImplementedCeSyncStart(LPCWSTR params);

HRESULT _NotImplementedCeSyncResume(void);

HRESULT _NotImplementedCeSyncPause(void);

BOOL _CeGetSystemMemoryDivision(
    LPDWORD lpdwStoragePages,
    LPDWORD lpdwRamPages,
    LPDWORD lpdwPageSize);

DWORD _CeSetSystemMemoryDivision(
        DWORD dwStoragePages);

BOOL _CeRegCopyFile(LPCWSTR filename);
BOOL _CeRegRestoreFile(LPCWSTR filename);

BOOL _CeKillAllApps();

DWORD _NotImplementedCeGetDiskFreeSpaceEx(
		LPCTSTR _lpDirectoryName, 
		PULARGE_INTEGER lpFreeBytesAvailable, 
		PULARGE_INTEGER lpTotalNumberOfBytes, 
		PULARGE_INTEGER lpTotalNumberOfFreeBytes);

#endif /* SWIG */

/*
 * CeRapiInvoke stuff
 */

#ifndef SWIG

ULONG _IRAPIStream_Release(IRAPIStream* stream);

HRESULT _IRAPIStream_Read(
        IRAPIStream* stream,
        void *pv,
        ULONG cb,
        ULONG *pcbRead);

HRESULT _IRAPIStream_Write(
        IRAPIStream* stream,
        void const *pv,
        ULONG cb,
        ULONG *pcbWritten);

int _IRAPIStream_GetRawSocket(IRAPIStream* stream);

HRESULT _CeRapiInvoke(
        LPCWSTR pDllPath,
        LPCWSTR pFunctionName,
        DWORD cbInput,
        const BYTE *pInput,
        DWORD *pcbOutput,
        BYTE **ppOutput,
        IRAPIStream **ppIRAPIStream,
        DWORD dwReserved);

HRESULT _CeRapiInvokeA(
        LPCSTR pDllPath,
        LPCSTR pFunctionName,
        DWORD cbInput,
        const BYTE *pInput,
        DWORD *pcbOutput,
        BYTE **ppOutput,
        IRAPIStream **ppIRAPIStream,
        DWORD dwReserved);

#endif /* SWIG */

#endif /* __rapi_api_h__ */


