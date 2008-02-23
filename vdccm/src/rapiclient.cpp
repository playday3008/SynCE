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
#include "cmdlineargs.h"

#include <multiplexer.h>

#include <iostream>
#include <iomanip>
#include <errno.h>
#include <synce.h>
#include <string.h>


using namespace std;

RapiClient::RapiClient( int fd, TCPServerSocket *tcpServerSocket )
        : TCPAcceptedSocket( fd, tcpServerSocket )
{
    Multiplexer::self() ->getReadManager() ->add
    ( this );
}


RapiClient::~RapiClient()
{}


bool RapiClient::readOnePackage(uint32_t length, unsigned char **buffer)
{
    size_t readLength;
    *buffer = new unsigned char[ length + 4 ];
    readLength = length;

    memcpy( *buffer, ( char * ) & length, 4 );
    if ( readNumBytes( *buffer + 4, readLength ) < ( ssize_t ) readLength ) {
        return false;
    }

    return true;
}


bool RapiClient::readOnePackage( unsigned char **buffer )
{
    uint32_t leLength;
    uint32_t length;

    if ( read( getDescriptor(), &leLength, 4 ) <= 0 ) {
        return false;
    }
    length = letoh32( leLength );

    return readOnePackage(length, buffer);
}



void RapiClient::printPackage( string origin, unsigned char *buf, unsigned int maxLength )
{
    if ( CmdLineArgs::getLogLevel() >= 4 ) {
        uint32_t length = letoh32( *( uint32_t * ) buf );
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
            } else if ( i + 1 == length + 4 || ( i + 1 ) == maxLength ) {
                int count = ( i + 1 ) % 8;
                for ( int n = 1; n < ( 8 - count ) * 5 + 2; n++ ) {
                    std::cout << " " << std::flush;
                }
                for ( int n = 0; n < count; n++ ) {
                    char out = ( isprint( lineBuf[ n ] ) ? lineBuf[ n ] : '.' );
                    std::cout << out << std::flush;
                }
                std::cout << std::endl;
            }
        }
    }
}
