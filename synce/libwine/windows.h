/***************************************************************************
 *   Copyright (C) 2001 by Ludovic LANGE                                   *
 *   ludovic.lange@free.fr                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************
 * Some parts are :                                                        *
 * Copyright (c) 1993-2000 the Wine project authors (see the file AUTHORS  *
 * for a complete list)                                                    *
 ***************************************************************************/
#ifndef WINDOWS_TYPES_H
#define WINDOWS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*#ifdef REAL_WCHAR_T
#include <wchar.h>
#endif*/
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

typedef unsigned char BYTE;
typedef BYTE * PBYTE;
typedef BYTE * LPBYTE;

typedef unsigned short USHORT;

typedef unsigned short WORD;
typedef WORD * LPWORD;

typedef unsigned long DWORD;
typedef DWORD * LPDWORD;

typedef unsigned int UINT;
typedef int INT;

typedef long BOOL;
typedef BOOL * LPBOOL;

typedef long LONG;
typedef LONG * PLONG;

typedef DWORD ULONG;

typedef double DOUBLE;

typedef char CHAR;
typedef CHAR * LPSTR;
typedef const CHAR * LPCSTR;


/*#ifdef REAL_WCHAR_T
typedef wchar_t WCHAR;
#else*/
typedef unsigned short WCHAR;
/*#endif*/
typedef unsigned short _WIN_WCHAR;

typedef WCHAR TCHAR;
typedef WCHAR * LPWSTR;
typedef TCHAR * LPTSTR;
typedef const WCHAR * LPCWSTR;
typedef const TCHAR * LPCTSTR;

typedef void VOID;
typedef VOID * PVOID;
typedef VOID * LPVOID;
typedef const VOID * LPCVOID;

typedef LONG HRESULT;
typedef void * HANDLE;


typedef HRESULT STDAPI;
#define STDAPI_(a) a
#define __stdcall
#define STDAPICALLTYPE

typedef char * LPSECURITY_ATTRIBUTES;
typedef char * LPSTARTUPINFO;
typedef char * LPPROCESS_INFORMATION;
typedef char * LPOVERLAPPED;
typedef long HKEY;
typedef HKEY * PHKEY;
typedef long REGSAM;

#define FAILED(a) (a<0)
#define E_SUCCESS			   0
#define ERROR_SUCCESS			   0
#define ERROR_FILE_NOT_FOUND		   2
#define ERROR_NOT_ENOUGH_MEMORY		   8
#define ERROR_SEEK			  25
#define ERROR_INVALID_PARAMETER		  87
#define ERROR_INSUFFICIENT_BUFFER	 122
#define ERROR_NO_DATA		 	 232
#define ERROR_NO_MORE_ITEMS         	 259
#define ERROR_KEY_DELETED		1018
#define E_FAIL				0x80004005
#define INVALID_HANDLE_VALUE		((HANDLE)-1)

typedef void * HWND;

#define LocalFree( ptr ) free( ptr );

typedef struct _PROCESS_INFORMATION {
	HANDLE hProcess;
	HANDLE hThread;
	DWORD  dwProcessId;
	DWORD  dwThreadId;
} PROCESS_INFORMATION, *LPROCESS_INFORMATION;

typedef struct _FILETIME { // ft seems to be : nbr of 100ns since Jan 1 1601 ???
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME;
#define LPFILETIME FILETIME *
#define PFILETIME FILETIME *

#define TRUE (BOOL)(1==1)
#define FALSE (BOOL)(!TRUE)




size_t _WIN_wcstombs( char *s, const TCHAR *pwcs, size_t n );
size_t _WIN_mbstowcs( _WIN_WCHAR * strDestination, char * strSource, size_t n );
size_t _WIN_wcslen( const _WIN_WCHAR * str );
int _WIN_wprintf( const WCHAR * format, ... );
unsigned int strlenW( const WCHAR * str );
WCHAR * strcpyW( WCHAR * dst, const WCHAR * src );
WCHAR * strcatW( WCHAR *dst, const WCHAR *src );
time_t DOSFS_FileTimeToUnixTime( const FILETIME *filetime, DWORD *remainder );
BOOL FileTimeToDosDateTime( const FILETIME * lpFileTime, LPWORD lpFatDate, LPWORD lpFatTime );

#define TEXT(a) (L##a)
#define _tprintf _WIN_wprintf
#define _tcscpy wcscpy
#define wprintf _WIN_wprintf
#define wcslen strlenW
#define wcstombs _WIN_wcstombs
#define mbstowcs _WIN_mbstowcs
#define wcslen strlenW
#define wcscpy strcpyW
#define wcscat strcatW









#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* WINDOWS_TYPES_H */
