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
#include "localconnectedsocket.h"
#include <netdb.h>

using namespace std;

LocalConnectedSocket::LocalConnectedSocket(string path)
 : LocalSocket(path)
{
    this->connected = false;
}


LocalConnectedSocket::LocalConnectedSocket(const LocalConnectedSocket &localConnectedSocket, bool releaseFromManager)
 : LocalSocket(localConnectedSocket, releaseFromManager)
{
    memcpy(&this->remoteAddress, &localConnectedSocket.remoteAddress, sizeof(remoteAddress));
    this->path = localConnectedSocket.path;
    this->connected = localConnectedSocket.connected;
}


LocalConnectedSocket::~LocalConnectedSocket()
{
}



/*!
    \fn LocalConnectedSocket::setConnected(bool connected)
 */
void LocalConnectedSocket::setConnected(const bool connected)
{
    this->connected = connected;
}


/*!
    \fn LocalConnectedSocket::connected()
 */
bool LocalConnectedSocket::isConnected() const
{
    return connected;
}


/*!
    \fn LocalConnectedSocket::setSocket(const int descriptor, const struct sockaddr_un remoteAddress, const string path)
 */
 bool LocalConnectedSocket::setSocket(const int descriptor, const struct sockaddr_un remoteAddress, const string path)
{
    socklen_t namelen = sizeof(struct sockaddr_un);

    setDescriptor(descriptor);
    memcpy(&this->remoteAddress, &remoteAddress, sizeof(remoteAddress));
    this->path = path;

    if (getsockname(descriptor, (struct sockaddr *) &localAddress, &namelen) < 0) {
        return false;
    }

    return true;
}



