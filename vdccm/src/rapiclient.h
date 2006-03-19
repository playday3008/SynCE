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
class RapiClient : public TCPAcceptedSocket
{
public:
    RapiClient(const TCPAcceptedSocket & tcpAcceptedSocket);

    virtual ~RapiClient();

    void disconnect();

protected:
    virtual void event();
    
    int readAll(char * buffer);    
};

#endif
