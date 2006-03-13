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
#include "localacceptedsocket.h"
#include "localserversocket.h"
#include <sys/socket.h>

LocalAcceptedSocket::LocalAcceptedSocket(string path)
 : LocalConnectedSocket(path)
{
}


LocalAcceptedSocket::LocalAcceptedSocket(const LocalAcceptedSocket &localAcceptedSocket, bool releaseFromManager)
 : LocalConnectedSocket(localAcceptedSocket, releaseFromManager)
{
    this->localServerSocket = localAcceptedSocket.localServerSocket;
}


LocalAcceptedSocket::~LocalAcceptedSocket()
{
}



void LocalAcceptedSocket::setServerSocket(const LocalServerSocket * localServerSocket)
{
    this->localServerSocket = localServerSocket;
}


const LocalServerSocket *LocalAcceptedSocket::getServerSocket() const
{
    return localServerSocket;
}



/*!
    \fn LocalAcceptedSocket::generate(int fd, LocalServerSocket* localServerSocket)
 */
LocalAcceptedSocket LocalAcceptedSocket::generate(int fd, LocalServerSocket* localServerSocket)
{
    LocalAcceptedSocket localAcceptedSocket(localServerSocket->getLocalPath());
    struct sockaddr_un addr;
    socklen_t len = sizeof(struct sockaddr);

    if (fd < 0) {
        return localAcceptedSocket;
    }

    if (getpeername(fd, (struct sockaddr *) &addr, &len) < 0) {
        return localAcceptedSocket;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &LocalSocket::TRUE, sizeof(LocalSocket::TRUE)) < 0) {
        return localAcceptedSocket;
    }

    localAcceptedSocket.setSocket(fd, addr);
    localAcceptedSocket.setConnected(true);
    localAcceptedSocket.setServerSocket(localServerSocket);

    return localAcceptedSocket;
}
