#ifndef __rapi2_api_h__
#define __rapi2_api_h__
#include "rapi.h"

DWORD _CeGetSpecialFolderPath2(
        int nFolder,
        DWORD nBufferLength,
        LPWSTR lpBuffer);

BOOL _CeFindAllFiles2(
        LPCWSTR szPath,
        DWORD dwFlags,
        LPDWORD lpdwFoundCount,
        LPLPCE_FIND_DATA ppFindDataArray);

HANDLE _CeFindFirstFile2(
        LPCWSTR lpFileName,
        LPCE_FIND_DATA lpFindFileData);

BOOL _CeFindNextFile2(
        HANDLE hFindFile,
        LPCE_FIND_DATA lpFindFileData);

BOOL _CeFindClose2(
        HANDLE hFindFile);

DWORD _CeGetFileAttributes2(
        LPCWSTR lpFileName);


HANDLE _CeCreateFile2(
        LPCWSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile);

BOOL _CeReadFile2(
        HANDLE hFile,
        LPVOID lpBuffer,
        DWORD nNumberOfBytesToRead,
        LPDWORD lpNumberOfBytesRead,
        LPOVERLAPPED lpOverlapped);

BOOL _CeDeleteFile2(
        LPCWSTR lpFileName);

BOOL _CeWriteFile2(
        HANDLE hFile,
        LPCVOID lpBuffer,
        DWORD nNumberOfBytesToWrite,
        LPDWORD lpNumberOfBytesWritten,
        LPOVERLAPPED lpOverlapped);

DWORD _CeSetFilePointer2(
        HANDLE hFile,
        LONG lDistanceToMove,
        PLONG lpDistanceToMoveHigh,
        DWORD dwMoveMethod);

BOOL _CeSetEndOfFile2(
        HANDLE hFile);

BOOL _CeGetFileTime2(
        HANDLE hFile,
        LPFILETIME lpCreationTime,
        LPFILETIME lpLastAccessTime,
        LPFILETIME lpLastWriteTime);

BOOL _CeSetFileTime2(
        HANDLE hFile,
        LPFILETIME lpCreationTime,
        LPFILETIME lpLastAccessTime,
        LPFILETIME lpLastWriteTime);

BOOL _CeCloseHandle2(
        HANDLE hObject);

BOOL _CeCreateDirectory2(
        LPCWSTR lpPathName,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes);

BOOL _CeRemoveDirectory2(
        LPCWSTR lpPathName);

DWORD _CeGetFileSize2(
        HANDLE hFile,
        LPDWORD lpFileSizeHigh);

BOOL _CeCopyFile2(
        LPCWSTR lpExistingFileName,
        LPCWSTR lpNewFileName,
        BOOL bFailIfExists);


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
        PFILETIME lpftLastWriteTime);

BOOL _CeCopyFileA2(
        LPCSTR lpExistingFileName,
        LPCSTR lpNewFileName,
        BOOL bFailIfExists);

BOOL _CeMoveFile2(
        LPCWSTR lpExistingFileName,
        LPCWSTR lpNewFileName);

BOOL _CeCreateProcess2(
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

void _CeGetSystemInfo2(
        LPSYSTEM_INFO lpSystemInfo);

BOOL _CeGetVersionEx2(
        LPCEOSVERSIONINFO lpVersionInformation);

BOOL _CeGetSystemPowerStatusEx2(
        PSYSTEM_POWER_STATUS_EX pSystemPowerStatus,
        BOOL refresh);

BOOL _CeGetStoreInformation2(
        LPSTORE_INFORMATION lpsi);

#if 0

BOOL _CeGetSystemMemoryDivision2(
        LPDWORD lpdwStoragePages,
    LPDWORD lpdwRamPages,
    LPDWORD lpdwPageSize);

#else

BOOL _NotImplementedCeGetSystemMemoryDivision2(
	LPDWORD lpdwStoragePages,
	LPDWORD lpdwRamPages,
	LPDWORD lpdwPageSize);

#endif

LONG _CeRegCreateKeyEx2(
        HKEY hKey,
        LPCWSTR lpszSubKey,
        DWORD Reserved,
        LPWSTR lpszClass,
        DWORD ulOptions,
        REGSAM samDesired,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        PHKEY phkResult,
        LPDWORD lpdwDisposition);

LONG _CeRegOpenKeyEx2(
        HKEY hKey,
        LPCWSTR lpszSubKey,
        DWORD ulOptions,
        REGSAM samDesired,
        PHKEY phkResult);

LONG _CeRegEnumValue2( 
        HKEY hKey, 
        DWORD dwIndex, 
        LPWSTR lpszValueName, 
        LPDWORD lpcbValueName, 
        LPDWORD lpReserved, 
        LPDWORD lpType, 
        LPBYTE lpData, 
        LPDWORD lpcbData); 

LONG _CeRegEnumKeyEx2(
        HKEY hKey,
        DWORD dwIndex,
        LPWSTR lpName,
        LPDWORD lpcbName,
        LPDWORD lpReserved,
        LPWSTR lpClass,
        LPDWORD lpcbClass,
        PFILETIME lpftLastWriteTime);

LONG _CeRegQueryValueEx2(
        HKEY hKey,
        LPCWSTR lpValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE lpData,
        LPDWORD lpcbData);

LONG _CeRegSetValueEx2(
        HKEY hKey,
        LPCWSTR lpValueName,
        DWORD Reserved,
        DWORD dwType,
        const BYTE *lpData,
        DWORD cbData);

LONG _CeRegCloseKey2(
        HKEY hKey);

LONG _CeRegDeleteKey2(
        HKEY hKey,
        LPCWSTR lpszSubKey);

LONG _CeRegDeleteValue2(
        HKEY hKey,
        LPCWSTR lpszValueName);

HRESULT _CeProcessConfig2(LPCWSTR config, DWORD flags, LPWSTR* reply);

BOOL _CeStartReplication2(void);

HRESULT _CeSyncStart2(LPCWSTR params);

HRESULT _CeSyncResume2(void);

HRESULT _CeSyncPause2(void);

BOOL _CeSyncTimeToPc2();

DWORD _CeGetDiskFreeSpaceEx2(
		LPCTSTR _lpDirectoryName, 
		PULARGE_INTEGER lpFreeBytesAvailable, 
		PULARGE_INTEGER lpTotalNumberOfBytes, 
		PULARGE_INTEGER lpTotalNumberOfFreeBytes);

BOOL _NotImplementedCeSetFileAttributes2(
	LPCWSTR lpFileName,
	DWORD dwFileAttributes);

BOOL _NotImplementedCeSHCreateShortcut2(
	LPCWSTR lpszShortcut,
	LPCWSTR lpszTarget);

BOOL _NotImplementedCeRegCopyFile2(LPCWSTR filename);

BOOL _NotImplementedCeRegRestoreFile2(LPCWSTR filename);

BOOL _NotImplementedCeKillAllApps2();

DWORD _NotImplementedCeSetSystemMemoryDivision2(
	DWORD dwStoragePages);

BOOL _NotImplementedCeOidGetInfo2(
	CEOID oid,
	CEOIDINFO *poidInfo);

BOOL _NotImplementedCeCheckPassword2(
	LPWSTR lpszPassword);

CEOID _NotImplementedCeCreateDatabase2(
	LPWSTR lpszName,
	DWORD dwDbaseType,
	WORD wNumSortOrder,
	SORTORDERSPEC *rgSortSpecs);

BOOL _NotImplementedCeDeleteDatabase2(
	CEOID oid);

BOOL _NotImplementedCeFindAllDatabases2(
	DWORD dwDbaseType,
	WORD wFlags,
	LPWORD cFindData,
	LPLPCEDB_FIND_DATA ppFindData);

HANDLE _NotImplementedCeFindFirstDatabase2(
	DWORD dwDbaseType);

CEOID _NotImplementedCeFindNextDatabase2(
	HANDLE hEnum);

HANDLE _NotImplementedCeOpenDatabase2(
	PCEOID poid,
	LPWSTR lpszName,
	CEPROPID propid,
	DWORD dwFlags,
	HWND hwndNotify);

CEOID _NotImplementedCeReadRecordProps2(
	HANDLE hDbase,
	DWORD dwFlags,
	LPWORD lpcPropID,
	CEPROPID *rgPropID,
	LPBYTE *lplpBuffer,
	LPDWORD lpcbBuffer);

CEOID _NotImplementedCeWriteRecordProps2(
	HANDLE hDbase,
	CEOID oidRecord,
	WORD cPropID,
	CEPROPVAL* rgPropVal);

CEOID _NotImplementedCeSeekDatabase2(
	HANDLE hDatabase,
	DWORD dwSeekType,
	DWORD dwValue,
	LPDWORD lpdwIndex);

BOOL _NotImplementedCeDeleteRecord2(
	HANDLE hDatabase,
	CEOID oidRecord);

BOOL _NotImplementedCeSetDatabaseInfo2(
	CEOID oidDbase,
	CEDBASEINFO* pNewInfo);

/*
 * CeRapiInvoke stuff
 */

#ifndef SWIG

ULONG _IRAPIStream_Release2(IRAPIStream* stream);

HRESULT _IRAPIStream_Read2(
        IRAPIStream* stream,
        void *pv,
        ULONG cb,
        ULONG *pcbRead);

HRESULT _IRAPIStream_Write2(
        IRAPIStream* stream,
        void const *pv,
        ULONG cb,
        ULONG *pcbWritten);

HRESULT _CeRapiInvoke2(
        LPCWSTR pDllPath,
        LPCWSTR pFunctionName,
        DWORD cbInput,
        const BYTE *pInput,
        DWORD *pcbOutput,
        BYTE **ppOutput,
        IRAPIStream **ppIRAPIStream,
        DWORD dwReserved);

HRESULT _CeRapiInvokeA2(
        LPCSTR pDllPath,
        LPCSTR pFunctionName,
        DWORD cbInput,
        const BYTE *pInput,
        DWORD *pcbOutput,
        BYTE **ppOutput,
        IRAPIStream **ppIRAPIStream,
        DWORD dwReserved);

#endif /* SWIG */

#endif /* __rapi2_api_h__ */
