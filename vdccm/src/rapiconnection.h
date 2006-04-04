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
class RapiProxyConnection;

class RapiConnection : public LocalServerSocket
{
public:
    RapiConnection(RapiProxyFactory *proxyFactory, std::string path, RapiServer *rapiServer, std::string deviceIpAddress );

    ~RapiConnection();
    void setHandshakeClient( RapiHandshakeClient *handshakeClient );
    void addProvisioningClient( RapiProvisioningClient *provisioningClient );
    void handshakeClientInitialized();
    void proxyConnectionClosed(RapiProxyConnection *rapiProxyConnection);
    void handshakeClientDisconnected();
    virtual void event();

private:
    void disconnectFromServer();
    RapiHandshakeClient * rapiHandshakeClient;
    std::string deviceIpAddress;
    RapiServer *rapiServer;
    std::list<RapiProxy *> rapiProxies;
    std::list<RapiProxyConnection *> rapiProxyConnections;

friend class RapiHandshakeClient;
};

#endif
