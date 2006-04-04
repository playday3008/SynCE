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
#ifndef RAPIHANDSHAKECLIENT_H
#define RAPIHANDSHAKECLIENT_H

#include "continousnode.h"
#include "rapiclient.h"

/**
	@author Richard van den Toorn <vdtoorn@users.sourceforge.net>
*/

class RapiConnection;

class RapiHandshakeClient : public RapiClient, public ContinousNode
{
public:
    RapiHandshakeClient(int fd, TCPServerSocket *tcpServerSocket);

    virtual ~RapiHandshakeClient();

    void keepAlive();
    void initiateProvisioningConnection();
    void setRapiConnection(RapiConnection *rapiConnection);


protected:

    virtual void event();
    virtual void shot();

private:
    int pendingPingRequests;
    RapiConnection *rapiConnection;
    uint32_t connectionCount;

    char *deviceName;
    unsigned char deviceGuid[0x10];
    uint32_t osVersionMajor;
    uint32_t osVersionMinor;
    uint32_t deviceVersion;
    uint32_t deviceProcessorType;
    uint32_t unknown1;
    uint32_t someOtherId;
    char plattformName[0x100];
    char modelName[0x100];
    uint32_t deviceId;

};

#endif
