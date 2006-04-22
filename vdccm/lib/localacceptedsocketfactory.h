//
// C++ Interface: localacceptedsocketfactory
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef LOCALACCEPTEDSOCKETFACTORY_H
#define LOCALACCEPTEDSOCKETFACTORY_H

class LocalAcceptedSocket;
class LocalServerSocket;

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/
class LocalAcceptedSocketFactory{
public:
    virtual ~LocalAcceptedSocketFactory() {};

    virtual LocalAcceptedSocket *socket(int fd, LocalServerSocket *localServerSocket) const = 0;

};

#endif
