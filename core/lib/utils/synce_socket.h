/* $Id$ */
#ifndef __synce_socket_h__
#define __synce_socket_h__

#include "synce.h"
#include <netinet/in.h> /* for sockaddr_in */

#ifdef __cplusplus
extern "C"
{
#endif

/** 
 * @defgroup SynceSocket Socket convenience functions
 * @ingroup SynceUtils
 * @brief A convenience class for socket operations
 *
 * @{ 
 */ 

struct _SynceSocket;
/** @brief A socket object
 * 
 * This is an opaque structure containing information about
 * a socket, obtained using synce_socket_new() and
 * destroyed with synce_socket_free().
 * It's contents should be manipulated via the synce_socket_*
 * series of functions.
 */ 
typedef struct _SynceSocket SynceSocket;

/*
 * Create new client socket
 */
SynceSocket* synce_socket_new(/* TODO: some parameters here */);

/*
 * Release client socket
 */
void synce_socket_free(SynceSocket* socket);

/*
  Get file descriptor so we can select() on many sockets including this
*/
int synce_socket_get_descriptor(SynceSocket* socket);

/** @brief An invalid socket descriptor
 * 
 * The value assigned to a socket file descriptor when it is
 * not open, or otherwise invalid.
 */ 
#define SYNCE_SOCKET_INVALID_DESCRIPTOR  (-1)

/*
 * Take ownership of an existing descriptor
 */
void synce_socket_take_descriptor(SynceSocket* socket, int fd);

/*
 * Connect to remote service
 */
bool synce_socket_connect(SynceSocket* socket, const char* host, uint16_t port);

/*
 * Connect to proxy service (vdccm)
 */
bool synce_socket_connect_proxy(SynceSocket* syncesock, const char* remoteIpAddress);

/*
 * Open listening socket
 */
bool synce_socket_listen(SynceSocket*, const char* host, uint16_t port);

/*
 * Accept incoming connections
 */
SynceSocket* synce_socket_accept(SynceSocket* socket, struct sockaddr_in* address);

/*
 * Close connection
 */
bool synce_socket_close(SynceSocket* socket);

/*
 * Write a number of bytes of data to socket
 */
bool synce_socket_write(SynceSocket* socket, const void* data, size_t size);

/*
 * Read a number of bytes of data from a socket
 */
bool synce_socket_read(SynceSocket* socket, void* data, size_t size);

/** @brief Events detectable on a socket
 *
 * Expand as needed, just use event numbers 1,2,4,8,16,32,...
 */
enum _SocketEvents
{
	/** No events detected in the timeout period */
	EVENT_TIMEOUT     = 1,
	/** Socket has data to read */
	EVENT_READ        = 2,
	/** Socket is writeable without blocking */
	EVENT_WRITE       = 4,
	/** Interrupted by a signal */
	EVENT_INTERRUPTED = 8,
	/** An error, hangup, or invalid socket */
  EVENT_ERROR       = 16,
};

/** @brief Events detectable on a socket
 */ 
typedef enum _SocketEvents SocketEvents;

/*
 * Wait for an event on a socket
 */
bool synce_socket_wait(SynceSocket* socket, int timeoutInSeconds, short* events);

/*
 * Get the number of bytes available on a socket
 */
bool synce_socket_available(SynceSocket* socket, unsigned* count);

/** @} */


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

#ifdef __cplusplus
}
#endif

#endif

