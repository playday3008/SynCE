// serversocket.h: interface for the ServerSocket class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVERSOCKET_H__D42EE250_FDB8_45DF_96A4_B4DC6750D0CA__INCLUDED_)
#define AFX_SERVERSOCKET_H__D42EE250_FDB8_45DF_96A4_B4DC6750D0CA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Winsock.h>

class ServerSocket  
{
public:
	ServerSocket();
	bool init();
	SOCKET acceptClient();
	virtual ~ServerSocket();

private:
	SOCKET s;
	SOCKET cs;
	bool connected;

};

#endif // !defined(AFX_SERVERSOCKET_H__D42EE250_FDB8_45DF_96A4_B4DC6750D0CA__INCLUDED_)
