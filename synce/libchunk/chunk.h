/***************************************************************************
 *   Copyright (C) 2001 by Ludovic LANGE                                   *
 *   ludovic.lange@free.fr                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "windows.h"
#include "little_endian.h"

typedef struct {
        unsigned long bufferlen;
        unsigned char * data;
} rapibuffer;

#ifdef DEBUG
#define DBG_printbuf(a) _DBG_printbuf( a )
#define DBG_printf _DBG_printf
#else
#define DBG_printbuf(a)
#define DBG_printf( str, args... )
#endif

/**
 * This library deals with packets (called 'rapibuffer') having the following format :
 * 0x00000000                   bufferlen: a long containing the len of the buffer.
 * 0x00000008                   Beginning of the data
 * ..........                   .....................
 * 0x00000008 + bufferlen       End of the data.
 */

/**
 * A debugging function.
 * prints the content of the buffer.
 * @param buf : a buffer
 */
void _DBG_printbuf( rapibuffer * buf );

/**
 * A debugging function.
 * prints the string.
 * @param str : the format string.
 */
void _DBG_printf( const char * str, ... );

/***************************************************************************/
/* internal inline functions                                               */
/***************************************************************************/

/* FIXME: conceptional fault: the next three functions are used in
   rapi.c:CeWriteFile() */

__inline__ void _setbufferlen(rapibuffer *buffer, unsigned long bufferlen)
{
	buffer->bufferlen = htole32(bufferlen);
}

__inline__ unsigned long _getbufferlen(rapibuffer *buffer)
{
	return letoh32(buffer->bufferlen);
}

/* A safer write(), since sockets might not write all but only some of
   the bytes requested */

__inline__ int safe_write(int fd, char *buf, int len)
{
	int done,todo=len;
	
	while(todo)
	{
		done=write(fd,buf,todo);
		if(done<=0) return(done);
		buf+=done;
		todo-=done;
	}
	return(len);
}

/**
 * Opens a socket to a specified host/port.
 * @param host : the host name.
 * @param clientPort : the port to connect to.
 * @returns the socket, or -1 in case of error.
 */
int SockOpen( const char *host, int clientPort );

/**
 * Initialises the specified buffer. Sets the size to 0.
 * @param destbuf : a rapibuffer.
 * @param destbuflen : the len of data in the buffer.
 * @returns 0.
 */
long initBuf( rapibuffer * destbuf, long destbuflen );

/**
 * Sends the buffer to the socket. The number of bytes to send is the buffersize field of the buffer ( aka *((long *)buffer[0]) ).
 * @param sock : the socket, previously opened.
 * @param buffer : a proper buffer.
 * @returns the number of bytes written. Same meaning as the write() function.
 */
int sendbuffer( int sock, rapibuffer * buffer );

/**
 * Reads <size> bytes from the socket into the buffer.
 * @param sock : the socket.
 * @param destbuf : the buffer.
 * @param size : number of bytes to read.
 * @returns number of bytes read. Same meaning as the read() function.
 */
/* int readbuffer( int sock, rapibuffer * destbuf, long size ); */

/**
 * Flushes the remaining unread bytes from the socket. Use only when debugging.
 * @param sock : the socket.
 */
void flushbuffer( int sock );

/**
 * A function that reads 4 bytes from the socket, and returns a long from it. Is the first function to call after
 * receiving an answer from the device.
 * @param sock : the socket.
 * @returns : the length of the following buffer.
 */
long getbufferlen( int sock );

/**
 * Reads a block of a give size into a buffer. Decreases a counter.
 * @param sock : the socket.
 * @param counter : pointer to a counter that will be decremented of the number of bytes read. Must be greater or equal than buflen, or NULL.
 * @param buffer : a buffer to store the data.
 * @param nbbytes : number of bytes to read.
 * @returns the number of bytes read.
 */
size_t getbufferchunk( int sock, long *counter, void * buffer, long nbbytes );

/**
 * Store a long param into the buffer, increments the bufferlen property of the buffer.
 * @param destbuf : the buffer. its bufferlen property will be increased by 4 ( i.e. sizeof(long) ).
 * @param destbuflen : the (allocated) maximum length the buffer can hold. A border not to cross.
 * @param param : the long to store in the buffer.
 * @returns the new bufferlen property of the buffer.
 */
long pushLong( rapibuffer * destbuf, long destbuflen, long param );
long getLong( int sock, long *bufsize );

/**
 * Store a short param into the buffer, increments the bufferlen property of the buffer.
 * @param destbuf : the buffer. its bufferlen property will be increased by 2 ( i.e. sizeof(short) ).
 * @param destbuflen : the (allocated) maximum length the buffer can hold. A border not to cross.
 * @param param : the long to store in the buffer.
 * @returns the new bufferlen property of the buffer.
 */
long pushShort( rapibuffer * destbuf, long destbuflen, short param );
short getShort( int sock, long *bufsize );

size_t getFileTime( int sock, long *bufsize, FILETIME * param );

/**
 * Store a wide string param into the buffer, increments the bufferlen property of the buffer.
 * @param destbuf : the buffer. its bufferlen property will be increased by twice the number of characters in the string ( i.e. wstrlen(string) ).
 * @param destbuflen : the (allocated) maximum length the buffer can hold. A border not to cross.
 * @param string : the string to store in the buffer.
 * @returns the new bufferlen property of the buffer.
 */
long pushString( rapibuffer * destbuf, long destbuflen, LPCWSTR /*const char * */ string );
WCHAR * getString( int sock, long *bufsize, long sz );

