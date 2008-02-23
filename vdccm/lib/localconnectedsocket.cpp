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
#include <string.h>

using namespace std;

LocalConnectedSocket::LocalConnectedSocket(string path)
 : LocalSocket(path)
{
    this->connected = false;
}


LocalConnectedSocket::LocalConnectedSocket()
 : LocalSocket()
{
    this->connected = false;
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


struct sockaddr_un LocalConnectedSocket::getRemoteSunAddr() const
{
    return remoteAddress;
}


/*!
    \fn LocalConnectedSocket::setSocket(const int descriptor, const struct sockaddr_un remoteAddress, const string path)
 */
 bool LocalConnectedSocket::setSocket(const int descriptor, const struct sockaddr_un remoteAddress)
{
    socklen_t namelen = sizeof(struct sockaddr_un);

    setDescriptor(descriptor);
    memcpy(&this->remoteAddress, &remoteAddress, sizeof(remoteAddress));

    if (getsockname(descriptor, (struct sockaddr *) &localAddress, &namelen) < 0) {
        return false;
    }

    return true;
}


/*!
    \fn LocalAcceptedSocket::generate(int fd, LocalServerSocket* localServerSocket)
 */
bool LocalConnectedSocket::setSocket(int fd)
{
    struct sockaddr_un addr;
    socklen_t len = sizeof(struct sockaddr);

    if (fd < 0) {
        return false;
    }

    if (getpeername(fd, (struct sockaddr *) &addr, &len) < 0) {
        return false;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &LocalSocket::FTRUE, sizeof(LocalSocket::FTRUE)) < 0) {
        return false;
    }

    this->setSocket(fd, addr);
    this->setConnected(true);

    return true;
}
