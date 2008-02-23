//
// C++ Interface: rapiclient
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RAPICLIENT_H
#define RAPICLIENT_H

#include <tcpacceptedsocket.h>

#include <limits.h>

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/
class RapiConnection;

class RapiClient : public TCPAcceptedSocket
{
public:
    RapiClient(int fd, TCPServerSocket *tcpServerSocket);

    ~RapiClient();

    void printPackage(std::string origin, unsigned char *buf, unsigned int maxLength=UINT_MAX );

protected:
    bool readOnePackage(unsigned char **buffer);

    bool readOnePackage(uint32_t length, unsigned char **buffer);
};

#endif
