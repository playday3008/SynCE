//
// C++ Interface: tcpacceptedsocketfactory
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TCPACCEPTEDSOCKETFACTORY_H
#define TCPACCEPTEDSOCKETFACTORY_H

class TCPAcceptedSocket;
class TCPServerSocket;

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/


class TCPAcceptedSocketFactory {
public:
    virtual ~TCPAcceptedSocketFactory() {};
    virtual TCPAcceptedSocket *socket(int fd, TCPServerSocket *serverSocket) const = 0;
};

#endif
