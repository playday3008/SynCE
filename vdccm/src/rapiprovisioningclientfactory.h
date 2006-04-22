//
// C++ Interface: rapiprovisioningclientfactory
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RAPIPROVISIONINGCLIENTFACTORY_H
#define RAPIPROVISIONINGCLIENTFACTORY_H

#include <tcpacceptedsocketfactory.h>

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/
class RapiProvisioningClientFactory : public TCPAcceptedSocketFactory
{
public:
    RapiProvisioningClientFactory();

    ~RapiProvisioningClientFactory();

    virtual TCPAcceptedSocket* socket(int fd, TCPServerSocket* serverSocket) const;

};

#endif
