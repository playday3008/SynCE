/* $Id$ */
#include "synce_socket.h"
#include "synce_log.h"
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include "synce_config.h"

#if !HAVE_INET_PTON
int inet_pton(int af, const char *src, void *dst);
#endif

#if HAVE_DMALLOC_H
#include "dmalloc.h"
#endif

#define RAPI_SOCKET_DEBUG 1

#if RAPI_SOCKET_DEBUG
#define synce_socket_trace(args...)    synce_trace(args)
#define synce_socket_warning(args...)  synce_warning(args)
#define synce_socket_error(args...)    synce_error(args)
#else
#define synce_socket_trace(args...)
#define synce_socket_warning(args...)
#define synce_socket_error(args...)
#endif

#define RAPI_SOCKET_INVALID_FD -1
#define RAPI_SOCKET_LISTEN_QUEUE  1024

struct _SynceSocket
{
	int fd;
};

SynceSocket* synce_socket_new(/* TODO: some parameters here */)
{
	SynceSocket* socket = calloc(1, sizeof(SynceSocket));
	
	if (socket)
	{
		socket->fd = RAPI_SOCKET_INVALID_FD;
	}
	
	return socket;
}

void synce_socket_free(SynceSocket* socket)
{
	if (socket)
	{
		synce_socket_close(socket);
		free(socket);
	}
}

int synce_socket_get_descriptor(SynceSocket* socket)
{
  return socket->fd;
}

static bool synce_socket_create(SynceSocket* syncesock)
{
	bool success = false;
	
	if (syncesock->fd != RAPI_SOCKET_INVALID_FD)
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

bool synce_socket_connect(SynceSocket* syncesock, const char* host, int port)
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

bool synce_socket_listen(SynceSocket* socket, const char* host, int port)
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

SynceSocket* synce_socket_accept(SynceSocket* server, struct sockaddr_in* address)
{
	struct sockaddr_in cliaddr;
	int clilen;
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

bool synce_socket_close(SynceSocket* socket)
{
	if (!socket)
	{
		synce_socket_error("socket is null");
		return false;
	}
	
	if (socket->fd != RAPI_SOCKET_INVALID_FD)
	{
		close(socket->fd);
		socket->fd = RAPI_SOCKET_INVALID_FD;
		return true;
	}
	return false;
}

bool synce_socket_write(SynceSocket* socket, const void* data, unsigned size)
{
	if ( RAPI_SOCKET_INVALID_FD == socket->fd )
	{
		synce_socket_error("Invalid file descriptor");
		return false;
	}

	return write(socket->fd, data, size) == size;
}

bool synce_socket_read(SynceSocket* socket, void* data, unsigned size)
{
	int bytes_needed = size;
	
	if ( RAPI_SOCKET_INVALID_FD == socket->fd )
	{
		synce_socket_error("Invalid file descriptor");
		return false;
	}

	while(bytes_needed > 0)
	{
		int result = read(socket->fd, data, bytes_needed);
	
		/* synce_socket_trace("read returned %i, needed %i bytes", result, bytes_needed); */

		if (result <= 0)
		{
			synce_socket_error("read failed, error: %i \"%s\"", errno, strerror(errno));
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

	return events;
}

bool synce_socket_wait(SynceSocket* socket, int timeoutInSeconds, SocketEvents* events)
{
	/**
	 * This can easily be replaced by select() if needed on some platform
	 */
	bool success = false;
	int result;
	struct pollfd pfd;
	
	if ( RAPI_SOCKET_INVALID_FD == socket->fd )
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
	
	while (pfd.revents != 0)
	{
		synce_socket_trace("what?");
		pfd.revents = 0;
	}

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

