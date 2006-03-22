//
// C++ Implementation: rapihandshakeclient
//
// Description:
//
//
// Author: Richard van den Toorn <vdtoorn@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "multiplexer.h"
#include "rapihandshakeclient.h"

#include <iostream>
using namespace std;

RapiHandshakeClient::RapiHandshakeClient(int fd, TCPServerSocket *tcpServerSocket)
    : RapiClient(fd, tcpServerSocket),
      ContinousNode( 30, 0 )
{
  _state = NoDataReceived;
}


RapiHandshakeClient::~RapiHandshakeClient()
{
}

void RapiHandshakeClient::keepAlive()
{
   Multiplexer::self()->getTimerNodeManager()->add(this);

  _state = KeepingAlive;
}

void RapiHandshakeClient::event()
{
  // buffer for read/write requests
  //
  char buffer[ 8192 ];
  memset( buffer, 0, sizeof(buffer) );

  if (readAll( buffer ) == 0) {
      disconnect();
      return;
  }

  if( _state == NoDataReceived )
  {
    // data not relevant, should be { 00, 00, 00, 00 }, just reply
    char response[4] = { 03, 00, 00, 00 };
    write( getDescriptor(), response, 4 );

    _state = Ping1Received;
  }
  else if( _state == Ping1Received )
  {
    // data not relevant, should be { 04, 00 ,00 ,00 }
    // nothing to reply, more data (info message) follows
    _state = Ping2Received;
  }
  else if( _state == Ping2Received )
  {
    // buffer contains info message
    // write 3 responses, response 1 is { 05, 00, 00, 00 }
    char response[4] = { 05, 00, 00, 00 };
    write( getDescriptor(), response, 4 );

    // response 2 is { 04, 00, 00, 00 }
    response[0] = 04;
    write( getDescriptor(), response, 4 );

    // response 3 is { 01, 00, 00, 00 }
    response[0] = 01;
    write( getDescriptor(), response, 4 );

    _state = InfoMessageReceived;
  }
  else if( _state == KeepingAlive )
  {
    printf( "alive: %02x %02x %02x %02x\n", buffer[0], buffer[1], buffer[2], buffer[3] );
  }
}

void RapiHandshakeClient::shot()
{
  char response[4] = { 01, 00, 00, 00 };
  write( getDescriptor(), response, 4 );
}
