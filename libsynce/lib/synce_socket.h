/* $Id$ */
#ifndef __synce_socket_h__
#define __synce_socket_h__

#include "synce.h"
#include <netinet/in.h> /* for sockaddr_in */

struct _SynceSocket;
typedef struct _SynceSocket SynceSocket;

/**
 * Create new client socket
 */
SynceSocket* synce_socket_new(/* TODO: some parameters here */);

/**
 * Release client socket
 */
void synce_socket_free(SynceSocket* socket);

/**
 * Connect to remote service
 */
bool synce_socket_connect(SynceSocket* socket, const char* host, int port);

/**
 * Open listening socket
 */
bool synce_socket_listen(SynceSocket*, const char* host, int port);

/**
 * Accept incoming connections
 */
SynceSocket* synce_socket_accept(SynceSocket* socket, struct sockaddr_in* address);

/**
 * Close connection
 */
bool synce_socket_close(SynceSocket* socket);

/**
 * Write a number of bytes of data to socket
 */
bool synce_socket_write(SynceSocket* socket, const void* data, unsigned size);

/**
 * Read a number of bytes of data from a socket
 */
bool synce_socket_read(SynceSocket* socket, void* data, unsigned size);

/**
 * This that can happen to a socket... :-)
 *
 * Expand as needed, just use event numbers 1,2,4,8,16,32,...
 */
enum _SocketEvents
{
	EVENT_TIMEOUT  = 1,
	EVENT_READ     = 2,
	EVENT_WRITE    = 4
};

typedef enum _SocketEvents SocketEvents;

/**
 * Wait for an event on a socket
 */
bool synce_socket_wait(SynceSocket* socket, int timeoutInSeconds, SocketEvents* events);


/*
 * Functions from password.c
 */

bool synce_password_send(
		SynceSocket *socket,
		const char *asciiPassword,
		unsigned char key);

bool synce_password_recv_reply(
		SynceSocket* socket,
		size_t size,
		bool* passwordCorrect);

#endif

