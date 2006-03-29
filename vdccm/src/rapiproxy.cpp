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


RapiProxy::RapiProxy(int fd, LocalServerSocket* serverSocket)
 : LocalAcceptedSocket(fd, serverSocket)
{
    Multiplexer::self()->getReadManager()->add(this);
}


RapiProxy::~RapiProxy()
{
    Multiplexer::self()->getReadManager()->remove(this);
    shutdown();
}


void RapiProxy::setRapiProxyConnection(RapiProxyConnection *rapiProxyConnection)
{
    this->rapiProxyConnection = rapiProxyConnection;
}


void RapiProxy::event()
{
    rapiProxyConnection->messageToDevice();
}
