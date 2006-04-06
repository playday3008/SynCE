//
// C++ Interface: rapiclient
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RAPIPROVISIONINGCLIENT_H
#define RAPIPROVISIONINGCLIENT_H

#include "rapiclient.h"

/**
	@author Richard van den Toorn <vdtoorn@users.sourceforge.net>
*/

class RapiProxyConnection;

class RapiProvisioningClient : public RapiClient
{
public:
    RapiProvisioningClient(int fd, TCPServerSocket *tcpServerSocket);

    ~RapiProvisioningClient();

    void setRapiProxyConnection(RapiProxyConnection *rapiProxyConnection);

protected:

    virtual void event(Descriptor::eventType et);

private:
    bool initialized;
    RapiProxyConnection *rapiProxyConnection;
};

#endif
