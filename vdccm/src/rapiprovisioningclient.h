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
#include <list>

/**
	@author Richard van den Toorn <vdtoorn@users.sourceforge.net>
*/

class RapiProxy;

class RapiProvisioningClient : public RapiClient
{
public:
    RapiProvisioningClient(int fd, TCPServerSocket *tcpServerSocket);

    ~RapiProvisioningClient();

    bool forwardBytes(RapiProxy *rapiProxy);

protected:

      virtual void event();

private:

    // make a packet of bytes from the rapimessages namespace, return size of packet
    void makePacket( const unsigned char* prefix, const char* string, unsigned char* buffer );

    enum State {
      NoDataReceived = 0,
      Ping1Received,
      State2,
      State3,
      State4,
      State5,
      State6,
      State7,
      State8,
      State9,
      State10
    };

    State _state;
    std::list<RapiProxy *> rapiProxies;
    unsigned int mtuWH;
};

#endif
