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

class RapiHandshakeClient;
class RapiProvisioningClient;
class RapiServer;
class RapiClient;
class RapiProxy;
class RapiProxyFactory;

class RapiConnection : public LocalServerSocket
{
public:
    RapiConnection(RapiProxyFactory *proxyFactory, std::string path, RapiServer *rapiServer, std::string deviceIpAddress );

    ~RapiConnection();
    void setHandshakeClient( RapiHandshakeClient *handshakeClient );
    void setProvisioningClient( RapiProvisioningClient *provisioningClient );
    void keepAlive();
    void messageToDevice(RapiProxy *rapiProxy);
    void registerProxy(RapiProxy *rapiProxy);
    RapiProvisioningClient *getRapiProvisioningClient();
    RapiHandshakeClient *getRapiHandshakeClient();

protected:
    void disconnectFromServer();
    void disconnect( RapiClient * );

private:
    RapiHandshakeClient * _rapiHandshakeClient;
    RapiProvisioningClient * _rapiProvisioningClient;
    std::string deviceIpAddress;
    RapiServer *rapiServer;
    std::list<RapiProxy *> rapiProxies;

friend class RapiClient;
};

#endif
