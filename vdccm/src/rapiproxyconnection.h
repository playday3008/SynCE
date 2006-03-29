//
// C++ Interface: rapiproxyconnection
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RAPIPROXYCONNECTION_H
#define RAPIPROXYCONNECTION_H

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/
class RapiConnection;
class RapiProxy;
class RapiProvisioningClient;

class RapiProxyConnection{
public:
    RapiProxyConnection(RapiConnection *rapiConnection, RapiProxy *rapiProxy, RapiProvisioningClient *rapiProvisioningClient);

    ~RapiProxyConnection();

    void messageToDevice();
    void messageToApplication();
    void provisioningClientInitialized();
    void provisioningClientNotInitialized();

    private:
        RapiConnection *rapiConnection;
        RapiProxy *rapiProxy;
        RapiProvisioningClient *rapiProvisioningClient;
        unsigned int mtuWH;
        unsigned char *dirDeviceBuf;
        unsigned char *dirApplicationBuf;
};

#endif
