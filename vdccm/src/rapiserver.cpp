//
// C++ Implementation: rapiserver
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "rapiserver.h"
#include "rapiclient.h"
#include "rapihandshakeclientfactory.h"
#include "rapiprovisioningclientfactory.h"
#include "rapihandshakeclient.h"
#include "rapiprovisioningclient.h"
#include <synce_log.h>
#include <tcpacceptedsocket.h>
#include <errno.h>

RapiServer::RapiServer(RapiHandshakeClientFactory *rhcf, RapiProvisioningClientFactory *rpcf, u_int16_t port, string interfaceName)
    : TCPServerSocket(NULL, port, interfaceName)
{
    rapiHandshakeClientFactory = rhcf;
    rapiProvisioningClientFactory = rpcf;
    _rapiHandshakeClient = NULL;
    _rapiProvisioningClient = NULL;
}


RapiServer::~RapiServer()
{
}


#include <iostream>
void RapiServer::event()
{
    std::cout << "990-server-event()" << endl;

    int fd;

    fd = ::accept(getDescriptor(), NULL, NULL);
    std::cout << "Descriptor: " << fd << std::endl;

    if (fd >= 0) {
        if (!_rapiHandshakeClient) {
            _rapiHandshakeClient =
                    dynamic_cast<RapiHandshakeClient *>(rapiHandshakeClientFactory->socket(fd, this));
        } else if( !_rapiProvisioningClient ) {
            _rapiProvisioningClient =
                    dynamic_cast<RapiProvisioningClient *>(rapiProvisioningClientFactory->socket(fd, this));
            _rapiHandshakeClient->keepAlive();
        }
    } else {
        synce_warning(strerror(errno));
    }
}
