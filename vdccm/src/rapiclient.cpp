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


bool RapiClient::readOnePackage(unsigned char **buffer)
{
    uint32_t length;
    size_t readLength;

    if ( read( getDescriptor(), &length, 4 ) <= 0 ) {
        return false;
    }
    cout << "Packagelength: " << hex << setw(6) << setfill('0') << length << endl;

    *buffer = new unsigned char[ length + 4 ];
    readLength = length;

    memcpy( *buffer, ( char * ) & length, 4 );
    if (readNumBytes( *buffer + 4, readLength ) < (ssize_t) readLength ) {
        return false;
    }

    return true;
}



void RapiClient::printPackage( string origin, unsigned char *buf, unsigned int maxLength )
{
    uint32_t length = *( uint32_t * ) buf;
    char lineBuf[ 8 ];

    std::cout << "Packageorigin: " << origin << std::endl;
    std::cout << "0x" << std::hex << std::setw( 8 ) << std::setfill( '0' ) << 0 << "  " << std::flush;
    for ( unsigned int i = 0; i < length + 4 && i < maxLength; i++ ) {
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
                std::cout << "0x" << std::hex << std::setw( 8 ) << std::setfill( '0' ) << i + 1 << "  " << std::flush;
            }
        } else if ( i + 1 == length + 4 || (i + 1) == maxLength) {
            int count = (i + 1) % 8;
            for (int n = 1; n < (8 - count) * 5 + 2; n++) {
                std::cout << " " << std::flush;
            }
            for (int n = 0; n < count; n++) {
                char out = ( isprint( lineBuf[ n ] ) ? lineBuf[ n ] : '.' );
                std::cout << out << std::flush;
            }
            std::cout << std::endl;
        }
    }
}
