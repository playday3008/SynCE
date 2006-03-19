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
  // no implementation
  //
}


int RapiClient::readAll(char * buffer)
{
  int totalBytes = 0;
  int nBytes = 0;
  char* bufptr = buffer;
  do
  {
    nBytes = read(getDescriptor(), bufptr, 768 );
    bufptr += nBytes;
    totalBytes += nBytes;
  } while( nBytes == 768 );
                         
  return totalBytes;
}
