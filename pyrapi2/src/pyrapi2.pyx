include "types.pxi"

cdef extern from "stdlib.h":
    void *malloc(size_t size)
    void free(void *ptr)

cdef extern from "synce.h":
    char *wstr_to_utf8(LPCWSTR unicode)
    LPWSTR wstr_from_utf8(char *utf8)
    void wstr_free_string(void *str)

cdef extern from "synce_log.h":
    void synce_log_set_level(int level)

cdef extern from "rapi.h":
    void *rapi_connection_from_path(char *path)
    void rapi_connection_select(void *connection)
    HRESULT CeRapiInit()
    HRESULT CeProcessConfig(LPCWSTR config, DWORD flags, LPWSTR* reply)
    LONG CeRegOpenKeyEx(HKEY hKey, LPCWSTR lpszSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
    LONG CeRegCreateKeyEx(HKEY hKey, LPCWSTR lpszSubKey, DWORD Reserved, LPWSTR lpszClass, DWORD ulOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
    LONG CeRegCloseKey(HKEY hKey)
    LONG CeRegQueryValueEx(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)


#
# Public constants
#
SYNCE_LOG_LEVEL_LOWEST  = 0

SYNCE_LOG_LEVEL_ERROR   = 1
SYNCE_LOG_LEVEL_WARNING = 2
SYNCE_LOG_LEVEL_INFO    = 3
SYNCE_LOG_LEVEL_DEBUG   = 4
SYNCE_LOG_LEVEL_TRACE   = SYNCE_LOG_LEVEL_DEBUG

SYNCE_LOG_LEVEL_HIGHEST = 5
SYNCE_LOG_LEVEL_DEFAULT = 2

ERROR_SUCCESS           = 0

REG_NONE                = 0
REG_SZ                  = 1
REG_EXPAND_SZ           = 2
REG_BINARY              = 3
REG_DWORD               = 4
REG_DWORD_LITTLE_ENDIAN = 4
REG_DWORD_BIG_ENDIAN    = 5
REG_LINK                = 6
REG_MULTI_SZ            = 7

REG_CREATED_NEW_KEY     = 1
REG_OPENED_EXISTING_KEY = 2


class RAPIError:
    def __init__(self, retval):
        self.retval = retval

    def __str__(self):
        return str(self.retval)


class RegKey:
    def __init__(self, handle, disposition=REG_OPENED_EXISTING_KEY):
        self.handle = handle
        self.disposition = disposition

    def open_sub_key(self, sub_key):
        cdef LPWSTR sub_key_w
        cdef HKEY opened_key

        if sub_key != None:
            sub_key_w = wstr_from_utf8(sub_key)
        else:
            sub_key_w = NULL

        retval = CeRegOpenKeyEx(self.handle, sub_key_w, 0, 0, &opened_key)

        if sub_key != None:
            wstr_free_string(sub_key_w)

        if retval != ERROR_SUCCESS:
            raise RAPIError(retval)

        return RegKey(opened_key)

    def create_sub_key(self, sub_key, key_class=""):
        cdef HKEY result
        cdef DWORD disposition
        cdef LPWSTR sub_key_w
        cdef LPWSTR key_class_w

        if sub_key != None:
            sub_key_w = wstr_from_utf8(sub_key)
        else:
            sub_key_w = NULL

        key_class_w = wstr_from_utf8(key_class)

        retval = CeRegCreateKeyEx(self.handle, sub_key_w, 0, key_class_w,
                                  0, 0, NULL, &result, &disposition)

        if sub_key != None:
            wstr_free_string(sub_key_w)

        wstr_free_string(key_class_w)

        if retval != ERROR_SUCCESS:
            raise RAPIError(retval)

        return RegKey(result, disposition)

    def query_value(self, value_name):
        cdef LPWSTR name_w
        cdef DWORD type
        cdef LPBYTE data
        cdef DWORD data_size
        cdef LPDWORD dw_ptr

        if value_name != None:
            name_w = wstr_from_utf8(value_name)
        else:
            name_w = NULL

        data = <LPBYTE> malloc(4096)
        data_size = 4096

        retval = CeRegQueryValueEx(self.handle, name_w, NULL,
                                   &type, data, &data_size)

        if retval != ERROR_SUCCESS:
            raise RAPIError(retval)

        if type == REG_DWORD:
            dw_ptr = <LPDWORD> data
            value = dw_ptr[0]
        elif type == REG_SZ:
            value = wstr_to_utf8(<LPCWSTR> data)
        else:
            # FIXME
            return <char *> data

        free(data)
        return value

    def close(self):
        retval = CeRegCloseKey(self.handle)
        if retval != ERROR_SUCCESS:
            raise RAPIError(retval)


class RAPISession:
    HKEY_CLASSES_ROOT       = RegKey(0x80000000)
    HKEY_CURRENT_USER       = RegKey(0x80000001)
    HKEY_LOCAL_MACHINE      = RegKey(0x80000002)
    HKEY_USERS              = RegKey(0x80000003)

    def __init__(self, log_level=0):
        cdef void *conn

        synce_log_set_level(log_level)
        conn = rapi_connection_from_path(NULL)
        rapi_connection_select(conn)
        CeRapiInit()

    def process_config(self, config, flags):
        cdef LPWSTR config_w
        cdef LPWSTR reply_w
        cdef char *reply

        config_w = wstr_from_utf8(config)
        retval = CeProcessConfig(config_w, flags, &reply_w)
        wstr_free_string(config_w)

        reply = wstr_to_utf8(reply_w)
        wstr_free_string(reply_w)

        if retval != 0:
            raise RAPIError(retval)

        return reply

