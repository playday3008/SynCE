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

#include <iostream>
#include "rapiconnection.h"

RapiClient::RapiClient(int fd, TCPServerSocket *tcpServerSocket)
    : TCPAcceptedSocket(fd, tcpServerSocket)
{
    Multiplexer::self()->getReadManager()->add(this);
    std::cout << "hier bin ich" << std::endl;
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

    rapiConnection->disconnect(this);

    delete this;
}


void RapiClient::setRapiConnection(RapiConnection *rapiConnection)
{
    this->rapiConnection = rapiConnection;
}


int RapiClient::readAll(char * buffer)
{
    int totalBytes = 0;
    int nBytes = 0;
    char* bufptr = buffer;
    do
    {
        nBytes = read(getDescriptor(), bufptr, 768 );
        if (nBytes == 0) {
            return 0;
        }
        bufptr += nBytes;
        totalBytes += nBytes;
    } while( nBytes == 768 );

    return totalBytes;
}
