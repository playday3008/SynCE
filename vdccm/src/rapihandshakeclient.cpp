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

#include "rapihandshakeclient.h"
#include "multiplexer.h"
#include "cmdlineargs.h"
#include "rapiconnection.h"
#include <synce_log.h>


RapiHandshakeClient::RapiHandshakeClient( int fd, TCPServerSocket *tcpServerSocket )
    : RapiClient( fd, tcpServerSocket )
{
    pendingPingRequests = 0;
    setBlocking();
    setReadTimeout( 5, 0 );
    connectionCount = 0;
}


RapiHandshakeClient::~RapiHandshakeClient()
{
    Multiplexer::self()->getReadManager()->remove(this);
    shutdown();
}


void RapiHandshakeClient::setRapiConnection(RapiConnection *rapiConnection)
{
    this->rapiConnection = rapiConnection;
}


void RapiHandshakeClient::event(Descriptor::eventType /*et*/)
{
    uint32_t leSignature;
    if (readNumBytes((unsigned char *) &leSignature, 4) != 4) {
        rapiConnection->handshakeClientDisconnected();
        return;
    }

    printPackage("RapiHandshakeClient", (unsigned char *) &leSignature, 4);

    uint32_t signature = letoh32(leSignature);

    if ( signature == 0x00 ) {
        // This is the initial package
        // write response, should { 03, 00, 00, 00 }
        char response[ 4 ] = { 03, 00, 00, 00 };
        write( getDescriptor(), response, 4 );
    } else if ( signature == 0x04) {
        // The next package is the info message
        unsigned char *buffer;
        if (!readOnePackage(&buffer)) {
            rapiConnection->handshakeClientDisconnected();
            return;
        }
        printPackage("RapiHandshakeClient", (unsigned char *) buffer);

        rapiConnection->handshakeClientInitialized(buffer);
        delete[] buffer;
    } else if ( signature == 0x02 ) {
        // This is a ping-reply
        pendingPingRequests--;
    }
}


void RapiHandshakeClient::initiateProvisioningConnection()
{
    connectionCount++;

    unsigned char package[12] = {
        0x05, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00
        // The last four bytes are filled with the connectionCounter
    };

    uint32_t *cc = (uint32_t *) (package + 8);

    *cc = htole32(connectionCount);

    write(getDescriptor(), package, 12);
}


void RapiHandshakeClient::shot()
{
    char response[ 4 ] = { 01, 00, 00, 00 };

    write( getDescriptor(), response, 4 );
    if ( pendingPingRequests >= CmdLineArgs::getMissingPingCount() ) {
        rapiConnection->handshakeClientDisconnected();
    } else {
        pendingPingRequests++;
    }
}
