/* $Id$ */
#ifndef __rapi_types_h__
#define __rapi_types_h__

#include "rapi_config.h"

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifndef __cplusplus
#if HAVE_STDBOOL_H
#include <stdbool.h>
#endif
#endif

#ifndef NULL
#include <stdlib.h>
#endif

/*
 * Simple types
 */

typedef uint8_t   BYTE;

typedef uint16_t  WORD;
typedef uint16_t  USHORT;
typedef uint16_t  WCHAR;
typedef WCHAR     TCHAR;

typedef int32_t   LONG;
typedef int32_t   HKEY;
typedef int32_t   REGSAM;

typedef uint32_t  DWORD;
typedef uint32_t  ULONG;
typedef uint32_t  HWND;
typedef uint32_t  BOOL;

typedef uint64_t  ULARGE_INTEGER


/*
 * Pointer types
 */

typedef void*   LPVOID;
typedef char*   LPSTR;
typedef BYTE*   LPBYTE;
typedef WORD*   LPWORD;
typedef WCHAR*  LPWSTR;
typedef HKEY*   PHKEY;
typedef DWORD*  LPDWORD;

typedef ULARGE_INTEGER*		PULARGE_INTEGER
typedef ULARGE_INTEGER*		LPULARGE_INTEGER



/*
 * Const pointer types
 */

typedef const void*   LPCVOID;
typedef const char*   LPCSTR;
typedef const char*   LPCTSTR;
typedef const WCHAR*  LPCWSTR;


/*
 * Misc types
 */

typedef struct _FILETIME
{
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

/* A handle  is usually a void*, but we must guarantee 32-bit! */
typedef uint32_t  HANDLE;

#define INVALID_HANDLE_VALUE ((HANDLE)-1)

/* HRESULT must be a signed integer if the FAILED() macro should work */
typedef int32_t  HRESULT;

#define STDAPI HRESULT

/*
 * Some error codes (HRESULTs)
 */

#define E_ABORT         0x80004004
#define E_ACCESSDENIED  0x80070005
#define E_FAIL          0x80004005
#define E_HANDLE        0x80070006
#define E_INVALIDARG    0x80070057
#define E_NOINTERFACE   0x80004002
#define E_NOTIMPL       0x80004001
#define E_OUTOFMEMORY   0x8007000E
#define E_PENDING       0x8000000A
#define E_POINTER       0x80004003
#define E_UNEXPECTED    0x8000FFFF
#define S_FALSE         0x00000001
#define S_OK            0x00000000

#define FAILED(x) (x<0)

/*
 * Error codes for registry functions
 */

#define ERROR_SUCCESS			   0
#define ERROR_FILE_NOT_FOUND		   2
#define ERROR_NOT_ENOUGH_MEMORY		   8
#define ERROR_SEEK			  25
#define ERROR_INVALID_PARAMETER		  87
#define ERROR_INSUFFICIENT_BUFFER	 122
#define ERROR_NO_DATA		 	 232
#define ERROR_NO_MORE_ITEMS         	 259
#define ERROR_KEY_DELETED		1018

/*
 * Other macros
 */

#define MAX_PATH  256


#endif

