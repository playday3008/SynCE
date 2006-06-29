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
#include "windowscedevicebase.h"

class DeviceManager;

/**
@author Volker Christian
*/

#define DCCM_PID_FILE           "dccm.pid"
#define DCCM_PORT               5679
#define DCCM_PING               0x12345678
#define DCCM_MAX_PACKET_SIZE  512
#define DCCM_MIN_PACKET_SIZE  0x24

class WindowsCEDevice : public TCPAcceptedSocket, WindowsCEDeviceBase {
public:
    WindowsCEDevice(int fd, TCPServerSocket *tcpServerSocket);
    void init(SynceSocket *synceSocket);

protected:
    ~WindowsCEDevice();

public:
    void ping();
    bool sendPassword(std::string password);
    void disconnect();
    bool isLocked() const;
    int getKey() const;
    std::string getDeviceAddress() const;
    std::string getDeviceClass() const;
    std::string getDeviceName() const;
    std::string getRealName() const;
    std::string getHardware() const;
    std::string getPassword() const;
    uint32_t getBuildNumber() const;
    uint32_t getOsVersion() const;
    uint32_t getProcessorType() const;
    uint32_t getPartnerId1() const;
    uint32_t getPartnerId2() const;
    uint32_t getPort() const;
    bool handleInfoMessage(uint32_t header);
    bool handlePasswordRequest(uint32_t header);
    bool handleEvent();

protected:
    void event(Descriptor::eventType et);
    virtual bool shutdown();
    static char* string_at(const char *buffer, size_t size, size_t offset);
    bool handlePasswordReply();

protected:
    SynceSocket *socket;
    int pingCount;
    bool passwordExpected;
    int key;
    std::string deviceName;
    std::string deviceClass;
    std::string hardware;
    std::string realName;
    uint16_t osVersion;
    uint16_t buildNumber;
    uint16_t processorType;
    uint32_t partnerId1;
    uint32_t partnerId2;
    bool deviceConnected;
    bool passwordPending;
    bool locked;

private:
    std::string password;
};

#endif
