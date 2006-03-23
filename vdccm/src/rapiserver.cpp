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
#include "rapiconnection.h"
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
}


RapiServer::~RapiServer()
{
}


void RapiServer::disconnect(string deviceIpAddress)
{
    map<string, RapiConnection*>::iterator it = rapiConnection.find(deviceIpAddress);
    RapiConnection *rc = (*it).second;
    rapiConnection.erase(it);
    delete rc;
}


#include <iostream>
#include <arpa/inet.h>
void RapiServer::event()
{
    std::cout << "990-server-event()" << endl;

    int fd;

    fd = ::accept(getDescriptor(), NULL, NULL);
    std::cout << "Descriptor: " << fd << std::endl;

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    if (fd < 0) {
        synce_warning(strerror(errno));
        return;
    }

    if (getpeername(fd, (struct sockaddr *) &addr, &len) < 0) {
        synce_warning(strerror(errno));
        return;
    }

    char ip_str[16];
    if (!inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str))) {
        ip_str[0] = '\0';
    }

    if (rapiConnection[ip_str] == NULL) {
        std::cout << "RapiHandshakeClient" << std::endl;
        // Rapi Handshake Client
        rapiConnection[ip_str] = new RapiConnection(this, ip_str);
        rapiConnection[ip_str]->setHandshakeClient(dynamic_cast<RapiHandshakeClient *>(rapiHandshakeClientFactory->socket(fd, this)));
    } else {
        std::cout << "RapiProvisioningClient" << std::endl;
        // Rapi Provisioning Client
        rapiConnection[ip_str]->setProvisioningClient(dynamic_cast<RapiProvisioningClient *>(rapiProvisioningClientFactory->socket(fd, this)));
        rapiConnection[ip_str]->keepAlive();
    }
}
