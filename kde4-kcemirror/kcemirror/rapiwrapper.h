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

#include <rapi.h>
#include <stdio.h>

#include <kurl.h>
#include <qstring.h>

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
        bool ret = false;

        if (!host.isEmpty()) {
            const char *hostc = host.toAscii().constData();
            synce::synce_set_connection_filename(hostc);
        } else {
            synce::synce_set_default_connection_filename();
        }

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

        if ((ret = CeCreateProcess(lpApplicationName,
                                  lpCommandLine,
                                  lpProcessAttributes,
                                  lpThreadAttributes,
                                  bInheritHandles,
                                  dwCreationFlags,
                                  lpEnvironment,
                                  lpCurrentDirectory,
                                  lpStartupInfo,
                                   lpProcessInformation))) {
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

        ret = synce::CeCopyFile(lpExistingFileName, lpNewFileName,
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

        h = synce::CeCreateFile(lpFileName,
                                dwDesiredAccess,
                                dwShareMode,
                                lpSecurityAttributes,
                                dwCreationDisposition,
                                dwFlagsAndAttributes,
                                hTemplateFile);

        return h;
    }

    static HRESULT rapiInvokeA(
        LPCSTR pDllPath,
        LPCSTR pFunctionName,
        DWORD cbInput,
        const BYTE *pInput,
        DWORD *pcbOutput,
        BYTE **ppOutput,
        synce::IRAPIStream **ppIRAPIStream,
        DWORD dwReserved)
    {
        HRESULT hr;

        hr = CeRapiInvokeA(
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
        return synce::CeWriteFile(
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

        return synce::CeGetVersionEx(version);
    }


    static void getSystemInfo(synce::SYSTEM_INFO *system)
    {
        memset(system, 0, sizeof(system));
        synce::CeGetSystemInfo(system);
    }


    static bool getSystemPowerStatusEx(synce::SYSTEM_POWER_STATUS_EX *power,
            bool boolval)
    {
        memset(power, 0, sizeof(synce::SYSTEM_POWER_STATUS_EX));
        return synce::CeGetSystemPowerStatusEx(power, boolval);
    }


    static bool getStoreInformation(synce::STORE_INFORMATION *store)
    {
        memset(store, 0, sizeof(store));
        return synce::CeGetStoreInformation(store);
    }


    static bool closeHandle(Qt::HANDLE hObject)
    {
        return synce::CeCloseHandle(hObject);
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
