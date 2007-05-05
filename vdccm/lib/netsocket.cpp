//
// C++ Implementation: netsocket
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "netsocket.h"
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>


NetSocket::NetSocket()
 : Descriptor()
{
}


NetSocket::~NetSocket()
{
}


bool NetSocket::setReadTimeout(int sec, int usec)
{
    bool ret = true;

    struct timeval tv;

    tv.tv_sec = sec;
    tv.tv_usec = usec;

    if (setsockopt(getDescriptor(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        ret = false;
    }

    return ret;
}


bool NetSocket::setWriteTimeout(int sec, int usec)
{
    bool ret = true;

    struct timeval tv;

    tv.tv_sec = sec;
    tv.tv_usec = usec;

    if (setsockopt(getDescriptor(), SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        ret = false;
    }

    return ret;
}


bool NetSocket::setNonBlocking()
{
    int flags = fcntl (getDescriptor(), F_GETFL);
    return fcntl (getDescriptor(), F_SETFL, flags | O_NONBLOCK) >= 0;
}


bool NetSocket::setBlocking()
{
    int flags = fcntl (getDescriptor(), F_GETFL);
    return fcntl (getDescriptor(), F_SETFL, flags & ~O_NONBLOCK) >= 0;
}
