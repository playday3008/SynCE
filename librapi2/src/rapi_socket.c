/* $Id$ */
#include "rapi_socket.h"
#include "rapi_endian.h"
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

/**
 * Write a number of bytes of data to socket
 */
static bool rapi_socket_write(RapiSocket* socket, void* data, unsigned size)
{
	return write(socket->fd, data, size) == size;
}

/**
 * Read a number of bytes of data from a socket
 */
static bool rapi_socket_read(RapiSocket* socket, void* data, unsigned size)
{
	int bytes_needed = size;
	
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

bool rapi_socket_send(RapiSocket* socket, RapiBuffer* buffer)
{
	u_int32_t size_le = htole32(rapi_buffer_get_size(buffer));

	if ( RAPI_SOCKET_INVALID_FD == socket->fd )
		goto fail;
	
	if ( !rapi_socket_write(socket, &size_le, sizeof(size_le)) )
		goto fail;

	if ( !rapi_socket_write(socket, 
				rapi_buffer_get_raw(buffer), 
				rapi_buffer_get_size(buffer)) )
		goto fail;

	return true;

fail:
	/* XXX: is it wise to close the connection here? */
	rapi_socket_close(socket);
	return false;
}

bool rapi_socket_recv(RapiSocket* socket, RapiBuffer* buffer)
{
	u_int32_t      size_le = 0;
	size_t         size    = 0;
	unsigned char* data    = NULL;
	
	if ( RAPI_SOCKET_INVALID_FD == socket->fd )
	{
		rapi_socket_error("Invalid file descriptor");
		goto fail;
	}

	if ( !rapi_socket_read(socket, &size_le, sizeof(size_le)) )
	{
		rapi_socket_error("Failed to read size");
		goto fail;
	}

	size = letoh32(size_le);

	rapi_socket_trace("Size = 0x%08x\n", size);

	data = malloc(size);
	if (!data)
	{
		rapi_socket_error("Failed to allocate 0x%08x bytes", size);
		goto fail;
	}

	if ( !rapi_socket_read(socket, data, size) )
	{
		rapi_socket_error("Failed to read 0x%08x bytes", size);
		goto fail;
	}

	if ( !rapi_buffer_reset(buffer, data, size) )
	{
		free(data);
		goto fail;
	}

	return true;
	
fail:
	/* XXX: is it wise to close the connection here? */
	rapi_socket_close(socket);
	return false;	
}

