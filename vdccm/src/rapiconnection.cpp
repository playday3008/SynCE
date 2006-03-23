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

#include <multiplexer.h>

using namespace std;

RapiConnection::RapiConnection(RapiProxyFactory *proxyFactory, string path, RapiServer *rapiServer, string deviceIpAddress) : LocalServerSocket(proxyFactory, path), _rapiHandshakeClient(NULL), _rapiProvisioningClient(NULL), deviceIpAddress(deviceIpAddress), rapiServer(rapiServer)
{
}


RapiConnection::~RapiConnection()
{
    list<RapiProxy *>::iterator it = rapiProxies.begin();
    while(it != rapiProxies.end()) {
        RapiProxy *rp = *it;
        it = rapiProxies.erase(it);
        delete rp;
    }

    Multiplexer::self()->getReadManager()->remove(this);

    shutdown();
}


void RapiConnection::disconnectFromServer()
{
    if (!_rapiHandshakeClient && !_rapiProvisioningClient) {
        rapiServer->disconnect(deviceIpAddress);
    }

    if (_rapiProvisioningClient) {
        _rapiProvisioningClient->disconnect();
    }
    if (_rapiHandshakeClient) {
        _rapiHandshakeClient->disconnect();
    }
}


void RapiConnection::disconnect(RapiClient *rapiClient)
{
    if (dynamic_cast<RapiHandshakeClient*>(rapiClient)) {
        _rapiHandshakeClient = NULL;
    } else if (dynamic_cast<RapiProvisioningClient*>(rapiClient)) {
        _rapiProvisioningClient = NULL;
    } else {
        return;
    }

    disconnectFromServer();
}


void RapiConnection::registerProxy(RapiProxy *rapiProxy)
{
    rapiProxies.push_back(rapiProxy);
}


void RapiConnection::setHandshakeClient(RapiHandshakeClient *handshakeClient)
{
    _rapiHandshakeClient = handshakeClient;
    handshakeClient->setRapiConnection(this);
}


RapiHandshakeClient *RapiConnection::getRapiHandshakeClient()
{
    return _rapiHandshakeClient;
}


void RapiConnection::setProvisioningClient(RapiProvisioningClient *provisioningClient)
{
    _rapiProvisioningClient = provisioningClient;
    provisioningClient->setRapiConnection(this);
}


RapiProvisioningClient *RapiConnection::getRapiProvisioningClient()
{
    return _rapiProvisioningClient;
}


void RapiConnection::keepAlive()
{
    _rapiHandshakeClient->keepAlive();
    listen();
    Multiplexer::self()->getReadManager()->add(this);
}

#include <iostream>
void RapiConnection::messageToDevice(RapiProxy *rapiProxy)
{
    // send message to device by forwarding the particular RapiProxy to the ...
    // RapiHandshakeClient or RapiProvisioningClient ... don't know which of them are
    // responsible for rapi-commands
    // return value of this communication should indicate a successfull/not successfull command
    // decide to disconnect the device/RapiProxy ...

    /*
    int ret = 0;
    if ( (ret = forwardRapiCommand(rapiProxy)) == RapiProxyClosedConnection) {
        rapiProxies.remove(rapiProxy);
        delete rapiProxy;
    } else if (ret == DeviceClosedConnection) {
        ... do nothing because device is disconnected automatically ...
    }
    */




    /*   test implementation -- simply read from socket and echo back */
    char buf[256];

    if (read(rapiProxy->getDescriptor(), buf, 256) == 0) {
        rapiProxies.remove(rapiProxy);
        delete rapiProxy;
        std::cout << "Deleted ProxyClient" << endl;
    } else {
        std::cout << "Proxy: " << buf << std::endl;
        write(rapiProxy->getDescriptor(), buf, strlen(buf));
    }
    /*   end test implementation */
}
