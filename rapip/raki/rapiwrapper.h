#ifndef RAPIWRAPPER_H
#define RAPIWRAPPER_H

#include <stdio.h>
#include <qstring.h>
#include <rapi.h>

#include <syslog.h>


class Ce
{
public:
  static bool rapiInit()
  {
    HRESULT hr;
    bool ret = false;

    if (!initialized) {
      hr = CeRapiInit();
      if (!FAILED(hr)) {
        ret = true;
        initialized = true;
        used++;
      }
    } else {
      ret = true;
      used++;
    }
    
    return ret;
  }

  
  static bool rapiUninit()
  {
    if (initialized) {
      used--;
      if (!used) {
        CeRapiUninit();
        initialized = false;
      }
    }
    
    return true;
  }

  
  static bool createProcess(
    LPCWSTR lpApplicationName,
    LPCWSTR lpCommandLine,
    void* lpProcessAttributes,
    void* lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPWSTR lpCurrentDirectory,
    void* lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation)
  {
    bool ret = false;

    if (ret = CeCreateProcess(lpApplicationName,
                              lpCommandLine,
                              lpProcessAttributes,
                              lpThreadAttributes,
                              bInheritHandles,
                              dwCreationFlags,
                              lpEnvironment,
                              lpCurrentDirectory,
                              lpStartupInfo,
                              lpProcessInformation)) {
      CeCloseHandle(lpProcessInformation->hProcess);
      CeCloseHandle(lpProcessInformation->hThread);
    }

    return ret;
  }


  static bool createDirectory(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes)
  {
    bool ret = false;

    ret = CeCreateDirectory(lpPathName, lpSecurityAttributes);

    return ret;
  }


  static bool copyFile(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists)
  {
    bool ret = false;

    ret = CeCopyFile(lpExistingFileName, lpNewFileName, bFailIfExists);

    return ret;
  }


  static HANDLE createFile(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
  {
    HANDLE h;

    h = CeCreateFile(lpFileName,
                     dwDesiredAccess,
                     dwShareMode,
                     lpSecurityAttributes,
                     dwCreationDisposition,
                     dwFlagsAndAttributes,
                     hTemplateFile);

    return h;
  }


  static bool writeFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped)
  {
    return CeWriteFile(
             hFile,
             lpBuffer,
             nNumberOfBytesToWrite,
             lpNumberOfBytesWritten,
             lpOverlapped);
  }
  
  
  static bool getVersionEx(CEOSVERSIONINFO *version) {
    memset(version, 0, sizeof(version));
    version->dwOSVersionInfoSize = sizeof(version);
    
    return CeGetVersionEx(version);
  }
  
  
  static void getSystemInfo(SYSTEM_INFO *system) {
    memset(system, 0, sizeof(system));
    CeGetSystemInfo(system);
  }
  
  
  static bool getSystemPowerStatusEx(SYSTEM_POWER_STATUS_EX *power, bool boolval) {
    memset(power, 0, sizeof(SYSTEM_POWER_STATUS_EX));
    return CeGetSystemPowerStatusEx(power, boolval);
  }
  
  
  static bool getStoreInformation(STORE_INFORMATION *store) {
    memset(store, 0, sizeof(store));
    return CeGetStoreInformation(store);
  }
  
  
  static bool closeHandle(HANDLE hObject)
  {
    return CeCloseHandle(hObject);
  }


  static WCHAR *wpath_from_upath(QString uPath)
  {
    WCHAR* wPath = NULL;

    uPath.replace('/', "\\");

    if (uPath.ascii() != NULL)
      wPath = synce::wstr_from_ascii(uPath.ascii());

    return wPath;
  }


  static void destroy_wpath(WCHAR *wPath)
  {
    synce::wstr_free_string(wPath);
  }

  
  static const int ANYFILE_BUFFER_SIZE = 16 * 1024;

private:

  struct _AnyFile
  {
    HANDLE remote;
    FILE*  local;
  };

  typedef struct _AnyFile AnyFile;

  static bool initialized;
  static int used;
};


#endif
