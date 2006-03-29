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
#include "cmdlineargs.h"
#include "rapiconnection.h"
#include <iostream>

RapiHandshakeClient::RapiHandshakeClient( int fd, TCPServerSocket *tcpServerSocket )
        : RapiClient( fd, tcpServerSocket ), ContinousNode( CmdLineArgs::getPingDelay(), 0 )
{
    _state = NoDataReceived;
    pendingPingRequests = 0;
    setBlocking();
    setReadTimeout( 5, 0 );
    connectionCount = 0;
}


RapiHandshakeClient::~RapiHandshakeClient()
{
    Multiplexer::self()->getReadManager()->remove(this);
    Multiplexer::self() ->getTimerNodeManager() ->remove(this);
    shutdown();
}


void RapiHandshakeClient::setRapiConnection(RapiConnection *rapiConnection)
{
    std::cout << "Handshakeclient-setRapiConnection() " << (void *) rapiConnection << std::endl;
    this->rapiConnection = rapiConnection;
}


void RapiHandshakeClient::keepAlive()
{
    Multiplexer::self() ->getTimerNodeManager() ->add
    ( this );

    _state = KeepingAlive;
}

void RapiHandshakeClient::event()
{
    if ( _state == NoDataReceived ) {
        // data not relevant, should be { 00, 00, 00, 00 }, just reply
        unsigned char buffer[ 4 ];
        if (readNumBytes(buffer, 4) != 4) {
            rapiConnection->handshakeClientDisconnected();
            return;
        }
        printPackage("RapiHandshakeClient", (unsigned char *) buffer);

        // write response, should { 03, 00, 00, 00 }
        char response[ 4 ] = { 03, 00, 00, 00 };
        write( getDescriptor(), response, 4 );

        _state = Ping1Received;
    } else if ( _state == Ping1Received ) {
        // data not relevant, should be { 04, 00 ,00 ,00 }
        unsigned char buffer[ 4 ];
        if (readNumBytes(buffer, 4) != 4) {
            rapiConnection->handshakeClientDisconnected();
            return;
        }
        printPackage("RapiHandshakeClient", (unsigned char *) buffer, 4);

        // nothing to reply, more data (info message) follows
        _state = Ping2Received;
    } else if ( _state == Ping2Received ) {
        // buffer contains info message
        unsigned char *buf;
        if (!readOnePackage(&buf)) {
            rapiConnection->handshakeClientDisconnected();
            return;
        }
        printPackage("RapiHandshakeClient", (unsigned char *) buf);

        delete[] buf;
        rapiConnection->handshakeClientInitialized();
        _state = InfoMessageReceived;
        keepAlive();
    } else if ( _state == KeepingAlive ) {
        unsigned char buffer[ 4 ];
        if (readNumBytes(buffer, 4) != 4) {
            rapiConnection->handshakeClientDisconnected();
            return;
        }

        printPackage("RapiHandshakeClient", buffer, 4);
        pendingPingRequests--;
    }
}


void RapiHandshakeClient::initiateProvisioningConnection()
{
    connectionCount++;
    // write 3 responses, response 1 is { 05, 00, 00, 00 }
    char response[ 4 ] = { 05, 00, 00, 00 };
    write( getDescriptor(), response, 4 );

        // response 2 is { 04, 00, 00, 00 }
    response[ 0 ] = 04;
    write( getDescriptor(), response, 4 );

        // response 3 is connectionCount
    write( getDescriptor(), &connectionCount, 4 );
}


void RapiHandshakeClient::shot()
{
    char response[ 4 ] = { 01, 00, 00, 00 };

    if ( writeable( 0, 0 ) ) {
        write( getDescriptor(), response, 4 );
        if ( pendingPingRequests >= CmdLineArgs::getMissingPingCount() ) {
            rapiConnection->handshakeClientDisconnected();
        } else {
            pendingPingRequests++;
        }
    } else {
        rapiConnection->handshakeClientDisconnected();
    }
}
