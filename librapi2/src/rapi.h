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

