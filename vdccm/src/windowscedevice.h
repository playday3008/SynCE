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
#ifndef WINDOWSCEDEVICE_H
#define WINDOWSCEDEVICE_H

#include <string>
#include <tcpacceptedsocket.h>
#include <synce.h>
#include <synce_socket.h>
#include <functional>

using namespace std;
using namespace synce;

class DeviceManager;

/**
@author Volker Christian
*/

#define DCCM_PID_FILE           "dccm.pid"
#define DCCM_PORT               5679
#define DCCM_PING               0x12345678
#define DCCM_MAX_PACKET_SIZE  512
#define DCCM_MIN_PACKET_SIZE  0x24

class WindowsCEDevice : public TCPAcceptedSocket {
public:
    WindowsCEDevice(int fd, TCPServerSocket *tcpServerSocket);
    void init(SynceSocket *synceSocket);

protected:
    ~WindowsCEDevice();

public:
    void ping();
    bool sendPassword(string password);
    void disconnect();
    bool operator==(const WindowsCEDevice &device) const { return this->deviceName == device.deviceName; }
    bool isLocked() const;
    int getKey() const;
    string getDeviceAddress() const;
    string getDeviceClass() const;
    string getDeviceName() const;
    string getHardware() const;
    string getPassword() const;
    uint16_t getBuildNumber() const;
    uint16_t getOsVersion() const;
    uint16_t getProcessorType() const;
    uint32_t getPartnerId1() const;
    uint32_t getPartnerId2() const;
    uint16_t getPort() const;
    bool handleInfoMessage(uint32_t header);
    bool handlePasswordRequest(uint32_t header);
    bool handleEvent();

    class EqualsTo : public unary_function<WindowsCEDevice *, bool> {
    public:
        EqualsTo(const WindowsCEDevice *device) : device(device) {}
        bool operator()(const WindowsCEDevice *device) {
            return (*this->device == *device);
        }
        const WindowsCEDevice *device;
    };

protected:
    void event();
    virtual bool shutdown();
    static char* string_at(const char *buffer, size_t size, size_t offset);
    bool handlePasswordReply();

protected:
    SynceSocket *socket;
    int pingCount;
    bool passwordExpected;
    int key;
    string deviceName;
    string deviceClass;
    string hardware;
    uint16_t osVersion;
    uint16_t buildNumber;
    uint16_t processorType;
    uint32_t partnerId1;
    uint32_t partnerId2;
    bool deviceConnected;
    bool passwordPending;
    bool locked;

private:
    string password;
};

#endif
