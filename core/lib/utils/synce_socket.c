/* $Id$ */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "synce_socket.h"
#include "synce_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/select.h>

#if !HAVE_INET_PTON
int inet_pton(int af, const char *src, void *dst);
#endif

// No AF_LOCAL on solaris
#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif

#if HAVE_DMALLOC_H
#include "dmalloc.h"
#endif

#define RAPI_SOCKET_DEBUG 0

#if RAPI_SOCKET_DEBUG
#define synce_socket_trace(args...)    synce_trace(args)
#define synce_socket_warning(args...)  synce_warning(args)
#else
#define synce_socket_trace(args...)
#define synce_socket_warning(args...)
#endif
#define synce_socket_error(args...)    synce_error(args)

#define RAPI_SOCKET_LISTEN_QUEUE  1024

/** 
 * @defgroup SynceSocket Socket convenience functions
 * @ingroup SynceUtils
 * @brief A convenience class for socket operations
 *
 * @{ 
 */ 

struct _SynceSocket
{
	int fd;
};

/** @brief Create a client socket
 * 
 * This function creates a new SynceSocket object.
 * 
 * @return the new socket
 */ 
SynceSocket* synce_socket_new(/* TODO: some parameters here */)
{
	SynceSocket* socket = calloc(1, sizeof(SynceSocket));

	if (socket)
	{
		socket->fd = SYNCE_SOCKET_INVALID_DESCRIPTOR;
	}

	return socket;
}

/** @brief Release a client socket
 * 
 * This function closes the object if it is open,
 * and free's the memory.
 * 
 * @param[in] socket the socket to free
 */ 
void synce_socket_free(SynceSocket* socket)
{
	if (socket)
	{
		synce_socket_close(socket);
		free(socket);
	}
}

/** @brief Obtain the descriptor from a socket
 * 
 * This function returns the file descriptor from a
 * socket.
 * 
 * @param[in] socket the socket
 * @return the file descriptor
 */ 
int synce_socket_get_descriptor(SynceSocket* socket)
{
  return socket->fd;
}

/** @brief Use the given descriptor for the socket object
 * 
 * This function releases any active file descriptor
 * for the socket, and replaces it with the supplied
 * descriptor.
 * 
 * @param[in] socket the socket
 * @param[in] fd the file descriptor to use
 */ 
void synce_socket_take_descriptor(SynceSocket* socket, int fd)
{
  if (socket->fd != SYNCE_SOCKET_INVALID_DESCRIPTOR)
    close(socket->fd);

  socket->fd = fd;
}

static bool synce_socket_create(SynceSocket* syncesock)
{
	bool success = false;

	if (syncesock->fd != SYNCE_SOCKET_INVALID_DESCRIPTOR)
	{
		synce_socket_error("already have a socket file descriptor");
		goto fail;
	}

	/* create socket */
	if ( (syncesock->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		goto fail;

	success = true;

fail:
	return success;
}

static bool synce_socket_create_proxy(SynceSocket* syncesock)
{
    bool success = false;

    if (syncesock->fd != SYNCE_SOCKET_INVALID_DESCRIPTOR)
    {
        synce_socket_error("already have a socket file descriptor");
        goto fail;
    }

    /* freate proxy-socket */
    if ( (syncesock->fd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
        goto fail;

    success = true;

fail:
    return success;
}

/** @brief Connect a socket to remote service
 * 
 * This function connects the given socket to the supplied
 * host and port.
 *
 * If the socket is already open it is first closed.
 * 
 * @param[in] syncesock the socket
 * @param[in] host host address in dotted quad format
 * @param[in] port port number to connect to
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_socket_connect(SynceSocket* syncesock, const char* host, uint16_t port)
{
	struct sockaddr_in servaddr;

	synce_socket_close(syncesock);

	if (!synce_socket_create(syncesock))
		goto fail;

	/* fill in address structure */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if ( inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0 )
		goto fail;

	/* connect */
	if ( connect(syncesock->fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0 )
		goto fail;

	return true;

fail:
	synce_socket_close(syncesock);
	return false;
}

/** @brief Connect a socket via vdccm proxy
 * 
 * This function connects the given socket via the vdccm
 * proxying functionality. Vdccm is a legacy implementation
 * of dccm, and this function should therefore not be used.
 *
 * If the socket is already open it is first closed.
 * 
 * @param[in] syncesock the socket
 * @param[in] remoteIpAddress IP address of the remote device
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_socket_connect_proxy(SynceSocket* syncesock, const char* remoteIpAddress)
{
    struct sockaddr_un proxyaddr;
    char *path = NULL;
    int length;

    synce_socket_close(syncesock);

    if (!synce_socket_create_proxy(syncesock))
        goto fail;

    if (!synce_get_subdirectory("rapi2", &path)) {
        goto fail;
    }

    length = snprintf(proxyaddr.sun_path, sizeof(proxyaddr.sun_path), "%s/%s", path, remoteIpAddress);
    if ((length < 0) || (length >= (int) sizeof(proxyaddr.sun_path)))
        goto fail;

    proxyaddr.sun_family = AF_LOCAL;
    if (connect(syncesock->fd, (struct sockaddr *) &proxyaddr, sizeof(proxyaddr)) < 0)
        goto fail;

    free(path);

    return true;

fail:
    if (path != NULL)
        free(path);

    synce_socket_close(syncesock);
    return false;
}

/** @brief Open a socket to listen for incoming connections
 * 
 * This function binds the given socket to the supplied
 * host address and port to listen for incoming connections.
 *
 * If host is not given, all addresses are bound.
 * 
 * @param[in] syncesock the socket
 * @param[in] host host address in dotted quad format
 * @param[in] port port number to connect to
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_socket_listen(SynceSocket* socket, const char* host, uint16_t port)
{
	struct sockaddr_in servaddr;
	int sock_opt;

	if (!synce_socket_create(socket))
		goto fail;

	/* set SO_REUSEADDR */
	sock_opt = 1;
	if ( setsockopt(socket->fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt)) < 0 )
	{
		synce_socket_error("setsockopt failed, error: %i \"%s\"", errno, strerror(errno));
		goto fail;
	}

	/* fill in address structure */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if (!host)
		host = "0.0.0.0";

	if ( inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0 )
		goto fail;

	if ( bind(socket->fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0 )
	{
		synce_socket_error("bind failed, error: %i \"%s\"", errno, strerror(errno));
		goto fail;
	}

	if ( listen(socket->fd, RAPI_SOCKET_LISTEN_QUEUE) < 0 )
	{
		synce_socket_error("listen failed, error: %i \"%s\"", errno, strerror(errno));
		goto fail;
	}

	return true;

fail:
	synce_socket_close(socket);
	return false;
}

/** @brief Accept an incoming connection
 * 
 * This function accepts an incoming connection from a listening socket.
 *
 * @param[in] server the listening socket
 * @param[out] address address of a location to store the client address, or NULL
 * @return new socket connection, or NULL on failure
 */ 
SynceSocket* synce_socket_accept(SynceSocket* server, struct sockaddr_in* address)
{
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	int connfd;
	SynceSocket* client = NULL;
	fd_set read_set;

	if (!address)
		address = &cliaddr;

	/*
	 * Section 15.6 of Unix Networking Programming by Richard Stevens have some
	 * things to say about this select/accept combination...
	 */

	FD_ZERO(&read_set);
	FD_SET(server->fd, &read_set);

	if ( select(server->fd + 1, &read_set, NULL, NULL, NULL) < 0 )
	{
		if (errno != EINTR)
			synce_socket_error("select failed, error: %i \"%s\"", errno, strerror(errno));
		goto exit;
	}

	clilen = sizeof(struct sockaddr_in);
	if ( (connfd = accept(server->fd, (struct sockaddr*)address, &clilen)) < 0 )
	{
		synce_socket_error("accept failed, error: %i \"%s\"", errno, strerror(errno));
		goto exit;
	}

	synce_socket_trace("accepted connection with file descriptor %i", connfd);

	client = synce_socket_new();
	if (!client)
	{
		synce_socket_error("failed to create new socket");
		goto exit;
	}

	client->fd = connfd;

exit:
	return client;
}

/** @brief Close a socket connection
 * 
 * This function closes a socket, if it is open.
 *
 * @param[in] socket socket to close
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_socket_close(SynceSocket* socket)
{
	if (!socket)
	{
		synce_socket_error("socket is null");
		return false;
	}

	if (socket->fd != SYNCE_SOCKET_INVALID_DESCRIPTOR)
	{
		close(socket->fd);
		socket->fd = SYNCE_SOCKET_INVALID_DESCRIPTOR;
		return true;
	}
	return false;
}

/** @brief Write to an open socket
 * 
 * This function writes the given data to an open socket.
 *
 * @param[in] socket socket to write to
 * @param[in] data data to be written
 * @param[in] size number of bytes to be written from data
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_socket_write(SynceSocket* socket, const void* data, size_t size)
{
	ssize_t bytes_left = size;

	if ( SYNCE_SOCKET_INVALID_DESCRIPTOR == socket->fd )
	{
		synce_socket_error("Invalid file descriptor");
		return false;
	}

	while (bytes_left > 0)
	{
		ssize_t result = write(socket->fd, data, bytes_left);

		if (result == 0)
		{
			/* Should only happen if 'bytes_left == 0'.  That is,
			 * no more data left to be written. */
			break;
		}
		else if (result < 0 && (errno == EINTR || errno == EAGAIN))
		{
			/* There was either an interrupt or some other reason
			 * we should try again. */
			continue;
		}
		else if (result < 0)
		{
			/* Something else went wrong. */
			synce_socket_error("write failed, error: %i \"%s\"", errno, strerror(errno));

			/* Close socket on unrecoverable errors */
			if (ECONNRESET == errno)  /* Connection reset by peer */
				synce_socket_close(socket);
			break;
		}

		bytes_left -= result;
		data = (char*)data + result;
	}

	return 0 == bytes_left;
}

/** @brief Read from an open socket
 * 
 * This function reads data from an open socket.
 *
 * @param[in] socket socket to read from
 * @param[in] data location to store the data
 * @param[in] size number of bytes to read
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_socket_read(SynceSocket* socket, void* data, size_t size)
{
	ssize_t bytes_needed = size;

	if ( SYNCE_SOCKET_INVALID_DESCRIPTOR == socket->fd )
	{
		synce_socket_error("Invalid file descriptor");
		return false;
	}

	while(bytes_needed > 0)
	{
		ssize_t result = read(socket->fd, data, bytes_needed);

		/* synce_socket_trace("read returned %i, needed %i bytes", result, bytes_needed); */

		if (result == 0)
		{
			/* EOF */
			break;
		}
		else if (result < 0 && (errno == EINTR || errno == EAGAIN))
		{
			/* There was either an interrupt or some other reason
			 * we should try again. */
			continue;
		}
		else if (result < 0)
		{
			/* Something else went wrong. */
			synce_socket_error("read failed, error: %i \"%s\"", errno, strerror(errno));

			/* Close socket on unrecoverable errors */
			if (ECONNRESET == errno)  /* Connection reset by peer */
				synce_socket_close(socket);
			break;
		}

		bytes_needed -= result;
		data = (char*)data + result;
	}

	return 0 == bytes_needed;
}

#if HAVE_POLL

/**
 * Convert from SocketEvents to poll events
 */
static short to_poll_events(SocketEvents events)
{
	short poll_events = 0;

	if (events & EVENT_READ)
		poll_events |= POLLIN;

	if (events & EVENT_WRITE)
		poll_events |= POLLOUT;

	return poll_events;
}

/**
 * Convert to SocketEvents from poll events
 */
static SocketEvents from_poll_events(short poll_events)
{
	SocketEvents events = 0;

	if (poll_events & POLLIN)
		events |= EVENT_READ;

	if (poll_events & POLLOUT)
		events |= EVENT_WRITE;

  if (poll_events & (POLLERR|POLLHUP|POLLNVAL))
    events |= EVENT_ERROR;

	return events;
}

/** @brief Wait for an event on a socket
 * 
 * This function waits for particular events to occur
 * on a socket, and reports the event types encountered.
 *
 * @param[in] socket socket to check
 * @param[in] timeoutInSeconds time to wait in seconds, 0 to report immediately, or a negative value to wait until an event is detected
 * @param[in,out] events the events to check for; on return is set to the events detected
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_socket_wait(SynceSocket* socket, int timeoutInSeconds, short* events)
{
	/*
	 * This can easily be replaced by select() if needed on some platform
	 */
	bool success = false;
	int result;
	struct pollfd pfd;

  if (!socket)
  {
 		synce_socket_error("SynceSocket is NULL");
		return false;
 }

	if ( SYNCE_SOCKET_INVALID_DESCRIPTOR == socket->fd )
	{
		synce_socket_error("Invalid file descriptor");
		return false;
	}

	if ( !events )
	{
		synce_socket_error("Events parameter is NULL");
		return false;
	}

	pfd.fd = socket->fd;
	pfd.events = to_poll_events(*events);
	pfd.revents = 0;

  *events = 0;

#if 0
	while (pfd.revents != 0)
	{
		synce_socket_trace("what?");
		pfd.revents = 0;
	}
#endif

	result = poll(&pfd, 1, timeoutInSeconds * 1000);

	switch (result)
	{
		case 0:
			*events = EVENT_TIMEOUT;
			break;

		case 1:
			*events = from_poll_events(pfd.revents);
			break;

		default:
			if (errno == EINTR)
			{
				*events = EVENT_INTERRUPTED;
			}
			else
			{
				synce_socket_error("poll failed (returned %i), error: %i \"%s\"",
						result, errno, strerror(errno));
				goto exit;

			}
			break;
	}

	success = true;

exit:
	return success;
}

#endif /* HAVE_POLL */

/** @brief Check amount available for reading on a socket
 * 
 * This function checks for the number of bytes available for
 * reading on a socket.
 *
 * @param[in] socket socket to check
 * @param[out] count bytes available to read
 * @return TRUE on success, FALSE on failure
 */ 
bool synce_socket_available(SynceSocket* socket, unsigned* count)
{
#ifdef FIONREAD
	if (ioctl(socket->fd, FIONREAD, count) < 0)
	{
		synce_socket_error("FIONREAD failed, error: %i \"%s\"",
						errno, strerror(errno));
		return false;
	}

	return true;
#else
	return false;
#endif
}

/** @} */
