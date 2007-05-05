//
// C++ Interface: rapiproxy
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RAPIPROXY_H
#define RAPIPROXY_H

#include <localacceptedsocket.h>

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/
class LocalServerSocket;
class RapiProxyConnection;

class RapiProxy : public LocalAcceptedSocket
{
public:
    RapiProxy(int fd, LocalServerSocket* serverSocket);

    ~RapiProxy();

    void event(Descriptor::eventType et);

    void setRapiProxyConnection(RapiProxyConnection *rapiProxyConnection);

    private:
        RapiProxyConnection *rapiProxyConnection;
};

#endif
