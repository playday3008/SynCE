/* $Id$ */
#ifndef __rapi_h__
#define __rapi_h__

#include "rapi_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * Initialization functions
 */

#define CERAPI_E_ALREADYINITIALIZED  0x8004101

HRESULT CeRapiInit(void);

STDAPI CeRapiUninit(void);

BOOL CeCheckPassword( 
		LPWSTR lpszPassword);


/*
 * Misc functions
 */

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

#define CSIDL_PROGRAMS           0x0002
#define CSIDL_PERSONAL           0x0005
#define CSIDL_FAVORITES_GRYPHON  0x0006
#define CSIDL_STARTUP            0x0007
#define CSIDL_RECENT             0x0008
#define CSIDL_STARTMENU          0x000b
#define CSIDL_DESKTOPDIRECTORY   0x0010
#define CSIDL_FONTS              0x0014
#define CSIDL_FAVORITES          0x0016

BOOL CeDeleteFile(
		LPCWSTR lpFileName);

DWORD CeGetSpecialFolderPath( 
		int nFolder, 
		DWORD nBufferLength, 
		LPWSTR lpBuffer);

BOOL CeCopyFileA(
		LPCSTR lpExistingFileName, 
		LPCSTR lpNewFileName, 
		BOOL bFailIfExists);

BOOL CeCopyFile(
		LPCWSTR lpExistingFileName, 
		LPCWSTR lpNewFileName, 
		BOOL bFailIfExists);

#ifdef __cplusplus
}
#endif


#endif

