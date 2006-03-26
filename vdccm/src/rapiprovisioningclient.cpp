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

#include "rapiconnection.h"
#include "rapiprovisioningclient.h"
#include "rapimessages.h"
#include "rapiproxy.h"

#include <synce.h>

using namespace RapiMessages;
using namespace synce;

#include <iostream>
#include <iomanip>

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

    getRapiConnection()->provisioningClientReachedState9();
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

void RapiProvisioningClient::printPackage(unsigned char *buf)
{
    uint32_t length = *(uint32_t *) buf;
    char lineBuf[8];

    std::cout << "0x" << std::hex << std::setw(4) << std::setfill('0') << 0 << "  " << std::flush;
    for (unsigned int i = 0; i < length + 4; i++) {
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') <<  (int) buf[i] << " " << std::flush;
        lineBuf[i % 8] = buf[i];
        if ((i + 1) % 8 == 0) {
            std::cout << " " << std::flush;
            for (int n = 0; n < 8; n++) {
                char out = (isprint(lineBuf[n]) ? lineBuf[n] : '.');
                std::cout << out << std::flush;
            }
            std::cout << std::endl;
            if ((i + 1) < length + 4) {
                std::cout << "0x" << std::hex << std::setw(4) << std::setfill('0') << i + 1 << "  " << std::flush;
            }
        } else if (i + 1 == length + 4) {
            std::cout << std::endl;
        }
    }
}


#include <iostream>
bool RapiProvisioningClient::forwardBytes(RapiProxy *rapiProxy)
{
    uint32_t length;

    if (read(rapiProxy->getDescriptor(), &length, 4) <= 0) {
        return false;
    }

    unsigned char *buf = new unsigned char[length + 4];

    memcpy(buf, (char *) &length, 4);
    if (rapiProxy->readNumBytes(buf + 4, length) < length) {
        return false;
    }


    std::cout << "Application --> Device" << std::endl;
    std::cout << "======================" << std::endl;
    printPackage(buf);

    write(getDescriptor(), buf, length + 4);

    delete[] buf;

    if (read(getDescriptor(), &length, 4) <= 0) {
        return true;
    }

    buf = new unsigned char[length + 4];

    memcpy(buf, (unsigned char *) &length, 4);
    if (readNumBytes(buf + 4, length) < length) {
        return true;
    }

    std::cout << std::endl << "Device --> Application" << std::endl;
    std::cout << "======================" << std::endl;
    printPackage(buf);
    std::cout << "---------------------------------------------------------" << std::endl;

    write(rapiProxy->getDescriptor(), buf, length + 4);

    delete[] buf;

    return true;
}
