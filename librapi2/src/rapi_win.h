/* $Id$ */
#ifndef __rapi_win_h__
#define __rapi_win_h__

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef NULL
#include <stdlib.h>
#endif

/*
 * Simple types
 */

typedef u_int32_t  DWORD;
typedef u_int32_t  BOOL;
typedef u_int16_t  WCHAR;

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

typedef const char*   LPCSTR;
typedef const WCHAR*  LPCWSTR;


/*
 * Misc types
 */

/* This is usually a void*, but we must guarantee 32-bit! */
typedef u_int32_t  HANDLE;


#endif

