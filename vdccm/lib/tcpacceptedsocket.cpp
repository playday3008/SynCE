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
#include "tcpacceptedsocket.h"
#include "tcpserversocket.h"
#include <netdb.h>

using namespace std;

TCPAcceptedSocket::TCPAcceptedSocket(uint16_t port, string interfaceName)
 : TCPConnectedSocket(port, interfaceName)
{
    this->tcpServerSocket = NULL;
}


TCPAcceptedSocket::TCPAcceptedSocket(const TCPAcceptedSocket &tcpAcceptedSocket, bool releaseFromManager)
 : TCPConnectedSocket(tcpAcceptedSocket, releaseFromManager)
{
    this->tcpServerSocket = tcpAcceptedSocket.tcpServerSocket;
}

TCPAcceptedSocket::TCPAcceptedSocket()
 : TCPConnectedSocket()
{
    this->tcpServerSocket = NULL;
}


/*!
    \fn TCPAcceptedSocket::generate(int fd, TCPServerSocket *tcpServerSocket)
 */
TCPAcceptedSocket TCPAcceptedSocket::generate(int fd, TCPServerSocket *tcpServerSocket)
{
    TCPAcceptedSocket tcpAcceptedSocket;

    if (tcpServerSocket != NULL) {
        TCPAcceptedSocket tcpas = TCPAcceptedSocket(tcpServerSocket->getConfiguredLocalPort(), tcpServerSocket->getConfiguredLocalInterfaceName());
        tcpAcceptedSocket = tcpas;
    } else {
        tcpAcceptedSocket = TCPAcceptedSocket();
    }

    tcpAcceptedSocket._generate(fd);

    tcpAcceptedSocket.setServerSocket(tcpServerSocket);

    return tcpAcceptedSocket;
}


TCPAcceptedSocket::~TCPAcceptedSocket()
{
}


/*!
    \fn TCPAcceptedSocket::setServerSocket(TCPServerSocket *tcpServerSocket)
 */
void TCPAcceptedSocket::setServerSocket(TCPServerSocket *tcpServerSocket)
{
    this->tcpServerSocket = tcpServerSocket;
}


/*!
    \fn TCPAcceptedSocket::getTCPServerSocket() const
 */
const TCPServerSocket* TCPAcceptedSocket::getTCPServerSocket() const
{
    return tcpServerSocket;
}
