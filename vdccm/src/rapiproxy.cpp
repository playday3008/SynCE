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
#include "rapiconnection.h"
#include <localserversocket.h>
#include <multiplexer.h>

RapiProxy::RapiProxy(int fd, LocalServerSocket* serverSocket)
 : LocalAcceptedSocket(fd, serverSocket)
{
    (dynamic_cast<RapiConnection *>(getServerSocket()))->registerProxy(this);
    Multiplexer::self()->getReadManager()->add(this);
}


RapiProxy::~RapiProxy()
{
    Multiplexer::self()->getReadManager()->remove(this);
    shutdown();
}


void RapiProxy::event()
{
    (dynamic_cast<RapiConnection *>(getServerSocket()))->messageToDevice(this);
}
