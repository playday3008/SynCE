//
// C++ Implementation: udpsocket
//
// Description:
//
//
// Author: Volker Christian <voc@synce.org>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "udpsocket.h"
#include <netdb.h>
#include <string.h>

using namespace std;

UDPSocket::UDPSocket(uint16_t remotePort, string remoteInterfaceName, uint16_t localPort, string localInterfaceName) : NetSocket(), remotePort(remotePort), remoteInterfaceName(remoteInterfaceName), localPort(localPort), localInterfaceName(localInterfaceName)
{
}


UDPSocket::~UDPSocket()
{
}


bool UDPSocket::sendTo(uint16_t remotePort, string remoteInterface, unsigned char *data, size_t length)
{
    struct hostent *rhostent;
    struct sockaddr_in raddr;
    int fd;

    if ((rhostent = gethostbyname(remoteInterface.c_str())) == NULL) {
        return false;
    }
    if ((fd = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return false;
    }

    raddr.sin_port = htons(remotePort);
    raddr.sin_family = AF_INET;
    memcpy(&raddr.sin_addr, rhostent->h_addr, rhostent->h_length);

    if (sendto(fd, &data, length, 0, (struct sockaddr *) &raddr, sizeof(raddr)) < 0) {
        return false;
    }

    return true;
}
