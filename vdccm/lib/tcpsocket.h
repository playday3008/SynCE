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
#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <string>
#include <netinet/in.h>
#include "netsocket.h"

/**
@author Volker Christian
*/
class TCPSocket : public NetSocket
{
public:
    TCPSocket(uint16_t port, std::string interfaceName = "");
    ~TCPSocket();
    virtual bool shutdown();
    uint16_t getConfiguredLocalPort() const;
    std::string getConfiguredLocalInterfaceName() const;
    struct sockaddr_in getLocalSinAddr() const;
    std::string getLocalAddress() const;
    uint16_t getLocalPort() const;
    std::string getLocalInterfaceName() const;

protected:
    TCPSocket();
    virtual bool socket();
    virtual bool bind();

protected:
    static int FTRUE;
    static int FFALSE;
    static unsigned long int INADDRANY;

protected:
    std::string interfaceName;
    uint16_t port;
    struct sockaddr_in localAddress;
};

#endif
