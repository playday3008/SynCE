// eventprocessor.cpp: implementation of the EventProcessor class.
//
//////////////////////////////////////////////////////////////////////

#include "eventprocessor.h"
#include "vnckeymap.h"
#include "huffmanencoder.h"
#include "rleencoder.h"

#define MOUSE_PRESSED	1
#define MOUSE_RELEASED  2
#define MOUSE_MOVED		3
#define MOUSE_WHEEL		4
#define KEY_PRESSED		5
#define KEY_RELEASED	6

#define LEFT_BUTTON		1
#define RIGHT_BUTTON	2
#define MID_BUTTON		3


EventProcessor::EventProcessor(SOCKET cs, HANDLE clientEvent)
{
	this->cs = cs;
	this->clientEvent = clientEvent;
}


void EventProcessor::start()
{
	bool running = true;
	char buf[4 * sizeof(u_long)];
	fd_set readFd;
	FD_ZERO(&readFd);

	vncKeymap vk;

	do {
		FD_SET(cs, &readFd);

		int selret = select(0, &readFd, NULL, NULL, NULL);

		if (selret > 0) {
			int ret = recv(cs, (char *) buf, sizeof(u_long) * 4, 0);

			if (ret <= 0 || ret == WSAECONNRESET) {
				running = false;
			} else {
				u_long cmd = ntohl(*(u_long *) &buf[sizeof(UINT32) * 0]);
				u_long button = ntohl(*(u_long *) &buf[sizeof(UINT32) * 1]);
				u_long x = ntohl(*(u_long *) &buf[sizeof(UINT32) * 2]);
				u_long y = ntohl(*(u_long *) &buf[sizeof(UINT32) * 3]);
					
				DWORD dwFlags = MOUSEEVENTF_ABSOLUTE;
				DWORD dwData = 0;
				switch (cmd) {
				case MOUSE_PRESSED:
					switch(button) {
					case LEFT_BUTTON:
						dwFlags |= MOUSEEVENTF_LEFTDOWN;
						break;
					case RIGHT_BUTTON:
						dwFlags |= MOUSEEVENTF_RIGHTDOWN;
						break;
					case MID_BUTTON:
						dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
						break;
					}
					mouse_event(dwFlags, x, y, dwData, 0);
					break;
				case MOUSE_RELEASED:
					switch(button) {
					case LEFT_BUTTON:
						dwFlags |= MOUSEEVENTF_LEFTUP;
						break;
					case RIGHT_BUTTON:
						dwFlags |= MOUSEEVENTF_RIGHTUP;
						break;
					case MID_BUTTON:
						dwFlags |= MOUSEEVENTF_MIDDLEUP;
						break;
					}
					mouse_event(dwFlags, x, y, dwData, 0);
					break;
				case MOUSE_MOVED:
					dwFlags |= MOUSEEVENTF_MOVE;
					switch(button) {
					case LEFT_BUTTON:
						break;
					case RIGHT_BUTTON:
						break;
					case MID_BUTTON:
						break;
					}
					mouse_event(dwFlags, x, y, dwData, 0);
					break;
				case MOUSE_WHEEL:
					dwFlags = MOUSEEVENTF_WHEEL;
					dwData = x;
					mouse_event(dwFlags, x, y, dwData, 0);
					break;
				case KEY_PRESSED:
					vk.DoXkeysym(button, true);
					break;
				case KEY_RELEASED:
					vk.DoXkeysym(button, false);
					break;
				}
				if (PulseEvent(clientEvent) == 0){
					MessageBox(NULL, L"Could not Pulse Event", L"Snap", 
						MB_OK | MB_ICONERROR);
				}
			}
		} else {
			running = false;
		}
	} while (running);
}


EventProcessor::~EventProcessor()
{

}
