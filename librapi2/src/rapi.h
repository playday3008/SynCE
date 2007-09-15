/* $Id$ */
#ifndef __rapi_h__
#define __rapi_h__

/*
 * Note on #ifndef SWIG preprocessor token.
 *
 * The scripting langauge code in the pyrapi module uses
 * SWIG to generate the python wrapper code. To enable SWIG to
 * parse this header file some parts of it must be
 * selectively removed. SWIG defines "SWIG" as a
 * preproccessor token when is it parsing headers.
 * Therefore where you see #ifndef SWIG declarations
 * in this header they are intended to remove elements for
 * which the SWIG wrapper provides alternative
 * declarations. In most cases this is type and function
 * declarations.
 */

#ifndef SWIG
#include <synce.h>
#endif

#ifdef __cplusplus
namespace synce
{
extern "C"
{
#endif

/*
 * SynCE support for switching between multiple devices
 *
 * Example code for two devices follows!
 *
 * It shows two different ways to get a RapiConnection object.
 *
 *   RapiConnection* a = rapi_connection_from_path("/home/david/.synce/a");
 *   rapi_connection_select(a);
 *   CeRapiInit()
 *
 *   SynceInfo* info_b = synce_info_new("/home/david/.synce/b");
 *   RapiConnection* b = rapi_connection_from_info(info_b);
 *   rapi_connection_select(b);
 *   CeRapiInit()
 *
 *   rapi_connection_select(a);
 *   ...some RAPI calls to device A...
 *
 *   rapi_connection_select(b);
 *   ...some RAPI calls to device B...
 *
 *   rapi_connection_select(a);
 *   CeRapiUninit();
 *   rapi_connection_destroy(a);
 *
 *   rapi_connection_select(b);
 *   CeRapiUninit();
 *   rapi_connection_destroy(b);
 */

typedef struct _RapiConnection RapiConnection;

/**
 * Create a connection - use this before you call CeRapiInit()
 *
 * @path Parameter sent to synce_info_new() in libsynce/lib/info.c
 */
RapiConnection* rapi_connection_from_path(const char* path);

/**
 * Create a connection - low-level version
 */
RapiConnection* rapi_connection_from_info(SynceInfo* info);

/** Select what connection is used for RAPI calls */
void rapi_connection_select(RapiConnection* connection);

/** Destroy connection object - use this after CeRapiUninit() */
void rapi_connection_destroy(RapiConnection* connection);

/*
 * Main RAPI functions
 */

#define CERAPI_E_ALREADYINITIALIZED  0x8004101

#ifndef SWIG

HRESULT CeRapiFreeBuffer(
		LPVOID Buffer);

HRESULT CeRapiInit(void);

STDAPI CeRapiUninit(void);

HRESULT CeRapiGetError(void);

/** Not part of the real RAPI.
 * Use it instead of Win32's GetLastError() */
DWORD CeRapiGetLastError();

#endif /* SWIG */


/*
 * File access functions
 */

/* dwShareMode */
#define GENERIC_WRITE              0x40000000
#define GENERIC_READ               0x80000000
#define FILE_SHARE_READ            0x00000001

/* dwCreationDisposition */
#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5
#define OPEN_FOR_LOADER     6

/* dwFlagsAndAttributes */
#define FILE_ATTRIBUTE_READONLY   0x00000001
#define FILE_ATTRIBUTE_HIDDEN   0x00000002
#define FILE_ATTRIBUTE_SYSTEM   0x00000004
#define FILE_ATTRIBUTE_1    0x00000008

#define FILE_ATTRIBUTE_DIRECTORY  0x00000010
#define FILE_ATTRIBUTE_ARCHIVE    0x00000020
#define FILE_ATTRIBUTE_INROM    0x00000040
#define FILE_ATTRIBUTE_NORMAL   0x00000080

#define FILE_ATTRIBUTE_TEMPORARY  0x00000100
#define FILE_ATTRIBUTE_2          0x00000200
#define FILE_ATTRIBUTE_3          0x00000400
#define FILE_ATTRIBUTE_COMPRESSED 0x00000800

#define FILE_ATTRIBUTE_ROMSTATICREF 0x00001000
#define FILE_ATTRIBUTE_ROMMODULE  0x00002000
#define FILE_ATTRIBUTE_4          0x00004000
#define FILE_ATTRIBUTE_5          0x00008000

#define FILE_ATTRIBUTE_HAS_CHILDREN 0x00010000
#define FILE_ATTRIBUTE_SHORTCUT   0x00020000
#define FILE_ATTRIBUTE_6          0x00040000
#define FILE_ATTRIBUTE_7          0x00080000

/* dwMoveMethod */
#define FILE_BEGIN      0
#define FILE_CURRENT    1
#define FILE_END        2


#ifndef SWIG

BOOL CeCloseHandle(
		HANDLE hObject);

typedef void* LPSECURITY_ATTRIBUTES;

HANDLE CeCreateFile(
		LPCWSTR lpFileName,
		DWORD dwDesiredAccess,
		DWORD dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		DWORD dwCreationDisposition,
		DWORD dwFlagsAndAttributes,
		HANDLE hTemplateFile);

typedef void* LPOVERLAPPED;

BOOL CeReadFile(
		HANDLE hFile,
		LPVOID lpBuffer,
		DWORD nNumberOfBytesToRead,
		LPDWORD lpNumberOfBytesRead,
		LPOVERLAPPED lpOverlapped);

BOOL CeWriteFile(
		HANDLE hFile,
		LPCVOID lpBuffer,
		DWORD nNumberOfBytesToWrite,
		LPDWORD lpNumberOfBytesWritten,
		LPOVERLAPPED lpOverlapped);

DWORD CeSetFilePointer(
		HANDLE hFile,
		LONG lDistanceToMove,
		PLONG lpDistanceToMoveHigh,
		DWORD dwMoveMethod);

BOOL CeSetEndOfFile(
                HANDLE hObject);

BOOL CeGetFileTime(
		HANDLE hFile,
		LPFILETIME lpCreationTime,
		LPFILETIME lpLastAccessTime,
		LPFILETIME lpLastWriteTime);

BOOL CeSetFileTime(
		HANDLE hFile,
		LPFILETIME lpCreationTime,
		LPFILETIME lpLastAccessTime,
		LPFILETIME lpLastWriteTime);

#endif /* SWIG */


/*
 * File management functions
 */

#define FAF_ATTRIBUTES      0x00001
#define FAF_CREATION_TIME   0x00002
#define FAF_LASTACCESS_TIME   0x00004
#define FAF_LASTWRITE_TIME    0x00008

#define FAF_SIZE_HIGH     0x00010
#define FAF_SIZE_LOW      0x00020
#define FAF_OID       0x00040
#define FAF_NAME      0x00080

#define FAF_ATTRIB_CHILDREN   0x01000
#define FAF_ATTRIB_NO_HIDDEN    0x02000
#define FAF_FOLDERS_ONLY    0x04000
#define FAF_NO_HIDDEN_SYS_ROMMODULES  0x08000

#define CSIDL_PROGRAMS           0x0002
#define CSIDL_PERSONAL           0x0005
#define CSIDL_FAVORITES_GRYPHON  0x0006
#define CSIDL_STARTUP            0x0007
#define CSIDL_RECENT             0x0008
#define CSIDL_STARTMENU          0x000b
#define CSIDL_DESKTOPDIRECTORY   0x0010
#define CSIDL_FONTS              0x0014
#define CSIDL_FAVORITES          0x0016

#ifndef SWIG

BOOL CeCopyFileA(
		LPCSTR lpExistingFileName,
		LPCSTR lpNewFileName,
		BOOL bFailIfExists);

BOOL CeCopyFile(
		LPCWSTR lpExistingFileName,
		LPCWSTR lpNewFileName,
		BOOL bFailIfExists);

BOOL CeCreateDirectory(
		LPCWSTR lpPathName,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes);

BOOL CeDeleteFile(
		LPCWSTR lpFileName);

typedef struct _CE_FIND_DATA {
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwOID;
	WCHAR cFileName[MAX_PATH];
} CE_FIND_DATA, *LPCE_FIND_DATA, **LPLPCE_FIND_DATA;

BOOL CeFindAllFiles(
		LPCWSTR szPath,
		DWORD dwFlags,
		LPDWORD lpdwFoundCount,
		LPLPCE_FIND_DATA ppFindDataArray);

HANDLE CeFindFirstFile(
		LPCWSTR lpFileName,
		LPCE_FIND_DATA lpFindFileData);

BOOL CeFindNextFile(
		HANDLE hFindFile,
		LPCE_FIND_DATA lpFindFileData);

BOOL CeFindClose(
		HANDLE hFindFile);

DWORD CeGetFileAttributes(
		LPCWSTR lpFileName);

DWORD CeGetFileSize(
		HANDLE hFile,
		LPDWORD lpFileSizeHigh);

DWORD CeGetSpecialFolderPath(
		int nFolder,
		DWORD nBufferLength,
		LPWSTR lpBuffer);

BOOL CeMoveFile(
		LPCWSTR lpExistingFileName,
		LPCWSTR lpNewFileName);

BOOL CeRemoveDirectory(
		LPCWSTR lpPathName);

BOOL CeSetFileAttributes(
		LPCWSTR lpFileName,
		DWORD dwFileAttributes);

BOOL CeSHCreateShortcut(
  LPCWSTR lpszShortcut,
  LPCWSTR lpszTarget);

BOOL CeSyncTimeToPc();

#endif /* SWIG */


/*
 * Database functions
 */

#define CEDB_MAXDBASENAMELEN    32
#define CEDB_MAXSORTORDER       4

#define CEDB_VALIDNAME          0x0001
#define CEDB_VALIDTYPE          0x0002
#define CEDB_VALIDSORTSPEC      0x0004
#define CEDB_VALIDMODTIME       0x0008

#define CEDB_AUTOINCREMENT              0x00000001
#define CEDB_NOCOMPRESS                 0x00010000

#define CEDB_SORT_DESCENDING            0x00000001
#define CEDB_SORT_CASEINSENSITIVE       0x00000002
#define CEDB_SORT_UNKNOWNFIRST          0x00000004
#define CEDB_SORT_GENERICORDER          0x00000008

#define CEDB_SEEK_CEOID                 0x00000001
#define CEDB_SEEK_BEGINNING             0x00000002
#define CEDB_SEEK_END                   0x00000004
#define CEDB_SEEK_CURRENT               0x00000008

#define CEDB_SEEK_VALUESMALLER          0x00000010
#define CEDB_SEEK_VALUEFIRSTEQUAL       0x00000020
#define CEDB_SEEK_VALUEGREATER          0x00000040
#define CEDB_SEEK_VALUENEXTEQUAL        0x00000080

#define CEDB_PROPNOTFOUND               0x0100
#define CEDB_PROPDELETE                 0x0200
#define CEDB_MAXDATABLOCKSIZE           4092
#define CEDB_MAXPROPDATASIZE            (CEDB_MAXDATABLOCKSIZE*16)
#define CEDB_MAXRECORDSIZE              (128*1024)
#define CEDB_ALLOWREALLOC               0x00000001

#define CEVT_I2         2
#define CEVT_I4         3
#define CEVT_R8         5
#define CEVT_BOOL       11
#define CEVT_UI2        18
#define CEVT_UI4        19
#define CEVT_LPWSTR     31
#define CEVT_FILETIME   64
#define CEVT_BLOB       65

/* Undocumented flag used by synchronization to denote an empty field */
#define CEVT_FLAG_EMPTY                 0x0400

#define FAD_OID                         0x0001
#define FAD_FLAGS                       0x0002
#define FAD_NAME                        0x0004
#define FAD_TYPE                        0x0008

#define FAD_NUM_RECORDS                 0x0010
#define FAD_NUM_SORT_ORDER              0x0020
#define FAD_SIZE                        0x0040
#define FAD_LAST_MODIFIED               0x0080

#define FAD_SORT_SPECS                  0x0100

#ifndef SWIG

typedef DWORD CEPROPID;
typedef CEPROPID *PCEPROPID;
typedef DWORD CEOID;
typedef CEOID *PCEOID;

typedef struct _CEBLOB {
	DWORD dwCount;
	LPBYTE lpb;
} CEBLOB;

typedef union _CEVALUNION {
	short iVal;
	USHORT uiVal;
	LONG lVal;
	ULONG ulVal;
	FILETIME filetime;
	LPWSTR lpwstr;
	CEBLOB blob;
	BOOL boolVal;
	double dblVal;
} CEVALUNION;

typedef struct _CEPROPVAL {
	CEPROPID propid;
	WORD wLenData;
	WORD wFlags;
	CEVALUNION val;
} CEPROPVAL;
typedef CEPROPVAL *PCEPROPVAL;

typedef struct _SORTORDERSPEC {
	CEPROPID propid;
	DWORD dwFlags;
} SORTORDERSPEC;

typedef struct _CEDBASEINFO {
	DWORD dwFlags;
	WCHAR szDbaseName[CEDB_MAXDBASENAMELEN];
	DWORD dwDbaseType;
	WORD wNumRecords;
	WORD wNumSortOrder;
	DWORD dwSize;
	FILETIME ftLastModified;
	SORTORDERSPEC rgSortSpecs[CEDB_MAXSORTORDER];
} CEDBASEINFO;

typedef struct _CEDB_FIND_DATA {
	CEOID OidDb;
	CEDBASEINFO DbInfo;
} CEDB_FIND_DATA, *LPCEDB_FIND_DATA, **LPLPCEDB_FIND_DATA;

CEOID CeCreateDatabase(
		LPWSTR lpszName,
		DWORD dwDbaseType,
		WORD wNumSortOrder,
		SORTORDERSPEC *rgSortSpecs);

BOOL CeDeleteDatabase(
		CEOID oid);

BOOL CeFindAllDatabases(
		DWORD dwDbaseType,
		WORD wFlags,
		LPWORD cFindData,
		LPLPCEDB_FIND_DATA ppFindData);

HANDLE CeFindFirstDatabase(
		DWORD dwDbaseType);

CEOID CeFindNextDatabase(
		HANDLE hEnum);

HANDLE CeOpenDatabase(
		PCEOID poid,
		LPWSTR lpszName,
		CEPROPID propid,
		DWORD dwFlags,
		HWND hwndNotify);

CEOID CeReadRecordProps(
		HANDLE hDbase,
		DWORD dwFlags,
		LPWORD lpcPropID,
		CEPROPID *rgPropID,
		LPBYTE *lplpBuffer,
		LPDWORD lpcbBuffer);

CEOID CeSeekDatabase(
		HANDLE hDatabase,
		DWORD dwSeekType,
		DWORD dwValue,
		LPDWORD lpdwIndex);

CEOID CeWriteRecordProps(
		HANDLE hDbase,
		CEOID oidRecord,
		WORD cPropID,
		CEPROPVAL *rgPropVal);

BOOL CeDeleteRecord(
    HANDLE hDatabase,
    CEOID oidRecord);

BOOL CeSetDatabaseInfo(
    CEOID oidDbase,
    CEDBASEINFO* pNewInfo);

#endif /* SWIG */

/*
 * Registry
 */

#define HKEY_CLASSES_ROOT           ((HKEY)0x80000000)
#define HKEY_CURRENT_USER           ((HKEY)0x80000001)
#define HKEY_LOCAL_MACHINE          ((HKEY)0x80000002)
#define HKEY_USERS                  ((HKEY)0x80000003)

#define REG_NONE                    0
#define REG_SZ                      1
#define REG_EXPAND_SZ               2
#define REG_BINARY                  3
#define REG_DWORD                   4
#define REG_DWORD_LITTLE_ENDIAN     4
#define REG_DWORD_BIG_ENDIAN        5
#define REG_LINK                    6
#define REG_MULTI_SZ                7

#define REG_CREATED_NEW_KEY         1
#define REG_OPENED_EXISTING_KEY     2

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
		LPDWORD lpdwDisposition);

LONG CeRegOpenKeyEx(
		HKEY hKey,
		LPCWSTR lpszSubKey,
		DWORD ulOptions,
		REGSAM samDesired,
		PHKEY phkResult);

LONG CeRegCloseKey(
		HKEY hKey);

LONG CeRegDeleteKey(
		HKEY hKey,
		LPCWSTR lpszSubKey);

LONG CeRegDeleteValue(
		HKEY hKey,
		LPCWSTR lpszValueName);

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
		PFILETIME lpftLastWriteTime);

LONG CeRegQueryValueEx(
		HKEY hKey,
		LPCWSTR lpValueName,
		LPDWORD lpReserved,
		LPDWORD lpType,
		LPBYTE lpData,
		LPDWORD lpcbData);

LONG CeRegEnumValue(
		HKEY hKey,
		DWORD dwIndex,
		LPWSTR lpszValueName,
		LPDWORD lpcbValueName,
		LPDWORD lpReserved,
		LPDWORD lpType,
		LPBYTE lpData,
		LPDWORD lpcbData);

LONG CeRegEnumKeyEx(
		HKEY hKey,
		DWORD dwIndex,
		LPWSTR lpName,
		LPDWORD lpcbName,
		LPDWORD lpReserved,
		LPWSTR lpClass,
		LPDWORD lpcbClass,
		PFILETIME lpftLastWriteTime);

LONG CeRegSetValueEx(
		HKEY hKey,
		LPCWSTR lpValueName,
		DWORD Reserved,
		DWORD dwType,
		const BYTE *lpData,
		DWORD cbData);

/*
 * Convenience functions for easy registry access
 */

bool rapi_reg_create_key(
		HKEY parent,
		const char* name,
		HKEY* key);

bool rapi_reg_open_key(
		HKEY parent, const char* name, HKEY* key);

bool rapi_reg_query_dword(
		HKEY key,
		const char* name,
		DWORD* value);

bool rapi_reg_query_string(
		HKEY key,
		const char* name,
		char** value);

#define rapi_reg_free_string(str) wstr_free_string(str)

bool rapi_reg_set_dword(
		HKEY key,
		const char* name,
		DWORD value);

bool rapi_reg_set_string(
		HKEY key,
		const char* name,
		const char *value);

#endif /* SWIG */

/*
 * Misc functions
 */

#define PROCESSOR_INTEL_386     386
#define PROCESSOR_INTEL_486     486
#define PROCESSOR_INTEL_PENTIUM 586
#define PROCESSOR_INTEL_PENTIUMII 686
#define PROCESSOR_MIPS_R4000    4000
#define PROCESSOR_ALPHA_21064   21064
#define PROCESSOR_PPC_403       403
#define PROCESSOR_PPC_601       601
#define PROCESSOR_PPC_603       603
#define PROCESSOR_PPC_604       604
#define PROCESSOR_PPC_620       620
#define PROCESSOR_HITACHI_SH3   10003
#define PROCESSOR_HITACHI_SH3E  10004
#define PROCESSOR_HITACHI_SH4   10005
#define PROCESSOR_MOTOROLA_821  821
#define PROCESSOR_SHx_SH3       103
#define PROCESSOR_SHx_SH4       104
#define PROCESSOR_STRONGARM     2577
#define PROCESSOR_ARM720        1824
#define PROCESSOR_ARM820        2080
#define PROCESSOR_ARM920        23360
#define PROCESSOR_ARM_7TDMI     70001

#define PROCESSOR_ARCHITECTURE_INTEL 0
#define PROCESSOR_ARCHITECTURE_MIPS  1
#define PROCESSOR_ARCHITECTURE_ALPHA 2
#define PROCESSOR_ARCHITECTURE_PPC   3
#define PROCESSOR_ARCHITECTURE_SHX   4
#define PROCESSOR_ARCHITECTURE_ARM   5
#define PROCESSOR_ARCHITECTURE_IA64  6
#define PROCESSOR_ARCHITECTURE_ALPHA64 7
#define PROCESSOR_ARCHITECTURE_UNKNOWN 0xFFFF

#define AC_LINE_OFFLINE                 0x00
#define AC_LINE_ONLINE                  0x01
#define AC_LINE_BACKUP_POWER            0x02
#define AC_LINE_UNKNOWN                 0xFF

#define BATTERY_FLAG_HIGH               0x01
#define BATTERY_FLAG_LOW                0x02
#define BATTERY_FLAG_CRITICAL           0x04
#define BATTERY_FLAG_CHARGING           0x08
#define BATTERY_FLAG_NO_BATTERY         0x80
#define BATTERY_FLAG_UNKNOWN            0xFF

#define BATTERY_PERCENTAGE_UNKNOWN      0xFF

#define BATTERY_LIFE_UNKNOWN        0xFFFFFFFF

#define VER_PLATFORM_WIN32s             0
#define VER_PLATFORM_WIN32_WINDOWS      1
#define VER_PLATFORM_WIN32_NT           2
#define VER_PLATFORM_WIN32_HH           3
#define VER_PLATFORM_WIN32_CE           3

#define OBJTYPE_INVALID     0
#define OBJTYPE_FILE        1
#define OBJTYPE_DIRECTORY   2
#define OBJTYPE_DATABASE    3
#define OBJTYPE_RECORD      4
/*
 * returned by CeOidGetInfo() for an ActiveSync notification when
 * an object has been deleted.
 */
#define OBJTYPE_DELETED     8

/* Flags for CeProcessConfig */

#define CONFIG_PROCESS_DOCUMENT   1
#define CONFIG_RETURN_METADATA    2

#ifndef SWIG

BOOL CeCheckPassword(
		LPWSTR lpszPassword);

typedef struct _PROCESS_INFORMATION {
	HANDLE hProcess;
	HANDLE hThread;
	DWORD dwProcessId;
	DWORD dwThreadId;
} PROCESS_INFORMATION;

typedef PROCESS_INFORMATION* LPPROCESS_INFORMATION;

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
		LPPROCESS_INFORMATION lpProcessInformation);

DWORD CeGetLastError( void );

typedef struct STORE_INFORMATION {
	DWORD dwStoreSize;
	DWORD dwFreeSize;
} STORE_INFORMATION, *LPSTORE_INFORMATION;

BOOL CeGetStoreInformation(
		LPSTORE_INFORMATION lpsi);

typedef struct _SYSTEM_INFO {
/*	DWORD dwOemId;*/
	WORD wProcessorArchitecture;
	WORD wReserved;
	DWORD dwPageSize;
	ULONG lpMinimumApplicationAddress;
	ULONG lpMaximumApplicationAddress;
	DWORD dwActiveProcessorMask;
	DWORD dwNumberOfProcessors;
	DWORD dwProcessorType;
	DWORD dwAllocationGranularity;
	WORD wProcessorLevel;
	WORD wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

void CeGetSystemInfo(
		LPSYSTEM_INFO lpSystemInfo);

typedef struct _SYSTEM_POWER_STATUS_EX {
	BYTE ACLineStatus;
	BYTE BatteryFlag;
	BYTE BatteryLifePercent;
	BYTE Reserved1;
	DWORD BatteryLifeTime;
	DWORD BatteryFullLifeTime;
	BYTE Reserved2;
	BYTE BackupBatteryFlag;
	BYTE BackupBatteryLifePercent;
	BYTE Reserved3;
	DWORD BackupBatteryLifeTime;
	DWORD BackupBatteryFullLifeTime;
} SYSTEM_POWER_STATUS_EX, *PSYSTEM_POWER_STATUS_EX, *LPSYSTEM_POWER_STATUS_EX;

BOOL CeGetSystemPowerStatusEx(PSYSTEM_POWER_STATUS_EX pSystemPowerStatus, BOOL refresh);

typedef struct _CEOSVERSIONINFO{
	DWORD dwOSVersionInfoSize;
	DWORD dwMajorVersion;
	DWORD dwMinorVersion;
	DWORD dwBuildNumber;
	DWORD dwPlatformId;
	WCHAR szCSDVersion[128];
} CEOSVERSIONINFO, *LPCEOSVERSIONINFO;

BOOL CeGetVersionEx(
		LPCEOSVERSIONINFO lpVersionInformation);

typedef struct _CEFILEINFO {
	DWORD dwAttributes;
	CEOID oidParent;
	WCHAR szFileName[MAX_PATH];
	FILETIME ftLastChanged;
	DWORD dwLength;
} CEFILEINFO;

typedef struct _CEDIRINFO {
	DWORD dwAttributes;
	CEOID oidParent;
	WCHAR szDirName[MAX_PATH];
} CEDIRINFO;

typedef struct _CERECORDINFO {
	CEOID oidParent;
} CERECORDINFO;

typedef struct _CEOIDINFO {
	WORD wObjType;
	WORD wPad;
	union {
		CEFILEINFO infFile;
		CEDIRINFO infDirectory;
		CEDBASEINFO infDatabase;
		CERECORDINFO infRecord;
	} u;
} CEOIDINFO;

BOOL CeOidGetInfo(
		CEOID oid,
		CEOIDINFO *poidInfo);

HRESULT CeProcessConfig(LPCWSTR config, DWORD flags, LPWSTR* reply);

BOOL CeStartReplication( void );

HRESULT CeSyncStart( LPCWSTR params );

HRESULT CeSyncResume( void );

HRESULT CeSyncPause( void );

#define SYSMEM_CHANGED        0
#define SYSMEM_MUSTREBOOT     1
#define SYSMEM_REBOOTPENDING  2
#define SYSMEM_FAILED         3

BOOL CeGetSystemMemoryDivision(
    LPDWORD lpdwStoragePages,
    LPDWORD lpdwRamPages,
    LPDWORD lpdwPageSize);

DWORD CeSetSystemMemoryDivision(
    DWORD dwStoragePages);

BOOL CeRegCopyFile(LPCWSTR filename);
BOOL CeRegRestoreFile(LPCWSTR filename);

BOOL CeKillAllApps();

#endif /* SWIG */

/*
 * CeRapiInvoke stuff
 */

struct _IRAPIStream;
typedef struct _IRAPIStream IRAPIStream;

#ifndef SWIG

ULONG IRAPIStream_Release(IRAPIStream* stream);

HRESULT IRAPIStream_Read(
		IRAPIStream* stream,
		void *pv,
		ULONG cb,
		ULONG *pcbRead);

HRESULT IRAPIStream_Write(
		IRAPIStream* stream,
		void const *pv,
		ULONG cb,
		ULONG *pcbWritten);

int IRAPIStream_GetRawSocket(IRAPIStream* stream);

HRESULT CeRapiInvoke(
		LPCWSTR pDllPath,
		LPCWSTR pFunctionName,
		DWORD cbInput,
		const BYTE *pInput,
		DWORD *pcbOutput,
		BYTE **ppOutput,
		IRAPIStream **ppIRAPIStream,
		DWORD dwReserved);

HRESULT CeRapiInvokeA(
		LPCSTR pDllPath,
		LPCSTR pFunctionName,
		DWORD cbInput,
		const BYTE *pInput,
		DWORD *pcbOutput,
		BYTE **ppOutput,
		IRAPIStream **ppIRAPIStream,
		DWORD dwReserved);

#endif /* SWIG */

#ifdef __cplusplus
}
}
#endif

#endif

