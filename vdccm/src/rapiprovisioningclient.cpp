//
// C++ Implementation: rapiprovisioningclient
//
// Description:
//
//
// Author: Richard van den Toorn <vdtoorn@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "rapiprovisioningclient.h"
#include "rapimessages.h"

#include <synce.h>

using namespace RapiMessages;
using namespace synce;

#include <iostream>

RapiProvisioningClient::RapiProvisioningClient(int fd, TCPServerSocket *tcpServerSocket)
    : RapiClient(fd, tcpServerSocket)
{
  _state = NoDataReceived;
}

RapiProvisioningClient::~RapiProvisioningClient()
{
}

void RapiProvisioningClient::event( void )
{
  // buffer for read/write requests
  //
  char buffer[ 8192 ];
  memset( buffer, 0, sizeof(buffer) );
  int nBytes = readAll( buffer );

  if (nBytes == 0) {
      disconnect();
      return;
  }

  std::cout << "nbytes=" << nBytes << std::endl;

  if( _state == NoDataReceived )
  {
    // data not relevant, should be { 01, 00, 00, 00 },
    // reply with some unknown packet from the RapiMessage namespace
    write( getDescriptor(), unknownPacket1, sizeof(unknownPacket1) );

    _state = Ping1Received;
  }
  else if( _state == Ping1Received )
  {
    unsigned char packet[278];
    makePacket( proxyEntriesPrefix, proxyEntriesString, packet );
    write( getDescriptor(), packet, sizeof(packet) );

    _state = State2;
  }
  else if( _state == State2 )
  {
    unsigned char packet[688];
    makePacket( proxyEntries2Prefix, proxyEntries2String, packet );
    write( getDescriptor(), packet, sizeof(packet) );

    _state = State3;
  }
  else if( _state == State3 )
  {
    unsigned char packet[472];
    makePacket( netEntriesPrefix, netEntriesString, packet );
    write( getDescriptor(), packet, sizeof(packet) );

    _state = State4;
  }
  else if( _state == State4 )
  {
    unsigned char packet[478];
    makePacket( syncQueryPrefix, syncQueryString, packet );
    write( getDescriptor(), packet, sizeof(packet) );

    _state = State5;
  }
  else if( _state == State5 )
  {
    unsigned char packet[478];
    makePacket( syncSourcesQueryPrefix, syncSourcesQueryString, packet );
    write( getDescriptor(), packet, sizeof(packet) );

    _state = State6;
  }
  else if( _state == State6 )
  {
    unsigned char packet[118] = { 0x72, 0x00, 0x00, 0x00,
                                  0x31, 0x00, 0x00, 0x00,
                                  0x02, 0x00, 0x00, 0x80,
                                  0x60, 0x00, 0x00, 0x00 };

    const char* string = "Software\\Microsoft\\Windows CE Services\\Partners";
    const unsigned char* unicode = (unsigned char*)wstr_from_ascii( string );
    const int sizeofUnicode =  2 * (strlen( string ) + 1);
    memcpy( packet + 18, unicode, sizeofUnicode );

    // append even more..?
    memcpy( packet + 18 + sizeofUnicode, "\02\00\00\00\00\00", 6 );
    write( getDescriptor(), packet, sizeof(packet) );

    _state = State7;
  }
  else if( _state == State7 )
  {
    const unsigned char packet[] = { 0x18, 0x00, 0x00, 0x00,
                                     0x31, 0x00, 0x00, 0x00,
                                     0xc0, 0x42, 0x0b, 0x04,
                                     0x06, 0x00, 0x00, 0x00,
                                     0x50, 0x00, 0x31, 0x00,
                                     0x00, 0x00, 0x02, 0x00,
                                     0x00, 0x00, 0x00, 0x00 };

    write( getDescriptor(), packet, sizeof(packet) );

    _state = State8;
  }
  else if( _state == State8 )
  {
    const char packet[] = { 0x1c, 0x00, 0x00, 0x00,
                            0x37, 0x00, 0x00, 0x00,
                            0x20, 0x43, 0x0b, 0x04,
                            0x0c, 0x00, 0x00, 0x00,
                            0x50, 0x00, 0x4e, 0x00,
                            0x61, 0x00, 0x6d, 0x00,
                            0x65, 0x00, 0x00, 0x00,
                            0x08, 0x02, 0x00, 0x00 };

    write( getDescriptor(), packet, sizeof(packet) );

    _state = State9;
  }
}

void RapiProvisioningClient::makePacket( const unsigned char* prefix,
                                        const char* string,
                                        unsigned char * packet )
{
  const int sizeofPrefix = 12; // always ?
  const int sizeofUnicode = 2 * (strlen(string) + 1);
  const unsigned char* unicode = (unsigned char *)wstr_from_ascii( string );
  const unsigned char  ping[4] = { 01, 00, 00, 00 };

  memcpy( packet, prefix, sizeofPrefix );
  memcpy( packet + sizeofPrefix, unicode, sizeofUnicode );
  memcpy( packet + sizeofPrefix + sizeofUnicode, ping, sizeof(ping) );
}
