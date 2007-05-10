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
#include "utils.h"

#include <signal.h>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <synce.h>
#include <synce_log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

using namespace std;
using namespace synce;

#define DCCM_PID_FILE           "dccm.pid"

bool Utils::_isRunning = true;
bool Utils::_disconnectDevices = false;

Utils::Utils()
{
}


Utils::~Utils()
{
}


bool Utils::alreadyRunning(bool noCheck)
{
    char *path;
    bool ret = false;

    if (!synce_get_directory(&path)) {
        synce_error("Faild to get configuration directory name.");
        ret = false;;
    } else {
        struct stat dummy;
        string pid_file = string(path) + "/" + DCCM_PID_FILE;
        if (0 == stat(pid_file.c_str(), &dummy)) {
            ifstream pidFile(pid_file.c_str());

            if (pidFile.good()) {
                string pid;
                pidFile >> pid;
                string dccmCmdLineFilePath = string("/proc/") + pid + "/cmdline";
                if (0 == stat(dccmCmdLineFilePath.c_str(), &dummy)) {
                    string cmdLine;
                    ifstream dccmCmdLineFile(dccmCmdLineFilePath.c_str());
                    if (dccmCmdLineFile.good()) {
                        dccmCmdLineFile >> cmdLine;
                        if (!strstr(cmdLine.c_str(), "dccm")) {
                            removePidFile();
                        } else {
                            synce_error("A (v)dccm is already running with PID %s.",
                                        pid.c_str());
                            ret = true;
                        }
                    } else {
                        synce_error("Could not read proc-entry %s. Dccm not started", dccmCmdLineFilePath.c_str());
                        ret = true;
                    }
                    dccmCmdLineFile.close();
                } else {
                    removePidFile();
                }
            } else {
                synce_error("Could not read %s: %s. Dccm not started", pid_file.c_str(), strerror(errno));
                ret = true;
            }
            pidFile.close();
        } else {
            if (!writePidFile()) {
                synce_error("Could not write ~/.synce/dccm.pid file. Check permission");
                ret = true;
            }
        }

        free(path);
    }

    return ret;
}


bool Utils::writePidFile()
{
    char *path;
    bool ret = true;
    string pid_file;

    if (!synce_get_directory(&path)) {
        synce_error("Faild to get configuration directory name.");
        ret = false;
    } else {
        pid_file = string(path) + "/" + DCCM_PID_FILE;
        unlink(pid_file.c_str());
        ofstream pidFile(pid_file.c_str());

        if (pidFile.good()) {
            pidFile << getpid();
        } else {
            synce_error("Failed to open %s for writing: %s", pid_file.c_str(), strerror(errno));
            ret = false;
        }

        pidFile.close();
        free(path);
    }


    return ret;
}


void Utils::removePidFile()
{
    char *path;

    if (!synce_get_directory(&path)) {
        synce_error("Faild to get configuration directory name.");
    } else {
        string pid_file = string(path) + "/" + DCCM_PID_FILE;
        if (unlink(pid_file.c_str()) < 0) {
            if (errno != ENOENT) {
                synce_error("Could not remove %s: %s", pid_file.c_str(), strerror(errno));
            }
        }
        free(path);
    }
}


void Utils::vdccm_handle_sighup(int n)
{
    _disconnectDevices = true;
}


void Utils::vdccm_handle_terminating_signals(int n)
{
    _isRunning = false;
}


void Utils::setupSignals()
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, vdccm_handle_sighup);
    signal(SIGTERM, vdccm_handle_terminating_signals);
    signal(SIGINT, vdccm_handle_terminating_signals);
    signal(SIGABRT, vdccm_handle_terminating_signals);
    signal(SIGQUIT, vdccm_handle_terminating_signals);
}


bool Utils::isRunning()
{
    return _isRunning;
}


bool Utils::disconnectDevices()
{
    bool ret = _disconnectDevices;
    _disconnectDevices = false;

    return ret;
}


void Utils::runScripts(string action, string deviceName)
{
    char* directory = NULL;
    DIR* dir = NULL;
    struct dirent* entry = NULL;

    if (!synce_get_script_directory(&directory)) {
        synce_error("Failed to get script directory");
        goto exit;
    }

    dir = opendir(directory);
    if (!dir) {
        synce_error("Failed to open script directory");
        goto exit;
    }

    while ((entry = readdir(dir)) != NULL) {
        char path[MAX_PATH];
        struct stat info;

        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        if (lstat(path, &info) < 0)
            continue;

        if (!(info.st_mode & S_IFREG))
            continue;

        synce_trace("Running script: %s %s", path, action.c_str());

        string command = string(path) + " " + action + " " + deviceName;

        system(command.c_str());
    }

exit:
    if (directory)
        free(directory);

    if (dir)
        closedir(dir);
}


bool Utils::dropRootPrivileg()
{
    bool ret = true;

    uid_t uid = getuid();
    if (seteuid(uid) == -1) {
        ret = false;
    }

    return ret;
}


bool Utils::acquireRootPrivileg()
{
    bool ret = true;

    if (seteuid(0) == -1) {
        ret = false;
    }

    return ret;
}


bool Utils::checkStartingUser(bool bypassRootCheck)
{
    bool ret = true;

    uid_t uid = getuid();
    uid_t euid = geteuid();

    // we should be started suid-root but not directly by root
    if ((uid == 0 && !bypassRootCheck) || euid != 0) {
        ret = false;
    }

    return ret;
}
