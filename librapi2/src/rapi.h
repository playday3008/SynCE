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
 * File functions
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

DWORD CeGetSpecialFolderPath( 
		int nFolder, 
		DWORD nBufferLength, 
		LPWSTR lpBuffer);

BOOL CeCloseHandle( 
		HANDLE hObject);

BOOL CeCopyFileA(
		LPCSTR lpExistingFileName, 
		LPCSTR lpNewFileName, 
		BOOL bFailIfExists);

BOOL CeCopyFile(
		LPCWSTR lpExistingFileName, 
		LPCWSTR lpNewFileName, 
		BOOL bFailIfExists);

typedef void* LPOVERLAPPED;

BOOL CeReadFile( 
		HANDLE hFile, 
		LPVOID lpBuffer, 
		DWORD nNumberOfBytesToRead, 
		LPDWORD lpNumberOfBytesRead, 
		LPOVERLAPPED lpOverlapped);

#ifdef __cplusplus
}
#endif


#endif

