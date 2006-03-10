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
#include "rapiclient.h"
#include <synce_log.h>
#include <tcpacceptedsocket.h>
#include <errno.h>

RapiServer::RapiServer(u_int16_t port, string interfaceName)
    : TCPServerSocket(port, interfaceName)
{
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
        new RapiClient(tcs);
    } else {
        synce_warning(strerror(errno));
    }
}
