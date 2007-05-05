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
#include "tcpsocket.h"
#include <arpa/inet.h>
#include <netdb.h>


using namespace std;

int TCPSocket::FTRUE = 1;
int TCPSocket::FFALSE = 0;
unsigned long int TCPSocket::INADDRANY = INADDR_ANY;

TCPSocket::TCPSocket(uint16_t port, string interfaceName)
 : NetSocket()
{
    this->port = port;
    this->interfaceName = interfaceName;
}


TCPSocket::TCPSocket()
 : NetSocket()
{
    this->port = 0;
    this->interfaceName = "";
}


TCPSocket::~TCPSocket()
{
}


bool TCPSocket::socket()
{
    int fd;

    if ((fd = ::socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        return false;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &TCPSocket::FTRUE, sizeof(int)) < 0) {
        return false;
    }

    setDescriptor(fd);

    return true;
}


bool TCPSocket::bind()
{
    struct hostent *localHostent;
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(port);

    if (interfaceName.empty()) {
        memcpy(&localAddress.sin_addr, &TCPSocket::INADDRANY, sizeof(TCPSocket::INADDRANY));
    } else {
        localHostent = gethostbyname(interfaceName.c_str());
        if (localHostent != NULL) {
            memcpy(&localAddress.sin_addr, localHostent->h_addr, localHostent->h_length);
        } else {
            return false;
        }
    }

    if (::bind(getDescriptor(), (struct sockaddr *) &localAddress, sizeof(localAddress)) < 0) {
        return false;
    }

    return true;
}


/*!
    \fn TCPSocket::shutdown()
 */
bool TCPSocket::shutdown()
{
    int ret;

    ret = ::shutdown(getDescriptor(), 2);

    if (ret != 0) {
        return false;
    }

    return close();
}


uint16_t TCPSocket::getConfiguredLocalPort() const
{
    return port;
}


string TCPSocket::getConfiguredLocalInterfaceName() const
{
    return interfaceName;
}


/*!
    \fn TCPSocket::getLocalAddress() const
 */
struct sockaddr_in TCPSocket::getLocalSinAddr() const
{
    return localAddress;
}


/*!
    \fn TCPSocket::getLocalAddress() const
 */
string TCPSocket::getLocalAddress() const
{
    char ip_str[16];
    if (!inet_ntop(AF_INET, &localAddress.sin_addr, ip_str, sizeof(ip_str))) {
        ip_str[0] = '\0';
    }

    return string(ip_str);
}


/*!
    \fn TCPSocket::getLocalPort() const
 */
uint16_t TCPSocket::getLocalPort() const
{
    return ntohs(localAddress.sin_port);
}


/*!
    \fn TCPSocket::getLocalInterfaceName() const
 */
string TCPSocket::getLocalInterfaceName() const
{
    return interfaceName;
}
