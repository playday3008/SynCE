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
#ifndef TCPCONNECTEDSOCKET_H
#define TCPCONNECTEDSOCKET_H

#include <tcpsocket.h>

// class TCPServerSocket;

/**
@author Volker Christian
*/
class TCPConnectedSocket : public TCPSocket
{
public:
    TCPConnectedSocket(uint16_t port, std::string interfaceName);
    TCPConnectedSocket(const TCPConnectedSocket &tcpConnectedSocket, bool releaseFromManager = false);
    ~TCPConnectedSocket();
    bool isConnected() const;
    TCPConnectedSocket& operator=(const TCPConnectedSocket &tcpConnectedSocket)
    {
        memcpy(&this->remoteAddress, &tcpConnectedSocket.remoteAddress, sizeof(remoteAddress));
        this->connected = tcpConnectedSocket.connected;

        return *this;
    }
    struct sockaddr_in getRemoteSinAddr() const;
    std::string getRemoteAddress() const;
    uint16_t getRemotePort() const;
    static TCPConnectedSocket generate(int fd);

protected:
    TCPConnectedSocket();
    bool setSocket(int descriptor, struct sockaddr_in remoteAddress);
    void setConnected(bool connected);
    bool _generate(int fd);

private:
    bool connected;
    struct sockaddr_in remoteAddress;
};

#endif
