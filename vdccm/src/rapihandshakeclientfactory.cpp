//
// C++ Implementation: rapihandshakeclientfactory
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "rapihandshakeclientfactory.h"
#include "rapihandshakeclient.h"

RapiHandshakeClientFactory::RapiHandshakeClientFactory()
 : TCPAcceptedSocketFactory()
{
}


RapiHandshakeClientFactory::~RapiHandshakeClientFactory()
{
}


TCPAcceptedSocket* RapiHandshakeClientFactory::socket(int fd, TCPServerSocket* serverSocket) const
{
    return new RapiHandshakeClient(fd, serverSocket);
}

