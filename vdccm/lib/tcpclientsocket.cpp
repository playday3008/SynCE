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
#include "tcpclientsocket.h"
#include <netdb.h>

using namespace std;

TCPClientSocket::TCPClientSocket(uint16_t remotePort, string remoteInterfaceName, uint16_t localPort, string localInterfaceName)
 : TCPConnectedSocket(localPort, localInterfaceName)
{
    this->remotePort = remotePort;
    this->remoteInterfaceName = remoteInterfaceName;
}


TCPClientSocket::TCPClientSocket()
 : TCPConnectedSocket()
{
    this->remotePort = 0;
    this->remoteInterfaceName = "";
}


TCPClientSocket::~TCPClientSocket()
{
}


/*!
    \fn TCPClientSocket::connect()
 */
bool TCPClientSocket::connect()
{
    int fd;

    struct hostent *rhostent;
    struct sockaddr_in raddr;

    if ((rhostent = gethostbyname(remoteInterfaceName.c_str())) == NULL) {
        return false;
    }
    if ((fd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return false;
    }

    raddr.sin_port = htons(remotePort);
    raddr.sin_family = AF_INET;
    memcpy(&raddr.sin_addr, rhostent->h_addr, rhostent->h_length);

    if (::connect(fd, (struct sockaddr *) &raddr, sizeof(raddr)) < 0) {
        return false;
    }

    return this->setSocket(fd, raddr);
}


/*!
    \fn TCPClientSocket::getConfiguredRemoteInterfaceName()
 */
string TCPClientSocket::getConfiguredRemoteInterfaceName()
{
    return remoteInterfaceName;
}


/*!
    \fn TCPClientSocket::getConfiguredRemotePort()
 */
uint16_t TCPClientSocket::getConfiguredRemotePort()
{
    return remotePort;
}
