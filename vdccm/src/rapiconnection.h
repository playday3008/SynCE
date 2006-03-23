//
// C++ Interface: rapiconnection
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RAPICONNECTION_H
#define RAPICONNECTION_H
/**
    @author Volker Christian <voc@users.sourceforge.net>
*/

#include <string>

class RapiHandshakeClient;
class RapiProvisioningClient;
class RapiServer;
class RapiClient;

class RapiConnection
{
public:
    RapiConnection( RapiServer *rapiServer, std::string deviceIpAddress );

    ~RapiConnection();
    void setHandshakeClient( RapiHandshakeClient *handshakeClient );
    void setProvisioningClient( RapiProvisioningClient *provisioningClient );
    void keepAlive();
protected:
    void disconnectFromServer();
    void disconnect( RapiClient * );

private:
    RapiHandshakeClient * _rapiHandshakeClient;
    RapiProvisioningClient * _rapiProvisioningClient;
    std::string deviceIpAddress;
    RapiServer *rapiServer;

friend class RapiClient;
};

#endif
