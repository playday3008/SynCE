#ifndef RAPIWRAPPER_H
#define RAPIWRAPPER_H

#include <stdio.h>
#include <qstring.h>
#include <rapi.h>


class Ce
{
public:
  static bool rapiInit()
  {
    HRESULT hr;
    bool ret = false;

    if (!initialized) {
      hr = synce::CeRapiInit();
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
        synce::CeRapiUninit();
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
    synce::LPPROCESS_INFORMATION lpProcessInformation)
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
      synce::CeCloseHandle(lpProcessInformation->hProcess);
      synce::CeCloseHandle(lpProcessInformation->hThread);
    }

    return ret;
  }


  static bool createDirectory(
    LPCWSTR lpPathName,
    synce::LPSECURITY_ATTRIBUTES lpSecurityAttributes)
  {
    bool ret = false;

    ret = synce::CeCreateDirectory(lpPathName, lpSecurityAttributes);

    return ret;
  }


  static bool copyFile(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists)
  {
    bool ret = false;

    ret = synce::CeCopyFile(lpExistingFileName, lpNewFileName, bFailIfExists);

    return ret;
  }


  static HANDLE createFile(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    synce::LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
  {
    HANDLE h;

    h = synce::CeCreateFile(lpFileName,
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
    synce::LPOVERLAPPED lpOverlapped)
  {
    return synce::CeWriteFile(
             hFile,
             lpBuffer,
             nNumberOfBytesToWrite,
             lpNumberOfBytesWritten,
             lpOverlapped);
  }
  
  
  static bool getVersionEx(synce::CEOSVERSIONINFO *version) {
    memset(version, 0, sizeof(version));
    version->dwOSVersionInfoSize = sizeof(version);
    
    return CeGetVersionEx(version);
  }
  
  
  static void getSystemInfo(synce::SYSTEM_INFO *system) {
    memset(system, 0, sizeof(system));
    CeGetSystemInfo(system);
  }
  
  
  static bool getSystemPowerStatusEx(synce::SYSTEM_POWER_STATUS_EX *power, bool boolval) {
    memset(power, 0, sizeof(synce::SYSTEM_POWER_STATUS_EX));
    return CeGetSystemPowerStatusEx(power, boolval);
  }
  
  
  static bool getStoreInformation(synce::STORE_INFORMATION *store) {
    memset(store, 0, sizeof(synce::STORE_INFORMATION));
    return CeGetStoreInformation(store);
  }
  
  
  static bool closeHandle(HANDLE hObject)
  {
    return synce::CeCloseHandle(hObject);
  }


  static WCHAR *wpath_from_upath(QString uPath)
  {
    WCHAR* wPath = NULL;

    if (uPath.ascii() != NULL)
      wPath = synce::wstr_from_ascii(uPath.ascii());
      
    if (wPath) 
        for (WCHAR* p = wPath; *p; p++) 
            if (*p == '/') 
                *p = '\\'; 

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

  static bool initialized;
  static int used;

public:
  typedef struct _AnyFile AnyFile;
};


#endif
