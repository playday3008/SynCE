/* $Id$ */
#ifndef __rapi_h__
#define __rapi_h__

#include "rapi_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * Main RAPI functions
 */

#define CERAPI_E_ALREADYINITIALIZED  0x8004101

HRESULT CeRapiFreeBuffer( 
		LPVOID Buffer);

HRESULT CeRapiInit(void);

STDAPI CeRapiUninit(void);

/*
 * Misc functions
 */

BOOL CeCheckPassword( 
		LPWSTR lpszPassword);

DWORD CeGetLastError( void );

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

DWORD CeStartReplication( void );


/*
 * File access functions
 */

BOOL CeCloseHandle( 
		HANDLE hObject);

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


/*
 * File management functions
 */

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

#define CSIDL_PROGRAMS           0x0002
#define CSIDL_PERSONAL           0x0005
#define CSIDL_FAVORITES_GRYPHON  0x0006
#define CSIDL_STARTUP            0x0007
#define CSIDL_RECENT             0x0008
#define CSIDL_STARTMENU          0x000b
#define CSIDL_DESKTOPDIRECTORY   0x0010
#define CSIDL_FONTS              0x0014
#define CSIDL_FAVORITES          0x0016

DWORD CeGetSpecialFolderPath( 
		int nFolder, 
		DWORD nBufferLength, 
		LPWSTR lpBuffer);

BOOL CeMoveFile(
		LPCWSTR lpExistingFileName, 
		LPCWSTR lpNewFileName);

BOOL CeRemoveDirectory(
		LPCWSTR lpPathName);

#ifdef __cplusplus
}
#endif


#endif

