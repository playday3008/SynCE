/* $Id$ */
#include "synce_socket.h"
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

SynceSocket* synce_socket_accept(SynceSocket* server)
{
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	int connfd;
	SynceSocket* client = NULL;
	
	clilen = sizeof(cliaddr);
	if ( (connfd = accept(server->fd, (struct sockaddr*)&cliaddr, &clilen)) < 0 )
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
	
	/* TODO: do something with client address? */

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
		int result = read(socket->fd, data, size);
	
		synce_socket_trace("read returned %i, needed %i bytes", result, bytes_needed);

		if (result < 0)
		{
			synce_socket_error("read failed, error: %i \"%s\"", errno, strerror(errno));
			break;
		}
		else if (result == 0)
		{
			break;
		}

		bytes_needed -= result;
		data = (char*)data + result;
	}
	 
	return 0 == bytes_needed;
}


