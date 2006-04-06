/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/
#include "synceclient.h"
#include <multiplexer.h>
#include "windowscedevicebase.h"
#include <errno.h>
#include <string.h>
#include <synce_log.h>

using namespace std;

SynCEClient::SynCEClient(int fd, LocalServerSocket *localServerSocket)
: LocalAcceptedSocket(fd, localServerSocket) {
    Multiplexer::self()->getReadManager()->add(this);
    DeviceManager::self()->addClient(this);
}


SynCEClient::~SynCEClient()
{}


void SynCEClient::disconnect()
{
    DeviceManager::self()->removeClient( this );

    Multiplexer::self()->getReadManager()->remove(this);

    shutdown();

    delete this;
}


/*!
    \fn SynCEClient::event(Descriptor::eventType et)
 */
void SynCEClient::event(Descriptor::eventType /*et*/)
{
    char buffer[ 256 ];
    int n;

    if ( ( n = read( getDescriptor(), buffer, 256 ) ) > 0 ) {
        buffer[ n ] = '\0';

        synce_trace( "SynCEClient reply: %s", buffer );

        switch ( buffer[ 0 ] ) {
        case 'R': {
                char * password = &buffer[ 1 ];
                char *name = strsep( &password, "=" );
                WindowsCEDeviceBase *windowsCEDevice = DeviceManager::self()->getPasswordPendingDevice( name );
                if ( windowsCEDevice != NULL ) {
                    synce_trace( "Sending Password to: %s", name );
                    if ( !windowsCEDevice->sendPassword( password ) ) {
                        synce_trace( "failed to send password to %s", name );
                        windowsCEDevice->disconnect();
                    }
                } else {
                    synce_trace( "Got Password for %s from SynCEClient but no device waiting", name );
                }
            }
            break;
        case 'A':{
                char *deviceName = &buffer[1];
                DeviceManager::self()->setAsDefaultDevice(deviceName);
            }
        case 'D': {
                synce_trace( "Disconnecting %s", &buffer[ 1 ] );
                char *name = &buffer[ 1 ];
                WindowsCEDeviceBase *windowsCEDevice = DeviceManager::self()->getConnectedDevice(name);
                if ( windowsCEDevice != NULL ) {
                    windowsCEDevice->disconnect();
                } else {
                    synce_trace( "Got deisconnect command for %s from SynCEClient but no device connected", name );
                }
            }
            break;
        default:
            synce_trace( "Unknown command from SynCEClient" );
            disconnect();
            break;
        }
    } else {
        disconnect();
    }
}


/*!
    \fn SynCEClient::writeToClient(char command, string name)
 */
bool SynCEClient::writeToClient(char command, string &name)
{
    string message = command + name + ";";

    if (write(getDescriptor(), message.c_str(), message.length()) != (ssize_t) message.length()) {
        synce_error("Writing to SynCE-Client vailed: %s", strerror(errno));
        return false;
    }

    return true;
}


/*!
    \fn SynCEClient::deviceConnected(string &name);
 */
bool SynCEClient::deviceConnected(string &name)
{
    if (!writeToClient('C', name)) {
        disconnect();
        return false;
    }
    return true;
}


/*!
    \fn SynCEClient::deviceDisconnected(string &name)
 */
bool SynCEClient::deviceDisconnected(string &name)
{
    if (!writeToClient('D', name)) {
        disconnect();
        return false;
    }
    return true;
}


/*!
    \fn SynCEClient::deviceRequestsPassword(string &name)
 */
bool SynCEClient::deviceRequestsPassword(string &name)
{
    if (!writeToClient('P', name)) {
        disconnect();
        return false;
    }
    return true;
}




/*!
    \fn SynCEClient::passwordRejected(string &name)
 */
bool SynCEClient::passwordRejected(string &name)
{
    if (!writeToClient('R', name)) {
        disconnect();
        return false;
    }
    return true;
}
