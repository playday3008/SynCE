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
#include "localsocket.h"
#include <sys/socket.h>

#ifndef PF_LOCAL
#define PF_LOCAL PF_UNIX
#endif

#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif

using namespace std;

int LocalSocket::FTRUE = 1;
int LocalSocket::FFALSE = 0;

LocalSocket::LocalSocket(string path)
 : NetSocket()
{
    this->path = path;
}


LocalSocket::LocalSocket()
 : NetSocket()
{
    this->path = "";
}


LocalSocket::~LocalSocket()
{
}


bool LocalSocket::socket()
{
    int fd;

    if ((fd = ::socket(PF_LOCAL, SOCK_STREAM, 0)) < 0) {
        return false;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &LocalSocket::FTRUE, sizeof(int)) < 0) {
        return false;
    }

    setDescriptor(fd);

    return true;
}


bool LocalSocket::bind()
{
    localAddress.sun_family = AF_LOCAL;
    strncpy(localAddress.sun_path, path.c_str(), sizeof(localAddress.sun_path));

    size_t size = (offsetof (struct sockaddr_un, sun_path) + strlen(localAddress.sun_path) + 1);

    if (::bind(getDescriptor(), (struct sockaddr *) &localAddress, size) < 0) {
        return false;
    }

    return true;
}


/*!
    \fn TCPSocket::shutdown()
 */
bool LocalSocket::shutdown()
{
    int ret;

    ret = ::shutdown(getDescriptor(), 2);

    if (ret != 0) {
        return false;
    }

    return close();
}


string LocalSocket::getLocalPath() const
{
    return path;
}
