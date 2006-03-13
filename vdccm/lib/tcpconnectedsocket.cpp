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
#include "tcpconnectedsocket.h"
#include <arpa/inet.h>
#include <netdb.h>

using namespace std;

TCPConnectedSocket::TCPConnectedSocket(uint16_t port, string interfaceName)
 : TCPSocket(port, interfaceName)
{
    this->connected = false;
}


TCPConnectedSocket::TCPConnectedSocket(const TCPConnectedSocket &tcpConnectedSocket, bool releaseFromManager)
 : TCPSocket(tcpConnectedSocket, releaseFromManager)
{
    memcpy(&this->remoteAddress, &tcpConnectedSocket.remoteAddress, sizeof(remoteAddress));
    this->connected = tcpConnectedSocket.connected;
}


TCPConnectedSocket::TCPConnectedSocket()
 : TCPSocket()
{
    this->connected = false;
}


TCPConnectedSocket::~TCPConnectedSocket()
{
}



/*!
    \fn TCPConnectedSocket::setConnected(bool connected)
 */
void TCPConnectedSocket::setConnected(bool connected)
{
    this->connected = connected;
}


/*!
    \fn TCPConnectedSocket::connected()
 */
bool TCPConnectedSocket::isConnected() const
{
    return connected;
}


/*!
    \fn TCPConnectedSocket::setSocket(int descriptor, struct sockaddr_in remoteAddress, const struct hostent * remoteHostent)
 */
 bool TCPConnectedSocket::setSocket(int descriptor, struct sockaddr_in remoteAddress)
{
    socklen_t namelen = sizeof(struct sockaddr_in);

    setDescriptor(descriptor);
    memcpy(&this->remoteAddress, &remoteAddress, sizeof(remoteAddress));
    if (getsockname(descriptor, (struct sockaddr *) &localAddress, &namelen) < 0) {
        return false;
    }
    if ((localHostent = gethostbyaddr((char *) &localAddress,  sizeof(localAddress), AF_INET)) == NULL) {
        return false;
    }

    return true;
}



/*!
    \fn TCPConnectedSocket::getRemoteAddress() const
 */
struct sockaddr_in TCPConnectedSocket::getRemoteSinAddr() const
{
    return remoteAddress;
}


/*!
    \fn TCPConnectedSocket::getRemoteAddress() const
 */
string TCPConnectedSocket::getRemoteAddress() const
{
    char ip_str[16];
    if (!inet_ntop(AF_INET, &remoteAddress.sin_addr, ip_str, sizeof(ip_str))) {
        ip_str[0] = '\0';
    }

    return string(ip_str);
}


/*!
    \fn TCPConnectedSocket::getRemotePort() const
 */
uint16_t TCPConnectedSocket::getRemotePort() const
{
    return ntohs(remoteAddress.sin_port);
}


/*!
    \fn TCPConnectedSocket::_generate(int fd)
 */
bool TCPConnectedSocket::_generate(int fd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    if (fd < 0) {
        return false;
    }

    if (getpeername(fd, (struct sockaddr *) &addr, &len) < 0) {
        return false;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &TCPSocket::TRUE, sizeof(TCPSocket::TRUE)) < 0) {
        return false;
    }

    setSocket(fd, addr);
    setConnected(true);

    return true;
}


/*!
    \fn TCPConnectedSocket::generate(int fd)
 */
TCPConnectedSocket TCPConnectedSocket::generate(int fd)
{
    TCPConnectedSocket tcpConnectedSocket;

    tcpConnectedSocket._generate(fd);

    return tcpConnectedSocket;
}
