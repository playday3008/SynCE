//
// C++ Interface: windowscedevicefactory
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef WINDOWSCEDEVICEFACTORY_H
#define WINDOWSCEDEVICEFACTORY_H

#include <tcpacceptedsocketfactory.h>

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/
class WindowsCEDeviceFactory : public TCPAcceptedSocketFactory
{
public:
    WindowsCEDeviceFactory();

    ~WindowsCEDeviceFactory();

    virtual TCPAcceptedSocket* socket(int fd, TCPServerSocket *serverSocket) const;
};

#endif
