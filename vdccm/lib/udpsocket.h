//
// C++ Interface: udpsocket
//
// Description:
//
//
// Author: Volker Christian <voc@synce.org>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include <netsocket.h>
#include <string>

/**
	@author Volker Christian <voc@synce.org>
*/
class UDPSocket : public NetSocket
{
public:
    UDPSocket(uint16_t remotePort, std::string remoteInterfaceName, uint16_t localPort = 0, std::string localInterfaceName = "");
    ~UDPSocket();

    static bool sendTo(uint16_t remotePort, std::string remoteInterface, unsigned char *data, size_t length);

protected:
    UDPSocket();
    uint16_t remotePort;
    std::string remoteInterfaceName;

    uint16_t localPort;
    std::string localInterfaceName;
};

#endif
