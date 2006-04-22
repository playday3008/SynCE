//
// C++ Interface: rapiproxyfactory
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RAPIPROXYFACTORY_H
#define RAPIPROXYFACTORY_H

#include <localacceptedsocketfactory.h>

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/
class RapiProxyFactory : public LocalAcceptedSocketFactory
{
public:
    RapiProxyFactory();

    ~RapiProxyFactory();

    virtual LocalAcceptedSocket* socket(int fd, LocalServerSocket* serverSocket) const;

};

#endif
