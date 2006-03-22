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
#include "localclientsocket.h"
#include <sys/socket.h>

#ifndef PF_LOCAL
#define PF_LOCAL PF_UNIX
#endif

#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif

using namespace std;

LocalClientSocket::LocalClientSocket(string path)
 : LocalConnectedSocket(path)
{
    this->path = path;
}


LocalClientSocket::LocalClientSocket()
 : LocalConnectedSocket()
{
    this->path = "";
}


LocalClientSocket::~LocalClientSocket()
{
}



/*!
    \fn TCPClientSocket::connect()
 */
bool LocalClientSocket::connect()
{
    int fd;
    struct sockaddr_un raddr;

    if ((fd = ::socket(PF_LOCAL, SOCK_STREAM, 0)) < 0) {
        return false;
    }

    raddr.sun_family = AF_LOCAL;
    strncpy(raddr.sun_path, path.c_str(), sizeof(raddr.sun_path));

    size_t size = (offsetof (struct sockaddr_un, sun_path) + strlen(raddr.sun_path) + 1);

    if (::connect(fd, (struct sockaddr *) &raddr, size) < 0) {
        return false;
    }

    return this->setSocket(fd, raddr);
}

