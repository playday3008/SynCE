/* $Id: rapi_api.c 3812 2009-07-19 18:54:50Z mark_ellis $ */
#undef __STRICT_ANSI__
#define _GNU_SOURCE
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "rapi_ops.h"
#include "rapi_context.h"
#include "rapi2.h"

#include <string.h>
#include <dbus/dbus-glib.h>

#if ENABLE_HAL_SUPPORT
#include <dbus/dbus-glib-lowlevel.h>
#include <libhal.h>
#endif

#define DBUS_SERVICE "org.freedesktop.DBus"
#define DBUS_IFACE   "org.freedesktop.DBus"
#define DBUS_PATH    "/org/freedesktop/DBus"

#if ENABLE_HAL_SUPPORT
#define HAL_SERVICE   "org.freedesktop.Hal"
#define HAL_MGR_PATH  "/org/freedesktop/Hal/Manager"
#define HAL_MGR_IFACE "org.freedesktop.Hal.Manager"
#endif

#if ENABLE_UDEV_SUPPORT
#define DCCM_SERVICE   "org.synce.dccm"
#define DCCM_MGR_PATH  "/org/synce/dccm/DeviceManager"
#define DCCM_MGR_IFACE "org.synce.dccm.DeviceManager"
#define DCCM_DEV_IFACE "org.synce.dccm.Device"
#endif

/* we only need one instance of IRAPIDesktop */
static IRAPIDesktop *irapi_desktop = NULL;


/*
 * IRAPISession
 */

struct _IRAPISession {
        IRAPIDevice *device;
        RapiContext *context;
        int refcount;
};


void
IRAPISession_AddRef(IRAPISession *session)
{
        session->refcount++;
        return;
}

void
IRAPISession_Release(IRAPISession *session)
{
        session->refcount--;
        if (session->refcount > 0)
                return;

        IRAPIDevice_Release(session->device);
        rapi_context_unref(session->context);
        free(session);
        return;
}

/*
 * Standard rapi-calls valid for PocketPC 2002/2003 and Windows Mobile 5
 */
HRESULT
IRAPISession_CeRapiFreeBuffer(IRAPISession *session,
                              LPVOID Buffer)
{
        free(Buffer);
        return S_OK;
}

HRESULT
IRAPISession_CeRapiInit(IRAPISession *session)
{
        RapiContext * context = session->context;
        return rapi_context_connect(context);
}

HRESULT
IRAPISession_CeRapiUninit(IRAPISession *session)
{
        RapiContext * context = session->context;
        return rapi_context_disconnect(context);
}

HRESULT
IRAPISession_CeRapiGetError(IRAPISession *session)
{
        RapiContext * context = session->context;
        return context->rapi_error;
}

DWORD
IRAPISession_CeGetLastError(IRAPISession *session)
{
        RapiContext* context = session->context;
        return context->last_error;
}


/*
 * Implementation of calls that differ on WM5 and pre-WM5
 * devices, requires indirect calls to the correct function
 */

#ifndef SWIG

BOOL
IRAPISession_CeCloseHandle(IRAPISession *session,
                           HANDLE hObject)
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeCloseHandle ) ( context, hObject );
}

HANDLE
IRAPISession_CeCreateFile(IRAPISession *session,
                          LPCWSTR lpFileName,
                          DWORD dwDesiredAccess,
                          DWORD dwShareMode,
                          LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                          DWORD dwCreationDisposition,
                          DWORD dwFlagsAndAttributes,
                          HANDLE hTemplateFile )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return INVALID_HANDLE_VALUE;
        }

        return ( *context->rapi_ops->CeCreateFile ) (context,
                                                     lpFileName,
                                                     dwDesiredAccess,
                                                     dwShareMode,
                                                     lpSecurityAttributes,
                                                     dwCreationDisposition,
                                                     dwFlagsAndAttributes,
                                                     hTemplateFile );
}


BOOL
IRAPISession_CeReadFile(IRAPISession *session,
                        HANDLE hFile,
                        LPVOID lpBuffer,
                        DWORD nNumberOfBytesToRead,
                        LPDWORD lpNumberOfBytesRead,
                        LPOVERLAPPED lpOverlapped )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeReadFile ) (context,
                                                   hFile,
                                                   lpBuffer,
                                                   nNumberOfBytesToRead,
                                                   lpNumberOfBytesRead,
                                                   lpOverlapped );
}


BOOL
IRAPISession_CeWriteFile(IRAPISession *session,
                         HANDLE hFile,
                         LPCVOID lpBuffer,
                         DWORD nNumberOfBytesToWrite,
                         LPDWORD lpNumberOfBytesWritten,
                         LPOVERLAPPED lpOverlapped )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeWriteFile ) (context,
                                                    hFile,
                                                    lpBuffer,
                                                    nNumberOfBytesToWrite,
                                                    lpNumberOfBytesWritten,
                                                    lpOverlapped );
}


DWORD
IRAPISession_CeSetFilePointer(IRAPISession *session,
                              HANDLE hFile,
                              LONG lDistanceToMove,
                              PLONG lpDistanceToMoveHigh,
                              DWORD dwMoveMethod )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return 0xFFFFFFFF;
        }

        return ( *context->rapi_ops->CeSetFilePointer ) (
                                                         context,
                                                         hFile,
                                                         lDistanceToMove,
                                                         lpDistanceToMoveHigh,
                                                         dwMoveMethod );
}

BOOL
IRAPISession_CeSetEndOfFile(IRAPISession *session,
                            HANDLE hFile)
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeSetEndOfFile ) ( context, hFile );
}

BOOL
IRAPISession_CeGetFileTime(IRAPISession *session,
                           HANDLE hFile,
                           LPFILETIME lpCreationTime,
                           LPFILETIME lpLastAccessTime,
                           LPFILETIME lpLastWriteTime )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeGetFileTime ) (context,
                                                      hFile,
                                                      lpCreationTime,
                                                      lpLastAccessTime,
                                                      lpLastWriteTime );
}

BOOL
IRAPISession_CeSetFileTime(IRAPISession *session,
                           HANDLE hFile,
                           LPFILETIME lpCreationTime,
                           LPFILETIME lpLastAccessTime,
                           LPFILETIME lpLastWriteTime )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeSetFileTime ) (context,
                                                      hFile,
                                                      lpCreationTime,
                                                      lpLastAccessTime,
                                                      lpLastWriteTime );
}

#endif /* SWIG */


/*
* File management functions
*/

#ifndef SWIG

BOOL
IRAPISession_CeCopyFile(IRAPISession *session,
                        LPCWSTR lpExistingFileName,
                        LPCWSTR lpNewFileName,
                        BOOL bFailIfExists )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeCopyFile ) (context,
                                                   lpExistingFileName,
                                                   lpNewFileName,
                                                   bFailIfExists );
}


BOOL
IRAPISession_CeCreateDirectory(IRAPISession *session,
                               LPCWSTR lpPathName,
                               LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeCreateDirectory ) (context,
                                                          lpPathName,
                                                          lpSecurityAttributes );
}


BOOL
IRAPISession_CeDeleteFile(IRAPISession *session,
                          LPCWSTR lpFileName )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeDeleteFile ) (context,
                                                     lpFileName );
}


BOOL
IRAPISession_CeFindAllFiles(IRAPISession *session,
                            LPCWSTR szPath,
                            DWORD dwFlags,
                            LPDWORD lpdwFoundCount,
                            LPLPCE_FIND_DATA ppFindDataArray )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeFindAllFiles ) (context,
                                                       szPath,
                                                       dwFlags,
                                                       lpdwFoundCount,
                                                       ppFindDataArray );
}


HANDLE
IRAPISession_CeFindFirstFile(IRAPISession *session,
                             LPCWSTR lpFileName,
                             LPCE_FIND_DATA lpFindFileData )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return INVALID_HANDLE_VALUE;
        }

        return ( *context->rapi_ops->CeFindFirstFile ) (context,
                                                        lpFileName,
                                                        lpFindFileData );
}


BOOL
IRAPISession_CeFindNextFile(IRAPISession *session,
                            HANDLE hFindFile,
                            LPCE_FIND_DATA lpFindFileData )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeFindNextFile ) (context,
                                                       hFindFile,
                                                       lpFindFileData );
}


BOOL
IRAPISession_CeFindClose(IRAPISession *session,
                         HANDLE hFindFile )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeFindClose ) ( context, hFindFile );
}


DWORD
IRAPISession_CeGetFileAttributes(IRAPISession *session,
                                 LPCWSTR lpFileName )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return 0xFFFFFFFF;
        }

        return ( *context->rapi_ops->CeGetFileAttributes ) ( context, lpFileName );
}


DWORD
IRAPISession_CeGetFileSize(IRAPISession *session,
                           HANDLE hFile,
                           LPDWORD lpFileSizeHigh )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return INVALID_FILE_SIZE;
        }

        return ( *context->rapi_ops->CeGetFileSize ) (context,
                                                      hFile,
                                                      lpFileSizeHigh );
}


DWORD
IRAPISession_CeGetSpecialFolderPath(IRAPISession *session,
                                    int nFolder,
                                    DWORD nBufferLength,
                                    LPWSTR lpBuffer )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return 0;
        }

        return ( *context->rapi_ops->CeGetSpecialFolderPath ) (context,
                                                               nFolder,
                                                               nBufferLength,
                                                               lpBuffer );
}


BOOL
IRAPISession_CeMoveFile(IRAPISession *session,
                        LPCWSTR lpExistingFileName,
                        LPCWSTR lpNewFileName )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeMoveFile ) (context,
                                                   lpExistingFileName,
                                                   lpNewFileName );
}


BOOL
IRAPISession_CeRemoveDirectory(IRAPISession *session,
                               LPCWSTR lpPathName )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeRemoveDirectory ) ( context, lpPathName );
}


BOOL
IRAPISession_CeSetFileAttributes(IRAPISession *session,
                                 LPCWSTR lpFileName,
                                 DWORD dwFileAttributes )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeSetFileAttributes ) (context,
                                                            lpFileName,
                                                            dwFileAttributes );
}


BOOL
IRAPISession_CeSHCreateShortcut(IRAPISession *session,
                                LPCWSTR lpszShortcut,
                                LPCWSTR lpszTarget )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeSHCreateShortcut ) (context,
                                                           lpszShortcut,
                                                           lpszTarget );
}


BOOL
IRAPISession_CeSyncTimeToPc(IRAPISession *session)
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeSyncTimeToPc ) ( context );
}


#endif /* SWIG */


/*
* Database functions
*/

#ifndef SWIG

CEOID
IRAPISession_CeCreateDatabase(IRAPISession *session,
                              LPWSTR lpszName,
                              DWORD dwDbaseType,
                              WORD wNumSortOrder,
                              SORTORDERSPEC *rgSortSpecs )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return 0;
        }

        return ( *context->rapi_ops->CeCreateDatabase ) (context,
                                                         lpszName,
                                                         dwDbaseType,
                                                         wNumSortOrder,
                                                         rgSortSpecs );
}


BOOL
IRAPISession_CeDeleteDatabase(IRAPISession *session, CEOID oid )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeDeleteDatabase ) ( context, oid );
}


BOOL
IRAPISession_CeFindAllDatabases(IRAPISession *session,
                                DWORD dwDbaseType,
                                WORD wFlags,
                                LPWORD cFindData,
                                LPLPCEDB_FIND_DATA ppFindData )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeFindAllDatabases ) (context,
                                                           dwDbaseType,
                                                           wFlags,
                                                           cFindData,
                                                           ppFindData );
}


HANDLE
IRAPISession_CeFindFirstDatabase(IRAPISession *session,
                                 DWORD dwDbaseType )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return INVALID_HANDLE_VALUE;
        }

        return ( *context->rapi_ops->CeFindFirstDatabase ) (context,
                                                            dwDbaseType );
}


CEOID
IRAPISession_CeFindNextDatabase(IRAPISession *session,
                                HANDLE hEnum )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return 0;
        }

        return ( *context->rapi_ops->CeFindNextDatabase ) (context,
                                                           hEnum );
}


HANDLE
IRAPISession_CeOpenDatabase(IRAPISession *session,
                            PCEOID poid,
                            LPWSTR lpszName,
                            CEPROPID propid,
                            DWORD dwFlags,
                            HWND hwndNotify )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return INVALID_HANDLE_VALUE;
        }

        return ( *context->rapi_ops->CeOpenDatabase ) (context,
                                                       poid,
                                                       lpszName,
                                                       propid,
                                                       dwFlags,
                                                       hwndNotify );
}


CEOID
IRAPISession_CeReadRecordProps(IRAPISession *session,
                               HANDLE hDbase,
                               DWORD dwFlags,
                               LPWORD lpcPropID,
                               CEPROPID *rgPropID,
                               LPBYTE *lplpBuffer,
                               LPDWORD lpcbBuffer )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return 0;
        }

        return ( *context->rapi_ops->CeReadRecordProps ) (context,
                                                          hDbase,
                                                          dwFlags,
                                                          lpcPropID,
                                                          rgPropID,
                                                          lplpBuffer,
                                                          lpcbBuffer );
}


CEOID
IRAPISession_CeSeekDatabase(IRAPISession *session,
                            HANDLE hDatabase,
                            DWORD dwSeekType,
                            DWORD dwValue,
                            LPDWORD lpdwIndex )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return 0;
        }

        return ( *context->rapi_ops->CeSeekDatabase ) (context,
                                                       hDatabase,
                                                       dwSeekType,
                                                       dwValue,
                                                       lpdwIndex );
}


CEOID
IRAPISession_CeWriteRecordProps(IRAPISession *session,
                                HANDLE hDbase,
                                CEOID oidRecord,
                                WORD cPropID,
                                CEPROPVAL *rgPropVal )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return 0;
        }

        return ( *context->rapi_ops->CeWriteRecordProps ) (context,
                                                           hDbase,
                                                           oidRecord,
                                                           cPropID,
                                                           rgPropVal );
}


BOOL
IRAPISession_CeDeleteRecord(IRAPISession *session,
                            HANDLE hDatabase,
                            CEOID oidRecord )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeDeleteRecord ) (context,
                                                       hDatabase, oidRecord );
}


BOOL
IRAPISession_CeSetDatabaseInfo(IRAPISession *session,
                               CEOID oidDbase,
                               CEDBASEINFO* pNewInfo )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeSetDatabaseInfo ) (context,
                                                          oidDbase,
                                                          pNewInfo );
}


#endif /* SWIG */

/*
* Registry
*/

#ifndef SWIG

LONG
IRAPISession_CeRegCreateKeyEx(IRAPISession *session,
                              HKEY hKey,
                              LPCWSTR lpszSubKey,
                              DWORD Reserved,
                              LPWSTR lpszClass,
                              DWORD ulOptions,
                              REGSAM samDesired,
                              LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                              PHKEY phkResult,
                              LPDWORD lpdwDisposition )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return ERROR_NOT_CONNECTED;
        }

        return ( *context->rapi_ops->CeRegCreateKeyEx ) (context,
                                                         hKey,
                                                         lpszSubKey,
                                                         Reserved,
                                                         lpszClass,
                                                         ulOptions,
                                                         samDesired,
                                                         lpSecurityAttributes,
                                                         phkResult,
                                                         lpdwDisposition );
}


LONG
IRAPISession_CeRegOpenKeyEx(IRAPISession *session,
                            HKEY hKey,
                            LPCWSTR lpszSubKey,
                            DWORD ulOptions,
                            REGSAM samDesired,
                            PHKEY phkResult )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return ERROR_NOT_CONNECTED;
        }

        return ( *context->rapi_ops->CeRegOpenKeyEx ) (context,
                                                       hKey,
                                                       lpszSubKey,
                                                       ulOptions,
                                                       samDesired,
                                                       phkResult );
}


LONG
IRAPISession_CeRegCloseKey(IRAPISession *session,
                           HKEY hKey )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return ERROR_NOT_CONNECTED;
        }

        return ( *context->rapi_ops->CeRegCloseKey ) (context,
                                                      hKey );
}


LONG
IRAPISession_CeRegDeleteKey(IRAPISession *session,
                            HKEY hKey,
                            LPCWSTR lpszSubKey )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return ERROR_NOT_CONNECTED;
        }

        return ( *context->rapi_ops->CeRegDeleteKey ) (context,
                                                       hKey,
                                                       lpszSubKey);
}


LONG
IRAPISession_CeRegDeleteValue(IRAPISession *session,
                              HKEY hKey,
                              LPCWSTR lpszValueName )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return ERROR_NOT_CONNECTED;
        }

        return ( *context->rapi_ops->CeRegDeleteValue ) (context,
                                                         hKey,
                                                         lpszValueName);
}


DWORD
IRAPISession_CeGetDiskFreeSpaceEx(IRAPISession *session,
                                  LPCTSTR _lpDirectoryName, 
                                  PULARGE_INTEGER lpFreeBytesAvailable, 
                                  PULARGE_INTEGER lpTotalNumberOfBytes, 
                                  PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeGetDiskFreeSpaceEx ) (context,
                                                             _lpDirectoryName,
                                                             lpFreeBytesAvailable,
                                                             lpTotalNumberOfBytes,
                                                             lpTotalNumberOfFreeBytes);
}


LONG
IRAPISession_CeRegQueryInfoKey(IRAPISession *session,
                               HKEY hKey,
                               LPWSTR lpClass,
                               LPDWORD lpcbClass,
                               LPDWORD lpReserved,
                               LPDWORD lpcSubKeys,
                               LPDWORD lpcbMaxSubKeyLen,
                               LPDWORD lpcbMaxClassLen,
                               LPDWORD lpcValues,
                               LPDWORD lpcbMaxValueNameLen,
                               LPDWORD lpcbMaxValueLen,
                               LPDWORD lpcbSecurityDescriptor,
                               PFILETIME lpftLastWriteTime )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return ERROR_NOT_CONNECTED;
        }

        return ( *context->rapi_ops->CeRegQueryInfoKey ) (context,
                                                          hKey,
                                                          lpClass,
                                                          lpcbClass,
                                                          lpReserved,
                                                          lpcSubKeys,
                                                          lpcbMaxSubKeyLen,
                                                          lpcbMaxClassLen,
                                                          lpcValues,
                                                          lpcbMaxValueNameLen,
                                                          lpcbMaxValueLen,
                                                          lpcbSecurityDescriptor,
                                                          lpftLastWriteTime );
}


LONG
IRAPISession_CeRegQueryValueEx(IRAPISession *session,
                               HKEY hKey,
                               LPCWSTR lpValueName,
                               LPDWORD lpReserved,
                               LPDWORD lpType,
                               LPBYTE lpData,
                               LPDWORD lpcbData )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return ERROR_NOT_CONNECTED;
        }

        return ( *context->rapi_ops->CeRegQueryValueEx ) (context,
                                                          hKey,
                                                          lpValueName,
                                                          lpReserved,
                                                          lpType,
                                                          lpData,
                                                          lpcbData );
}


LONG
IRAPISession_CeRegEnumValue(IRAPISession *session,
                            HKEY hKey,
                            DWORD dwIndex,
                            LPWSTR lpszValueName,
                            LPDWORD lpcbValueName,
                            LPDWORD lpReserved,
                            LPDWORD lpType,
                            LPBYTE lpData,
                            LPDWORD lpcbData )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return ERROR_NOT_CONNECTED;
        }

        return ( *context->rapi_ops->CeRegEnumValue ) (context,
                                                       hKey,
                                                       dwIndex,
                                                       lpszValueName,
                                                       lpcbValueName,
                                                       lpReserved,
                                                       lpType,
                                                       lpData,
                                                       lpcbData );
}


LONG
IRAPISession_CeRegEnumKeyEx(IRAPISession *session,
                            HKEY hKey,
                            DWORD dwIndex,
                            LPWSTR lpName,
                            LPDWORD lpcbName,
                            LPDWORD lpReserved,
                            LPWSTR lpClass,
                            LPDWORD lpcbClass,
                            PFILETIME lpftLastWriteTime )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return ERROR_NOT_CONNECTED;
        }

        return ( *context->rapi_ops->CeRegEnumKeyEx ) (context,
                                                       hKey,
                                                       dwIndex,
                                                       lpName,
                                                       lpcbName,
                                                       lpReserved,
                                                       lpClass,
                                                       lpcbClass,
                                                       lpftLastWriteTime );
}


LONG
IRAPISession_CeRegSetValueEx(IRAPISession *session,
                             HKEY hKey,
                             LPCWSTR lpValueName,
                             DWORD Reserved,
                             DWORD dwType,
                             const BYTE *lpData,
                             DWORD cbData )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return ERROR_NOT_CONNECTED;
        }

        return ( *context->rapi_ops->CeRegSetValueEx ) (context,
                                                        hKey,
                                                        lpValueName,
                                                        Reserved,
                                                        dwType,
                                                        lpData,
                                                        cbData );
}

#endif /* SWIG */

/*
* Misc functions
*/

#ifndef SWIG

BOOL
IRAPISession_CeCheckPassword(IRAPISession *session,
                             LPWSTR lpszPassword )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeCheckPassword ) ( context, lpszPassword );
}


BOOL
IRAPISession_CeCreateProcess(IRAPISession *session,
                             LPCWSTR lpApplicationName,
                             LPCWSTR lpCommandLine,
                             void* lpProcessAttributes,
                             void* lpThreadAttributes,
                             BOOL bInheritHandles,
                             DWORD dwCreationFlags,
                             LPVOID lpEnvironment,
                             LPWSTR lpCurrentDirectory,
                             void* lpStartupInfo,
                             LPPROCESS_INFORMATION lpProcessInformation )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeCreateProcess ) (context,
                                                        lpApplicationName,
                                                        lpCommandLine,
                                                        lpProcessAttributes,
                                                        lpThreadAttributes,
                                                        bInheritHandles,
                                                        dwCreationFlags,
                                                        lpEnvironment,
                                                        lpCurrentDirectory,
                                                        lpStartupInfo,
                                                        lpProcessInformation );
}


BOOL
IRAPISession_CeGetStoreInformation(IRAPISession *session,
                                   LPSTORE_INFORMATION lpsi )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeGetStoreInformation ) ( context, lpsi );
}


void
IRAPISession_CeGetSystemInfo(IRAPISession *session,
                             LPSYSTEM_INFO lpSystemInfo )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return;
        }

        return ( *context->rapi_ops->CeGetSystemInfo ) (context,
                                                        lpSystemInfo );
}


BOOL
IRAPISession_CeGetSystemPowerStatusEx(IRAPISession *session,
                                      PSYSTEM_POWER_STATUS_EX pSystemPowerStatus,
                                      BOOL refresh )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeGetSystemPowerStatusEx ) ( context, pSystemPowerStatus, refresh );
}


BOOL
IRAPISession_CeGetVersionEx(IRAPISession *session,
                            LPCEOSVERSIONINFO lpVersionInformation )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeGetVersionEx ) (context,
                                                       lpVersionInformation );
}


BOOL
IRAPISession_CeOidGetInfo(IRAPISession *session,
                          CEOID oid,
                          CEOIDINFO *poidInfo )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeOidGetInfo ) (context,
                                                     oid,
                                                     poidInfo );
}


HRESULT
IRAPISession_CeProcessConfig(IRAPISession *session,
                             LPCWSTR config,
                             DWORD flags,
                             LPWSTR *reply )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return E_UNEXPECTED;
        }

        return ( *context->rapi_ops->CeProcessConfig ) ( context, config, flags, reply );
}


BOOL
IRAPISession_CeStartReplication(IRAPISession *session)
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeStartReplication ) ( context );
}


HRESULT
IRAPISession_CeSyncStart(IRAPISession *session, LPCWSTR params )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return E_UNEXPECTED;
        }

        return ( *context->rapi_ops->CeSyncStart ) ( context, params );
}


HRESULT
IRAPISession_CeSyncResume(IRAPISession *session)
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return E_UNEXPECTED;
        }

        return ( *context->rapi_ops->CeSyncResume ) ( context );
}


HRESULT
IRAPISession_CeSyncPause(IRAPISession *session)
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return E_UNEXPECTED;
        }

        return ( *context->rapi_ops->CeSyncPause ) ( context );
}


BOOL
IRAPISession_CeGetSystemMemoryDivision(IRAPISession *session,
                                       LPDWORD lpdwStoragePages,
                                       LPDWORD lpdwRamPages,
                                       LPDWORD lpdwPageSize )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeGetSystemMemoryDivision ) (context,
                                                                  lpdwStoragePages,
                                                                  lpdwRamPages,
                                                                  lpdwPageSize );
}


DWORD
IRAPISession_CeSetSystemMemoryDivision(IRAPISession *session,
                                       DWORD dwStoragePages )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return 3;
        }

        return ( *context->rapi_ops->CeSetSystemMemoryDivision ) ( context, dwStoragePages );
}


BOOL
IRAPISession_CeRegCopyFile(IRAPISession *session,
                           LPCWSTR filename )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeRegCopyFile ) ( context, filename );
}


BOOL
IRAPISession_CeRegRestoreFile(IRAPISession *session,
                              LPCWSTR filename )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeRegRestoreFile ) ( context, filename );
}


BOOL
IRAPISession_CeKillAllApps(IRAPISession *session)
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                context->rapi_error = S_OK;
                context->last_error = ERROR_NOT_CONNECTED;
                return false;
        }

        return ( *context->rapi_ops->CeKillAllApps ) ( context );
}


#endif /* SWIG */

/*
* CeRapiInvoke stuff
*/

#ifndef SWIG

HRESULT
IRAPISession_CeRapiInvoke(IRAPISession *session,
                          LPCWSTR pDllPath,
                          LPCWSTR pFunctionName,
                          DWORD cbInput,
                          const BYTE *pInput,
                          DWORD *pcbOutput,
                          BYTE **ppOutput,
                          IRAPIStream **ppIRAPIStream,
                          DWORD dwReserved )
{
        RapiContext * context = session->context;
        if (!context->is_initialized) {
                return E_UNEXPECTED;
        }

        return ( *context->rapi_ops->CeRapiInvoke ) (context,
                                                     pDllPath,
                                                     pFunctionName,
                                                     cbInput,
                                                     pInput,
                                                     pcbOutput,
                                                     ppOutput,
                                                     ppIRAPIStream,
                                                     dwReserved );
}

#endif /* SWIG */



/*
 * IRAPIDevice
 */

struct _IRAPIDevice {
        char *obj_path;
        SynceInfo *info;
        int refcount;
        RAPI_DEVICESTATUS status;
};


void
IRAPIDevice_AddRef(IRAPIDevice *self)
{
        self->refcount++;
        return;
}

void
IRAPIDevice_Release(IRAPIDevice *self)
{
        self->refcount--;
        if (self->refcount > 0)
                return;

        free(self->obj_path);
        if (self->info)
                synce_info_destroy(self->info);
        free(self);
        return;
}


HRESULT
IRAPIDevice_CreateSession(IRAPIDevice *self, IRAPISession** ppISession)
{
        IRAPISession *session = calloc(1, sizeof(IRAPISession));
        if (!session)
                return E_OUTOFMEMORY;

        session->context = rapi_context_new();

        if (!session->context) {
                synce_error("Failed to create RapiContext object");
                free(session);
                return E_FAIL;
        }

        session->context->info = self->info;
        IRAPIDevice_AddRef(self);
        session->device = self;
        session->refcount = 1;

        *ppISession = session;
        return S_OK;
}


HRESULT
IRAPIDevice_GetConnectionInfo(IRAPIDevice *self, RAPI_CONNECTIONINFO* pConnInfo)
{
        return E_NOTIMPL;
}

HRESULT
IRAPIDevice_GetConnectStat(IRAPIDevice *self, RAPI_DEVICESTATUS* pStat)
{
        RAPI_DEVICESTATUS *status = NULL;
        status = calloc(1, sizeof(RAPI_DEVICESTATUS));
        if (!status)
                return E_OUTOFMEMORY;

        *status = self->status;
        pStat = status;

        return S_OK;
}

HRESULT
IRAPIDevice_GetDeviceInfo(IRAPIDevice *self, RAPI_DEVICEINFO* pDevInfo)
{
        return E_NOTIMPL;
}



/*
 * IRAPIEnumDevices
 */

struct _IRAPIEnumDevices {
        GList *devices;
        unsigned count;
        GList *current;
        int refcount;
};


void
IRAPIEnumDevices_AddRef(IRAPIEnumDevices *self)
{
        self->refcount++;
        return;
}

void
IRAPIEnumDevices_Release(IRAPIEnumDevices *self)
{
        self->refcount--;
        if (self->refcount > 0)
                return;

        GList *iter = self->devices;
        while (iter) {
                IRAPIDevice_Release(iter->data);
                iter = g_list_next(iter);
        }
        g_list_free(self->devices);

        free(self);
        return;
}

HRESULT
IRAPIEnumDevices_Clone(IRAPIEnumDevices *self, IRAPIEnumDevices** ppIEnum)
{
        IRAPIEnumDevices *new_enum;
        new_enum = calloc(1, sizeof(IRAPIEnumDevices));
        if (!new_enum)
                return E_OUTOFMEMORY;
        memset(new_enum, 0, sizeof(IRAPIEnumDevices));
        new_enum->refcount = 1;

        GList *device = self->devices;
        while (device) {
                IRAPIDevice_AddRef(device->data);
                new_enum->devices = g_list_append(new_enum->devices, device->data);
                new_enum->count++;

                device = g_list_next(device);
        }


        new_enum->current = new_enum->devices;

        *ppIEnum = new_enum;
        return S_OK;
}

HRESULT
IRAPIEnumDevices_GetCount(IRAPIEnumDevices *self, ULONG* pcElt)
{
        *pcElt = self->count;
        return S_OK;
}

HRESULT
IRAPIEnumDevices_Next(IRAPIEnumDevices *self, IRAPIDevice** ppIDevice)
{
        if (!(self->current))
                return MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_NO_MORE_DEVICES);

        *ppIDevice = self->current->data;

        self->current = g_list_next(self->current);

        return S_OK;
}

HRESULT
IRAPIEnumDevices_Reset(IRAPIEnumDevices *self)
{
        self->current = self->devices;
        return S_OK;
}

HRESULT
IRAPIEnumDevices_Skip(IRAPIEnumDevices *self, ULONG cElt)
{
        uint i;
        for (i = 0; i < cElt; i++) {
                if (!(self->current))
                        return MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,ERROR_NO_MORE_DEVICES);
                self->current = g_list_next(self->current);
        }

        return S_OK;
}



/*
 * IRAPIDesktop
*/

struct _IRAPIDesktop {
        int refcount;

        /* hal */
        DBusGConnection *dbus_connection;
        DBusGProxy *dbus_proxy;
#if ENABLE_HAL_SUPPORT
        LibHalContext *hal_ctx;
#endif
#if ENABLE_UDEV_SUPPORT
        DBusGProxy *dev_mgr_proxy;
#endif

        /* list of device udi's ?*/
        GList *devices;

#if 0   /* sinks to notify */
        GList *sinks;
#endif
};

#if 0
static void
IRAPIDesktop_device_connected_cb()
{
        /*
            add device data to list
            for all sinks
            call sink->OnDeviceConnected
        */
}

static void
IRAPIDesktop_device_disconnected_cb()
{
        /*
        for all sinks
          call sink->OnDeviceDisconnected
        remove device data from list
        */
}
#endif

#if ENABLE_HAL_SUPPORT
static void
hal_device_connected_cb(LibHalContext *ctx, const char *udi)
{
        IRAPIDesktop *self = libhal_ctx_get_user_data(ctx);

        synce_debug("found device: %s", udi);


        IRAPIDevice *newdev = calloc(1, sizeof(IRAPIDevice));
        if (!newdev) {
                synce_error("failed to allocate IRAPIDevice");
                return;
        }

        newdev->obj_path = strdup(udi);
        newdev->info = synce_info_new_by_field(INFO_OBJECT_PATH, newdev->obj_path);
        newdev->status = RAPI_DEVICE_CONNECTED;
        newdev->refcount = 1;

        self->devices = g_list_append(self->devices, newdev);

        return;
}

static void
hal_device_disconnected_cb(LibHalContext *ctx, const char *udi)
{
        IRAPIDesktop *self = libhal_ctx_get_user_data(ctx);

        GList *device = self->devices;
        while (device) {
                if (strcmp(device->data, udi) == 0)
                        break;

                device = g_list_next(device);
        }

        if (!device)
                return;

        synce_debug("Received device disconnected from hal: %s", udi);

        ((IRAPIDevice*)device->data)->status = RAPI_DEVICE_DISCONNECTED;
        IRAPIDevice_Release(device->data);
        self->devices = g_list_delete_link(self->devices, device);

        return;
}


static void
hal_disconnect(IRAPIDesktop *self)
{
        DBusError dbus_error;
        dbus_error_init(&dbus_error);

        if (!libhal_ctx_shutdown(self->hal_ctx, &dbus_error)) {
                synce_error("Failed to shutdown hal context: %s: %s", dbus_error.name, dbus_error.message);
                dbus_error_free(&dbus_error);
        }

        libhal_ctx_free(self->hal_ctx);
        self->hal_ctx = NULL;

        GList *device = self->devices;
        while (device) {
                if (strncmp(((IRAPIDevice*)device->data)->obj_path, "/org/freedesktop/Hal/", 21) == 0) {
                        synce_debug("removing device %s", ((IRAPIDevice*)device->data)->obj_path);
                        ((IRAPIDevice*)device->data)->status = RAPI_DEVICE_DISCONNECTED;
                        IRAPIDevice_Release((IRAPIDevice*)device->data);
                        self->devices = g_list_delete_link(self->devices, device);
                        device = self->devices;
                        continue;
                }
                device = g_list_next(device);
        }
}

static void
hal_connect(IRAPIDesktop *self)
{
        DBusError dbus_error;
        gchar **dev_list = NULL;
        gint i, num_devices;
        gchar *udi = NULL;

        dbus_error_init(&dbus_error);

        if (!(self->hal_ctx = libhal_ctx_new())) {
                synce_error("Failed to get hal context");
                goto error_exit;
        }

        if (!libhal_ctx_set_dbus_connection(self->hal_ctx, dbus_g_connection_get_connection(self->dbus_connection))) {
                synce_error("Failed to set DBus connection for hal context");
                goto error_exit;
        }

        if (!libhal_ctx_set_user_data(self->hal_ctx, self)) {
                synce_error("Failed to set user data for hal context");
                goto error_exit;
        }

        if (!libhal_ctx_init(self->hal_ctx, &dbus_error)) {
                synce_error("Failed to initialise hal context: %s: %s", dbus_error.name, dbus_error.message);
                goto error_exit;
        }

        if (!libhal_ctx_set_device_added(self->hal_ctx, hal_device_connected_cb)) {
                synce_error("Failed to set hal device added callback");
                goto error_exit;
        }

        if (!libhal_ctx_set_device_removed(self->hal_ctx, hal_device_disconnected_cb)) {
                synce_error("Failed to set hal device removed callback");
                goto error_exit;
        }

#if 0 /* do we need this ? */
        if (!libhal_ctx_set_device_property_modified(self->hal_ctx, hal_device_password_status_changed_cb)) {
                synce_error("Failed to set hal device property modified callback");
                goto error_exit;
        }
#endif

        /* currently connected devices */

        dev_list = libhal_manager_find_device_string_match(self->hal_ctx,
                                                           "pda.platform",
                                                           "pocketpc",
                                                           &num_devices,
                                                           &dbus_error);
        if (dbus_error_is_set(&dbus_error)) {
                synce_error("Failed to obtain list of attached devices: %s: %s", dbus_error.name, dbus_error.message);
                dbus_error_free(&dbus_error);
        }

        for (i = 0; i < num_devices; i++) {
                udi = dev_list[i];
                synce_debug("found device: %s", udi);

                IRAPIDevice *newdev = calloc(1, sizeof(IRAPIDevice));
                if (!newdev) {
                        synce_error("failed to allocate IRAPIDevice");
                        break;
                }

                newdev->obj_path = strdup(udi);
                newdev->info = synce_info_new_by_field(INFO_OBJECT_PATH, newdev->obj_path);
                newdev->status = RAPI_DEVICE_CONNECTED;
                newdev->refcount = 1;

                self->devices = g_list_append(self->devices, newdev);

        }
        libhal_free_string_array(dev_list);

        return;

 error_exit:
        if (dbus_error_is_set(&dbus_error))
                dbus_error_free(&dbus_error);
        if (self->hal_ctx) {
                libhal_ctx_shutdown(self->hal_ctx, NULL);
                libhal_ctx_free(self->hal_ctx);
        }
        return;
}
#endif

#if ENABLE_UDEV_SUPPORT
static void
udev_device_connected_cb(DBusGProxy *proxy,
			 gchar *obj_path,
			 gpointer user_data)
{
        IRAPIDesktop *self = (IRAPIDesktop*)user_data;

        synce_debug("found device: %s", obj_path);

        IRAPIDevice *newdev = calloc(1, sizeof(IRAPIDevice));
        if (!newdev) {
                synce_error("failed to allocate IRAPIDevice");
                return;
        }

        newdev->obj_path = strdup(obj_path);
        newdev->info = synce_info_new_by_field(INFO_OBJECT_PATH, newdev->obj_path);
        newdev->status = RAPI_DEVICE_CONNECTED;
        newdev->refcount = 1;

        self->devices = g_list_append(self->devices, newdev);

        return;
}

static void
udev_device_disconnected_cb(DBusGProxy *proxy,
			    gchar *obj_path,
			    gpointer user_data)
{
        IRAPIDesktop *self = (IRAPIDesktop*)user_data;

        GList *device = self->devices;
        while (device) {
                if (strcmp(device->data, obj_path) == 0)
                        break;

                device = g_list_next(device);
        }

        if (!device)
                return;

        synce_debug("Received device disconnected from dccm: %s", obj_path);

        ((IRAPIDevice*)device->data)->status = RAPI_DEVICE_DISCONNECTED;
        IRAPIDevice_Release(device->data);
        self->devices = g_list_delete_link(self->devices, device);

        return;
}


static void
udev_disconnect(IRAPIDesktop *self)
{

        dbus_g_proxy_disconnect_signal(self->dev_mgr_proxy, "DeviceConnected",
				       G_CALLBACK(udev_device_connected_cb), self);

	dbus_g_proxy_disconnect_signal(self->dev_mgr_proxy, "DeviceDisconnected",
				       G_CALLBACK(udev_device_disconnected_cb), self);

	g_object_unref(self->dev_mgr_proxy);
	self->dev_mgr_proxy = NULL;

        GList *device = self->devices;
        while (device) {
                if (strncmp(((IRAPIDevice*)device->data)->obj_path, "/org/synce/dccm/", 16) == 0) {
                        synce_debug("removing device %s", ((IRAPIDevice*)device->data)->obj_path);
                        ((IRAPIDevice*)device->data)->status = RAPI_DEVICE_DISCONNECTED;
                        IRAPIDevice_Release((IRAPIDevice*)device->data);
                        self->devices = g_list_delete_link(self->devices, device);
                        device = self->devices;
                        continue;
                }
                device = g_list_next(device);
        }
}

static void
udev_connect(IRAPIDesktop *self)
{
        GError *error;
        GPtrArray *dev_list = NULL;
        guint i;
        gchar *obj_path = NULL;

	self->dev_mgr_proxy = dbus_g_proxy_new_for_name(self->dbus_connection,
							DCCM_SERVICE,
							DCCM_MGR_PATH,
							DCCM_MGR_IFACE);
	if (self->dev_mgr_proxy == NULL) {
                synce_error("Failed to create proxy to device manager");
		return;
	}

	dbus_g_proxy_add_signal(self->dev_mgr_proxy, "DeviceConnected",
				G_TYPE_STRING, G_TYPE_INVALID);

	dbus_g_proxy_add_signal(self->dev_mgr_proxy, "DeviceDisconnected",
				G_TYPE_STRING, G_TYPE_INVALID);

	dbus_g_proxy_connect_signal(self->dev_mgr_proxy, "DeviceConnected",
				    G_CALLBACK(udev_device_connected_cb), self, NULL);

	dbus_g_proxy_connect_signal(self->dev_mgr_proxy, "DeviceDisconnected",
				    G_CALLBACK(udev_device_disconnected_cb), self, NULL);

        /* currently connected devices */

	if (!(dbus_g_proxy_call(self->dev_mgr_proxy, "GetConnectedDevices",
				&error, G_TYPE_INVALID,
				dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_OBJECT_PATH),
				&dev_list,
				G_TYPE_INVALID))) {
	        synce_error("Error getting device list from dccm: %s", error->message);
		g_error_free(error);
		return;
	}

        for (i = 0; i < dev_list->len; i++) {
                obj_path = (gchar *)g_ptr_array_index(dev_list, i);
                synce_debug("found device: %s", obj_path);

                IRAPIDevice *newdev = calloc(1, sizeof(IRAPIDevice));
                if (!newdev) {
                        synce_error("failed to allocate IRAPIDevice");
                        break;
                }

                newdev->obj_path = obj_path;
                newdev->info = synce_info_new_by_field(INFO_OBJECT_PATH, newdev->obj_path);
                newdev->status = RAPI_DEVICE_CONNECTED;
                newdev->refcount = 1;

                self->devices = g_list_append(self->devices, newdev);

        }
	g_ptr_array_free(dev_list, TRUE);

        return;
}
#endif

static void
dbus_name_owner_changed_cb(DBusGProxy *proxy,
                           gchar *name,
                           gchar *old_owner,
                           gchar *new_owner,
                           gpointer user_data)
{
        IRAPIDesktop *self = (IRAPIDesktop*)user_data;

#if ENABLE_HAL_SUPPORT
        if (strcmp(name, HAL_SERVICE) == 0) {

	        /* If this parameter is empty, hal just came online */

                if (strcmp(old_owner, "") == 0) {
                        synce_debug("%s: hal came online", G_STRFUNC);
			hal_connect(self);

			return;
		}

                /* If this parameter is empty, hal just went offline */

                if (strcmp(new_owner, "") == 0) {
		        g_debug("%s: hal went offline", G_STRFUNC);
			hal_disconnect(self);

			return;
		}
	}
#endif

#if ENABLE_UDEV_SUPPORT
        if (strcmp(name, DCCM_SERVICE) == 0) {

	        /* If this parameter is empty, dccm just came online */

                if (strcmp(old_owner, "") == 0) {
                        synce_debug("%s: dccm came online", G_STRFUNC);
			udev_connect(self);

			return;
		}

                /* If this parameter is empty, dccm just went offline */

                if (strcmp(new_owner, "") == 0) {
		        g_debug("%s: dccm went offline", G_STRFUNC);
			udev_disconnect(self);

			return;
		}
	}
#endif
}


static HRESULT
IRAPIDesktop_Init()
{
        g_type_init();

        IRAPIDesktop *self = NULL;

        self = calloc(1, sizeof(IRAPIDesktop));
        if (!self)
                return E_OUTOFMEMORY;

#if 0
        self->sinks = NULL;
#endif

        /*
           connect to hal, odccm, vdccm ?
           set up callbacks from hal and odccm 
           create initial devices
        */

        self->dbus_connection = NULL;
        self->dbus_proxy = NULL;
#if ENABLE_HAL_SUPPORT
        self->hal_ctx = NULL;
#endif
#if ENABLE_UDEV_SUPPORT
        self->devices = NULL;
#endif

        /* hal */

        GError *error = NULL;
        gboolean has_owner = FALSE;

        self->dbus_connection = dbus_g_bus_get(DBUS_BUS_SYSTEM,
                                               &error);
        if (self->dbus_connection == NULL) {
                synce_error("Failed to open connection to dbus: %s", error->message);
                g_error_free(error);
                return E_FAIL;
        }

        self->dbus_proxy = dbus_g_proxy_new_for_name(self->dbus_connection,
                                                     DBUS_SERVICE,
                                                     DBUS_PATH,
                                                     DBUS_IFACE);

        dbus_g_proxy_add_signal(self->dbus_proxy, "NameOwnerChanged",
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);

        dbus_g_proxy_connect_signal(self->dbus_proxy, "NameOwnerChanged",
                                    G_CALLBACK(dbus_name_owner_changed_cb), self, NULL);

#if ENABLE_HAL_SUPPORT
        if (!(dbus_g_proxy_call(self->dbus_proxy, "NameHasOwner",
                                &error,
                                G_TYPE_STRING, HAL_SERVICE,
                                G_TYPE_INVALID,
                                G_TYPE_BOOLEAN, &has_owner,
                                G_TYPE_INVALID))) {
                synce_error("%s: Error checking owner of %s: %s", G_STRFUNC, HAL_SERVICE, error->message);
                g_error_free(error);
                return E_FAIL;
        }

        if (has_owner)
                hal_connect(self);
#endif

#if ENABLE_UDEV_SUPPORT
        if (!(dbus_g_proxy_call(self->dbus_proxy, "NameHasOwner",
                                &error,
                                G_TYPE_STRING, DCCM_SERVICE,
                                G_TYPE_INVALID,
                                G_TYPE_BOOLEAN, &has_owner,
                                G_TYPE_INVALID))) {
                synce_error("%s: Error checking owner of %s: %s", G_STRFUNC, DCCM_SERVICE, error->message);
                g_error_free(error);
                return E_FAIL;
        }

        if (has_owner)
                udev_connect(self);
#endif

        irapi_desktop = self;

        return S_OK;
}

static HRESULT
IRAPIDesktop_Uninit()
{
        /*
           destroy devices
           remove callbacks from hal and odccm 
           disconnect from hal, odccm, vdccm ?
        */

        IRAPIDesktop *self = irapi_desktop;

#if ENABLE_HAL_SUPPORT
        if (self->hal_ctx)
                hal_disconnect(self);
#endif
#if ENABLE_HAL_SUPPORT
        if (self->dev_mgr_proxy)
                udev_disconnect(self);
#endif

        dbus_g_proxy_disconnect_signal (self->dbus_proxy, "NameOwnerChanged",
                                  G_CALLBACK(dbus_name_owner_changed_cb), NULL);

        g_object_unref(self->dbus_proxy);
        self->dbus_proxy = NULL;

        dbus_g_connection_unref(self->dbus_connection);
        self->dbus_connection = NULL;




        free(irapi_desktop);
        irapi_desktop = NULL;

        return S_OK;
}

HRESULT
IRAPIDesktop_Get(IRAPIDesktop **ppIRAPIDesktop)
{
        HRESULT hr;

        if (irapi_desktop != NULL) {
                *ppIRAPIDesktop = irapi_desktop;
                return S_OK;
        }

        hr = IRAPIDesktop_Init();
        if (SUCCEEDED(hr)) {
                *ppIRAPIDesktop = irapi_desktop;
                return S_OK;
        }
        return hr;
}

void
IRAPIDesktop_AddRef(IRAPIDesktop *self)
{
        self->refcount++;
        return;
}

void
IRAPIDesktop_Release(IRAPIDesktop *self)
{
        self->refcount--;
        if (self->refcount > 0)
                return;

        IRAPIDesktop_Uninit();
        return;
}


HRESULT
IRAPIDesktop_Advise(IRAPIDesktop *self, IRAPISink* pISink, DWORD* pdwContext)
{
        /*
          add sink to list of sinks;
        */
        return E_NOTIMPL;
}

HRESULT
IRAPIDesktop_EnumDevices(IRAPIDesktop *self, IRAPIEnumDevices** ppIEnum)
{
        IRAPIEnumDevices *enum_dev;
        enum_dev = calloc(1, sizeof(IRAPIEnumDevices));
        if (!enum_dev)
                return E_OUTOFMEMORY;
        memset(enum_dev, 0, sizeof(IRAPIEnumDevices));
        enum_dev->refcount = 1;

        GList *device = self->devices;
        while (device) {
                IRAPIDevice_AddRef(device->data);

                enum_dev->devices = g_list_append(enum_dev->devices, device->data);

                enum_dev->count++;

                device = g_list_next(device);
        }
        enum_dev->current = enum_dev->devices;

        *ppIEnum = enum_dev;
        return S_OK;
}

HRESULT
IRAPIDesktop_FindDevice(IRAPIDesktop *self, RAPIDEVICEID *pDeviceID, RAPI_GETDEVICEOPCODE opFlags, IRAPIDevice** ppIDevice)
{
        return E_NOTIMPL;
}

HRESULT
IRAPIDesktop_UnAdvise(IRAPIDesktop *self, DWORD dwContext)
{
        return E_NOTIMPL;
}

