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

typedef uint32_t  DWORD;
typedef uint32_t  BOOL;
typedef uint16_t  WCHAR;
typedef WCHAR      TCHAR;

/*
 * Pointer types
 */

typedef void*   LPVOID;
typedef DWORD*  LPDWORD;
typedef char*   LPSTR;
typedef WCHAR*  LPWSTR;


/*
 * Const pointer types
 */

typedef const void*   LPCVOID;
typedef const char*   LPCSTR;
typedef const WCHAR*  LPCWSTR;


/*
 * Misc types
 */

typedef struct _FILETIME
{
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME, *LPFILETIME;

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
 * Other macros
 */

#define MAX_PATH  256


#endif

