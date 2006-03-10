//
// C++ Interface: rapiserver
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RAPISERVER_H
#define RAPISERVER_H

#include <tcpserversocket.h>
#include <string.h>

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/

using namespace std;

class RapiServer : public TCPServerSocket
{
public:
    RapiServer(u_int16_t port = 990, string interfaceName = "");

    ~RapiServer();

    void event();
};

#endif
