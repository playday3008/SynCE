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
#include <string>

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/
class RapiConnection;

class RapiClient : public TCPAcceptedSocket
{
public:
    RapiClient(int fd, TCPServerSocket *tcpServerSocket);

    ~RapiClient();

    void disconnect();

    void setRapiConnection(RapiConnection *rapiConnection);
    RapiConnection *getRapiConnection();

    size_t readNumBytes(unsigned char *buffer, size_t number);

    bool readOnePackage(unsigned char **buffer);

    void printPackage(std::string origin, unsigned char *buf, unsigned int maxLength=UINT_MAX );

    private:
        RapiConnection *rapiConnection;
};

#endif
