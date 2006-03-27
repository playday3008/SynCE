//
// C++ Implementation: rapiproxy
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "rapiproxy.h"
#include "rapiconnection.h"
#include <localserversocket.h>
#include <multiplexer.h>

RapiProxy::RapiProxy(int fd, LocalServerSocket* serverSocket)
 : LocalAcceptedSocket(fd, serverSocket)
{
    (dynamic_cast<RapiConnection *>(getServerSocket()))->registerProxy(this);
    Multiplexer::self()->getReadManager()->add(this);
}


#include <iostream>
RapiProxy::~RapiProxy()
{
    Multiplexer::self()->getReadManager()->remove(this);
    shutdown();
}


void RapiProxy::event()
{
    (dynamic_cast<RapiConnection *>(getServerSocket()))->messageToDevice(this);
}


size_t RapiProxy::readNumBytes(unsigned char *buffer, size_t numBytes)
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

