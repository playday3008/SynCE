//
// C++ Implementation: rapiprovisioningclientfactory
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "rapiprovisioningclientfactory.h"
#include "rapiprovisioningclient.h"

RapiProvisioningClientFactory::RapiProvisioningClientFactory()
 : TCPAcceptedSocketFactory()
{
}


RapiProvisioningClientFactory::~RapiProvisioningClientFactory()
{
}


TCPAcceptedSocket* RapiProvisioningClientFactory::socket(int fd, TCPServerSocket* serverSocket) const
{
    return new RapiProvisioningClient(fd, serverSocket);
}

