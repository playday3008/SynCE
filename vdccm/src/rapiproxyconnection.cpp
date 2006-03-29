//
// C++ Implementation: rapiproxyconnection
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "rapiproxyconnection.h"
#include "rapiproxy.h"
#include "rapiconnection.h"
#include "rapiprovisioningclient.h"
#include "cmdlineargs.h"
#include <multiplexer.h>


#include <iostream>

RapiProxyConnection::RapiProxyConnection(RapiConnection *rapiConnection, RapiProxy *rapiProxy,
                                         RapiProvisioningClient *rapiProvisioningClient)
    : rapiConnection(rapiConnection),
      rapiProxy(rapiProxy),
      rapiProvisioningClient(rapiProvisioningClient)
{
    rapiProxy->setRapiProxyConnection(this);
    rapiProvisioningClient->setRapiProxyConnection(this);
    Multiplexer::self()->getReadManager()->add(rapiProvisioningClient);
    mtuWH = rapiProvisioningClient->getMTU() - 40;

    dirDeviceBuf = new unsigned char[mtuWH];
    dirApplicationBuf = new unsigned char[mtuWH];
}


RapiProxyConnection::~RapiProxyConnection()
{
    Multiplexer::self()->getReadManager()->remove(rapiProvisioningClient);
    Multiplexer::self()->getReadManager()->remove(rapiProxy);
    rapiProvisioningClient->shutdown();
    rapiProxy->shutdown();

    delete rapiProvisioningClient;
    delete rapiProxy;
    delete[] dirDeviceBuf;
    delete[] dirApplicationBuf;
}


void RapiProxyConnection::provisioningClientInitialized()
{
    Multiplexer::self()->getReadManager()->add(rapiProxy);
}


void RapiProxyConnection::provisioningClientNotInitialized()
{
    rapiConnection->proxyConnectionClosed(this);
}


void RapiProxyConnection::messageToDevice()
{


    if ( CmdLineArgs::getLogLevel() < 3 ) {
        ssize_t r;

        if ((r = read(rapiProxy->getDescriptor(), dirApplicationBuf, mtuWH)) <= 0) {
            rapiConnection->proxyConnectionClosed(this);
            return ;
        } else {
            write(rapiProvisioningClient->getDescriptor(), dirApplicationBuf, r);
        }

        /*
        unsigned char * buf = new unsigned char[ mtuWH ];
        uint32_t length;

        if ( read( rapiProxy->getDescriptor(), &length, 4 ) <= 0 ) {
            rapiConnection->proxyConnectionClosed(this);
            return;
        }

        unsigned int remBytes = length;

        write( rapiProvisioningClient->getDescriptor(), ( char * ) & length, 4 );
        while ( remBytes > 0 ) {
            size_t numRead = ( remBytes < mtuWH ) ? remBytes : mtuWH;
            if ( rapiProxy->readNumBytes( buf, numRead ) < (ssize_t) numRead ) {
                delete[] buf;
                rapiConnection->proxyConnectionClosed(this);
                return;
            }
            remBytes -= numRead;
            if ( write( rapiProvisioningClient->getDescriptor(), buf, numRead ) < ( int ) numRead ) {
                delete[] buf;
                rapiConnection->proxyConnectionClosed(this);
                return;
            }
        }

        delete[] buf;
        */
    } else {
        uint32_t length;

        if ( read( rapiProxy->getDescriptor(), &length, 4 ) <= 0 ) {
            rapiConnection->proxyConnectionClosed(this);
            return;
        }
        unsigned char *buf = new unsigned char[ length + 4 ];
        size_t readLength = length;

        memcpy( buf, ( char * ) & length, 4 );
        if ( rapiProxy->readNumBytes( buf + 4, readLength ) < (ssize_t) readLength ) {
            delete[] buf;
            rapiConnection->proxyConnectionClosed(this);
            return;
        }


        std::cout << "Application --> Device" << std::endl;
        std::cout << "======================" << std::endl;
        rapiProvisioningClient->printPackage( "RapiProxy", buf );

        write( rapiProvisioningClient->getDescriptor(), buf, length + 4 );

        delete[] buf;
    }
}


void RapiProxyConnection::messageToApplication()
{
    if ( CmdLineArgs::getLogLevel() < 3 ) {
        ssize_t r;

        if ((r = read(rapiProvisioningClient->getDescriptor(), dirApplicationBuf, mtuWH)) <= 0) {
            rapiConnection->proxyConnectionClosed(this);
            return ;
        } else {
            write(rapiProxy->getDescriptor(), dirApplicationBuf, r);
        }


        /*
        uint32_t length;
        if ( read( rapiProvisioningClient->getDescriptor(), &length, 4 ) <= 0 ) {
            rapiConnection->proxyConnectionClosed(this);
            return ;
        }
        unsigned char * buf = new unsigned char[ mtuWH ];
        unsigned int remBytes = length;

        write( rapiProxy->getDescriptor(), ( char * ) & length, 4 );
        while ( remBytes > 0 ) {
            size_t numRead = ( remBytes < mtuWH ) ? remBytes : mtuWH;
            if ( rapiProvisioningClient->readNumBytes( buf, numRead ) < (ssize_t) numRead ) {
                delete[] buf;
                rapiConnection->proxyConnectionClosed(this);
                return ;
            }
            remBytes -= numRead;
            write( rapiProxy->getDescriptor(), buf, numRead );
        }
        delete[] buf;
        */
    } else {
        uint32_t length;
        if ( read( rapiProvisioningClient->getDescriptor(), &length, 4 ) <= 0 ) {
            rapiConnection->proxyConnectionClosed(this);
            return ;
        }

        size_t readLength = length;

        unsigned char *buf = new unsigned char[ length + 4 ];

        memcpy( buf, ( unsigned char * ) & length, 4 );

        if ( rapiProvisioningClient->readNumBytes( buf + 4, readLength ) < (ssize_t) readLength ) {
            delete[] buf;
            rapiConnection->proxyConnectionClosed(this);
            return ;
        }

        std::cout << std::endl << "Device --> Application" << std::endl;
        std::cout << "======================" << std::endl;
        rapiProvisioningClient->printPackage( "RapiProxy", buf );
        std::cout << "-------------------------------------------------------------" << std::endl;

        write( rapiProxy->getDescriptor(), buf, length + 4 );

        delete[] buf;
    }
}
