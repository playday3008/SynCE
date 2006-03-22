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

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/

class RapiHandshakeClientFactory;
class RapiProvisioningClientFactory;

class RapiHandshakeClient;
class RapiProvisioningClient;


using namespace std;

class RapiServer : public TCPServerSocket
{
public:
    RapiServer(RapiHandshakeClientFactory *rhcf, RapiProvisioningClientFactory *rpcf, u_int16_t port = 990, string interfaceName = "");

    ~RapiServer();

    void event();

private:
    RapiHandshakeClientFactory *rapiHandshakeClientFactory;
    RapiProvisioningClientFactory *rapiProvisioningClientFactory;

    RapiHandshakeClient * _rapiHandshakeClient;
    RapiProvisioningClient * _rapiProvisioningClient;
};

#endif
