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

#define RAPI_SOCKET_DEBUG 1

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

bool rapi_socket_connect(RapiSocket* rapisock, const char* host, int port)
{
	struct sockaddr_in servaddr;
	
	rapi_socket_close(rapisock);

	/* create socket */
	if ( (rapisock->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
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

bool rapi_socket_close(RapiSocket* socket)
{
	if (socket->fd != RAPI_SOCKET_INVALID_FD)
	{
		close(socket->fd);
		socket->fd = RAPI_SOCKET_INVALID_FD;
		return true;
	}
	return false;
}

bool rapi_socket_write(RapiSocket* socket, void* data, unsigned size)
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


