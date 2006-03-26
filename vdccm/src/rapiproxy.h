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

class RapiProxy : public LocalAcceptedSocket
{
public:
    RapiProxy(int fd, LocalServerSocket* serverSocket);

    ~RapiProxy();

    void event();

    size_t readNumBytes(unsigned char *buffer, size_t number);
};

#endif
