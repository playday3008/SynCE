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
#include <iomanip>
#include <errno.h>
#include "rapiconnection.h"

using namespace std;

RapiClient::RapiClient(int fd, TCPServerSocket *tcpServerSocket)
    : TCPAcceptedSocket(fd, tcpServerSocket)
{
    Multiplexer::self()->getReadManager()->add(this);
}


RapiClient::~RapiClient()
{
}


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
    int nBytes = 0;
    unsigned char *bufptr = buffer;

    do {
        nBytes = read(getDescriptor(), bufptr, 768);
        if (nBytes <= 0) {
            return 0;
        } else {
            bufptr += nBytes;
            totalBytes +=nBytes;
        }
    } while (totalBytes < numBytes);

    return totalBytes;
}


void RapiClient::printPackage( unsigned char *buf )
{
    uint32_t length = *( uint32_t * ) buf;
    char lineBuf[ 8 ];

    std::cout << "0x" << std::hex << std::setw( 4 ) << std::setfill( '0' ) << 0 << "  " << std::flush;
    for ( unsigned int i = 0; i < length + 4; i++ ) {
        std::cout << "0x" << std::hex << std::setw( 2 ) << std::setfill( '0' ) << ( int ) buf[ i ] << " " << std::flush;
        lineBuf[ i % 8 ] = buf[ i ];
        if ( ( i + 1 ) % 8 == 0 ) {
            std::cout << " " << std::flush;
            for ( int n = 0; n < 8; n++ ) {
                char out = ( isprint( lineBuf[ n ] ) ? lineBuf[ n ] : '.' );
                std::cout << out << std::flush;
            }
            std::cout << std::endl;
            if ( ( i + 1 ) < length + 4 ) {
                std::cout << "0x" << std::hex << std::setw( 4 ) << std::setfill( '0' ) << i + 1 << "  " << std::flush;
            }
        } else if ( i + 1 == length + 4 ) {
            std::cout << std::endl;
        }
    }
}
