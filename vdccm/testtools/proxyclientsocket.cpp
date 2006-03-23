//
// C++ Implementation: proxyclientsocket
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "proxyclientsocket.h"
#include <iostream>

using namespace std;

ProxyClientSocket::ProxyClientSocket(char *path)
    : LocalClientSocket(path), ContinousNode(1, 0)
{
}


ProxyClientSocket::~ProxyClientSocket()
{
}


void ProxyClientSocket::event()
{
    char buf[256];

    int n = read(getDescriptor(), buf, 256);

    if (n == 0) {
        exit(0);
    }
    cout << "Echo: " << buf << endl;
}


void ProxyClientSocket::shot()
{
    write(getDescriptor(), "Hallo", 6);
}

