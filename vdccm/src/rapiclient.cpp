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
}


RapiClient::~RapiClient()
{
}


#include <iostream>

using namespace std;

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


RapiConnection *RapiClient::getRapiConnection()
{
    return rapiConnection;
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


size_t RapiClient::readNumBytes(unsigned char *buffer, size_t numBytes)
{
    size_t totalBytes = 0;
    size_t nBytes = 0;
    unsigned char *bufptr = buffer;

    do {
        nBytes = read(getDescriptor(), bufptr, 768);
        if (nBytes == 0) {
            return 0;
        }
        bufptr += nBytes;
        totalBytes +=nBytes;
    } while (totalBytes < numBytes);

    return totalBytes;
}
