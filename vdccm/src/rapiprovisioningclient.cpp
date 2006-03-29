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
#include "cmdlineargs.h"

#include <iostream>
#include <synce.h>

using namespace RapiMessages;
using namespace synce;

RapiProvisioningClient::RapiProvisioningClient( int fd, TCPServerSocket *tcpServerSocket )
        : RapiClient( fd, tcpServerSocket )
{
    _state = NoDataReceived;
    setBlocking();
    setReadTimeout( 5, 0 );
    mtuWH = getMTU() - 40;
}

RapiProvisioningClient::~RapiProvisioningClient()
{}

void RapiProvisioningClient::event( void )
{
    if ( _state == NoDataReceived ) {
        std::cout << "State0" << std::endl;
        // 4Byte static conteng 0x01, 0x00, 0x00, 0x00
        unsigned char buffer[ 4 ];
        if ( readNumBytes( buffer, 4 ) != 4 ) {
            disconnect();
            return ;
        }
        printPackage( "RapiProvisioningClient", ( unsigned char * ) buffer, 4 );
        // data not relevant, should be { 01, 00, 00, 00 },
        unsigned char timePackage[ 24 ] = { 0x14, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00,
                                            0x80, 0xed, 0x50, 0x8a,
                                            0xb5, 0x3e, 0xc6, 0x01,
                                            0x00, 0x00, 0x00, 0x00,
                                            0x10, 0x27, 0x00, 0x00 };

        uint32_t *setTime = ( uint32_t * ) ( timePackage + 0x04 );
        uint32_t *lowDateTime = ( uint32_t * ) ( timePackage + 0x08 );
        uint32_t *highDateTime = ( uint32_t * ) ( timePackage + 0x0c );

        time_t t = time( NULL );
        struct tm *tim = gmtime( &t );

        TIME_FIELDS tf;
        tf.Year = 1900 + tim->tm_year;
        tf.Month = 1 + tim->tm_mon;
        tf.Day = tim->tm_mday;
        tf.Hour = tim->tm_hour;
        tf.Minute = tim->tm_min;
        tf.Second = tim->tm_sec;
        tf.Milliseconds = 0;
        tf.Weekday = 0;

        FILETIME ft;
        if ( !time_fields_to_filetime( &tf, &ft ) ) {
            printf( "Wired error\n" );
            exit( 0 );
        }

        *setTime = ( CmdLineArgs::getSyncClock() ) ? ( uint32_t ) 0x00000001 : ( uint32_t ) 0x00000000;
        *lowDateTime = ( uint32_t ) ft.dwLowDateTime;
        *highDateTime = ( uint32_t ) ft.dwHighDateTime;
        write( getDescriptor(), timePackage, sizeof( timePackage ) );

        _state = Ping1Received;
    } else if ( _state == Ping1Received ) {
        std::cout << "State1" << std::endl;
        unsigned char *buf;
        if ( !readOnePackage( &buf ) ) {
            disconnect();
            return ;
        }
        printPackage( "RapiProvisioningClient", ( unsigned char * ) buf );
        delete[] buf;

        unsigned char packet[ 278 ];
        makePacket( proxyEntriesPrefix, proxyEntriesString, packet );
        write( getDescriptor(), packet, sizeof( packet ) );

        _state = State2;
    } else if ( _state == State2 ) {
        std::cout << "State2" << std::endl;
        unsigned char *buf;
        if ( !readOnePackage( &buf ) ) {
            disconnect();
            return ;
        }
        printPackage( "RapiProvisioningClient", ( unsigned char * ) buf );
        delete[] buf;

        unsigned char packet[ 688 ];
        makePacket( proxyEntries2Prefix, proxyEntries2String, packet );
        write( getDescriptor(), packet, sizeof( packet ) );

        _state = State3;
    } else if ( _state == State3 ) {
        std::cout << "State3" << std::endl;
        // 0x03b6
        unsigned char *buf;
        if ( !readOnePackage( &buf ) ) {
            disconnect();
            return ;
        }
        printPackage( "RapiProvisioningClient", ( unsigned char * ) buf );
        delete[] buf;

        unsigned char packet[ 472 ];
        makePacket( netEntriesPrefix, netEntriesString, packet );
        write( getDescriptor(), packet, sizeof( packet ) );

        _state = State4;
    } else if ( _state == State4 ) {
        std::cout << "State4" << std::endl;
        // 0x03b6
        unsigned char *buf;
        if ( !readOnePackage( &buf ) ) {
            disconnect();
            return ;
        }
        printPackage( "RapiProvisioningClient", ( unsigned char * ) buf );
        delete[] buf;

        unsigned char packet[ 478 ];
        makePacket( syncQueryPrefix, syncQueryString, packet );
        write( getDescriptor(), packet, sizeof( packet ) );

        _state = State5;
    } else if ( _state == State5 ) {
        std::cout << "State5" << std::endl;
        unsigned char *buf;
        if ( !readOnePackage( &buf ) ) {
            disconnect();
            return ;
        }
        printPackage( "RapiProvisioningClient", ( unsigned char * ) buf );
        delete[] buf;

        unsigned char packet[ 478 ];
        makePacket( syncSourcesQueryPrefix, syncSourcesQueryString, packet );
        write( getDescriptor(), packet, sizeof( packet ) );

        _state = State6;
    } else if ( _state == State6 ) {
        std::cout << "State6" << std::endl;
        unsigned char *buf;
        if ( !readOnePackage( &buf ) ) {
            disconnect();
            return ;
        }
        printPackage( "RapiProvisioningClient", ( unsigned char * ) buf );
        delete[] buf;

        unsigned char packet[ 118 ] = { 0x72, 0x00, 0x00, 0x00,
                                        0x31, 0x00, 0x00, 0x00,
                                        0x02, 0x00, 0x00, 0x80,
                                        0x60, 0x00, 0x00, 0x00 };

        const char* string = "Software\\Microsoft\\Windows CE Services\\Partners";
        const unsigned char* unicode = ( unsigned char* ) wstr_from_ascii( string );
        const int sizeofUnicode = 2 * ( strlen( string ) + 1 );
        memcpy( packet + 18, unicode, sizeofUnicode );

        // append even more..?
        memcpy( packet + 18 + sizeofUnicode, "\02\00\00\00\00\00", 6 );
        write( getDescriptor(), packet, sizeof( packet ) );

        _state = State7;
    } else if ( _state == State7 ) {
        std::cout << "State7" << std::endl;
        unsigned char *buf;
        if ( !readOnePackage( &buf ) ) {
            disconnect();
            return ;
        }
        printPackage( "RapiProvisioningClient", ( unsigned char * ) buf );

        const unsigned char packet[] = {
                                           0x18, 0x00, 0x00, 0x00,
                                           0x31, 0x00, 0x00, 0x00,
                                           0xc0, 0x42, 0x0b, 0x04,
                                           0x06, 0x00, 0x00, 0x00,
                                           0x50, 0x00, 0x31, 0x00,
                                           0x00, 0x00, 0x02, 0x00,
                                           0x00, 0x00, 0x00, 0x00
                                       };

        write( getDescriptor(), packet, sizeof( packet ) );

        _state = State8;
    } else if ( _state == State8 ) {
        std::cout << "State8" << std::endl;
        unsigned char *buf;
        if ( !readOnePackage( &buf ) ) {
            disconnect();
            return ;
        }
        printPackage( "RapiProvisioningClient", ( unsigned char * ) buf );

        delete[] buf;
        const char packet[] = {
                                  0x1c, 0x00, 0x00, 0x00,
                                  0x37, 0x00, 0x00, 0x00,
                                  0x20, 0x43, 0x0b, 0x04,
                                  0x0c, 0x00, 0x00, 0x00,
                                  0x50, 0x00, 0x4e, 0x00,
                                  0x61, 0x00, 0x6d, 0x00,
                                  0x65, 0x00, 0x00, 0x00,
                                  0x08, 0x02, 0x00, 0x00
                              };

        write( getDescriptor(), packet, sizeof( packet ) );

        _state = State9;
    } else if ( _state == State9 ) {
        std::cout << "State9" << std::endl;
        unsigned char *buf;
        if ( !readOnePackage( &buf ) ) {
            disconnect();
            return ;
        }
        printPackage( "RapiProvisioningClient", ( unsigned char * ) buf );

        _state = State10;
        getRapiConnection() ->provisioningClientReachedState9();
        std::cout << "Connection established" << std::endl;
    } else if ( _state == State10 ) {
        RapiProxy * rapiProxy = *( rapiProxies.begin() );
        rapiProxies.remove( rapiProxy );

        uint32_t length;

        if ( read( getDescriptor(), &length, 4 ) <= 0 ) {
            disconnect();
            return ;
        }

        if ( CmdLineArgs::getLogLevel() < 3 ) {
            unsigned char * buf = new unsigned char[ mtuWH ];
            unsigned int remBytes = length;

            if (getRapiConnection()->rapiProxyAlive(rapiProxy)) {
                write( rapiProxy->getDescriptor(), ( char * ) & length, 4 );
            }
            while ( remBytes > 0 ) {
                size_t numRead = ( remBytes < mtuWH ) ? remBytes : mtuWH;
                if ( readNumBytes( buf, numRead ) < (ssize_t) numRead ) {
                    delete[] buf;
                    ssize_t readNumBytes(unsigned char *buffer, size_t number);
                    disconnect();
                    return ;
                }
                remBytes -= numRead;
                if ( getRapiConnection() ->rapiProxyAlive( rapiProxy ) ) {
                    write( rapiProxy->getDescriptor(), buf, numRead );
                }
            }
            delete[] buf;
        } else {
            size_t readLength = length;

            unsigned char *buf = new unsigned char[ length + 4 ];

            memcpy( buf, ( unsigned char * ) & length, 4 );

            if ( readNumBytes( buf + 4, readLength ) < (ssize_t) readLength ) {
                delete[] buf;
                disconnect();
                return ;
            }

            if ( getRapiConnection() ->rapiProxyAlive( rapiProxy ) ) {
                std::cout << std::endl << "Device --> Application" << std::endl;
                std::cout << "======================" << std::endl;
                printPackage( "RapiProxy", buf );
                std::cout << "-------------------------------------------------------------" << std::endl;

                write( rapiProxy->getDescriptor(), buf, length + 4 );
            }
            delete[] buf;
        }

    }
}


void RapiProvisioningClient::makePacket( const unsigned char* prefix,
        const char* string,
        unsigned char * packet )
{
    const int sizeofPrefix = 12; // always ?
    const int sizeofUnicode = 2 * ( strlen( string ) + 1 );
    const unsigned char* unicode = ( unsigned char * ) wstr_from_ascii( string );
    const unsigned char ping[ 4 ] = {
                                        01, 00, 00, 00
                                    };

    memcpy( packet, prefix, sizeofPrefix );
    memcpy( packet + sizeofPrefix, unicode, sizeofUnicode );
    memcpy( packet + sizeofPrefix + sizeofUnicode, ping, sizeof( ping ) );
}




#include <iostream>
bool RapiProvisioningClient::forwardBytes( RapiProxy *rapiProxy )
{
    uint32_t length;

    if ( read( rapiProxy->getDescriptor(), &length, 4 ) <= 0 ) {
        rapiProxies.remove( rapiProxy );
        return false;
    }

    if ( CmdLineArgs::getLogLevel() < 3 ) {
        unsigned char * buf = new unsigned char[ mtuWH ];
        unsigned int remBytes = length;

        write( getDescriptor(), ( char * ) & length, 4 );
        while ( remBytes > 0 ) {
            size_t numRead = ( remBytes < mtuWH ) ? remBytes : mtuWH;
            if ( rapiProxy->readNumBytes( buf, numRead ) < (ssize_t) numRead ) {
                rapiProxies.remove( rapiProxy );
                delete[] buf;
                return false;
            }
            remBytes -= numRead;
            if ( write( getDescriptor(), buf, numRead ) < ( int ) numRead ) {
                rapiProxies.remove( rapiProxy );
                delete[] buf;
                return true;
            }
        }

        delete[] buf;
    } else {

        unsigned char *buf = new unsigned char[ length + 4 ];
        size_t readLength = length;

        memcpy( buf, ( char * ) & length, 4 );
        if ( rapiProxy->readNumBytes( buf + 4, readLength ) < (ssize_t) readLength ) {
            rapiProxies.remove( rapiProxy );
            delete[] buf;
            return false;
        }


        std::cout << "Application --> Device" << std::endl;
        std::cout << "======================" << std::endl;
        printPackage( "RapiProxy", buf );

        write( getDescriptor(), buf, length + 4 );

        delete[] buf;
    }
    rapiProxies.push_back( rapiProxy );


    return true;
}
