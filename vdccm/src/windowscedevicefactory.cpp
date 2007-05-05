//
// C++ Implementation: windowscedevicefactory
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "windowscedevicefactory.h"
#include "windowscedevice.h"

WindowsCEDeviceFactory::WindowsCEDeviceFactory()
 : TCPAcceptedSocketFactory()
{
}


WindowsCEDeviceFactory::~WindowsCEDeviceFactory()
{
}


TCPAcceptedSocket* WindowsCEDeviceFactory::socket(int fd, TCPServerSocket *serverSocket) const
{
    return new WindowsCEDevice(fd, serverSocket);
}
