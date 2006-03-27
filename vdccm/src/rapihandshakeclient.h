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
class RapiHandshakeClient : public RapiClient, public ContinousNode
{
public:
    RapiHandshakeClient(int fd, TCPServerSocket *tcpServerSocket);

    virtual ~RapiHandshakeClient();

    void keepAlive();

protected:

    virtual void event();
    virtual void shot();

private:

    enum State {
      NoDataReceived = 0,
      Ping1Received,
      Ping2Received,
      InfoMessageReceived,
      KeepingAlive
    };

    State _state;
    int pendingPingRequests;
};

#endif
