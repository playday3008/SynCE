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

#include <config.h>

#include "synceclient.h"
#include "devicemanager.h"
#include "windowscedevicebase.h"
#include "utils.h"
#include "cmdlineargs.h"

#include <algorithm>
#include <synce_log.h>

using namespace std;

DeviceManager *DeviceManager::deviceManager = NULL;

DeviceManager::DeviceManager()
 : ContinousNode(CmdLineArgs::getPingDelay(), 0)
{
}


DeviceManager::~DeviceManager()
{
}

void DeviceManager::init()
{
#ifdef ENABLE_DESKTOP_INTEGRATION
    eventManager = _vdccm_get_event_manager ();
#endif
}

DeviceManager *DeviceManager::self()
{
    if (deviceManager == NULL) {
        deviceManager = new DeviceManager();
    }

    return deviceManager;
}


void DeviceManager::addConnectedDevice(WindowsCEDeviceBase * windowsCEDevice)
{
    list<SynCEClient *>::iterator it;

    string deviceName = (!CmdLineArgs::useIp()) ? windowsCEDevice->getDeviceName() : windowsCEDevice->getDeviceAddress();

    if (find_if(connectedDevices.begin(), connectedDevices.end(), WindowsCEDeviceBase::EqualsTo(windowsCEDevice)) == connectedDevices.end()) {
#ifdef ENABLE_DESKTOP_INTEGRATION
        _vdccm_event_manager_device_connected(eventManager, windowsCEDevice);
#endif

        connectedDevices.push_back(windowsCEDevice);
        connectionFileManager.writeConnectionFile(windowsCEDevice);
        for (it = connectedClients.begin(); it != connectedClients.end(); ++it) {
            if (!(*it)->deviceConnected(deviceName)) {
                break;
            }
        }
        synce_info("Device connected: %s", deviceName.c_str());
    } else {
        // A device with same name is already known here!
        windowsCEDevice->disconnect();
    }
}


void DeviceManager::removeConnectedDevice(WindowsCEDeviceBase * windowsCEDevice)
{
    list<SynCEClient *>::iterator it;

    if (find_if(connectedDevices.begin(), connectedDevices.end(), WindowsCEDeviceBase::EqualsTo(windowsCEDevice)) != connectedDevices.end()) {
        string deviceName = (!CmdLineArgs::useIp()) ? windowsCEDevice->getDeviceName() : windowsCEDevice->getDeviceAddress();
        for (it = connectedClients.begin(); it != connectedClients.end(); ++it) {
            if (!(*it)->deviceDisconnected(deviceName)) {
                break;
            }
        }
#ifdef ENABLE_DESKTOP_INTEGRATION
        _vdccm_event_manager_device_disconnected(eventManager, windowsCEDevice);
#endif

        connectionFileManager.removeConnectionFile(windowsCEDevice);
        connectedDevices.remove(windowsCEDevice);
        synce_info("Device disconnected: %s", deviceName.c_str());
    }
}


bool DeviceManager::addPasswordPendingDevice(WindowsCEDeviceBase * windowsCEDevice)
{
    list<SynCEClient *>::iterator it;

    if (connectedClients.size() == 0) {
        return false;
    }

    if (find_if(passwordPendingDevices.begin(), passwordPendingDevices.end(), WindowsCEDeviceBase::EqualsTo(windowsCEDevice)) == passwordPendingDevices.end()) {
        string deviceName = (!CmdLineArgs::useIp()) ? windowsCEDevice->getDeviceName() : windowsCEDevice->getDeviceAddress();
        for (it = connectedClients.begin(); it != connectedClients.end(); ++it) {
            if (!(*it)->deviceRequestsPassword(deviceName)) {
                break;
            }
        }
        synce_info("Device pending for password: %s", deviceName.c_str());
        passwordPendingDevices.push_back(windowsCEDevice);
    } else {
        windowsCEDevice->disconnect();
        return false;
    }

    return true;
}


void DeviceManager::removePasswordPendingDevice(WindowsCEDeviceBase * windowsCEDevice)
{
    if (find_if(passwordPendingDevices.begin(), passwordPendingDevices.end(), WindowsCEDeviceBase::EqualsTo(windowsCEDevice)) != passwordPendingDevices.end()) {
        passwordPendingDevices.remove(windowsCEDevice);
        synce_info("Device no longer pending for password: %s", windowsCEDevice->getDeviceName().c_str());
    }
}


void DeviceManager::passwordRejected(WindowsCEDeviceBase * windowsCEDevice)
{
    list<SynCEClient *>::iterator it;

    string deviceName = (!CmdLineArgs::useIp()) ? windowsCEDevice->getDeviceName() : windowsCEDevice->getDeviceAddress();
    for (it = connectedClients.begin(); it != connectedClients.end(); ++it) {
        if (!(*it)->passwordRejected(deviceName)) {
            break;
        }
    }
    synce_info("Passord rejected: %s", deviceName.c_str());
}


void DeviceManager::addClient(SynCEClient * synCEClient)
{
    connectedClients.push_back(synCEClient);

    list<WindowsCEDeviceBase *>::iterator it;
    for (it = connectedDevices.begin(); it != connectedDevices.end(); ++it) {
        string deviceName = (!CmdLineArgs::useIp()) ? (*it)->getDeviceName() : (*it)->getDeviceAddress();
        if (!synCEClient->deviceConnected(deviceName)) {
            break;
        }
    }
    synce_info("SynCE-Client connected");
}


void DeviceManager::removeClient(SynCEClient * synCEClient)
{
    connectedClients.remove(synCEClient);
    synce_info("SynCE-Client disconnected");
}

#include <iostream>
using namespace std;
WindowsCEDeviceBase *DeviceManager::getConnectedDevice(string name)
{
    list<WindowsCEDeviceBase *>::iterator it;
    for (it = connectedDevices.begin(); it != connectedDevices.end(); ++it) {
        string deviceName = (!CmdLineArgs::useIp()) ? (*it)->getDeviceName() : (*it)->getDeviceAddress();
        if (deviceName == name) {
            return *it;
        }
    }

    return NULL;
}


WindowsCEDeviceBase *DeviceManager::getPasswordPendingDevice(string name)
{
    list<WindowsCEDeviceBase *>::iterator it;

    for (it = passwordPendingDevices.begin(); it != passwordPendingDevices.end(); ++it) {
        string deviceName = (!CmdLineArgs::useIp()) ? (*it)->getDeviceName() : (*it)->getDeviceAddress();
        if (deviceName == name) {
            return *it;
        }
    }

    return NULL;
}


void DeviceManager::shot()
{
    list<WindowsCEDeviceBase *>::iterator it = connectedDevices.begin();

    while(it != connectedDevices.end()) {
        WindowsCEDeviceBase *wced = *it;
        ++it;
        wced->ping();
    }
}


/*!
    \fn DeviceManager::shutdown()
 */
void DeviceManager::shutdownDevices()
{
    list<WindowsCEDeviceBase *>::iterator it = connectedDevices.begin();

    while(it != connectedDevices.end()) {
        WindowsCEDeviceBase *windowsCEDevice = *it;
        ++it;
        windowsCEDevice->disconnect();
    }
}


void DeviceManager::shutdownClients()
{
    list<SynCEClient *>::iterator it = connectedClients.begin();
    while(it != connectedClients.end()) {
        SynCEClient *synCEClient = *it;
        ++it;
        synCEClient->disconnect();
    }
}


void DeviceManager::shutdown()
{
    shutdownDevices();
    shutdownClients();
}


/*!
    \fn DeviceManager::setAsDefaultDevice(string name)
 */
void DeviceManager::setAsDefaultDevice(string name)
{
    for (list<WindowsCEDeviceBase *>::iterator it = connectedDevices.begin(); it != connectedDevices.end(); ++it) {
        string deviceName = (!CmdLineArgs::useIp()) ? (*it)->getDeviceName() : (*it)->getDeviceAddress();
        if (deviceName == name) {
            connectionFileManager.writeDefaultConnectionFile(*it);
            break;
        }
    }
    synce_info("Set as default device: %s", name.c_str());
}
