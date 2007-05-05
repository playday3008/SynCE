//
// C++ Interface: netsocket
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef NETSOCKET_H
#define NETSOCKET_H

#include <descriptor.h>

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/
class NetSocket : public Descriptor
{
public:
    NetSocket();

    ~NetSocket();

    bool setReadTimeout(int sec, int usec);
    bool setWriteTimeout(int sec, int usec);
    bool setNonBlocking();
    bool setBlocking();
};

#endif
