//
// C++ Interface: rapiconnection
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RAPICONNECTION_H
#define RAPICONNECTION_H
/**
    @author Volker Christian <voc@users.sourceforge.net>
*/

#include <string>
#include <list>
#include <localserversocket.h>
#include "windowscedevicebase.h"

class RapiHandshakeClient;
class RapiProvisioningClient;
class RapiServer;
class RapiClient;
class RapiProxy;
class RapiProxyFactory;
class RapiProxyConnection;

class RapiConnection : public LocalServerSocket, WindowsCEDeviceBase
{
public:
    RapiConnection(RapiProxyFactory *proxyFactory, std::string path, RapiServer *rapiServer, std::string deviceIpAddress );

    ~RapiConnection();
    void setHandshakeClient( RapiHandshakeClient *handshakeClient );
    void addProvisioningClient( RapiProvisioningClient *provisioningClient );
    void handshakeClientInitialized(unsigned char *buffer);
    void proxyConnectionClosed(RapiProxyConnection *rapiProxyConnection);
    void handshakeClientDisconnected();
    virtual void event(Descriptor::eventType et);

    void disconnect();
    void ping();

    bool sendPassword(std::string password);

    std::string getDeviceAddress() const;
    std::string getDeviceClass() const;
    std::string getDeviceName() const;
    std::string getRealName() const;
    std::string getHardware() const;
    std::string getPassword() const;
    std::string getTransport() const {
        return "rndis";
    };
    uint32_t getBuildNumber() const;
    uint32_t getOsVersion() const;
    uint32_t getProcessorType() const;
    uint32_t getPartnerId1() const;
    uint32_t getPartnerId2() const;
    uint32_t getPort() const;

    bool isLocked() const;
    int getKey() const;

private:
    void disconnectFromServer();
    RapiHandshakeClient * rapiHandshakeClient;
    std::string deviceIpAddress;
    RapiServer *rapiServer;
    std::list<RapiProxy *> rapiProxies;
    std::list<RapiProxyConnection *> rapiProxyConnections;

    std::string deviceName;
    unsigned char deviceGuid[0x10];
    uint32_t osVersionMajor;
    uint32_t osVersionMinor;
    uint32_t deviceVersion;
    uint32_t deviceProcessorType;
    uint32_t unknown1;
    uint32_t currentPartnerId;
    std::string plattformName;
    std::string modelName;
    uint32_t deviceId;
    uint32_t key;

    bool passwordExpected;
    bool passwordPending;
    bool locked;

    std::string password;

friend class RapiHandshakeClient;
};

#endif
