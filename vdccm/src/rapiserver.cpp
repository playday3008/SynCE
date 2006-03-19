//
// C++ Implementation: rapiserver
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "rapiserver.h"
#include "rapihandshakeclient.h"
#include "rapiprovisioningclient.h"
#include <synce_log.h>
#include <tcpacceptedsocket.h>
#include <errno.h>


RapiServer::RapiServer(u_int16_t port, string interfaceName)
    : TCPServerSocket(port, interfaceName)
{
  _rapiHandshakeClient = 0;
  _rapiProvisioningClient = 0;
}


RapiServer::~RapiServer()
{
}


#include <iostream>
void RapiServer::event()
{
    std::cout << "990-server-event()" << endl;
    TCPAcceptedSocket tcs = accept();
    if (tcs.isConnected()) {
      if( !_rapiHandshakeClient ) {
        _rapiHandshakeClient = new RapiHandshakeClient(tcs); 
      } else if( !_rapiProvisioningClient ) {
          _rapiProvisioningClient = new RapiProvisioningClient(tcs);
          _rapiHandshakeClient->keepAlive();
      }
    } else {
        synce_warning(strerror(errno));
    }
}
