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

#include "rapiclient.h"

/**
	@author Richard van den Toorn <vdtoorn@users.sourceforge.net>
*/

class RapiConnection;

class RapiHandshakeClient : public RapiClient
{
public:
    RapiHandshakeClient(int fd, TCPServerSocket *tcpServerSocket);

    virtual ~RapiHandshakeClient();

    void initiateProvisioningConnection();
    void setRapiConnection(RapiConnection *rapiConnection);


protected:

    virtual void event(Descriptor::eventType et);
    virtual void shot();

private:
    int pendingPingRequests;
    RapiConnection *rapiConnection;
    uint32_t connectionCount;

    /*
    std::string deviceName;
    unsigned char deviceGuid[0x10];
    uint32_t osVersionMajor;
    uint32_t osVersionMinor;
    uint32_t deviceVersion;
    uint32_t deviceProcessorType;
    uint32_t unknown1;
    uint32_t someOtherId;
    std::string plattformName;
    std::string modelName;
    uint32_t deviceId;
    */

    friend class RapiConnection;
};

#endif
