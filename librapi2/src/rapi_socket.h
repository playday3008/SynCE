/* $Id$ */
#ifndef __rapi_socket_h__
#define __rapi_socket_h__

#include "rapi_internal.h"
#include "rapi_buffer.h"

struct _RapiSocket;
typedef struct _RapiSocket RapiSocket;

/**
 * Create new client socket
 */
RapiSocket* rapi_socket_new(/* TODO: some parameters here */);

/**
 * Release client socket
 */
void rapi_socket_free(RapiSocket* socket);

/**
 * Connect to remote service
 */
bool rapi_socket_connect(RapiSocket* socket, const char* host, int port);

/**
 * Close connection
 */
bool rapi_socket_close(RapiSocket* socket);

/**
 * Send a buffer on the socket
 */
bool rapi_socket_send(RapiSocket* socket, RapiBuffer* buffer);

/**
 * Receive a buffer on the socket
 */
bool rapi_socket_recv(RapiSocket* socket, RapiBuffer* buffer);

/**
 * Write a number of bytes of data to socket
 */
bool rapi_socket_write(RapiSocket* socket, void* data, unsigned size);

	/**
 * Read a number of bytes of data from a socket
 */
bool rapi_socket_read(RapiSocket* socket, void* data, unsigned size);

#endif

