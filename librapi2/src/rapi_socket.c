/* $Id$ */
#include "rapi_socket.h"
#include "rapi_endian.h"
#include "rapi_internal.h"
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#if HAVE_DMALLOC_H
#include "dmalloc.h"
#endif

#define RAPI_SOCKET_DEBUG 0

#if RAPI_SOCKET_DEBUG
#define rapi_socket_trace(args...)    rapi_trace(args)
#define rapi_socket_warning(args...)  rapi_warning(args)
#define rapi_socket_error(args...)    rapi_error(args)
#else
#define rapi_socket_trace(args...)
#define rapi_socket_warning(args...)
#define rapi_socket_error(args...)
#endif

#define RAPI_SOCKET_INVALID_FD -1
#define RAPI_SOCKET_LISTEN_QUEUE  1024

struct _RapiSocket
{
	int fd;
};

RapiSocket* rapi_socket_new(/* TODO: some parameters here */)
{
	RapiSocket* socket = calloc(1, sizeof(RapiSocket));
	
	if (socket)
	{
		socket->fd = RAPI_SOCKET_INVALID_FD;
	}
	
	return socket;
}

void rapi_socket_free(RapiSocket* socket)
{
	if (socket)
	{
		rapi_socket_close(socket);
		free(socket);
	}
}

static bool rapi_socket_create(RapiSocket* rapisock)
{
	bool success = false;
	
	if (rapisock->fd != RAPI_SOCKET_INVALID_FD)
	{
		rapi_socket_error("already have a socket file descriptor");
		goto fail;
	}
	
	/* create socket */
	if ( (rapisock->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		goto fail;

	success = true;
	
fail:
	return success;
}

bool rapi_socket_connect(RapiSocket* rapisock, const char* host, int port)
{
	struct sockaddr_in servaddr;
	
	rapi_socket_close(rapisock);

	if (!rapi_socket_create(rapisock))
		goto fail;

	/* fill in address structure */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if ( inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0 )
		goto fail;
	
	/* connect */
	if ( connect(rapisock->fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0 )
		goto fail;

	return true;

fail:
	rapi_socket_close(rapisock);
	return false;
}

bool rapi_socket_listen(RapiSocket* socket, const char* host, int port)
{
	struct sockaddr_in servaddr;
	int sock_opt;

	if (!rapi_socket_create(socket))
		goto fail;

	/* set SO_REUSEADDR */
	sock_opt = 1;
	if ( setsockopt(socket->fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt)) < 0 )
	{
		rapi_socket_error("setsockopt failed, error: %i \"%s\"", errno, strerror(errno));
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
		rapi_socket_error("bind failed, error: %i \"%s\"", errno, strerror(errno));
		goto fail;
	}

	if ( listen(socket->fd, RAPI_SOCKET_LISTEN_QUEUE) < 0 )
	{
		rapi_socket_error("listen failed, error: %i \"%s\"", errno, strerror(errno));
		goto fail;
	}

	return true;

fail:
	rapi_socket_close(socket);
	return false;
}

RapiSocket* rapi_socket_accept(RapiSocket* server)
{
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	int connfd;
	RapiSocket* client = NULL;
	
	clilen = sizeof(cliaddr);
	if ( (connfd = accept(server->fd, (struct sockaddr*)&cliaddr, &clilen)) < 0 )
	{
		rapi_socket_error("accept failed, error: %i \"%s\"", errno, strerror(errno));
		goto exit;
	}

	rapi_socket_trace("accepted connection with file descriptor %i", connfd);
	
	client = rapi_socket_new();
	if (!client)
	{
		rapi_socket_error("failed to create new socket");
		goto exit;
	}

	client->fd = connfd;
	
	/* TODO: do something with client address? */

exit:
	return client;
}

bool rapi_socket_close(RapiSocket* socket)
{
	if (!socket)
	{
		rapi_socket_error("socket is null");
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

bool rapi_socket_write(RapiSocket* socket, const void* data, unsigned size)
{
	if ( RAPI_SOCKET_INVALID_FD == socket->fd )
	{
		rapi_socket_error("Invalid file descriptor");
		return false;
	}

	return write(socket->fd, data, size) == size;
}

bool rapi_socket_read(RapiSocket* socket, void* data, unsigned size)
{
	int bytes_needed = size;
	
	if ( RAPI_SOCKET_INVALID_FD == socket->fd )
	{
		rapi_socket_error("Invalid file descriptor");
		return false;
	}

	while(bytes_needed > 0)
	{
		int result = read(socket->fd, data, size);
	
		rapi_socket_trace("read returned %i, needed %i bytes", result, bytes_needed);

		if (result < 0)
		{
			rapi_socket_error("read failed, error: %i \"%s\"", errno, strerror(errno));
			break;
		}
		else if (result == 0)
		{
			break;
		}

		bytes_needed -= result;
		data += result;
	}
	
	 
	return 0 == bytes_needed;
}


