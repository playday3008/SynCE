//
// C++ Implementation: rapiconnection
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "rapiconnection.h"
#include "rapiserver.h"
#include "rapihandshakeclient.h"
#include "rapiprovisioningclient.h"
#include "rapiproxyfactory.h"
#include "rapiproxy.h"
#include "rapiproxyconnection.h"
#include "devicemanager.h"
#include "cmdlineargs.h"
#include "synce.h"
#include "synce_log.h"
#include <multiplexer.h>

using namespace std;

RapiConnection::RapiConnection(RapiProxyFactory *proxyFactory, string path, RapiServer *rapiServer, string deviceIpAddress) : LocalServerSocket(proxyFactory, path), rapiHandshakeClient(NULL), deviceIpAddress(deviceIpAddress), rapiServer(rapiServer)
{
    passwordExpected = false;
    passwordPending = false;
    locked = false;
}


RapiConnection::~RapiConnection()
{
    rapiServer->disconnect(deviceIpAddress);
    DeviceManager::self()->removeConnectedDevice(this);

    list<RapiProxy *>::iterator it = rapiProxies.begin();
    while(it != rapiProxies.end()) {
        RapiProxy *rp = *it;
        rp->shutdown();
        it = rapiProxies.erase(it);
        delete rp;
    }

    list<RapiProxyConnection*>::iterator rit = rapiProxyConnections.begin();
    while(rit != rapiProxyConnections.end()) {
        RapiProxyConnection *rpc = *rit;
        rit = rapiProxyConnections.erase(rit);
        delete rpc;
    }

    if (rapiHandshakeClient) {
        delete rapiHandshakeClient;
    }

    Multiplexer::self()->getReadManager()->remove(this);

    shutdown();
}


void RapiConnection::setHandshakeClient(RapiHandshakeClient *handshakeClient)
{
    rapiHandshakeClient = handshakeClient;
    handshakeClient->setRapiConnection(this);
}


void RapiConnection::addProvisioningClient(RapiProvisioningClient *provisioningClient)
{
    if (rapiProxies.begin() != rapiProxies.end()) {
        RapiProxy *rapiProxy = *rapiProxies.begin();
        rapiProxies.remove(rapiProxy);
        RapiProxyConnection *rapiProxyConnection = new RapiProxyConnection(this, rapiProxy, provisioningClient);
        rapiProxyConnections.push_back(rapiProxyConnection);
    } else {
        provisioningClient->shutdown();
        delete provisioningClient;
        delete this;
    }
}


void RapiConnection::proxyConnectionClosed(RapiProxyConnection *rapiProxyConnection)
{
    list<RapiProxyConnection*>::iterator it = find(rapiProxyConnections.begin(), rapiProxyConnections.end(), rapiProxyConnection);

    if (it != rapiProxyConnections.end()) {
        RapiProxyConnection *rpc = *it;
        rapiProxyConnections.erase(it);
        delete rpc;
    }
}


void RapiConnection::handshakeClientInitialized(unsigned char *buffer)
{
    unsigned int length = letoh32(*((uint32_t *) buffer));

    int deviceGuidOffset = 0x04;
    int deviceGuidLength = 0x10;
    memcpy(deviceGuid, buffer + deviceGuidOffset, deviceGuidLength);
    if (CmdLineArgs::getLogLevel() >= 3) {
        for (int i = 0; i < deviceGuidLength; i++) {
            fprintf(stderr, "-%0X", deviceGuid[i]);
        }
        printf("-\n");
    }

    int osVersionMajorOffset = deviceGuidOffset + deviceGuidLength;
    osVersionMajor = letoh32(*(uint32_t *)(buffer + osVersionMajorOffset));
    int osVersionMinorOffset = osVersionMajorOffset + sizeof(uint32_t);
    osVersionMinor = letoh32(*(uint32_t *)(buffer + osVersionMinorOffset));
    synce_info("OSVersion: %d.%d", osVersionMajor, osVersionMinor);

    int deviceNameLengthOffset = osVersionMinorOffset + sizeof(uint32_t);
    uint32_t deviceNameLength = letoh32(*(uint32_t *) (buffer + deviceNameLengthOffset));

    int deviceNameOffset = deviceNameLengthOffset + sizeof(uint32_t);
    deviceName = synce::wstr_to_ascii((WCHAR *)(buffer + deviceNameOffset));
    synce_info("DeviceName: %s", deviceName.c_str());

    int deviceVersionOffset = deviceNameOffset + (deviceNameLength + 1) * sizeof(WCHAR);
    deviceVersion = letoh32(*(uint32_t *) (buffer + deviceVersionOffset));
    synce_info("DeviceVersion: %p", deviceVersion);

    int deviceProcessorOffset = deviceVersionOffset + sizeof(uint32_t);
    deviceProcessorType = letoh32(*(int32_t *) (buffer + deviceProcessorOffset));
    synce_info("DeviceProzessorType: %p", deviceProcessorType);

    int unknown1Offset = deviceProcessorOffset + sizeof(uint32_t);
    unknown1 = letoh32(*(int32_t *) (buffer + unknown1Offset));
    synce_info("Unknown1: %p", unknown1);

    int currentPartnerIdOffset = unknown1Offset + sizeof(uint32_t);
    currentPartnerId = letoh32(*(int32_t *) (buffer + currentPartnerIdOffset));
    synce_info("CurrentPartnerId: %p", currentPartnerId);

    int deviceIdOffset = currentPartnerIdOffset + sizeof(uint32_t);
    deviceId = letoh32(*(int32_t *) (buffer + deviceIdOffset));
    synce_info("DeviceId: %p", deviceId);

    int plattformNameLengthOffset = deviceIdOffset + sizeof(uint32_t);
    int plattformNameLength = letoh32(*(int32_t *) (buffer + plattformNameLengthOffset));

    int plattformNameOffset = plattformNameLengthOffset + sizeof(uint32_t);
    plattformName = string((char *) (buffer + plattformNameOffset));
    synce_info("PlattformName: %s", plattformName.c_str());

    int modelNameLengthOffset = plattformNameOffset + plattformNameLength;

    int modelNameOffset = modelNameLengthOffset + sizeof(uint32_t);
    modelName = string((char *) (buffer + modelNameOffset));
    synce_info("ModelName: %s", modelName.c_str());

    key = *((uint32_t *) (buffer + length));
    synce_info("Key: 0x%08x", key);

    if (key != 0) {
        passwordExpected = true;
        locked = true;

        if ( !CmdLineArgs::getPassword().empty() ) {
            synce_info("sending password");
            sendPassword(CmdLineArgs::getPassword());
        } else {
            synce_info("waiting for password");

            passwordPending = true;
            if (!DeviceManager::self()->addPasswordPendingDevice(this)) {
                synce_error("failed to add device to list of password-pending devices");
            }
        }
    } else {
        listen();
        Multiplexer::self()->getReadManager()->add(this);
        DeviceManager::self()->addConnectedDevice(this);
    }

    /*
    int unknown2Offset = modelNameOffset + modelNameLength;
    int unknonw2Length = 0x48;
    */
}


void RapiConnection::handshakeClientDisconnected()
{
    delete this;
}


void RapiConnection::event(Descriptor::eventType /*et*/)
{
    RapiProxy *rapiProxy = dynamic_cast<RapiProxy *>(accept());
    rapiProxies.push_back(rapiProxy);
    rapiHandshakeClient->initiateProvisioningConnection();
}


/*========================================================================*/
std::string RapiConnection::getDeviceAddress() const
{
    return rapiHandshakeClient->getRemoteAddress();
}


std::string RapiConnection::getDeviceClass() const
{
    return plattformName;
}


std::string RapiConnection::getDeviceName() const
{
    return deviceName;
}


std::string RapiConnection::getRealName() const
{
    return getDeviceName();
}


std::string RapiConnection::getHardware() const
{
    return modelName;
}


std::string RapiConnection::getPassword() const
{
    return password;
}


uint32_t RapiConnection::getBuildNumber() const
{
    return deviceVersion;
}


uint32_t RapiConnection::getOsVersion() const
{
    return osVersionMajor;
}


uint32_t RapiConnection::getProcessorType() const
{
    return deviceProcessorType;
}


uint32_t RapiConnection::getPartnerId1() const
{
    return 0;
}


uint32_t RapiConnection::getPartnerId2() const
{
    return 0;
}


uint32_t RapiConnection::getPort() const
{
    return rapiHandshakeClient->getRemotePort();
}


bool RapiConnection::isLocked() const
{
    return locked;
}


int RapiConnection::getKey() const
{
    return key;
}


void RapiConnection::disconnect()
{
    synce_trace("disconnect");

    if (passwordPending) {
        DeviceManager::self()->removePasswordPendingDevice(this);
    }

    delete this;
}


void RapiConnection::ping()
{
    rapiHandshakeClient->shot();
}

bool RapiConnection::sendPassword(string password)
{
    if (!passwordExpected)
        return true;

    passwordExpected = false;

    bool ret = false;

    this->password = password;

    int encodedPasswordSize = 2 * password.size();

    // Send the length
    synce_trace("sending length");
    uint16_t size_le = htole16(encodedPasswordSize);
    int len = rapiHandshakeClient->writeNumBytes((unsigned char *) &size_le, sizeof(size_le));
    if (len != sizeof(size_le))
        return ret;

    // Then the password
    unsigned char *encodedPassword = (unsigned char *) synce::wstr_from_utf8(password.c_str());

    for (int i = 0; i < encodedPasswordSize; i++) {
        encodedPassword[i] ^= key;
    }

    synce_trace("sending encoded password");
    len = rapiHandshakeClient->writeNumBytes(encodedPassword, encodedPasswordSize);

    synce::wstr_free_string(encodedPassword);

    if (len != encodedPasswordSize)
        return ret;

    synce_trace("waiting for response");
    // Read the response, 2s timeout
    if (!rapiHandshakeClient->dataPending(2, 0)) {
        synce_trace("no response");
        return ret;
    }

    synce_trace("reading response");
    uint16_t result;
    result = rapiHandshakeClient->readNumBytes((unsigned char *) &result, sizeof(result));
    if (result != sizeof(result))
        return ret;

    result = letoh16(result);

    if (result != 0) {
        synce_info("password accepted");

        ret = true;

        sleep(1); //delay the connection report to the SynCE client
                  // - it seems WinCE needs some time to saddle down

        listen();
        Multiplexer::self()->getReadManager()->add(this);
        DeviceManager::self()->addConnectedDevice(this);

        if (passwordPending) {
            DeviceManager::self()->removePasswordPendingDevice(this);
            passwordPending = false;
        }
    } else {
        DeviceManager::self()->passwordRejected(this);
        synce_error("password rejected");
    }

    return ret;
}

