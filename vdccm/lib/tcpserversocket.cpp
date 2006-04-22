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
#include "tcpserversocket.h"
#include "tcpacceptedsocket.h"
#include "tcpacceptedsocketfactory.h"

using namespace std;

TCPServerSocket::TCPServerSocket(const TCPAcceptedSocketFactory *tcpAcceptedSocketFactory, uint16_t port, string interfaceName)
    : TCPSocket(port, interfaceName), tcpAcceptedSocketFactory(tcpAcceptedSocketFactory)
{
}


TCPServerSocket::~TCPServerSocket()
{
}


bool TCPServerSocket::listen(int backlog)
{
    if (!socket()) {
        return false;
    }

    if (!bind()) {
        return false;
    }

    if (::listen(getDescriptor(), backlog) < 0) {
        return false;
    }

    return true;
}


/*!
    \fn TCPServerSocket::accept()
 */
TCPAcceptedSocket* TCPServerSocket::accept()
{
    int fd;

    fd = ::accept(getDescriptor(), NULL, NULL);

    TCPAcceptedSocket *tas = NULL;

    if (fd >= 0) {
        tas = tcpAcceptedSocketFactory->socket(fd, this);
    }

    return tas;
}


void TCPServerSocket::event(Descriptor::eventType /*et*/)
{
    if (!accept()) {
        // some error handling
    }
}
