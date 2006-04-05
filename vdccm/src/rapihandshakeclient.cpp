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
#include <synce.h>
#include <synce_log.h>
#include <iostream>

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


void RapiHandshakeClient::event()
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
        /*
        int deviceGuidOffset = 0x04;
        int deviceGuidLength = 0x10;
        memcpy(deviceGuid, buffer + deviceGuidOffset, deviceGuidLength);
        if (CmdLineArgs::getLogLevel() >= 3) {
            for (int i = 0; i < deviceGuidLength; i++) {
                fprintf(stderr, "-%0X", deviceGuid[i]);
            }
            printf("-\n");
        }

        int osVersionMajorOffset = deviceGuidOffset + deviceGuidLength;
        osVersionMajor = letoh32(*(uint32_t *)(buffer + osVersionMajorOffset));
        int osVersionMinorOffset = osVersionMajorOffset + sizeof(uint32_t);
        osVersionMinor = letoh32(*(uint32_t *)(buffer + osVersionMinorOffset));
        synce_info("OSVersion: %d.%d", osVersionMajor, osVersionMinor);

        int deviceNameLengthOffset = osVersionMinorOffset + sizeof(uint32_t);
        uint32_t deviceNameLength = letoh32(*(uint32_t *) (buffer + deviceNameLengthOffset));

        int deviceNameOffset = deviceNameLengthOffset + sizeof(uint32_t);
        deviceName = synce::wstr_to_ascii((WCHAR *)(buffer + deviceNameOffset));
        synce_info("DeviceName: %s", deviceName.c_str());

        int deviceVersionOffset = deviceNameOffset + (deviceNameLength + 1) * sizeof(WCHAR);
        deviceVersion = letoh32(*(uint32_t *) (buffer + deviceVersionOffset));
        synce_info("DeviceVersion: %p", deviceVersion);

        int deviceProcessorOffset = deviceVersionOffset + sizeof(uint32_t);
        deviceProcessorType = letoh32(*(int32_t *) (buffer + deviceProcessorOffset));
        synce_info("DeviceProzessorType: %p", deviceProcessorType);

        int unknown1Offset = deviceProcessorOffset + sizeof(uint32_t);
        unknown1 = letoh32(*(int32_t *) (buffer + unknown1Offset));
        synce_info("Unknown1: %p", unknown1);

        int someOtherIdOffset = unknown1Offset + sizeof(uint32_t);
        someOtherId = letoh32(*(int32_t *) (buffer + someOtherIdOffset));
        synce_info("SomeOtherId: %p", someOtherId);

        int deviceIdOffset = someOtherIdOffset + sizeof(uint32_t);
        deviceId = letoh32(*(int32_t *) (buffer + deviceIdOffset));
        synce_info("DeviceId: %p", deviceId);

        int plattformNameLengthOffset = deviceIdOffset + sizeof(uint32_t);
        int plattformNameLength = letoh32(*(int32_t *) (buffer + plattformNameLengthOffset));

        int plattformNameOffset = plattformNameLengthOffset + sizeof(uint32_t);
        plattformName = string((char *) (buffer + plattformNameOffset));
        synce_info("PlattformName: %s", plattformName.c_str());

        int modelNameLengthOffset = plattformNameOffset + plattformNameLength;

        int modelNameOffset = modelNameLengthOffset + sizeof(uint32_t);
        modelName = string((char *) (buffer + modelNameOffset));
        synce_info("ModelName: %s", modelName.c_str());


        int unknown2Offset = modelNameOffset + modelNameLength;
        int unknonw2Length = 0x48;
        */

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
