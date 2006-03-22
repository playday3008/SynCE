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
#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <list>
#include <string>

#include <continousnode.h>
#include "connectionfilemanager.h"

using namespace std;

class WindowsCEDevice;
class SynCEClient;

/**
@author Volker Christian
*/
class DeviceManager : public ContinousNode {
public:

    static DeviceManager *self();
    ~DeviceManager();

    void addConnectedDevice(WindowsCEDevice * windowsCEDevice);
    void removeConnectedDevice(WindowsCEDevice * windowsCEDevice);
    bool addPasswordPendingDevice(WindowsCEDevice * windowsCEDevice);
    void removePasswordPendingDevice(WindowsCEDevice * windowsCEDevice);
    void passwordRejected(WindowsCEDevice * windowsCEDevice);
    void addClient(SynCEClient * synCEClient);
    void removeClient(SynCEClient * synCEClient);
    WindowsCEDevice *getConnectedDevice(string name);
    WindowsCEDevice *getPasswordPendingDevice(string name);
    void shutdownDevices();
    void shutdownClients();
    void shutdown();
    void setAsDefaultDevice(string name);

protected:
    virtual void shot();

private:
    DeviceManager();
    list<WindowsCEDevice *> connectedDevices;
    list<WindowsCEDevice *> passwordPendingDevices;
    list<SynCEClient *> connectedClients;
    static DeviceManager *deviceManager;

protected:

    ConnectionFileManager connectionFileManager;
};

#endif
