//
// C++ Interface: rapihandshakeclientfactory
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RAPIHANDSHAKECLIENTFACTORY_H
#define RAPIHANDSHAKECLIENTFACTORY_H

#include <tcpacceptedsocketfactory.h>

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/
class RapiHandshakeClientFactory : public TCPAcceptedSocketFactory
{
public:
    RapiHandshakeClientFactory();

    ~RapiHandshakeClientFactory();

    virtual TCPAcceptedSocket* socket(int fd, TCPServerSocket* serverSocket) const;

};

#endif
