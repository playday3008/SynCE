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
#include "windowscedevice.h"
#include "cmdlineargs.h"
#include <descriptormanager.h>
#include <tcpserversocket.h>
#include <multiplexer.h>
#include <synce_log.h>
#include <cstdlib>
#include <cctype>

#include <devicemanager.h>

using namespace std;

WindowsCEDevice::WindowsCEDevice(int fd, TCPServerSocket *tcpServerSocket)
 : TCPAcceptedSocket( fd, tcpServerSocket )
{
    deviceConnected = false;
    passwordExpected = false;
    passwordPending = false;
    locked = false;
    pingCount = 0;
}

void WindowsCEDevice::init(SynceSocket *synceSocket) {
    this->socket = synceSocket;
    Multiplexer::self()->getReadManager()->add( this );
}

WindowsCEDevice::~WindowsCEDevice()
{
}


bool WindowsCEDevice::shutdown()
{
    bool ret = synce_socket_close(socket);
    synce_socket_free(socket);

    return ret;
}


void WindowsCEDevice::disconnect()
{
    synce_trace("Disconnect");

    Multiplexer::self()->getReadManager()->remove(this);

    if (deviceConnected) {
        DeviceManager::self()->removeConnectedDevice(this);
    }

    if (passwordPending) {
        DeviceManager::self()->removePasswordPendingDevice(this);
    }

    shutdown();

    delete this;
}


/*!
    \fn WindowsCEDevice::handleInfoMessage(uint32_t header)
 */
bool WindowsCEDevice::handleInfoMessage(uint32_t header)
{
    bool ret = true;
    char *buffer = NULL;

    synce_trace( "this is an information message" );
    buffer = new char[ header ];
    if ( buffer != NULL ) {
        if (synce_socket_read(socket, buffer, header)) {
						osVersion = letoh16( *( uint16_t* ) ( buffer + 0x04 ) ) & 0xFF;
            buildNumber = letoh16( *( uint16_t* ) ( buffer + 0x06 ) );
            processorType = letoh16( *( uint16_t* ) ( buffer + 0x08 ) );
            partnerId1 = letoh32( *( uint32_t* ) ( buffer + 0x10 ) );
            partnerId2 = letoh32( *( uint32_t* ) ( buffer + 0x14 ) );
            char *tmp = ( char * ) string_at( buffer, header, 0x18 );
            deviceName = tmp;
            realName = tmp;
            free(tmp);

            string::iterator it;
            for (it = deviceName.begin(); it != deviceName.end(); ++it) {
                *it = tolower(*it);
            }

            tmp = ( char * ) string_at( buffer, header, 0x1c );
            deviceClass = tmp;
            free(tmp);

            tmp = ( char * ) string_at( buffer, header, 0x20 );
            hardware = tmp;
            free(tmp);

            delete[] buffer;
            buffer = NULL;

            if (!locked) {
                DeviceManager::self()->addConnectedDevice(this);
                deviceConnected = true;
            }
        } else {
            delete[] buffer;
            ret = false;
        }
    } else {
        synce_error( "Failed to allocate %i (0x%08x) bytes", header, header );
        ret = false;
    }

    return ret;
}


/*!
    \fn WindowsCEDevice::handlePasswordRequest()
 */
bool WindowsCEDevice::handlePasswordRequest(uint32_t header)
{
    bool ret = true;

    synce_trace("this is an password challenge");
    key = header & 0xff;
    passwordExpected = true;
    locked = true;

    if (dataPending(2, 0)) {
        if (handleEvent()) { // Call handleEvent() recursive because we need the information package!
            if ( !CmdLineArgs::getPassword().empty() ) {
                if ( !sendPassword(CmdLineArgs::getPassword())) {
                    ret = false;
                }
            } else {
                passwordPending = true;
                if (!DeviceManager::self()->addPasswordPendingDevice(this)) {
                    ret = false;
                }
            }
        } else {
            ret = false;
        }
    } else {
        ret = false;
    }

    return ret;
}


/*!
    \fn WindowsCEDevice::handleEvent()
 */
bool WindowsCEDevice::handleEvent()
{
    bool ret = true;
    uint32_t header;

    if (synce_socket_read(socket, &header, sizeof(header)) > 0) {
        synce_trace("Header: %d", header);
        if ( header == 0 ) {
            synce_trace( "initialization package" );
        } else if ( header == DCCM_PING ) {
            synce_trace( "this is a ping reply" );
            pingCount = 0;
        } else if (header >= DCCM_MAX_PACKET_SIZE) {
            if (!handlePasswordRequest(header)) {
                synce_error("Could not handle password request");
                ret = false;
            }
        } else if ( header >= DCCM_MIN_PACKET_SIZE ) {
            if (!handleInfoMessage(header)) {
                synce_error("Could not handle info message");
                ret = false;
            }
        } else {
            synce_error( "Packet is smaller than expected" );
            ret = false;
        }
    } else {
        synce_error( "Failed to read header" );
        ret = false;
    }

    return ret;
}


void WindowsCEDevice::event(Descriptor::eventType /*et*/)
{
    if (!handleEvent()) {
        disconnect();
    }
}


/*!
    \fn WindowsCEDevice::handlePasswordReply()
 */
bool WindowsCEDevice::handlePasswordReply()
{
    bool result;
    uint8_t dummy;

    if (dataPending(2, 0)) {
        if (synce_password_recv_reply(socket, sizeof(result), &result)) {
            if (result) {
                synce_trace("Password accepted");
                if (sizeof(result) == 1) {
                    // We have to read the second byte of the password reply as we are c++ bool, which is 1 byte long instead the SynCE bool which is two byte long.
                    if (synce_socket_read(socket, &dummy, 1) <= 0) {
                        result = false;
                    }
                }
            }
        } else {
            synce_error("Faile to read password reply");
            result = false;
        }
    } else {
        result = false;
    }

    return result;
}


bool WindowsCEDevice::sendPassword(string password)
{
    bool ret = true;

    if (passwordExpected) {
        passwordExpected = false;
        this->password = password;
        if (synce_password_send(socket, password.c_str(), key)) {
            if (handlePasswordReply()) {
                sleep(1); //delay the connection report to the SynCE-Client
                          // - it seams WinCE needs some time to saddle down
                DeviceManager::self()->addConnectedDevice(this);
                deviceConnected = true;
            } else {
                DeviceManager::self()->passwordRejected(this);
                synce_error("Password rejected");
                ret = false;
            }
            if (passwordPending) {
                DeviceManager::self()->removePasswordPendingDevice(this);
                passwordPending = false;
            }
        } else {
            ret = false;
        }
    }

    return ret;
}


void WindowsCEDevice::ping()
{
    const uint32_t ping = htole32(DCCM_PING);

    if (synce_socket_write(socket, &ping, sizeof(ping))) {
        if (++pingCount == CmdLineArgs::getMissingPingCount()) {
            synce_error("%s disconnected due to %d missed pings", deviceName.c_str(), CmdLineArgs::getMissingPingCount());
            disconnect();
        }
    } else {
        synce_error("failed to send ping");
        disconnect();
    }
}


char* WindowsCEDevice::string_at(const char *buffer, size_t size, size_t offset)
{
    size_t string_offset = letoh32(*(uint32_t*)(buffer + offset));

    if (string_offset < size) {
        return synce::wstr_to_ascii((WCHAR*)(buffer + string_offset));
    } else {
        synce_error("String offset too large: 0x%08x", string_offset);
        return NULL;
    }
}



/*!
    \fn WindowsCEDevice::isLocked() const
 */
bool WindowsCEDevice::isLocked() const
{
    return locked;
}


/*!
    \fn WindowsCEDevice::getKey() const
 */
int WindowsCEDevice::getKey() const
{
    return key;
}


/*!
    \fn WindowsCEDevice::getDeviceAddress() const
 */
string WindowsCEDevice::getDeviceAddress() const
{
    return getRemoteAddress();
}


/*!
    \fn WindowsCEDevice::getDeviceClass() const
 */
string WindowsCEDevice::getDeviceClass() const
{
    return deviceClass;
}


/*!
    \fn WindowsCEDevice::getDeviceName() const
 */
string WindowsCEDevice::getDeviceName() const
{
    return deviceName;
}


/*!
    \fn WindowsCEDevice::getRealName() const
 */
string WindowsCEDevice::getRealName() const
{
    return realName;
}


/*!
    \fn WindowsCEDevice::getHardware() const
 */
string WindowsCEDevice::getHardware() const
{
    return hardware;
}


/*!
    \fn WindowsCEDevice::getPassword() const
 */
string WindowsCEDevice::getPassword() const
{
    return password;
}


/*!
    \fn WindowsCEDevice::getBuildNumber() const
 */
uint32_t WindowsCEDevice::getBuildNumber() const
{
    return buildNumber;
}


/*!
    \fn WindowsCEDevice::getOsVersion() const
 */
uint32_t WindowsCEDevice::getOsVersion() const
{
    return osVersion;
}


/*!
    \fn WindowsCEDevice::getProcessorType() const
 */
uint32_t WindowsCEDevice::getProcessorType() const
{
    return processorType;
}


/*!
    \fn WindowsCEDevice::getPartnerId1() const
 */
uint32_t WindowsCEDevice::getPartnerId1() const
{
    return partnerId1;
}


/*!
    \fn WindowsCEDevice::getPartnerId2() const
 */
uint32_t WindowsCEDevice::getPartnerId2() const
{
    return partnerId2;
}


/*!
    \fn WindowsCEDevice::getPort() const
 */
uint32_t WindowsCEDevice::getPort() const
{
    return getRemotePort();
}
