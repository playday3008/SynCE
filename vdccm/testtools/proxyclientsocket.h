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
#include <synce.h>

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/
class ProxyClientSocket : public LocalClientSocket, public ContinousNode
{
public:
    ProxyClientSocket(char *path);

    ~ProxyClientSocket();

    void event(Descriptor::eventType et);

    void shot();

private:
//    void interpretBuffer(unsigned char* buf);
    void ceGetSpecialFolderPath(uint32_t folderType);
    void ceFindAllFiles(char *path);
    void ceWhichCallSetsTheTimeOnTheDevice(int setClock);

    void printPackage(unsigned char *buf);
    bool writePackage(unsigned char *buf);
    bool readPackage(unsigned char **ergBuf);
    size_t readNumBytes(unsigned char *buffer, size_t numBytes);
};

#endif
