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

using namespace std;

RapiConnection::RapiConnection(RapiServer *rapiServer, string deviceIpAddress) : deviceIpAddress(deviceIpAddress), rapiServer(rapiServer)
{
}


RapiConnection::~RapiConnection()
{
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


void RapiConnection::setHandshakeClient(RapiHandshakeClient *handshakeClient)
{
    _rapiHandshakeClient = handshakeClient;
    handshakeClient->setRapiConnection(this);
}


void RapiConnection::setProvisioningClient(RapiProvisioningClient *provisioningClient)
{
    _rapiProvisioningClient = provisioningClient;
    provisioningClient->setRapiConnection(this);
}


void RapiConnection::keepAlive()
{
    _rapiHandshakeClient->keepAlive();
}
