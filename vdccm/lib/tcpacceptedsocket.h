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
#ifndef TCPACCEPTEDSOCKET_H
#define TCPACCEPTEDSOCKET_H

#include <tcpconnectedsocket.h>
#include <string>

class TCPServerSocket;

/**
@author Volker Christian
*/
class TCPAcceptedSocket : public TCPConnectedSocket
{
public:
    TCPAcceptedSocket(const TCPAcceptedSocket &tcpAcceptedSocket, bool releaseFromManager = false);
    static TCPAcceptedSocket generate(int fd, TCPServerSocket *tcpServerSocket = NULL);
    ~TCPAcceptedSocket();
//    const TCPServerSocket *getServerSocket() const;
    TCPAcceptedSocket& operator=(const TCPAcceptedSocket &tcpConnectedSocket)
    {
        this->tcpServerSocket = tcpConnectedSocket.tcpServerSocket;

        return *this;
    }
    void setServerSocket(TCPServerSocket *tcpServerSocket);
    const TCPServerSocket* getTCPServerSocket() const;

protected:
    TCPAcceptedSocket(uint16_t port, std::string interfaceName);
    TCPAcceptedSocket();
private:
//    void setServerSocket(const TCPServerSocket * tcpServerSocket);
    const TCPServerSocket *tcpServerSocket;

friend class TCPServerSocket;
};

#endif
