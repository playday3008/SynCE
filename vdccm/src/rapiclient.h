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

    int readAll(char * buffer);

    void setRapiConnection(RapiConnection *rapiConnection);
    private:
        RapiConnection *rapiConnection;
};

#endif
