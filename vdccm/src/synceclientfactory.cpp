//
// C++ Implementation: synceclientfactory
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "synceclientfactory.h"
#include "synceclient.h"

SynCEClientFactory::SynCEClientFactory()
 : LocalAcceptedSocketFactory()
{
}


SynCEClientFactory::~SynCEClientFactory()
{
}


LocalAcceptedSocket* SynCEClientFactory::socket(int fd, LocalServerSocket* localServerSocket) const
{
    return new SynCEClient(fd, localServerSocket);
}
