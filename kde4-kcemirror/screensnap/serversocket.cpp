// serversocket.cpp: implementation of the ServerSocket class.
//
//////////////////////////////////////////////////////////////////////

#include "serversocket.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ServerSocket::ServerSocket()
{
	connected = false;
}


bool ServerSocket::init()
{
    struct sockaddr_in inet_address;
	inet_address.sin_family = AF_INET;
	inet_address.sin_port = htons(1234);
	unsigned int inaddr_any = INADDR_ANY;
	WORD wVersion;
	WSADATA wsaData;
	int lo, hi;
	int min = 1, maj = 1;

	wVersion = MAKEWORD(maj, min);
	
	int err = WSAStartup(wVersion, &wsaData);

	if (err != 0) {
		MessageBox(NULL, L"Invalid WSAStartupcall", L"Snap", MB_OK);
		return false;
	}

	lo = LOBYTE(wsaData.wVersion);
	hi = HIBYTE(wsaData.wVersion);

	if ((lo != maj) || (hi != min)) {
		WSACleanup();
		MessageBox(NULL, L"Winsock-Version not supported", L"Snap", MB_OK);
		return false;
	}

	s = socket(AF_INET, SOCK_STREAM, 0);

	if (s == INVALID_SOCKET) {
		MessageBox(NULL, L"Could not create ServerSocket", L"Snap", MB_OK);
		return false;
	}

	memcpy(&inet_address.sin_addr, &inaddr_any, sizeof(INADDR_ANY));

	if (bind(s, (const sockaddr *) &inet_address, 
			sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
		MessageBox(NULL, L"Could not bind ServerSocket", L"Snap", 
				MB_OK | MB_ICONERROR);
		return false;
	}

	if (listen(s, 5) == SOCKET_ERROR) {
		MessageBox(NULL, L"Could not listen on ServerSocket", L"Snap", 
				MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}


SOCKET ServerSocket::acceptClient()
{
	cs = accept(s, NULL, NULL);

	if (cs == INVALID_SOCKET) {
		MessageBox(NULL, L"Could not accept on ServerSocket", L"Snap", 
				MB_OK | MB_ICONERROR);
	}

	connected = true;

	return cs;
}


ServerSocket::~ServerSocket()
{
	closesocket(s);
	
	if (connected) {
		shutdown(cs, 2);
		closesocket(cs);
	}
}
