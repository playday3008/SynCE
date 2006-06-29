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

    /*
    int unknown2Offset = modelNameOffset + modelNameLength;
    int unknonw2Length = 0x48;
    */

    listen();
    Multiplexer::self()->getReadManager()->add(this);
    DeviceManager::self()->addConnectedDevice(this);
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
    return string("");
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
    return false;
}


int RapiConnection::getKey() const
{
    return 0;
}


void RapiConnection::disconnect()
{
    delete this;
}


void RapiConnection::ping()
{
    rapiHandshakeClient->shot();
}
