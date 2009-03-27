include "types.pxi"
import sys

cdef extern from "Python.h":
    object PyString_FromStringAndSize ( char *, int )
    
cdef extern from "stdlib.h":
    void *malloc(size_t size)
    void *realloc(void *ptr, size_t size)
    void free(void *ptr)

cdef extern from "synce.h":
    char *wstr_to_utf8(LPCWSTR unicode) nogil
    LPWSTR wstr_from_utf8(char *utf8) nogil
    void wstr_free_string(void *str) nogil
    char* synce_strerror(DWORD error) nogil

cdef extern from "synce_log.h":
    void synce_log_set_level(int level)

cdef extern from "rapi.h":
    # connection functions
    void *rapi_connection_from_name(char *device_name)
    void rapi_connection_select(void *connection)
    void rapi_connection_destroy(void *connection)
    HRESULT CeRapiInit()
    STDAPI CeRapiUninit()

    HRESULT CeProcessConfig(LPCWSTR config, DWORD flags, LPWSTR* reply) nogil
    LONG CeRegCreateKeyEx(HKEY hKey, LPCWSTR lpszSubKey, DWORD Reserved, LPWSTR lpszClass, DWORD ulOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
    LONG CeRegOpenKeyEx(HKEY hKey, LPCWSTR lpszSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
    LONG CeRegCloseKey(HKEY hKey)
    LONG CeRegDeleteKey(HKEY hKey, LPCWSTR lpszSubKey)
    LONG CeRegDeleteValue(HKEY hKey, LPCWSTR lpszValueName)
    LONG CeRegQueryValueEx(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
    LONG CeRegSetValueEx(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, DWORD dwType, BYTE *lpData, DWORD cbData)
    BOOL CeStartReplication()
    HRESULT CeSyncStart(LPCWSTR params)
    HRESULT CeSyncResume()
    HRESULT CeSyncPause()
    HRESULT CeSyncTimeToPc()
    BOOL CeGetSystemPowerStatusEx(PSYSTEM_POWER_STATUS_EX pSystemPowerStatus, BOOL refresh) nogil
    DWORD CeGetDiskFreeSpaceEx( LPCTSTR _lpDirectoryName, PULARGE_INTEGER lpFreeBytesAvailable, PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes)
    BOOL CeFindAllFiles( LPCWSTR szPath, DWORD dwFlags, LPDWORD lpdwFoundCount, LPLPCE_FIND_DATA ppFindDataArray)

    BOOL CeCloseHandle(HANDLE hObject)
    HANDLE CeCreateFile( LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
    BOOL CeWriteFile( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) nogil
    BOOL CeReadFile( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)

    BOOL CeCreateProcess( LPCWSTR lpApplicationName, LPCWSTR lpCommandLine, void* lpProcessAttributes, void* lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPWSTR lpCurrentDirectory, void* lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
    
    VOID CeGetSystemInfo( LPSYSTEM_INFO lpSystemInfo)
    BOOL CeGetVersionEx( LPCEOSVERSIONINFO lpVersionInformation)
    HRESULT CeRapiGetError()
    DWORD CeGetLastError()

#
# Public constants
#

# from synce_log.h
SYNCE_LOG_LEVEL_LOWEST  = 0

SYNCE_LOG_LEVEL_ERROR   = 1
SYNCE_LOG_LEVEL_WARNING = 2
SYNCE_LOG_LEVEL_INFO    = 3
SYNCE_LOG_LEVEL_DEBUG   = 4
SYNCE_LOG_LEVEL_TRACE   = SYNCE_LOG_LEVEL_DEBUG

SYNCE_LOG_LEVEL_HIGHEST = 5
SYNCE_LOG_LEVEL_DEFAULT = 2

# from synce_sys_error.h
ERROR_SUCCESS           = 0

# from synce_types.h
FALSE                   = 0
TRUE                    = 1

# some HRESULTs
E_ABORT                 = 0x80004004
E_ACCESSDENIED          = 0x80070005
E_FAIL                  = 0x80004005
E_HANDLE                = 0x80070006
E_OUTOFMEMORY           = 0x8007000E
E_INVALIDARG            = 0x80070057
E_NOINTERFACE           = 0x80004002
E_NOTIMPL               = 0x80004001
E_OUTOFMEMORY           = 0x8007000E
E_PENDING               = 0x8000000A
E_POINTER               = 0x80004003
E_UNEXPECTED            = 0x8000FFFF
S_FALSE                 = 0x00000001
S_OK                    = 0x00000000


# from rapi.h
# registry value types
REG_NONE                = 0
REG_SZ                  = 1
REG_EXPAND_SZ           = 2
REG_BINARY              = 3
REG_DWORD               = 4
REG_DWORD_LITTLE_ENDIAN = 4
REG_DWORD_BIG_ENDIAN    = 5
REG_LINK                = 6
REG_MULTI_SZ            = 7

# registry key dispositions
REG_CREATED_NEW_KEY     = 1
REG_OPENED_EXISTING_KEY = 2

# battery status
BATTERY_FLAG_HIGH          =    0x01
BATTERY_FLAG_LOW           =    0x02
BATTERY_FLAG_CRITICAL      =    0x04
BATTERY_FLAG_CHARGING      =    0x08
BATTERY_FLAG_NO_BATTERY    =    0x80
BATTERY_FLAG_UNKNOWN       =    0xFF

# flags for finding file info
FAF_ATTRIBUTES               = 0x00001
FAF_CREATION_TIME            = 0x00002
FAF_LASTACCESS_TIME          = 0x00004
FAF_LASTWRITE_TIME           = 0x00008

FAF_SIZE_HIGH                = 0x00010
FAF_SIZE_LOW                 = 0x00020
FAF_OID                      = 0x00040
FAF_NAME                     = 0x00080

FAF_ATTRIB_CHILDREN          = 0x01000
FAF_ATTRIB_NO_HIDDEN         = 0x02000
FAF_FOLDERS_ONLY             = 0x04000
FAF_NO_HIDDEN_SYS_ROMMODULES = 0x08000

CSIDL_PROGRAMS               = 0x0002
CSIDL_PERSONAL               = 0x0005
CSIDL_FAVORITES_GRYPHON      = 0x0006
CSIDL_STARTUP                = 0x0007
CSIDL_RECENT                 = 0x0008
CSIDL_STARTMENU              = 0x000b
CSIDL_DESKTOPDIRECTORY       = 0x0010
CSIDL_FONTS                  = 0x0014
CSIDL_FAVORITES              = 0x0016

#dwShareMode 
GENERIC_WRITE                = 0x40000000
GENERIC_READ                 = 0x80000000
FILE_SHARE_READ              = 0x00000001

#dwCreationDisposition 
CREATE_NEW                   = 1
CREATE_ALWAYS                = 2
OPEN_EXISTING                = 3
OPEN_ALWAYS                  = 4
TRUNCATE_EXISTING            = 5
OPEN_FOR_LOADER              = 6

#dwFlagsAndAttributes 
FILE_ATTRIBUTE_READONLY      = 0x00000001
FILE_ATTRIBUTE_HIDDEN        = 0x00000002
FILE_ATTRIBUTE_SYSTEM        = 0x00000004
FILE_ATTRIBUTE_1             = 0x00000008

FILE_ATTRIBUTE_DIRECTORY     = 0x00000010
FILE_ATTRIBUTE_ARCHIVE       = 0x00000020
FILE_ATTRIBUTE_INROM         = 0x00000040
FILE_ATTRIBUTE_NORMAL        = 0x00000080

FILE_ATTRIBUTE_TEMPORARY     = 0x00000100
FILE_ATTRIBUTE_2             = 0x00000200
FILE_ATTRIBUTE_3             = 0x00000400
FILE_ATTRIBUTE_COMPRESSED    = 0x00000800

FILE_ATTRIBUTE_ROMSTATICREF  = 0x00001000
FILE_ATTRIBUTE_ROMMODULE     = 0x00002000
FILE_ATTRIBUTE_4             = 0x00004000
FILE_ATTRIBUTE_5             = 0x00008000

FILE_ATTRIBUTE_HAS_CHILDREN  = 0x00010000
FILE_ATTRIBUTE_SHORTCUT      = 0x00020000
FILE_ATTRIBUTE_6             = 0x00040000
FILE_ATTRIBUTE_7             = 0x00080000


#
# RAPIError is a subclass of Exception

class RAPIError(Exception):
    def __init__(self, err_code=None):
        cdef HRESULT hr
        cdef DWORD last_error

        if err_code != None:
            self.err_code = err_code
        else:
            hr = CeRapiGetError()
            if hr < 0:
                self.err_code = hr
            else:
                last_error = CeGetLastError()
                self.err_code = last_error

    def __str__(self):
        return str(self.err_code)+": "+str(synce_strerror(self.err_code))


class RegKey(object):
    def __init__(self, rapi_session, handle, disposition=REG_OPENED_EXISTING_KEY):
        self.rapi_session = rapi_session
        self.handle = handle
        self.disposition = disposition
        self._host_le = (sys.byteorder == "little")

    def __del__(self):
        self.close()

    def open_sub_key(self, sub_key):
        cdef LPWSTR sub_key_w
        cdef HKEY opened_key

        if sub_key != None:
            sub_key_w = wstr_from_utf8(sub_key)
        else:
            sub_key_w = NULL

        self.rapi_session.__session_select__()

        retval = CeRegOpenKeyEx(self.handle, sub_key_w, 0, 0, &opened_key)

        if sub_key_w != NULL:
            wstr_free_string(sub_key_w)

        if retval != ERROR_SUCCESS:
            raise RAPIError(retval)

        return RegKey(self.rapi_session, opened_key)

    def create_sub_key(self, sub_key, key_class=""):
        cdef HKEY new_key
        cdef DWORD disposition
        cdef LPWSTR sub_key_w
        cdef LPWSTR key_class_w

        if sub_key != None:
            sub_key_w = wstr_from_utf8(sub_key)
        else:
            sub_key_w = NULL

        key_class_w = wstr_from_utf8(key_class)

        self.rapi_session.__session_select__()

        retval = CeRegCreateKeyEx(self.handle, sub_key_w, 0, key_class_w,
                                  0, 0, NULL, &new_key, &disposition)

        if sub_key_w != NULL:
            wstr_free_string(sub_key_w)

        wstr_free_string(key_class_w)

        if retval != ERROR_SUCCESS:
            raise RAPIError(retval)

        return RegKey(self.rapi_session, new_key, disposition)

    def _dword_swap(self, dw):
        return (dw & 0x000000FF) << 24 | \
               (dw & 0x0000FF00) << 8  | \
               (dw & 0x00FF0000) >> 8  | \
               (dw & 0xFF000000) >> 24

    def _dword_le_to_host(self, dw):
        if self._host_le:
            return dw
        else:
            return self._dword_swap(dw)

    def _dword_le_from_host(self, dw):
        if self._host_le:
            return dw
        else:
            return self._dword_swap(dw)

    def _dword_be_to_host(self, dw):
        if self._host_le:
            return self._dword_swap(dw)
        else:
            return dw

    def _dword_be_from_host(self, dw):
        if self._host_le:
            return self._dword_swap(dw)
        else:
            return dw

    def query_value(self, value_name):
        cdef LPWSTR name_w
        cdef DWORD type
        cdef LPBYTE data
        cdef DWORD data_size
        cdef LPDWORD dw_ptr

        name_w = NULL
        data = NULL

        try:
            if value_name != None:
                name_w = wstr_from_utf8(value_name)
            else:
                name_w = NULL

            self.rapi_session.__session_select__()

            data_size = 0
            retval = CeRegQueryValueEx(self.handle, name_w, NULL,
                                       &type, NULL, &data_size)

            if retval != ERROR_SUCCESS:
                raise RAPIError(retval)

            data = <LPBYTE> malloc(data_size)

            retval = CeRegQueryValueEx(self.handle, name_w, NULL,
                                       &type, data, &data_size)

            if retval != ERROR_SUCCESS:
                raise RAPIError(retval)

            if type == REG_NONE:
                value = PyString_FromStringAndSize(<char *>data, data_size)
            elif type == REG_SZ or type == REG_EXPAND_SZ:
                value = wstr_to_utf8(<LPCWSTR> data)
            elif type == REG_BINARY:
                value = PyString_FromStringAndSize(<char *>data, data_size)
            elif type == REG_DWORD:
                dw_ptr = <LPDWORD> data
                value = self._dword_le_to_host(dw_ptr[0])
            elif type == REG_DWORD_BIG_ENDIAN:
                dw_ptr = <LPDWORD> data
                value = self._dword_be_to_host(dw_ptr[0])
            elif type == REG_LINK:
                value = PyString_FromStringAndSize(<char *>data, data_size)
            elif type == REG_MULTI_SZ:
                # FIXME - this is doable
                value = PyString_FromStringAndSize(<char *>data, data_size)
            else:
                value = PyString_FromStringAndSize(<char *>data, data_size)

            return value
        finally:
            if name_w != NULL:
                wstr_free_string(name_w)

            if data != NULL:
                free(data)

    def set_value(self, value_name, value_data, value_type=None):
        cdef LPWSTR name_w
        cdef LPBYTE data
        cdef DWORD data_size

        name_w = NULL
        data = NULL

        try:
            if value_type == None:
                if isinstance(value_data, basestring):
                    value_type = REG_SZ
                else:
                    value_type = REG_DWORD

            if value_name != None:
                name_w = wstr_from_utf8(value_name)
            else:
                name_w = NULL

            if value_type == REG_SZ:
                data = <LPBYTE> wstr_from_utf8(value_data)
                data_size = (len(value_data) + 1) * 2
            elif value_type == REG_DWORD:
                data = <LPBYTE> malloc(4)
                (<LPDWORD> data)[0] = self._dword_le_from_host(value_data)
                data_size = 4
            else:
                raise RAPIError(E_NOTIMPL)

            self.rapi_session.__session_select__()

            retval = CeRegSetValueEx(self.handle, name_w, 0, value_type, data, data_size)

            if retval != ERROR_SUCCESS:
                raise RAPIError(retval)

            return
        finally:
            if name_w != NULL:
                wstr_free_string(name_w)
            if data != NULL:
                if type == REG_SZ:
                    wstr_free_string(data)
                else:
                    free(data)

    def delete_sub_key(self, sub_key):
        cdef LPWSTR key_w

        key_w = wstr_from_utf8(sub_key)
        self.rapi_session.__session_select__()

        retval = CeRegDeleteKey(self.handle, key_w)
        wstr_free_string(key_w)

        if retval != ERROR_SUCCESS:
            raise RAPIError(retval)

        return

    def delete_value(self, value_name=None):
        cdef LPWSTR name_w

        if value_name != None:
            name_w = wstr_from_utf8(value_name)
        else:
            name_w = NULL

        self.rapi_session.__session_select__()

        retval = CeRegDeleteValue(self.handle, name_w)

        if name_w != NULL:
            wstr_free_string(name_w)

        if retval != ERROR_SUCCESS:
            raise RAPIError(retval)

        return

    def close(self):
        # we probably dont want to close root keys
        if self.handle == 0x80000000 or self.handle == 0x80000001 or self.handle == 0x80000002 or self.handle == 0x80000003 or self.handle == 0:
            return

        self.rapi_session.__session_select__()

        retval = CeRegCloseKey(self.handle)
        self.handle = 0
        if retval != ERROR_SUCCESS:
            raise RAPIError(retval)


cdef class RAPISession:
    cdef void *rapi_conn
    cdef HKEY_CLASSES_ROOT_regkey
    cdef HKEY_CURRENT_USER_regkey
    cdef HKEY_LOCAL_MACHINE_regkey
    cdef HKEY_USERS_regkey

    def __cinit__(self, device=None, log_level=SYNCE_LOG_LEVEL_LOWEST, *args, **keywords):
        synce_log_set_level(log_level)

        if device == None:
            self.rapi_conn = rapi_connection_from_name(NULL)
        else:
            self.rapi_conn = rapi_connection_from_name(device)

        if self.rapi_conn == NULL:
            raise RAPIError(E_FAIL)

        rapi_connection_select(self.rapi_conn)
        
        retval = CeRapiInit()
        
        if retval != 0:
            raise RAPIError(retval)


    def __init__(self, device=None, log_level=SYNCE_LOG_LEVEL_LOWEST):
        self.HKEY_CLASSES_ROOT_regkey       = RegKey(self, 0x80000000)
        self.HKEY_CURRENT_USER_regkey       = RegKey(self, 0x80000001)
        self.HKEY_LOCAL_MACHINE_regkey      = RegKey(self, 0x80000002)
        self.HKEY_USERS_regkey              = RegKey(self, 0x80000003)


    def __dealloc__(self):
        if self.rapi_conn != NULL:
            rapi_connection_select(self.rapi_conn)
            CeRapiUninit()
            rapi_connection_destroy(self.rapi_conn)
            self.rapi_conn = NULL


    def __getattr__(self, name):
        if name == "HKEY_CLASSES_ROOT" or name == "HKCR":
            return self.HKEY_CLASSES_ROOT_regkey
        elif name == "HKEY_CURRENT_USER" or name == "HKCU":
            return self.HKEY_CURRENT_USER_regkey
        elif name == "HKEY_LOCAL_MACHINE" or name == "HKLM":
            return self.HKEY_LOCAL_MACHINE_regkey
        elif name == "HKEY_USERS" or name == "HKU":
            return self.HKEY_USERS_regkey
        else:
            raise AttributeError("%s instance has no attribute '%s'" % (self.__class__.__name__, name))


    def __session_select__(self):
        rapi_connection_select(self.rapi_conn)


    def process_config(self, config, flags):
        cdef LPWSTR config_w
        cdef LPWSTR reply_w
        cdef DWORD flags_c
        cdef HRESULT retval
        cdef char *reply

        reply_w = NULL;

        config_w = wstr_from_utf8(config)
        flags_c = flags

        self.__session_select__()

        with nogil:
            retval = CeProcessConfig(config_w, flags_c, &reply_w)
            wstr_free_string(config_w)

            reply = wstr_to_utf8(reply_w)
            wstr_free_string(reply_w)

        if retval != 0:
            raise RAPIError(retval)

        if reply == NULL:
            return ""
        else:
            return reply

    def start_replication(self):
        self.__session_select__()

        retval = CeStartReplication()
        if retval != TRUE:
            raise RAPIError(retval)

    def sync_start(self, params):
        cdef LPWSTR params_w

        params_w = wstr_from_utf8(params)

        self.__session_select__()

        retval = CeSyncStart(params_w)
        wstr_free_string(params_w)
        if retval != 0:
            raise RAPIError

    def sync_resume(self):
        self.__session_select__()

        retval = CeSyncResume()
        if retval != 0:
            raise RAPIError

    def sync_pause(self):
        self.__session_select__()

        retval = CeSyncPause()
        if retval != 0:
            raise RAPIError

    def SyncTimeToPc(self):
        self.__session_select__()

        return CeSyncTimeToPc()

    def getDiskFreeSpaceEx(self, path):
        cdef ULARGE_INTEGER freeBytesAvailable  
        cdef ULARGE_INTEGER totalNumberOfBytes  
        cdef ULARGE_INTEGER totalNumberOfFreeBytes 

        self.__session_select__()

        retval = CeGetDiskFreeSpaceEx( path, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes) 

        #This functions returns 0 if something went wrong....
        if retval == 0:
            raise RAPIError

        #return a tuple at the moment, maybe later make this a list
        return (freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes) 

    def getSystemPowerStatus(self, refresh):
        cdef SYSTEM_POWER_STATUS_EX powerStatus
        cdef BOOL retval

        self.__session_select__()

        with nogil:
            retval = CeGetSystemPowerStatusEx( &powerStatus, 0 )
        #In contrast to other functions, this returns a boolean,
        #denoting whether the call was succesfull
        if retval == FALSE:
            raise RAPIError

        #Now construct the dictionary:
        result = dict()
        result["ACLineStatus"] = powerStatus.ACLineStatus
        result["BatteryFlag"]               = powerStatus.BatteryFlag
        result["BatteryLifePercent"]        = powerStatus.BatteryLifePercent
        result["Reserved1"]                 = powerStatus.Reserved1
        result["BatteryLifeTime"]           = powerStatus.BatteryLifeTime
        result["BatteryFullLifeTime"]       = powerStatus.BatteryFullLifeTime
        result["Reserved2"]                 = powerStatus.Reserved2
        result["BackupBatteryFlag"]         = powerStatus.BackupBatteryFlag
        result["BackupBatteryLifePercent"]  = powerStatus.BackupBatteryLifePercent
        result["Reserved3"]                 = powerStatus.Reserved3
        result["BackupBatteryLifeTime"]     = powerStatus.BackupBatteryLifeTime
        result["BackupBatteryFullLifeTime"] = powerStatus.BackupBatteryFullLifeTime
        return result


    def findAllFiles( self, query, flags ):
        cdef LPWSTR query_w
        query_w = wstr_from_utf8(query)

        cdef LPCE_FIND_DATA find_data 
        cdef DWORD numberOfFiles

        cdef CE_FIND_DATA found_file

        self.__session_select__()

        retval = CeFindAllFiles( query_w, flags , &numberOfFiles, &find_data )
        wstr_free_string(query_w)
        if retval == 0:
            raise RAPIError

        #Now create a list of dictionaries
        result = [] 
        i=0
        while i < numberOfFiles:
            found_file = find_data[ i ] 
            
            this_file = dict()

            if flags & FAF_ATTRIBUTES:
                this_file["Attributes"] = found_file.dwFileAttributes

            if flags & FAF_CREATION_TIME:
                this_file["CreationLowDateTime"] = found_file.ftCreationTime.dwLowDateTime
                this_file["CreationHighDateTime"] = found_file.ftCreationTime.dwHighDateTime
            
            if flags & FAF_LASTACCESS_TIME:
                this_file["LastAccessLowDateTime"] = found_file.ftLastAccessTime.dwLowDateTime
                this_file["LastAccessHighDateTime"] = found_file.ftLastAccessTime.dwHighDateTime
            
            if flags & FAF_LASTWRITE_TIME:
                this_file["LastWriteLowDateTime"] = found_file.ftCreationTime.dwLowDateTime
                this_file["LastWriteHighDateTime"] = found_file.ftCreationTime.dwHighDateTime

            if flags & FAF_SIZE_HIGH:
                this_file["SizeHigh"] = found_file.nFileSizeHigh
             
            if flags & FAF_SIZE_LOW:
                this_file["SizeLow"] = found_file.nFileSizeLow
            
            if flags & FAF_OID:
                this_file["OID"] = found_file.dwOID

            if flags & FAF_NAME:
                this_file["Name"] = wstr_to_utf8(found_file.cFileName)


            result.append( this_file )
            i = i + 1


        return result


    def closeHandle(self, hObject):
        self.__session_select__()

        retval = CeCloseHandle( hObject ) 
        #Non-zero indicates success, zero indicates failure
        if retval == 0:
            raise RAPIError


    def createFile(self, filename, desiredAccess, shareMode, createDisposition, flagsAndAttributes):
        cdef LPWSTR filename_w
        cdef HANDLE fileHandle

        filename_w = wstr_from_utf8(filename)

        self.__session_select__()

        fileHandle = CeCreateFile( filename_w, desiredAccess, shareMode, NULL, createDisposition, flagsAndAttributes, 0 ) 

        wstr_free_string(filename_w)
        if fileHandle == <HANDLE> -1:
            raise RAPIError

        return fileHandle


    def readFile( self, fileHandle, numberOfBytesToRead ):
        cdef DWORD numberOfBytesRead
        cdef DWORD dwNumberOfBytesToRead
        dwNumberOfBytesToRead = numberOfBytesToRead
        cdef LPBYTE c_buffer
       
        c_buffer = <LPBYTE> malloc (numberOfBytesToRead)
        
        self.__session_select__()

        CeReadFile( fileHandle, c_buffer , numberOfBytesToRead, &numberOfBytesRead, NULL )

        if retval == 0:
            free(c_buffer)
            raise RAPIError
        
        if numberOfBytesRead < numberOfBytesToRead:
            c_buffer = <LPBYTE> realloc( c_buffer, numberOfBytesRead)

        cdef object returnstring
        returnstring = PyString_FromStringAndSize(<char *>c_buffer, numberOfBytesRead)

        free(c_buffer)
        return returnstring, numberOfBytesRead


    def writeFile( self, fileHandle, buffer, numberOfBytesToWrite ):
        cdef HANDLE hFile
        cdef char * lpBuffer
        cdef DWORD nNumberOfBytesToWrite
        cdef DWORD numberOfBytesWritten
        cdef BOOL retval

        hFile = fileHandle
        lpBuffer = buffer
        nNumberOfBytesToWrite = numberOfBytesToWrite

        self.__session_select__()

        with nogil:
            retval = CeWriteFile( hFile, lpBuffer , nNumberOfBytesToWrite, &numberOfBytesWritten, NULL)

        if retval == FALSE:
            raise RAPIError

        return numberOfBytesWritten

    
    #TODO: Provide the user with the processInformation
    def createProcess(self, applicationName, applicationParams):
        cdef PROCESS_INFORMATION processInformation
        cdef LPWSTR applicationName_w
        cdef LPWSTR applicationParams_w

        applicationName_w = wstr_from_utf8(applicationName)
        applicationParams_w = wstr_from_utf8(applicationParams)

        self.__session_select__()

        retval = CeCreateProcess( applicationName_w, applicationParams_w, NULL, NULL, False, 0, NULL, NULL, NULL, &processInformation)

        wstr_free_string(applicationName_w)
        wstr_free_string(applicationParams_w)

        if retval == 0:
            raise RAPIError

        return retval


    def getVersion(self):
        cdef CEOSVERSIONINFO osVersionInfo
        
        self.__session_select__()

        retval = CeGetVersionEx( &osVersionInfo)
     
        if retval == 0:
            raise RAPIError

		#Now construct the dictionary:
        result = dict()
        result["OSVersionInfoSize"] = osVersionInfo.dwOSVersionInfoSize
        result["MajorVersion"]      = osVersionInfo.dwMajorVersion
        result["MinorVersion"]      = osVersionInfo.dwMinorVersion
        result["BuildNumber"]       = osVersionInfo.dwBuildNumber
        result["PlatformId"]        = osVersionInfo.dwPlatformId
        
        return result
    
    def getSystemInfo(self):
        cdef SYSTEM_INFO systemInfo
        
        self.__session_select__()

        CeGetSystemInfo( &systemInfo )
        
        result = dict()
        result["ProcessorArchitecture"]                 = systemInfo.wProcessorArchitecture
        result["Reserved"]                              = systemInfo.wReserved
        result["PageSize"]                              = systemInfo.dwPageSize
        result["MinimumApplicationAddress"]             = systemInfo.lpMinimumApplicationAddress
        result["MaximumApplicationAddress"]             = systemInfo.lpMaximumApplicationAddress
        result["ActiveProcessorMask"]                   = systemInfo.dwActiveProcessorMask
        result["NumberOfProcessors"]                    = systemInfo.dwNumberOfProcessors
        result["ProcessorType"]                         = systemInfo.dwProcessorType
        result["AllocationGranularity"]                 = systemInfo.dwAllocationGranularity
        result["ProcessorLevel"]                        = systemInfo.wProcessorLevel
        result["ProcessorRevision"]                     = systemInfo.wProcessorRevision
        
        return result
