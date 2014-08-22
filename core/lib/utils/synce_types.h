/* $Id$ */
#ifndef __synce_types_h__
#define __synce_types_h__

#ifndef __synce_h__
#error Do not include this file directly, use synce.h
#endif

/** 
 * @defgroup SynceTypes Data types used in Windows
 * @ingroup SynceUtils
 * @brief Data types used in Windows, defined here to simplify communication.
 *
 * @{ 
 */ 

/** @name Simple types
 */
/**@{*/
typedef void      VOID;

typedef uint8_t   BYTE;
typedef BYTE      BOOLEAN;

typedef int16_t   CSHORT;

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

typedef int64_t LONGLONG;
typedef uint64_t  ULARGE_INTEGER;


/* XXX: sizeof(double) must be 8 */
typedef double    DATE;
/**@}*/


/** @name Pointer types
 */
/**@{*/
typedef void*   LPVOID;
typedef char*   LPSTR;
typedef BYTE*   LPBYTE;
typedef WORD*   LPWORD;
typedef WCHAR*  LPWSTR;
typedef HKEY*   PHKEY;
typedef DWORD*  LPDWORD;
typedef LONG*   PLONG;
typedef ULARGE_INTEGER*         PULARGE_INTEGER ;
typedef ULARGE_INTEGER*         LPULARGE_INTEGER;
/**@}*/



/** @name Const pointer types
 */
/**@{*/
typedef const void*   LPCVOID;
typedef const char*   LPCSTR;
typedef const WCHAR*  LPCWSTR;
typedef const char*   LPCTSTR ;
/**@}*/


/** @name Miscellaneous types
 */
/**@{*/
typedef struct _TIME_FIELDS
{
  CSHORT Year;          /* Specifies a value from 1601 on. */
  CSHORT Month;         /* Specifies a value from 1 to 12. */
  CSHORT Day;           /* Specifies a value from 1 to 31. */
  CSHORT Hour;          /* Specifies a value from 0 to 23. */
  CSHORT Minute;        /* Specifies a value from 0 to 59. */
  CSHORT Second;        /* Specifies a value from 0 to 59. */
  CSHORT Milliseconds;  /* Specifies a value from 0 to 999. */
  CSHORT Weekday;       /* Specifies a value from 0 to 6 (Sunday to Saturday). */
} TIME_FIELDS, *PTIME_FIELDS;

typedef union _LARGE_INTEGER {
  struct {
    DWORD LowPart;
    LONG HighPart;
  } u;
  LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef struct _FILETIME
{
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

/* A handle  is usually a void*, but we must guarantee 32-bit! */
/** @brief Handle to a remote file, object etc. */
typedef uint32_t  HANDLE;

/** @brief Value assigned to an invalid #HANDLE */
#define INVALID_HANDLE_VALUE ((HANDLE)-1)

/**@}*/

/**@}*/

/** 
 * @addtogroup SynceMisc
 * @{ 
 */

/** @name HRESULT */
/** 
 * @{ 
 */

/** @brief Composite return value
 *
 * HRESULT is a composite value returned by various MS functions.
 * The components we are concerned with are severity, facility, and code.
 * It must be a signed integer if the FAILED() macro is to work.
 */
typedef int32_t  HRESULT;

#define STDAPI HRESULT

#define SEVERITY_SUCCESS	0
#define SEVERITY_ERROR		1

#define FACILITY_NULL		0
#define FACILITY_RPC		1
#define FACILITY_DISPATCH	2
#define FACILITY_STORAGE	3
#define FACILITY_ITF		4
#define FACILITY_WIN32		7
#define FACILITY_WINDOWS	8
#define FACILITY_SECURITY	9
#define FACILITY_SSPI		9
#define FACILITY_CONTROL	10
#define FACILITY_CERT		11
#define FACILITY_INTERNET	12
#define FACILITY_MEDIASERVER	13
#define FACILITY_MSMQ		14
#define FACILITY_SETUPAPI	15
#define FACILITY_SCARD		16
#define FACILITY_COMPLUS	17
#define FACILITY_AAF		18
#define FACILITY_URT		19
#define FACILITY_ACS		20
#define FACILITY_DPLAY		21
#define FACILITY_UMI		22
#define FACILITY_SXS		23
#define FACILITY_WINDOWS_CE	24
#define FACILITY_HTTP		25
#define FACILITY_USERMODE_COMMONLOG	26
#define FACILITY_USERMODE_FILTER_MANAGER	31
#define FACILITY_BACKGROUNDCOPY		32
#define FACILITY_CONFIGURATION		33
#define FACILITY_STATE_MANAGEMENT	34
#define FACILITY_METADIRECTORY		35
#define FACILITY_WINDOWSUPDATE		36
#define FACILITY_DIRECTORYSERVICE	37
#define FACILITY_GRAPHICS		38
#define FACILITY_SHELL			39
#define FACILITY_TPM_SERVICES		40
#define FACILITY_TPM_SOFTWARE		41
#define FACILITY_PLA			48
#define FACILITY_FVE			49
#define FACILITY_FWP			50
#define FACILITY_WINRM			51
#define FACILITY_NDIS			52
#define FACILITY_USERMODE_HYPERVISOR	53
#define FACILITY_CMI			54
#define FACILITY_WINDOWS_DEFENDER	80


/** @brief Assemble an HRESULT
 *
 * Returns an HRESULT from it's component parts.
 * @param[in] sev SEVERITY_SUCCESS or SEVERITY_FAILURE
 * @param[in] fac facility code
 * @param[in] code error code
 * @return the assembled HRESULT
 */
#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT) (((DWORD)(sev)<<31) | ((DWORD)(fac)<<16) | ((DWORD)(code))) )

/** @brief Extracts the error code portion of the HRESULT
 *
 * @param[in] hr the HRESULT containing the required error code
 * @return the error code
 */
#define HRESULT_CODE(hr)    ((hr) & 0xFFFF)

/** @brief Extracts the facility code portion of the HRESULT
 *
 * @param[in] hr the HRESULT containing the required facility code
 * @return the facility code
 */
#define HRESULT_FACILITY(hr)  (((hr) >> 16) & 0x1fff)

/** @brief Extracts the severity code portion of the HRESULT
 *
 * @param[in] hr the HRESULT containing the required severity code
 * @return the severity code
 */
#define HRESULT_SEVERITY(hr)  (((hr) >> 31) & 0x1)
 
/** @brief Tests the severity bit of the SCODE or HRESULT
 *
 * Tests the severity bit of the SCODE or HRESULT.
 * Returns TRUE if the severity is zero ie. if the code
 * represents success, and FALSE if it is one
 *
 * @param[in] hr the HRESULT to test
 * @return TRUE if the severity is zero and FALSE if it is one
 */
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)

/* Tests the severity bit of the SCODE or HRESULT; returns TRUE if the severity is one and FALSE if it is zero */
/** @brief Tests the severity bit of the SCODE or HRESULT
 *
 * Tests the severity bit of the SCODE or HRESULT.
 * Returns TRUE if the severity is one ie. if the code
 * represents failure, and FALSE if it is zero
 *
 * @param[in] hr the HRESULT to test
 * @return TRUE if the severity is one and FALSE if it is zero
 */
#define FAILED(hr) (((HRESULT)(hr)) < 0)

/** @brief Return an HRESULT using the given error code
 * 
 * This is an alternative to the function HRESULT_FROM_WIN32().
 * It maps the provided error code into an HRESULT, with a
 * severity of SEVERITY_ERROR, and a facility of FACILITY_WIN32.
 * 
 * The function version prevents a double evaluation of 'x'. This
 * version is present if you prefer to use the macro.
 *
 * @param[in] x error code
 * @return HRESULT for the given error code
 */ 
#define __HRESULT_FROM_WIN32(x) ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)))

HRESULT HRESULT_FROM_WIN32(DWORD x);

/*
 * Some predefined error codes (HRESULTs)
 */

/** @brief Predefined HRESULT for E_PENDING, FACILITY_NULL; "The data necessary to complete this operation is not yet available" */
#define E_PENDING       0x8000000A

/** @brief Predefined HRESULT for E_NOTIMPL, FACILITY_NULL; "Not implemented" */
#define E_NOTIMPL       0x80004001
/** @brief Predefined HRESULT for E_NOINTERFACE, FACILITY_NULL; "No such interface supported" */
#define E_NOINTERFACE   0x80004002
/** @brief Predefined HRESULT for E_POINTER, FACILITY_NULL; "Invalid pointer" */
#define E_POINTER       0x80004003
/** @brief Predefined HRESULT for E_ABORT, FACILITY_NULL; "Operation aborted" */
#define E_ABORT         0x80004004
/** @brief Predefined HRESULT for E_FAIL, FACILITY_NULL; "Unspecified error" */
#define E_FAIL          0x80004005
/** @brief Predefined HRESULT for E_UNEXPECTED, FACILITY_NULL; "Catastrophic failure" */
#define E_UNEXPECTED    0x8000FFFF

/** @brief Predefined HRESULT for ERROR_ACCESS_DENIED, FACILITY_WIN32 */
#define E_ACCESSDENIED  0x80070005
/** @brief Predefined HRESULT for ERROR_INVALID_HANDLE, FACILITY_WIN32 */
#define E_HANDLE        0x80070006
/** @brief Predefined HRESULT for ERROR_OUTOFMEMORY, FACILITY_WIN32 */
#define E_OUTOFMEMORY   0x8007000E
/** @brief Predefined HRESULT for ERROR_INVALID_PARAMETER, FACILITY_WIN32 */
#define E_INVALIDARG    0x80070057

/** @brief Predefined HRESULT for S_FALSE, FACILITY_NULL; "No error information available" */
#define S_FALSE         0x00000001
/** @brief Predefined HRESULT for S_OK, FACILITY_NULL; "Success" */
#define S_OK            0x00000000

/**@}*/
/**@}*/

/*
 * System Errors
 */

#include <synce_sys_error.h>

/*
 * Other macros
 */
/** 
 * @addtogroup SynceTypes
 * @{ 
 */ 

#define MAX_PATH  260

#ifndef FALSE
#define FALSE false
#endif

#ifndef TRUE
#define TRUE true
#endif

/** @} */

#endif

