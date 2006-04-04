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
#include <synce.h>

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

    forwardBuffer = new unsigned char[mtuWH];
}


RapiProxyConnection::~RapiProxyConnection()
{
    Multiplexer::self()->getReadManager()->remove(rapiProvisioningClient);
    Multiplexer::self()->getReadManager()->remove(rapiProxy);
    rapiProvisioningClient->shutdown();
    rapiProxy->shutdown();

    delete rapiProvisioningClient;
    delete rapiProxy;
    delete[] forwardBuffer;
}


void RapiProxyConnection::provisioningClientInitialized()
{
    Multiplexer::self()->getReadManager()->add(rapiProxy);
}


void RapiProxyConnection::provisioningClientNotInitialized()
{
    rapiConnection->proxyConnectionClosed(this);
}


void RapiProxyConnection::forwardMessage(Descriptor *from, Descriptor *to)
{
    if ( CmdLineArgs::getLogLevel() <= 3 ) {
        ssize_t r;

        if ((r = read(from->getDescriptor(), forwardBuffer, mtuWH)) <= 0) {
            rapiConnection->proxyConnectionClosed(this);
            return ;
        } else {
            write(to->getDescriptor(), forwardBuffer, r);
        }
    } else {
        uint32_t leLength;

        if ( read( from->getDescriptor(), &leLength, 4 ) <= 0 ) {
            rapiConnection->proxyConnectionClosed(this);
            return;
        }
        size_t length = letoh32(leLength);

        unsigned char *buf = new unsigned char[ length + 4 ];

        memcpy( buf, ( char * ) & length, 4 );
        if ( from->readNumBytes( buf + 4, length ) < (ssize_t) length ) {
            delete[] buf;
            rapiConnection->proxyConnectionClosed(this);
            return;
        }

        std::cout << "Application --> Device" << std::endl;
        std::cout << "======================" << std::endl;
        rapiProvisioningClient->printPackage( "RapiProxy", buf );

        write( to->getDescriptor(), buf, length + 4 );

        delete[] buf;
    }
}


void RapiProxyConnection::messageToDevice()
{
    forwardMessage(rapiProxy, rapiProvisioningClient);
}


void RapiProxyConnection::messageToApplication()
{
    forwardMessage(rapiProvisioningClient, rapiProxy);
}
