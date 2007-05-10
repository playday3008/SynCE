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
#include "connectionfilemanager.h"
#include "windowscedevicebase.h"
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <synce_log.h>
#include "cmdlineargs.h"

using namespace synce;
using namespace std;

ConnectionFileManager::ConnectionFileManager()
{
}


ConnectionFileManager::~ConnectionFileManager()
{
}




/*!
    \fn ConnectionFileManager::writeConnectionFile(const WindowsCEDevice *windowsCEDevice)
 */
bool ConnectionFileManager::writeConnectionFile(const WindowsCEDeviceBase *windowsCEDevice)
{
    char *path = NULL;

    string deviceName = (!CmdLineArgs::useIp()) ? windowsCEDevice->getDeviceName() : windowsCEDevice->getDeviceAddress();

    if (!synce_get_directory(&path)) {
        synce_error("Faild to get configuration directory name.");
        return false;
    }

    string fullPathName = string(path) + "/" + deviceName;

    free(path);

    bool ret = _writeConnectionFile(fullPathName, windowsCEDevice);

    if (ret) {
        ret = writeDefaultConnectionFile(windowsCEDevice);
    }

    return ret;
}

string ConnectionFileManager::getDefaultConnectionFileName()
{
    char *default_path_name;

    if (!synce_get_connection_filename(&default_path_name)) {
        synce_error("Unable to get connection filename");
        return "";
    }

    string defaultPathName = default_path_name;

    free(default_path_name);

    return defaultPathName;
}

/*!
    \fn ConnectionFileManager::writeDefaultConnectionFile(const WindowsCEDevice *windowsCEDevice)
 */
bool ConnectionFileManager::writeDefaultConnectionFile(const WindowsCEDeviceBase *windowsCEDevice, bool isNewDevice)
{
    bool ret = false;

    string defaultPathName = getDefaultConnectionFileName();

    if (!defaultPathName.empty()) {
        if (isNewDevice) {
            defaultDevices.push_back(windowsCEDevice);
        }

        ret = _writeConnectionFile(defaultPathName, windowsCEDevice);
    }

    return ret;
}


/*!
    \fn ConnectionFileManager::_writeConnectionFile(string fileName, const WindowsCEDevice *windowsCEDevice)
 */
bool ConnectionFileManager::_writeConnectionFile(string fileName, const WindowsCEDeviceBase *windowsCEDevice)
{
    synce_trace("Writing client-file: %s", fileName.c_str());

    ofstream connectionFile(fileName.c_str());

    if (connectionFile.good()) {
        connectionFile << "# Modifications to this file will be lost next time a client connects to dccm" << endl << endl;
        connectionFile << "[dccm]" << endl;
        connectionFile << "pid=" << getpid() << endl << endl;
        connectionFile << "[device]" << endl;
        connectionFile << "os_version=" << windowsCEDevice->getOsVersion() << endl;
        connectionFile << "build_bumber=" << windowsCEDevice->getBuildNumber() << endl;
        connectionFile << "processor_type=" << windowsCEDevice->getProcessorType() << endl;
        connectionFile << "partner_id_1=" << windowsCEDevice->getPartnerId1() << endl;
        connectionFile << "partner_id_2=" << windowsCEDevice->getPartnerId2() << endl;
        connectionFile << "name=" << windowsCEDevice->getDeviceName() << endl;
        connectionFile << "real_name=" << windowsCEDevice->getRealName() << endl;
        connectionFile << "class=" << windowsCEDevice->getDeviceClass() << endl;
        connectionFile << "hardware=" << windowsCEDevice->getHardware() << endl;
        connectionFile << "ip=" << windowsCEDevice->getDeviceAddress() << endl;
        connectionFile << "port=" << windowsCEDevice->getPort() << endl << endl;
        connectionFile << "[connection]" << endl;
        connectionFile << "transport=" << windowsCEDevice->getTransport() << endl;
        if (windowsCEDevice->isLocked()) {
            connectionFile << "password=" << windowsCEDevice->getPassword() << endl;
            connectionFile << "key=" << windowsCEDevice->getKey() << endl;
        }
    }

    bool ret = connectionFile.good();

    connectionFile.close();

    return ret;
}


/*!
    \fn ConnectionFileManager::removeConnectionFile(string fileName)
 */
bool ConnectionFileManager::removeConnectionFile(const WindowsCEDeviceBase *windowsCEDevice)
{
    char *path = NULL;

    string deviceName = (!CmdLineArgs::useIp()) ? windowsCEDevice->getDeviceName() : windowsCEDevice->getDeviceAddress();

    if (!synce_get_directory(&path)) {
        synce_error("Faild to get configuration directory name.");
        return false;
    }

    string fullPathName = string(path) + "/" + deviceName;

    free(path);

    bool ret1 = unlink(fullPathName.c_str()) == 0;

    bool ret2 = removeDefaultConnectionFile(windowsCEDevice);

    return ret1 && ret2;
}


/*!
    \fn ConnectionFileManager::removeDefaultConnectionFile(const WindowsCEDevice *windowsCEDevice)
 */
bool ConnectionFileManager::removeDefaultConnectionFile(const WindowsCEDeviceBase *windowsCEDevice)
{
    bool ret1 = true;

    bool unlinked = false;
    if (defaultDevices.back() ==  windowsCEDevice) {
        string defaultPathName = getDefaultConnectionFileName();
        if (!defaultPathName.empty()) {
            ret1 = unlink(defaultPathName.c_str()) == 0;
            unlinked = true;
        }
    }

    bool erased;
    do {
        erased = false;
        list<const WindowsCEDeviceBase *>::iterator it = find(defaultDevices.begin(), defaultDevices.end(), windowsCEDevice);

        if (it != defaultDevices.end()) {
            defaultDevices.erase(it);
            erased = true;
        }
    } while (erased);

    bool ret2 = true;
    if (unlinked) {
        if (defaultDevices.size() > 0) {
            ret2 = writeDefaultConnectionFile(defaultDevices.back(), false);
        }
    }

    return ret1 && ret2;
}
