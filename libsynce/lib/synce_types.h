/* $Id$ */
#ifndef __synce_types_h__
#define __synce_types_h__

#ifndef __synce_h__
#error Do not include this file directly, use synce.h
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
typedef uint32_t  UINT;
typedef uint32_t  ULONG;
typedef uint32_t  HWND;
typedef uint32_t  BOOL;

/* XXX: sizeof(double) must be 8 */
typedef double    DATE;

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
#define E_OUTOFMEMORY   0x8007000E
#define E_INVALIDARG    0x80070057
#define E_NOINTERFACE   0x80004002
#define E_NOTIMPL       0x80004001
#define E_OUTOFMEMORY   0x8007000E
#define E_PENDING       0x8000000A
#define E_POINTER       0x80004003
#define E_UNEXPECTED    0x8000FFFF
#define S_FALSE         0x00000001
#define S_OK            0x00000000

#define SUCCEEDED(x) ((x)>=0)
#define FAILED(x) ((x)<0)

/*
 * System Errors
 *
 * Add more from this page if needed:
 *
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcesdkr/html/_sdk_error_values.asp
 */

#define ERROR_SUCCESS                  0
#define ERROR_FILE_NOT_FOUND           2
#define ERROR_PATH_NOT_FOUND           3
#define ERROR_ACCESS_DENIED            5
#define ERROR_NOT_ENOUGH_MEMORY        8
#define ERROR_SEEK                    25
#define ERROR_NOT_SUPPORTED           50
#define ERROR_FILE_EXISTS             80
#define ERROR_INVALID_PARAMETER       87
#define ERROR_INSUFFICIENT_BUFFER    122
#define ERROR_INVALID_NAME           123
#define ERROR_DIR_NOT_EMPTY          145
#define ERROR_ALREADY_EXISTS         183
#define ERROR_NO_DATA                232
#define ERROR_NO_MORE_ITEMS          259
#define ERROR_KEY_DELETED           1018

/*
 * Other macros
 */

#define MAX_PATH  256

#ifndef FALSE
#define FALSE false
#endif

#ifndef TRUE
#define TRUE true
#endif

#endif

