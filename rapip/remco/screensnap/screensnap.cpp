#include "serversocket.h"
#include "snap.h"
#include "eventprocessor.h"
#include <Winsock.h>

int WINAPI WinMain(	HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPTSTR    lpCmdLine,
					int       nCmdShow)
{
	ServerSocket serverSocket;

	if (serverSocket.init()) {
		SOCKET s = serverSocket.acceptClient();
		if (s != INVALID_SOCKET) {
			HANDLE clientEvent = CreateEvent (NULL, FALSE, FALSE, TEXT("WriteEvent"));
			Snap snap(s, clientEvent);
			if (snap.start()) {
				EventProcessor eventProcessor(s, clientEvent);
				eventProcessor.start();
			}
		}
	}

	return 0;
}
