cdef extern from "stddef.h":
    ctypedef unsigned int       size_t

cdef extern from "stdint.h":
    ctypedef unsigned char      uint8_t
    ctypedef short int          int16_t
    ctypedef unsigned short int uint16_t
    ctypedef int                int32_t
    # FIXME: this isn't portable
    ctypedef unsigned long      uint32_t
    ctypedef long int           int64_t

cdef extern from "synce.h":
    ctypedef void               VOID

    ctypedef uint8_t            BYTE
    ctypedef BYTE               BOOLEAN

    ctypedef int16_t            CSHORT

    ctypedef uint16_t           WORD
    ctypedef uint16_t           USHORT
    ctypedef uint16_t           WCHAR
    ctypedef WCHAR              TCHAR

    ctypedef int32_t            LONG
    ctypedef int32_t            HKEY
    ctypedef int32_t            REGSAM

    ctypedef uint32_t           DWORD
    ctypedef uint32_t           UINT
    ctypedef uint32_t           ULONG
    ctypedef uint32_t           HWND
    ctypedef uint32_t           BOOL

    ctypedef int64_t            LONGLONG

    # XXX: sizeof(double) must be 8 */
    ctypedef double             DATE

    ctypedef void*              LPVOID
    ctypedef char*              LPSTR
    ctypedef BYTE*              LPBYTE
    ctypedef WORD*              LPWORD
    ctypedef WCHAR*             LPWSTR
    ctypedef HKEY*              PHKEY
    ctypedef DWORD*             LPDWORD
    ctypedef LONG*              PLONG

    ctypedef void*              LPCVOID
    ctypedef char*              LPCSTR
    ctypedef WCHAR*             LPCWSTR

    ctypedef int32_t            HRESULT

cdef extern from "rapi.h":
    ctypedef void*              LPSECURITY_ATTRIBUTES

