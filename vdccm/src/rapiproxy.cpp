//
// C++ Implementation: rapiproxy
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "rapiproxy.h"
#include "rapiproxyconnection.h"
#include <multiplexer.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

RapiProxy::RapiProxy(int fd, LocalServerSocket* serverSocket)
 : LocalAcceptedSocket(fd, serverSocket)
{
    int value1;
    value1 = 1024;
    setsockopt(getDescriptor(), SOL_SOCKET, SO_SNDBUF, (char *) &value1, sizeof(int));
}


RapiProxy::~RapiProxy()
{
}


void RapiProxy::setRapiProxyConnection(RapiProxyConnection *rapiProxyConnection)
{
    this->rapiProxyConnection = rapiProxyConnection;
}


void RapiProxy::event(Descriptor::eventType et)
{
    switch(et) {
    case Descriptor::READ:
        rapiProxyConnection->messageToDevice();
        break;
    case Descriptor::WRITE:
        rapiProxyConnection->writeEnabled(this);
        break;
    case Descriptor::EXCEPTION:
        break;
    }
}
