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

#include <multiplexer.h>

using namespace std;

RapiConnection::RapiConnection(RapiProxyFactory *proxyFactory, string path, RapiServer *rapiServer, string deviceIpAddress) : LocalServerSocket(proxyFactory, path), _rapiHandshakeClient(NULL), deviceIpAddress(deviceIpAddress), rapiServer(rapiServer)
{
}


RapiConnection::~RapiConnection()
{
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

    if (_rapiHandshakeClient) {
        delete _rapiHandshakeClient;
    }

    Multiplexer::self()->getReadManager()->remove(this);

    shutdown();
}


void RapiConnection::disconnectFromServer()
{
    rapiServer->disconnect(deviceIpAddress);
}


void RapiConnection::setHandshakeClient(RapiHandshakeClient *handshakeClient)
{
    _rapiHandshakeClient = handshakeClient;
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
    }
}


void RapiConnection::proxyConnectionClosed(RapiProxyConnection *rapiProxyConnection)
{
    list<RapiProxyConnection*>::iterator it = find(rapiProxyConnections.begin(), rapiProxyConnections.end(), rapiProxyConnection);

    if (it != rapiProxyConnections.end()) {
        rapiProxyConnections.erase(it);
        delete *it;
    }
}


void RapiConnection::handshakeClientInitialized()
{
    listen();
    Multiplexer::self()->getReadManager()->add(this);
}



void RapiConnection::handshakeClientDisconnected()
{
    disconnectFromServer();
}


void RapiConnection::event()
{
    RapiProxy *rapiProxy = dynamic_cast<RapiProxy *>(accept());
    rapiProxies.push_back(rapiProxy);
    _rapiHandshakeClient->initiateProvisioningConnection();
}
