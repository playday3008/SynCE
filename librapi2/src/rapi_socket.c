/* $Id$ */
#include "rapi_socket.h"
#include "rapi_endian.h"
#include <stdlib.h>
#include <unistd.h>

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

bool rapi_socket_connect(RapiSocket* socket, const char* host, int port)
{
	/* TODO: implement */
	return false;
}

bool rapi_socket_close(RapiSocket* socket)
{
	if (socket->fd != RAPI_SOCKET_INVALID_FD)
	{
		close(socket->fd);
		socket->fd = RAPI_SOCKET_INVALID_FD;
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
	return read(socket->fd, data, size) == size;
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
		goto fail;

	if ( !rapi_socket_read(socket, &size_le, sizeof(size_le)) )
		goto fail;

	size = letoh32(size_le);

	data = malloc(size);
	if (!data)
		goto fail;

	if ( !rapi_buffer_reset(buffer, data, size) )
	{
		free(data);
		goto fail;
	}

	if ( !rapi_socket_read(socket, data, size) )
		goto fail;

	return true;
	
fail:
	/* XXX: is it wise to close the connection here? */
	rapi_socket_close(socket);
	return false;	
}

