/***************************************************************************
 *   Copyright (C) 2001 by Ludovic LANGE                                   *
 *   ludovic.lange@free.fr                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#define BUFSIZE 16384
#include <stdarg.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#include <string.h>
#include <unistd.h>

#include "chunk.h"
/*
#include <iconv.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
*/

/* ************************************************************************** */
/* Functions for debugging buffers */
/* ************************************************************************** */

void _DBG_printbuf( rapibuffer * buf )
{
	int i, j, k;
	long buflen;
	long size;
	long rem;
        unsigned char * bufchar;

	/* szbuf = 4 + ( ( long ) * ( ( long * ) ( buf ) ) ); */

        buflen = letoh32(buf->bufferlen);

        buflen += 4;

	printf( "Buflen = %04ld  ", buflen );
	size = buflen / 16;
	rem = buflen % 16;
        bufchar = (unsigned char *) buf;

	for ( k = 0, j = 0; ( k < size ) && ( k < 100 ); k++ )
	{
		for ( i = 0; i < 16; i++ )
		{
			printf( "%02X ", bufchar[ k * 16 + i ] );
			if ( i == 7 )
			{
				printf( " " );
			}
		}
		printf( "  " );
		for ( i = 0; i < 16; i++ )
		{
			printf( "%c", isprint( bufchar[ k * 16 + i ] ) ? bufchar[ k * 16 + i ] : '.' );
			if ( i == 7 )
			{
				printf( " " );
			}
		}
		printf( "\n                " );
	}
	if ( rem > 0 )
	{
		for ( i = 0; i < rem; i++ )
		{
			printf( "%02X ", bufchar[ k * 16 + i ] );
			if ( i == 7 )
			{
				printf( " " );
			}
		}
		for ( i = rem; i < 16; i++ )
		{
			printf( "   " );
		}
		printf( "  " );
		for ( i = 0; i < rem; i++ )
		{
			printf( "%c", isprint( bufchar[ k * 16 + i ] ) ? bufchar[ k * 16 + i ] : '.' );
			if ( i == 7 )
			{
				printf( " " );
			}
		}
	}
	printf( "\n" );
}

void _DBG_printf( const char * str, ... )
{
	va_list ap;
	va_start( ap, str );
	vfprintf( stderr, str, ap );
	va_end( ap );
}

/***************************************************************************/
/* Functions for sockets                                                   */
/***************************************************************************/

int SockOpen( const char *host, int clientPort )
{
	int sock;
#ifndef INET_ATON
	unsigned long inaddr;
#endif /* INET_ATON */
	struct sockaddr_in ad;
	struct hostent *hp;

	memset( &ad, 0, sizeof( ad ) );
	ad.sin_family = AF_INET;

	/* we'll accept a quad address */
#ifndef INET_ATON
	inaddr = inet_addr( host );
	if ( inaddr != INADDR_NONE )
	{
		memcpy( &ad.sin_addr, &inaddr, sizeof( inaddr ) );
	}
	else
#else
if ( !inet_aton( host, &ad.sin_addr ) )
#endif /* INET_ATON */
	{
		hp = gethostbyname( host );

		/*
		 * Add a check to make sure the address has a valid IPv4 or IPv6
		 * length.  This prevents buffer spamming by a broken DNS.
		 */
		if ( hp == NULL || ( hp->h_length != 4 && hp->h_length != 8 ) )
		{
			return -1;
		}
		memcpy( &ad.sin_addr, hp->h_addr, hp->h_length );
	}
	ad.sin_port = htons( clientPort );

	sock = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sock < 0 )
	{
		return -1;
	}
	if ( connect( sock, ( struct sockaddr * ) & ad, sizeof( ad ) ) < 0 )
	{
		close( sock );
		return -1;
	}

	return ( sock );
}

#define SKIP_HEADER 4

/* **************************************************************************** */
/* Functions for buffers                                                        */
/* **************************************************************************** */

long initBuf( rapibuffer * destbuf, long destbuflen )
{
        destbuf->bufferlen = 0;
        memset( &(destbuf->data), 0, destbuflen );
	return 0;
}

int sendbuffer( int sock, rapibuffer * buffer )
{
	long buflen;

	buflen=_getbufferlen(buffer);

	return write( sock, buffer, 4 + buflen );
}

/*int readbuffer( int sock, unsigned char * destbuf, long size )
{
	long szbuf;
	int result;
	struct timeval tv;
	fd_set set;

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	FD_ZERO( &set );
	FD_SET( sock, &set );

	result = select( ( 1 + sock ), &set, NULL, NULL, &tv );
	if ( result )
	{
		result = read( sock, destbuf, 4 );
		if ( result == 4 )
		{
			szbuf = ( long ) * ( ( long * ) ( destbuf ) );

			if ( szbuf > 0 )
			{
				if ( szbuf < size )
				{
					result = read( sock, ( destbuf + 4 ), szbuf );
				}
			}
		}
	}
	return result;
} */

void flushbuffer( int sock )
{
	long szbuf;
	struct timeval tv;
	fd_set set;
	size_t result;

	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	FD_ZERO( &set );
	FD_SET( sock, &set );

	szbuf = 0;
	do
	{
		result = select( ( 1 + sock ), &set, NULL, NULL, &tv );
		if ( result )
		{
			result = read( sock, &szbuf, 4 );
			if(result>0) DBG_printf("!!! flushbuffer: %d !!!\n",result);
		}
	}
	while ( result );
}

long getbufferlen( int sock )
{
	long buflen;
	struct timeval tv;
	fd_set set;
	size_t result;

	tv.tv_sec = 30;
	tv.tv_usec = 0;
	FD_ZERO( &set );
	FD_SET( sock, &set );

	buflen = 0;
	result = select( ( 1 + sock ), &set, NULL, NULL, &tv );
	if ( result )
	{
		result = read( sock, &buflen, 4 );
		if ( result == 4 )
		{
                        buflen = letoh32( buflen );
			if ( buflen < 0 )
			{
				buflen = 0;
			}
		}
		else
		{
			buflen = 0;
		}
	}
	else
	{
		DBG_printf("timeout!!!\n");
	}
	if ( buflen == 0 )
	{
		flushbuffer( sock );
	}
	return buflen;
}

size_t getbufferchunk( int sock, long *counter, void * buffer, long nbbytes )
{
	long szbuf;
	struct timeval tv;
	fd_set set;
	size_t result;

	szbuf = 0;
	result = 0;

	DBG_printf("read counter=%ld nbbytes=%ld\n",*counter,nbbytes);
	if( ( counter && ( *counter >= nbbytes ) ) || (!counter) )
	{
                while( nbbytes > 0 )
		{
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			FD_ZERO( &set );
			FD_SET( sock, &set );

			result = select( ( 1 + sock ), &set, NULL, NULL, &tv );
                        if ( result )
			{
				result = read( sock, buffer, nbbytes );
				if ( result > 0 )
				{
					nbbytes -= result;
                                        if( counter )
                                        {
					        ( *counter ) -= result;
                                        }
					buffer += result;
				}
			}
			else
			{
				nbbytes = 0;
				result = 0;
			}
		}
	}
	return result;
}

long pushLong( rapibuffer * destbuf, long destbuflen, long param )
{
	long buflen;
        unsigned char * destbufchar;

	buflen=_getbufferlen(destbuf);

        destbufchar = (unsigned char *) &(destbuf->data);

	if ( buflen < destbuflen )
	{
		* ( ( long * ) ( destbufchar + buflen ) ) = htole32(param);
		_setbufferlen(destbuf,buflen+sizeof(param));
	}
	return buflen + sizeof( param );
}

long getLong( int sock, long *buflen )
{
	long param;
	size_t result;

	result = getbufferchunk( sock, buflen, &param, sizeof( param ) );

	return param;
}

long pushShort( rapibuffer * destbuf, long destbuflen, short param )
{
	long buflen;
        unsigned char * destbufchar;

        buflen = _getbufferlen(destbuf);
	
        destbufchar = (unsigned char *) &(destbuf->data);

	if ( buflen < destbuflen )
	{
		* ( ( short * ) ( destbufchar + buflen ) ) = htole16(param);
		_setbufferlen(destbuf,buflen+sizeof(param));
	}
	return buflen + sizeof( param );
}

short getShort( int sock, long *buflen )
{
	short param;
	size_t result;

	result = getbufferchunk( sock, buflen, &param, sizeof( param ) );

	return param;
}

long pushString( rapibuffer * destbuf, long destbuflen, LPCWSTR string )
{
	long buflen;
	char * param;
	int i;
	size_t stlen;
        unsigned char * destbufchar;

	stlen = 1 + wcslen( string );

        buflen = _getbufferlen(destbuf);

        destbufchar = (unsigned char *) &(destbuf->data);

	if ( buflen < destbuflen )
	{
		param = ( char * ) string;
		for ( i = 0; i < stlen * sizeof( WCHAR ); i++ )
		{
        		* ( ( char * ) ( destbufchar + buflen + i) ) = param[ i ];
		}

		destbuf->bufferlen = htole32(buflen + stlen * sizeof( WCHAR ));
	}
	return buflen + stlen * sizeof( WCHAR );
}

size_t getFileTime( int sock, long *bufsize, FILETIME * param )
{
	size_t result = 0;

	if ( param )
	{
		result = getbufferchunk( sock, bufsize, param, sizeof( *param ) );
	}

	return result;
}

WCHAR * getString( int sock, long *bufsize, long sz )
{
#ifdef CONVERT_STRING
	iconv_t cd;
	size_t ibl, obl;
	char *iptr, *optr;
#endif
	size_t result;
	static WCHAR inbuf[ BUFSIZE ];

#ifdef REAL_WCHAR_T
	static _WIN_WCHAR destbuf[ BUFSIZE ];
	long index;
	result = getbufferchunk( sock, bufsize, destbuf, ( 2 * sz ) );
	for ( index = 0; index < sz; index ++ )
	{
		DBG_printf( "%c", ( destbuf[ index ] & 0xFF ) > 31 ? ( destbuf[ index ] & 0xFF ) : '.' );
		inbuf[ index ] = ( WCHAR ) destbuf[ index ];
	}
	DBG_printf( "\n" );
#else
	DBG_printf( "getString : size = %ld, lg = %ld\n", *bufsize, sz );
	result = getbufferchunk( sock, bufsize, inbuf, ( 2 * sz ) );
#endif


#ifdef CONVERT_STRING
	if ( result == sz )
	{
		cd = iconv_open( "latin1", "UCS-2" );
		iptr = inbuf;
		ibl = sz;
		obl = BUFSIZE;
		optr = destbuf;
		result = iconv( cd, &iptr, &ibl, &optr, &obl );
		iconv_close( cd );
	}
#endif
	return inbuf;
}

