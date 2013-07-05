/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/

#ifndef RAPIWRAPPER_H
#define RAPIWRAPPER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rapi2.h>
#include <stdio.h>

#include <kurl.h>
#include <qstring.h>
#include <KDebug>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

/**
@author Volker Christian,,,
*/

class Ce
{
public:
    static bool rapiInit(QString host)
    {
        HRESULT hr;
        synce::IRAPIDesktop *desktop = NULL;
        synce::IRAPIEnumDevices *enumdev = NULL;
        synce::IRAPIDevice *device = NULL;
        synce::RAPI_DEVICEINFO devinfo;
        bool ret = false;

        if (initialized) {
            ret = true;
            used++;
            return ret;
        }

        kDebug(2120) << "connecting to device " << host << endl;

        if (FAILED(hr = synce::IRAPIDesktop_Get(&desktop))) {
          return ret;
        }

        if (FAILED(hr = synce::IRAPIDesktop_EnumDevices(desktop, &enumdev))) {
          synce::IRAPIDesktop_Release(desktop);
          return ret;
        }

        while (SUCCEEDED(hr = synce::IRAPIEnumDevices_Next(enumdev, &device))) {
          if (host.isEmpty())
            break;

          if (FAILED(synce::IRAPIDevice_GetDeviceInfo(device, &devinfo))) {
            synce::IRAPIEnumDevices_Release(enumdev);
            synce::IRAPIDesktop_Release(desktop);
            return ret;
          }
          if (strcmp(host.toAscii().constData(), devinfo.bstrName) == 0)
            break;
        }

        if (FAILED(hr)) {
          synce::IRAPIEnumDevices_Release(enumdev);
          synce::IRAPIDesktop_Release(desktop);
          return ret;
        }

        synce::IRAPIDevice_AddRef(device);
        synce::IRAPIEnumDevices_Release(enumdev);
        enumdev = NULL;

        if (FAILED(hr = synce::IRAPIDevice_CreateSession(device, &session))) {
          synce::IRAPIDevice_Release(device);
          synce::IRAPIDesktop_Release(desktop);
          return ret;
        }

        synce::IRAPIDevice_Release(device);

        hr = synce::IRAPISession_CeRapiInit(session);
        if (!FAILED(hr)) {
            ret = true;
            initialized = true;
            used++;
        } else {
            synce::IRAPISession_Release(session);
            session = NULL;
        }

        return ret;
    }


    static bool rapiUninit()
    {
        if (initialized) {
            used--;
            if (!used) {
                synce::IRAPISession_CeRapiUninit(session);
                IRAPISession_Release(session);
                session = NULL;
                initialized = false;
            }
        }

        return true;
    }


    static QString getDeviceIp()
    {
        const char *device_ip = NULL;
        synce::IRAPIDevice *device = NULL;

        device = synce::IRAPISession_get_device(session);
        device_ip = synce::IRAPIDevice_get_device_ip(device);
        synce::IRAPIDevice_Release(device);
        return QString(device_ip);
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

        if ((ret = IRAPISession_CeCreateProcess(session,
                                  lpApplicationName,
                                  lpCommandLine,
                                  lpProcessAttributes,
                                  lpThreadAttributes,
                                  bInheritHandles,
                                  dwCreationFlags,
                                  lpEnvironment,
                                  lpCurrentDirectory,
                                  lpStartupInfo,
                                   lpProcessInformation))) {
            synce::IRAPISession_CeCloseHandle(session, lpProcessInformation->hProcess);
            synce::IRAPISession_CeCloseHandle(session, lpProcessInformation->hThread);
        }

        return ret;
    }


    static bool createDirectory(
        LPCWSTR lpPathName,
        synce::LPSECURITY_ATTRIBUTES lpSecurityAttributes)
    {
        bool ret = false;

        ret = synce::IRAPISession_CeCreateDirectory(session, lpPathName, lpSecurityAttributes);

        return ret;
    }


    static bool copyFile(
        LPCWSTR lpExistingFileName,
        LPCWSTR lpNewFileName,
        BOOL bFailIfExists)
    {
        bool ret = false;

        ret = synce::IRAPISession_CeCopyFile(session, lpExistingFileName, lpNewFileName,
                bFailIfExists);

        return ret;
    }


    static Qt::HANDLE createFile(
        LPCWSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        synce::LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        Qt::HANDLE hTemplateFile)
    {
        HANDLE h;

        h = synce::IRAPISession_CeCreateFile(session,
                                lpFileName,
                                dwDesiredAccess,
                                dwShareMode,
                                lpSecurityAttributes,
                                dwCreationDisposition,
                                dwFlagsAndAttributes,
                                hTemplateFile);

        return h;
    }

    static HRESULT rapiInvoke(
        LPCWSTR pDllPath,
        LPCWSTR pFunctionName,
        DWORD cbInput,
        const BYTE *pInput,
        DWORD *pcbOutput,
        BYTE **ppOutput,
        synce::IRAPIStream **ppIRAPIStream,
        DWORD dwReserved)
    {
        HRESULT hr;

        hr = IRAPISession_CeRapiInvoke(
                 session,
                 pDllPath,
                 pFunctionName,
                 cbInput,
                 pInput,
                 pcbOutput,
                 ppOutput,
                 ppIRAPIStream,
                 dwReserved);

        return hr;
    }


    static bool writeFile(
        Qt::HANDLE hFile,
        LPCVOID lpBuffer,
        DWORD nNumberOfBytesToWrite,
        LPDWORD lpNumberOfBytesWritten,
        synce::LPOVERLAPPED lpOverlapped)
    {
        return synce::IRAPISession_CeWriteFile(
                   session,
                   hFile,
                   lpBuffer,
                   nNumberOfBytesToWrite,
                   lpNumberOfBytesWritten,
                   lpOverlapped);
    }


    static bool getVersionEx(synce::CEOSVERSIONINFO *version)
    {
        memset(version, 0, sizeof(version));
        version->dwOSVersionInfoSize = sizeof(version);

        return synce::IRAPISession_CeGetVersionEx(session, version);
    }


    static void getSystemInfo(synce::SYSTEM_INFO *system)
    {
        memset(system, 0, sizeof(system));
        synce::IRAPISession_CeGetSystemInfo(session, system);
    }


    static bool getSystemPowerStatusEx(synce::SYSTEM_POWER_STATUS_EX *power,
            bool boolval)
    {
        memset(power, 0, sizeof(synce::SYSTEM_POWER_STATUS_EX));
        return synce::IRAPISession_CeGetSystemPowerStatusEx(session, power, boolval);
    }


    static bool getStoreInformation(synce::STORE_INFORMATION *store)
    {
        memset(store, 0, sizeof(store));
        return synce::IRAPISession_CeGetStoreInformation(session, store);
    }


    static bool closeHandle(Qt::HANDLE hObject)
    {
        return synce::IRAPISession_CeCloseHandle(session, hObject);
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
    static synce::IRAPISession* session;

public:
    typedef struct _AnyFile AnyFile;
};


#endif
