//
// C++ Implementation: rapiclient
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "rapiclient.h"
#include <multiplexer.h>


RapiClient::RapiClient(const TCPAcceptedSocket & tcpAcceptedSocket)
    : TCPAcceptedSocket(tcpAcceptedSocket)
{
    Multiplexer::self()->getReadManager()->add(this);
}


RapiClient::~RapiClient()
{
}


#include <iostream>
void RapiClient::disconnect()
{
    cout << "990-client-disconnect()" << endl;

    Multiplexer::self()->getReadManager()->remove(this);

    shutdown();

    delete this;
}


void RapiClient::event()
{
    char buffer[ 256 ];
    int n;

    std::cout << "990-client-event()" << endl;
    if ( ( n = read( getDescriptor(), buffer, 256 ) ) > 0 ) {
        buffer[ n ] = '\0';
        std::cout << buffer << endl;
    } else {
        disconnect();
    }
}
