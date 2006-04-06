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
#include "dccmserver.h"
#include "windowscedevice.h"
#include <synce_log.h>
#include <errno.h>
#include <string.h>
#include <tcpacceptedsocketfactory.h>

using namespace std;
using namespace synce;

DccmServer::DccmServer(TCPAcceptedSocketFactory *tasf, u_int16_t port, string interfaceName)
    : TCPServerSocket(tasf, port, interfaceName)
{
}


DccmServer::~DccmServer()
{
}


bool DccmServer::listen(int backlog)
{
    if (!(socket = synce_socket_new())) {
        return false;
    }

    if (!synce_socket_listen(socket, NULL, getConfiguredLocalPort())) {
        return false;
    }

    setDescriptor(synce_socket_get_descriptor(socket));

    return true;
}


bool DccmServer::shutdown()
{
    synce_socket_free(socket);
    return true;
}


void DccmServer::event(Descriptor::eventType /*et*/)
{
    SynceSocket *clientSocket = synce_socket_accept(socket, NULL);

    if (clientSocket != NULL) {
        WindowsCEDevice *wced = dynamic_cast<WindowsCEDevice *>(tcpAcceptedSocketFactory->socket(
                synce_socket_get_descriptor(clientSocket) , this));
        if (wced != NULL) {
            wced->init(clientSocket);
        } else {
            synce_warning("Device not accepted: %s", strerror(errno));
        }
    } else {
        synce_warning("Device not accepted: %s", strerror(errno));
    }
}
