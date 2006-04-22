//
// C++ Implementation: rapiproxyfactory
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "rapiproxyfactory.h"
#include "rapiproxy.h"

RapiProxyFactory::RapiProxyFactory()
 : LocalAcceptedSocketFactory()
{
}


RapiProxyFactory::~RapiProxyFactory()
{
}


LocalAcceptedSocket* RapiProxyFactory::socket(int fd, LocalServerSocket* serverSocket) const
{
    return new RapiProxy(fd, serverSocket);
}
