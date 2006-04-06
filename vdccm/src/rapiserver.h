//
// C++ Interface: rapiserver
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RAPISERVER_H
#define RAPISERVER_H

#include <tcpserversocket.h>
#include <string.h>
#include <map>

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/

class RapiHandshakeClientFactory;
class RapiProvisioningClientFactory;

class RapiConnection;

class RapiServer : public TCPServerSocket
{
public:
    RapiServer(RapiHandshakeClientFactory *rhcf, RapiProvisioningClientFactory *rpcf, u_int16_t port = 990, std::string interfaceName = "");

    ~RapiServer();

    void event(Descriptor::eventType et);
    void disconnect(std::string deviceIpAddress);

private:
    RapiHandshakeClientFactory *rapiHandshakeClientFactory;
    RapiProvisioningClientFactory *rapiProvisioningClientFactory;

    std::map<std::string, RapiConnection*> rapiConnection;
};

#endif
