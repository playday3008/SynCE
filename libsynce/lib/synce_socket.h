/* $Id$ */
#ifndef __synce_socket_h__
#define __synce_socket_h__

#include "synce.h"

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
SynceSocket* synce_socket_accept(SynceSocket* socket);

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

#endif

