//
// C++ Interface: proxyclientsocket
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef PROXYCLIENTSOCKET_H
#define PROXYCLIENTSOCKET_H

#include <localclientsocket.h>
#include <continousnode.h>

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/
class ProxyClientSocket : public LocalClientSocket, public ContinousNode
{
public:
    ProxyClientSocket(char *path);

    ~ProxyClientSocket();

    void event();

    void shot();

};

#endif
