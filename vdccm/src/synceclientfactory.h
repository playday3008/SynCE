//
// C++ Interface: synceclientfactory
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef SYNCECLIENTFACTORY_H
#define SYNCECLIENTFACTORY_H

#include <localacceptedsocketfactory.h>

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/
class SynCEClientFactory : public LocalAcceptedSocketFactory
{
public:
    SynCEClientFactory();

    ~SynCEClientFactory();

    virtual LocalAcceptedSocket* socket(int fd, LocalServerSocket* localServerSocket) const;

};

#endif
