/* $Id$ */
#ifndef __rapi_h__
#define __rapi_h__

#include "rapi_types.h"

/*
 * Initialization functions
 */

HRESULT CeRapiInit(void);

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

#endif

