/***************************************************************************
 *   Copyright (C) 2001 by Ludovic LANGE                                   *
 *   ludovic.lange@free.fr                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
/*
 * rapi.c
 * (c) 2001 Ludovic LANGE,
 * except SockOpen, (c) Unknown ? from socket.c / socket.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#undef VERSION

#define MIN(a,b) ((a)<(b)?(a):(b))

#include <iconv.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "little_endian.h"

#include "chunk.h"
#include "rapi.h"

/*
 * Global Variables ! Yuck !
 */
int sock = 0;
DWORD _lasterror = 0;
#define BUFSIZE 16384
/* unsigned char buffer[ BUFSIZE ]; */
rapibuffer * buffer = NULL;


/*=================================================================================================================*
 *=================================================================================================================*
 * RAPI - Global
 *=================================================================================================================*
 *=================================================================================================================*/

STDAPI_( DWORD ) CeGetSpecialFolderPath( int nFolder, DWORD nBufferLength, LPWSTR lpBuffer )
{
	long size = BUFSIZE;
	long lng;
	WCHAR * str;

	initBuf( buffer, size );
	pushLong( buffer, size, 0x44 ); 		/* Command */
	pushLong( buffer, size, nFolder ); 		/* Parameter1 : the folder */
	pushLong( buffer, size, nBufferLength ); 	/* Parameter2 : Buffer size that'll get the string */
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	lng = getLong( sock, &size );
	DBG_printf( "long 2 : %ld (0x%08lx)\n", lng, lng );
	lng = getLong( sock, &size );
	DBG_printf( "string size : %ld (0x%08lx)\n", lng, lng );
	str = getString( sock, &size, lng+1 );
	DBG_printf( "string1 : %s\n", str );
	if ( lpBuffer )
	{
		memcpy( lpBuffer, str, MIN( ((lng+1)*sizeof(WCHAR) ), nBufferLength*sizeof(WCHAR) ) );
	}
	return lng;
}

/* ================================================================================================================= */
/* ================================================================================================================= */
/*  RAPI - Registry */
/* ================================================================================================================= */
/* ================================================================================================================= */

/*
STDAPI_(LONG) CeRegDeleteKey( HKEY, LPCWSTR );
STDAPI_(LONG) CeRegDeleteValue( HKEY, LPCWSTR );
STDAPI_(LONG) CeRegQueryValueEx( HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD );
STDAPI_(LONG) CeRegSetValueEx( HKEY, LPCWSTR, DWORD, DWORD, LPBYTE, DWORD );
*/

STDAPI_( LONG ) CeRegCreateKeyEx( HKEY hKey, LPCWSTR lpszSubKey, DWORD Reserved, LPWSTR lpszClass, DWORD ulOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition )
{
	long size = BUFSIZE;
	long lng;
	long errcode;
	LONG result = ERROR_SUCCESS; /* May be result is really errcode. */

	DBG_printf( "CeRegCreatKeyEx( hKey = 0x%08X, lpszSubKey = 0x%08X, Reserved = 0x%08X, lpszClass = 0x%08X, ulOptions = 0x%08X, samDesired = 0x%08X, lpSecurityAttributes = 0x%08X, phkResult = 0x%08X, lpdwDisposition = 0x%08X )\n",
	            hKey, lpszSubKey, Reserved, lpszClass, ulOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x20 ); 			/* Command */
	pushLong( buffer, size, hKey ); 			/* Parameter1 : */
	pushLong( buffer, size, 0x01 ); 			/* Parameter2 : */
	pushLong( buffer, size, 1 + wcslen( lpszSubKey ) ); 	/* Parameter3 : */
	pushString( buffer, size, lpszSubKey ); 		/* Parameter4 : the path */
	pushLong( buffer, size, 0x01 ); 			/* Parameter5 : */
	pushLong( buffer, size, 0x01 ); 			/* Parameter6 : */
	pushShort( buffer, size, 0x00 );		 	/* Parameter7 : */

	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	errcode = getLong( sock, &size );
	DBG_printf( "errpresent : %ld (0x%08lx)\n", errcode, errcode );
	if ( errcode != 0 )
	{
		errcode = getLong( sock, &size );
		DBG_printf( "errcode : %ld (0x%08lx)\n", errcode, errcode );
	}
	else
	{
		lng = getLong( sock, &size );
		DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
		lng = getLong( sock, &size );
		DBG_printf( "long 2 : %ld (0x%08lx)\n", lng, lng );
		*phkResult = ( HKEY ) getLong( sock, &size );
		DBG_printf( "pHkey : %ld (0x%08lx)\n", *phkResult, *phkResult );
		*lpdwDisposition = ( DWORD ) getLong( sock, &size );
		DBG_printf( "lpdwDisposition : %ld (0x%08lx)\n", *lpdwDisposition, *lpdwDisposition );
	}
	return result;
}

STDAPI_( LONG ) CeRegOpenKeyEx( HKEY hKey, LPCWSTR lpszSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult )
{
	long size = BUFSIZE;
	long errcode;

	DBG_printf( "CeRegOpenKeyEx( hKey = 0x%08X, lpszSubKey = 0x%08X, ulOptions = 0x%08X, samDesired = 0x%08X, phkResult = 0x%08X )\n",
	            hKey, lpszSubKey, ulOptions, samDesired, phkResult );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x1E ); 	/* Command */
	pushLong( buffer, size, hKey ); 		/* Parameter1 : */
	pushLong( buffer, size, 0x01 ); 	/* Parameter2 : */
	pushLong( buffer, size, 1 + wcslen( lpszSubKey ) ); 	/* Parameter3 : */
	pushString( buffer, size, lpszSubKey ); 	/* Parameter4 : the path */
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	errcode = getLong( sock, &size );
	DBG_printf( "errcode : %ld (0x%08lx)\n", errcode, errcode );

	errcode = getLong( sock, &size );
	DBG_printf( "errpresent : %ld (0x%08lx)\n", errcode, errcode );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long : %ld (0x%08lx)\n", _lasterror, _lasterror );
	if ( errcode == 0 )
	{
		*phkResult = ( HKEY ) getLong( sock, &size );
		DBG_printf( "pHkey : %ld (0x%08lx)\n", *phkResult, *phkResult );
	}
	return errcode;
}

STDAPI_( LONG ) CeRegCloseKey( HKEY hKey )
{
	long size = BUFSIZE;
	long lng;
	long result;

	DBG_printf( "CeRegCloseKey( hKey = 0x%08X )\n",
	            hKey );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x21 ); 	/* Command */
	pushLong( buffer, size, hKey ); 		/* Parameter1 : */

	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );
	size = getbufferlen( sock );

	result = getLong( sock, &size );
	DBG_printf( "result : %ld (0x%08lx)\n", result, result );
	lng = getLong( sock, &size );
	DBG_printf( "long 2 : %ld (0x%08lx)\n", lng, lng );
	lng = getLong( sock, &size );
	DBG_printf( "long 3 : %ld (0x%08lx)\n", lng, lng );
	return result;
}

/*
 * Push parameter information to the buffer
 *
 * DWORD parameter values must be 4 byte little-endian
 * WORD parameter values must be 2 byte little-endian
 * WCHAR parameter values must be 2-byte UNICODE (is it endian stuff on these too?) 
 */
static void pushParameter(long size, void * parameterData, size_t parameterSize, int pushValue)
{
	if (parameterData)
	{
		pushLong( buffer, size, 1 );
		pushLong( buffer, size, parameterSize);

		if (pushValue)	/* pushValue is boolean */
		{
			long old_length;
			long new_length;

			pushLong( buffer, size, 1 );	/* data follows */

			old_length = _getbufferlen(buffer);
			new_length = old_length + parameterSize;

			if (new_length < size)
			{
				memcpy(((unsigned char*)&(buffer->data)) + old_length, parameterData, parameterSize);
				_setbufferlen(buffer, new_length);
			}
			else
			{
				DBG_printf( "rapibuffer too small, this will fail\n");
			}
		}
		else
		{
			pushLong( buffer, size, 0 );	/* no data follows */
		}
	}
	else
	{
		pushLong( buffer, size, 0 ); /* no parameter info */
	}
}

/**
 * Convert parameter to little endian before call to pushParameter
 */
#define pushParameterInt16(size, parameterData, pushValue) \
if (pushValue && parameterData) \
	{ *(u_int16_t*)parameterData = htole16(*(u_int16_t*)parameterData); } \
pushParameter(size, parameterData, sizeof(u_int16_t), pushValue);

#define pushParameterInt32(size, parameterData, pushValue) \
if (pushValue && parameterData) \
	{ *(u_int32_t*)parameterData = htole32(*(u_int32_t*)parameterData); } \
pushParameter(size, parameterData, sizeof(u_int32_t), pushValue)


/**
 * Pop parameter info from the buffer
 */
static void popParameter(long* size, void * parameterData, size_t parameterMaxSize)
{
	long lng;
	
	lng = getLong(sock, size);	/* parameter info available? */

	if (1 == lng)
	{
		size_t parameterRealSize = getLong(sock, size);	/* parameter value size in buffer */

		lng = getLong(sock, size); /* parameter value available? */

		if (1 == lng)
		{
			int overflow; 

			if (parameterData)
			{
				/* read directly to buffer */
				getbufferchunk(sock, size, parameterData, MIN(parameterRealSize, parameterMaxSize));
				overflow = parameterRealSize - parameterMaxSize;
			}
			else
			{
				/* throw it all away */
				overflow = parameterRealSize;
			}
				
			if (overflow > 0)
			{
				/* read overflowed buffer */
				void* tmp = malloc(overflow);
				getbufferchunk(sock, size, tmp, overflow);
				free(tmp);

				if (parameterData)
					DBG_printf("Overflow by %i bytes. Parameter size is %i bytes but max %i bytes was expected. (%i bytes remaining)\n",
							overflow, parameterRealSize, parameterMaxSize , *size);
			}
		}
		else if (0 != lng)
		{
			DBG_printf("popParameter (a): Expected 0 or 1 but got %i=0x%x (%i bytes remaining)\n", lng, lng, *size);
		}
	}
	else if (0 != lng)
	{
		DBG_printf("popParameter (b): Expected 0 or 1 but got %i=0x%x (%i bytes remaining)\n", lng, lng, *size);
	}
}

/**
 * Convert parameter data from little endian after call to popParameter
 */
#define popParameterInt16(size, parameterData) \
popParameter(size, parameterData, sizeof(u_int16_t)); \
if (parameterData) \
	{ *(u_int16_t*)parameterData = letoh16(*(u_int16_t*)parameterData); }

#define popParameterInt32(size, parameterData) \
popParameter(size, parameterData, sizeof(u_int32_t)); \
if (parameterData) \
	{ *(u_int32_t*)parameterData = letoh32(*(u_int32_t*)parameterData); }



STDAPI_( LONG ) CeRegQueryInfoKey( HKEY hKey, LPWSTR lpClass, LPDWORD lpcbClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen, LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime )
{
	long size = BUFSIZE;
	long lng;
	LONG result = ERROR_SUCCESS;

#if 0
	DBG_printf( "CeRegQueryInfoKey( hKey = 0x%08X, lpClass = 0x%08X, lpcbClass = 0x%08X, lpReserved = 0x%08X, lpcSubKeys = 0x%08X, lpcbMaxSubKeyLen = 0x%08X, lpcbMaxClassLen = 0x%08X, lpcValues = 0x%08X, lpcbMaxValueNameLen = 0x%08X, lpcbMaxValueLen = 0x%08X, lpcbSecurityDescriptor = 0x%08X, lpftLastWriteTime = 0x%08X )\n",
	            hKey, lpClass, lpcbClass, lpReserved, lpcSubKeys, lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime );
#endif

	initBuf( buffer, size );
	pushLong( buffer, size, 0x25 ); 	/* Command */
	pushLong( buffer, size, hKey ); 	/* hKey */

	pushParameter(size, lpClass, lpcbClass ? *lpcbClass * sizeof(WCHAR): 0, 0);
	pushParameterInt32(size, lpcbClass, 1);
	pushParameterInt32(size, lpReserved, 0);
	pushParameterInt32(size, lpcSubKeys, 0);
	pushParameterInt32(size, lpcbMaxSubKeyLen, 0);
	pushParameterInt32(size, lpcbMaxClassLen, 0);
	pushParameterInt32(size, lpcValues, 0);
	pushParameterInt32(size, lpcbMaxValueNameLen, 0);
	pushParameterInt32(size, lpcbMaxValueLen, 0);
	pushParameterInt32(size, lpcbSecurityDescriptor, 1);
	if (lpftLastWriteTime)
	{
		lpftLastWriteTime->dwLowDateTime = (DWORD)htole32(lpftLastWriteTime->dwLowDateTime);
		lpftLastWriteTime->dwHighDateTime = (DWORD)htole32(lpftLastWriteTime->dwHighDateTime);
	}
	pushParameter(size, lpftLastWriteTime, sizeof(FILETIME), 0);

/*	DBG_printbuf( buffer );*/
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	result = lng = getLong( sock, &size );
	_lasterror = lng = getLong( sock, &size );

	if (ERROR_SUCCESS == result)
	{
		popParameter(&size, lpClass, lpcbClass ? *lpcbClass: 0);
		popParameterInt32(&size, lpcbClass);
		popParameterInt32(&size, lpReserved);
		popParameterInt32(&size, lpcSubKeys);
		popParameterInt32(&size, lpcbMaxSubKeyLen);
		popParameterInt32(&size, lpcbMaxClassLen);
		popParameterInt32(&size, lpcValues);
		popParameterInt32(&size, lpcbMaxValueNameLen);
		popParameterInt32(&size, lpcbMaxValueLen);
		popParameterInt32(&size, lpcbSecurityDescriptor);
		popParameter(&size, lpftLastWriteTime, sizeof(FILETIME));
		if (lpftLastWriteTime)
		{
			lpftLastWriteTime->dwLowDateTime = (DWORD)letoh32(lpftLastWriteTime->dwLowDateTime);
			lpftLastWriteTime->dwHighDateTime = (DWORD)letoh32(lpftLastWriteTime->dwHighDateTime);
		}
	}

	if ( size > 0 )
	{
		DBG_printf( "size : %d\n", size );
/*		flushbuffer( sock );*/
	}
	return result;
}

STDAPI_( LONG ) CeRegEnumValue( HKEY hKey, DWORD dwIndex, LPWSTR lpszValueName, LPDWORD lpcbValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData )
{
	LONG result = ERROR_SUCCESS;
	long size = BUFSIZE;

#if 0
	DBG_printf( "CeRegEnumValue( hKey = 0x%08X, dwIndex = 0x%08X, lpszValueName = 0x%08X, lpcbValueName = 0x%08X, lpReserved = 0x%08X, lpType = 0x%08X, lpData = 0x%08X, lpcbData = 0x%08X )\n",
	            hKey, dwIndex, lpszValueName, ( *lpcbValueName ), lpReserved, ( *lpType ), ( *lpData ), ( *lpcbData ) );
#endif

	/* maybe check more parameters */
	if (!lpcbValueName || lpReserved)
		return 87; /* ERROR_INVALID_PARAMETER */
		
	initBuf( buffer, size );
	pushLong( buffer, size, 0x23 ); 	/* Command */
	pushLong( buffer, size, hKey ); 	/* Parameter1 : */
	pushLong( buffer, size, dwIndex ); 	/* Parameter2 : */

	pushParameter(size, lpszValueName, lpcbValueName ? *lpcbValueName * sizeof(WCHAR): 0, 0);
	pushParameterInt32(size, lpcbValueName, 1);
	pushParameterInt32(size, lpReserved, 1);
	pushParameterInt32(size, lpType, 1);
	pushParameter(size, lpData, lpcbData ? *lpcbData : 0, 0);
	pushParameterInt32(size, lpcbData, 1);

/*	DBG_printbuf( buffer );*/
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );
	
	result = getLong( sock, &size ); 
	_lasterror = getLong( sock, &size );

	if (ERROR_SUCCESS == result)
	{
		/* here comes a result again! */
		result = getLong( sock, &size );
	}
		
	if (ERROR_SUCCESS == result)
	{
		popParameter(&size, lpszValueName, lpcbValueName ? *lpcbValueName : 0);
		popParameterInt32(&size, lpcbValueName);
		popParameterInt32(&size, lpType);
		popParameter(&size, lpData, lpcbData ? *lpcbData : 0);
		popParameterInt32(&size, lpcbData);
	}

	if ( size > 0 )
	{
		DBG_printf( "size : %d\n", size );
/*		flushbuffer( sock );*/
	}
	return result;
}

STDAPI_( LONG ) CeRegEnumKeyEx( HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcbClass, PFILETIME lpftLastWriteTime )
{
	long size = BUFSIZE;
	long lng;
	WCHAR * str;
	int i;
	long errcode;
	long strsz;
	long maxsz;

	DBG_printf( "CeRegEnumKeyEx( hKey = 0x%08X, dwIndex = 0x%08X, lpcbName = 0x%08X, lpReserved = 0x%08X, lpClass = 0x%08X, lpcbClass = 0x%08X, lpftLastWriteTime = 0x%08X )\n",
	            hKey, dwIndex, lpName, lpcbName ? ( (void*)*lpcbName ) : lpcbName, lpReserved, lpClass, lpcbClass ? ( (void*)*lpcbClass ) : lpcbClass, lpftLastWriteTime );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x1F ); 	/* Command */
	pushLong( buffer, size, hKey ); 	/* Parameter1 : */
	pushLong( buffer, size, dwIndex ); 	/* Parameter2 : */
	pushLong( buffer, size, 0x01 ); 	/* Parameter3 : */
	pushLong( buffer, size, 0x0202 ); 	/* Parameter4 : */
	pushLong( buffer, size, 0x00 ); 	/* Parameter5 : */
	pushLong( buffer, size, 0x01 ); 	/* Parameter6 : */
	pushLong( buffer, size, 0x04 ); 	/* Parameter7 : */
	pushLong( buffer, size, 0x01 ); 	/* Parameter8 : */
	pushLong( buffer, size, lpcbName ? ( *lpcbName ) : 0 ); 	/* Parameter9 : */
	pushLong( buffer, size, 0x00 ); 	/* Parameter10 : */
	pushLong( buffer, size, 0x00 ); 	/* Parameter11 : */
	pushLong( buffer, size, 0x00 ); 	/* Parameter12 : */
	pushLong( buffer, size, 0x00 ); 	/* Parameter13 : */
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );
	lng = getLong( sock, &size );
	DBG_printf( "long : %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long : %ld (0x%08lx)\n", _lasterror, _lasterror );
	errcode = getLong( sock, &size );
	DBG_printf( "errcode : %ld (0x%08lx)\n", errcode, errcode );
	lng = getLong( sock, &size );
	DBG_printf( "long : %ld (0x%08lx)\n", lng, lng );
	if ( errcode == 0 )
	{
		strsz = getLong( sock, &size );
		DBG_printf( "long : %ld (0x%08lx)\n", strsz, strsz );
		maxsz = 0;
		if ( lpcbName != NULL )
		{
			maxsz = ( *lpcbName );
			( *lpcbName ) = strsz - 1;
		}
		lng = getLong( sock, &size );
		DBG_printf( "long : %ld (0x%08lx)\n", lng, lng );
		str = getString( sock, &size, strsz >> 1 );
		DBG_printf( "string1 : %s\n", str );
		if ( ( lpName != NULL ) )
		{
			memcpy( lpName, str, MIN( strsz, maxsz ) );
		}
		for ( i = 0; i < 7; i++ )
		{
			lng = getLong( sock, &size );
			DBG_printf( "long %d : %ld (0x%08lx)\n", i, lng, lng );
		}
	}

	return errcode;
}

/* ================================================================================================================= */
/* ================================================================================================================= */
/*  RAPI - Tests */
/* ================================================================================================================= */
/* ================================================================================================================= */

#if 0
DWORD CeTestSec1()
{
	long size=BUFSIZE;
	long lng;
	int result;
	long index;
 
	initBuf( buffer, size );
	pushLong( buffer, size, 0x27 );	/* Command */
	pushLong( buffer, size, 0x80000002 );		/* Parameter1 : */
	pushLong( buffer, size, 0x01 );	/* Parameter2 : */
	pushLong( buffer, size, 0x20 );	/* Parameter3 : */
	pushString( buffer, size, "Comm\\SecurityProviders\\SCHANNEL" );	/* Parameter4 : the path */
	pushLong( buffer, size, 0x01 );	/* Parameter2 : */
	pushLong( buffer, size, 0x01 );	/* Parameter2 : */
	pushShort( buffer, size, 0x00 );	/* Parameter2 : */
 
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );
	result = readbuffer( sock, buffer, size );
	DBG_printbuf( buffer );
	index = 0;
	return 2*lng;
}

DWORD CeTest0()
{
	long size=BUFSIZE;
	long lng;
	int result;
	long index;
	
	initBuf( buffer, size );
	pushLong( buffer, size, 0x1E );	/* Command */
	pushLong( buffer, size, 0x80000000 );		/* Parameter1 : */
	pushLong( buffer, size, 0x01 );	/* Parameter2 : */
	pushLong( buffer, size, 0x27 );	/* Parameter3 : */
	pushString( buffer, size, "{000214A0-0000-0000-C000-000000000046}" );	/* Parameter3 : the path */
 
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );
	result = readbuffer( sock, buffer, size );
	DBG_printbuf( buffer );
	index = 0;
	return 2*lng;
}
#endif

/* ================================================================================================================= */
/* ================================================================================================================= */
/*  RAPI - Files */
/* ================================================================================================================= */
/* ================================================================================================================= */

STDAPI_( BOOL ) CeFindAllFiles( LPCWSTR szPath, DWORD dwFlags, LPDWORD lpdwFoundCount, LPLPCE_FIND_DATA ppFindDataArray )
{
	long size = BUFSIZE;
	long lng;
	WCHAR * str;
	long stlen = 0;
	long i;
	CE_FIND_DATA *ptr;

	initBuf( buffer, size );
	pushLong( buffer, size, 0x09 ); 			/* Command */
	pushLong( buffer, size, 0x01 ); 			/* Parameter1 : */
	pushLong( buffer, size, 1 + wcslen( szPath ) ); 	/* Parameter2 : */
	pushString( buffer, size, szPath ); 			/* Parameter3 : the path */
	pushLong( buffer, size, dwFlags ); 			/* Parameter4 : Flags ? */
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );
	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	lng = getLong( sock, &size );
	DBG_printf( "long 2 (nb answer) : %ld (0x%08lx)\n", lng, lng );
	if ( lpdwFoundCount )
	{
		*lpdwFoundCount = lng;
	}
	if ( ppFindDataArray )
	{
		DBG_printf( "Avant allocation\n" );
		*ppFindDataArray = ( LPCE_FIND_DATA ) calloc( lng, sizeof( CE_FIND_DATA ) );
		DBG_printf( "Après allocation : *ppFindDataArray = %08X\n", *ppFindDataArray );
		if ( ( *ppFindDataArray ) )
		{
			for ( i = 0; i < lng; i++ )
			{
				ptr = &( ( *ppFindDataArray ) [ i ] );
				DBG_printf( "i=%d : ptr=%08X\n", i, ptr );
				if ( dwFlags & FAF_NAME )
				{
					stlen = getLong( sock, &size );
					DBG_printf( "string size : %ld (0x%08lx)\n", stlen, stlen );
				}
				if ( dwFlags & FAF_ATTRIBUTES )
				{
					ptr->dwFileAttributes = getLong( sock, &size );
					DBG_printf( "ptr->dwFileAttributes : %ld (0x%08lx)\n", ptr->dwFileAttributes, ptr->dwFileAttributes );
				}
				if ( dwFlags & FAF_CREATION_TIME )
				{
					ptr->ftCreationTime.dwLowDateTime = getLong( sock, &size );
					DBG_printf( "ptr->ftCreationTime.dwLowDateTime : %ld (0x%08lx)\n", ptr->ftCreationTime.dwLowDateTime, ptr->ftCreationTime.dwLowDateTime );
					ptr->ftCreationTime.dwHighDateTime = getLong( sock, &size );
					DBG_printf( "ptr->ftCreationTime.dwHighDateTime : %ld (0x%08lx)\n", ptr->ftCreationTime.dwHighDateTime, ptr->ftCreationTime.dwHighDateTime );
				}
				if ( dwFlags & FAF_LASTACCESS_TIME )
				{
					ptr->ftLastAccessTime.dwLowDateTime = getLong( sock, &size );
					DBG_printf( "ptr->ftLastAccessTime.dwLowDateTime : %ld (0x%08lx)\n", ptr->ftLastAccessTime.dwLowDateTime, ptr->ftLastAccessTime.dwLowDateTime );
					ptr->ftLastAccessTime.dwHighDateTime = getLong( sock, &size );
					DBG_printf( "ptr->ftLastAccessTime.dwHighDateTime : %ld (0x%08lx)\n", ptr->ftLastAccessTime.dwHighDateTime, ptr->ftLastAccessTime.dwHighDateTime );
				}
				if ( dwFlags & FAF_LASTWRITE_TIME )
				{
					ptr->ftLastWriteTime.dwLowDateTime = getLong( sock, &size );
					DBG_printf( "ptr->ftLastWriteTime.dwLowDateTime : %ld (0x%08lx)\n", ptr->ftLastWriteTime.dwLowDateTime, ptr->ftLastWriteTime.dwLowDateTime );
					ptr->ftLastWriteTime.dwHighDateTime = getLong( sock, &size );
					DBG_printf( "ptr->ftLastWriteTime.dwHighDateTime : %ld (0x%08lx)\n", ptr->ftLastWriteTime.dwHighDateTime, ptr->ftLastWriteTime.dwHighDateTime );
				}
				if ( dwFlags & FAF_SIZE_HIGH )
				{
					ptr->nFileSizeHigh = getLong( sock, &size );
					DBG_printf( "ptr->nFileSizeHigh : %ld (0x%08lx)\n", ptr->nFileSizeHigh, ptr->nFileSizeHigh );
				}
				if ( dwFlags & FAF_SIZE_LOW )
				{
					ptr->nFileSizeLow = getLong( sock, &size );
					DBG_printf( "ptr->nFileSizeLow : %ld (0x%08lx)\n", ptr->nFileSizeLow, ptr->nFileSizeLow );
				}
				if ( dwFlags & FAF_OID )
				{
					ptr->dwOID = getLong( sock, &size );
					DBG_printf( "ptr->dwOID : %ld (0x%08lx)\n", ptr->dwOID, ptr->dwOID );
				}
				if ( dwFlags & FAF_NAME )
				{
					str = getString( sock, &size, stlen );
					memcpy( ptr->cFileName, str, MAX_PATH );
					DBG_printf( "ptr->cFileName : %s\n", ptr->cFileName );
				}
			}
		}
	}
	return TRUE;
}

STDAPI_( HANDLE ) CeFindFirstFile( LPCWSTR lpFileName, LPCE_FIND_DATA lpFileFindData )
{
	long size = BUFSIZE;
	long lng;
	HANDLE result;
	size_t stlen;
	/* CEDB_FIND_DATA *ptr; */

	DBG_printf( "CeFindFirstFile( lpFileName = 0x%08X, lpFileFindData = 0x%08X )\n",
	            lpFileName, lpFileFindData );

	initBuf( buffer, size );
        DBG_printf(" line : %d\n", __LINE__ );
	pushLong( buffer, size, 0x00 ); 		/* Command */
        DBG_printf(" line : %d\n", __LINE__ );
	pushLong( buffer, size, 0x01 ); 		/* Parameter1 : */
        DBG_printf(" line : %d\n", __LINE__ );
	if ( lpFileName )
	{
		stlen = wcslen( lpFileName );
	}
	else
	{
		stlen = -1;
	}
	DBG_printf( "size : %d\n", stlen );
	pushLong( buffer, size, 1 + stlen ); 		/* Parameter2 : Flags ? */
	pushString( buffer, size, lpFileName ); 	/* Parameter3 : the path */
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2 (errorcode?): %ld (0x%08lx)\n", _lasterror, _lasterror );
	result = ( HANDLE ) getLong( sock, &size );
	DBG_printf( "long 3 : (size=%d) HANDLE %ld (0x%08lx)\n", size, result, result );
	lng = getLong( sock, &size );
	DBG_printf( "long 4 : %ld (0x%08lx)\n", lng, lng );
	if ( lpFileFindData )
	{
		lpFileFindData->dwFileAttributes = getLong( sock, &size );
		getbufferchunk( sock, &size, &( lpFileFindData->ftCreationTime ), sizeof( FILETIME ) );
		getbufferchunk( sock, &size, &( lpFileFindData->ftLastAccessTime ), sizeof( FILETIME ) );
		getbufferchunk( sock, &size, &( lpFileFindData->ftLastWriteTime ), sizeof( FILETIME ) );
		lpFileFindData->nFileSizeHigh = getLong( sock, &size );
		lpFileFindData->nFileSizeLow = getLong( sock, &size );
		lpFileFindData->dwOID = getLong( sock, &size );
		memset( &( lpFileFindData->cFileName ), 0, MAX_PATH );
		getbufferchunk( sock, &size, &( lpFileFindData->cFileName ), size );
	}

	DBG_printf( "size : %d\n", size );
	if ( size > 0 )
	{
		flushbuffer( sock );
	}
	DBG_printf( "long 5 : %ld (0x%08lx)\n", lpFileFindData->dwFileAttributes, lpFileFindData->dwFileAttributes );
	return result;
}

STDAPI_( BOOL ) CeFindNextFile( HANDLE hFindFile,  /*LPWIN32_FIND_DATA ?*/LPCE_FIND_DATA lpFileFindData )
{
	long size = BUFSIZE;
	long lng;
	int result;
	/* CEDB_FIND_DATA *ptr; */

	DBG_printf( "CeFindNextFile( hFindFile = 0x%08X, lpFileFindData = 0x%08X )\n",
	            hFindFile, lpFileFindData );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x01 ); 		/* Command */
	pushLong( buffer, size, ( long ) hFindFile ); 	/* Parameter1 : */
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2 (errorcode?): %ld (0x%08lx)\n", _lasterror, _lasterror );
	lng = getLong( sock, &size );
	DBG_printf( "long 3 (size=%d): %ld (0x%08lx)\n", size, lng, lng );
	lng = getLong( sock, &size );
	DBG_printf( "long 4 : %ld (0x%08lx)\n", lng, lng );
	if ( lpFileFindData )
	{
		lpFileFindData->dwFileAttributes = getLong( sock, &size );
		getbufferchunk( sock, &size, &( lpFileFindData->ftCreationTime ), sizeof( FILETIME ) );
		getbufferchunk( sock, &size, &( lpFileFindData->ftLastAccessTime ), sizeof( FILETIME ) );
		getbufferchunk( sock, &size, &( lpFileFindData->ftLastWriteTime ), sizeof( FILETIME ) );
		lpFileFindData->nFileSizeHigh = getLong( sock, &size );
		lpFileFindData->nFileSizeLow = getLong( sock, &size );
		lpFileFindData->dwOID = getLong( sock, &size );
		memset( &( lpFileFindData->cFileName ), 0, MAX_PATH );
		getbufferchunk( sock, &size, &( lpFileFindData->cFileName ), size );
	}
	DBG_printf( "size : %d\n", size );

	if ( size > 0 )
	{
		flushbuffer( sock );
	}
	result = ( _lasterror == 0 );
	return result;
}

STDAPI_( BOOL ) CeFindClose( HANDLE hFindFile )
{
	long size = BUFSIZE;
	long lng;

	DBG_printf( "CeFindClose()\n" );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x02 ); 		/* Command */
	pushLong( buffer, size, ( long ) hFindFile );	/* Parameter1 : */
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );
	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2 (lasterror): %ld (0x%08lx)\n", _lasterror, _lasterror );
	lng = getLong( sock, &size );
	DBG_printf( "long 3 : %ld (0x%08lx)\n", lng, lng );

	if ( size > 0 )
	{
		DBG_printf( "size : %d\n", size );
		flushbuffer( sock );
	}

	return lng;
}

STDAPI_( HANDLE ) CeCreateFile( LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile )
{
	long size = BUFSIZE;
	long lng;
	HANDLE result;
	long stlen;

	DBG_printf( "CeCreateFile( lpFileName = 0x%08X, dwDesiredAccess = 0x%08X, dwShareMode = 0x%08X )\n",
	            lpFileName, dwDesiredAccess, dwShareMode );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x05 ); 			/* Command */
	pushLong( buffer, size, dwDesiredAccess ); 		/* Parameter1 */
	pushLong( buffer, size, dwShareMode ); 			/* Parameter2 */
	pushLong( buffer, size, dwCreationDisposition ); 	/* Parameter3 */
	pushLong( buffer, size, dwFlagsAndAttributes ); 	/* Parameter4 */
	pushLong( buffer, size, ( long ) hTemplateFile ); 	/* Parameter5 */
	pushLong( buffer, size, 1 ); 				/* Parameter6 */
	stlen = wcslen( lpFileName );
	pushLong( buffer, size, 1 + stlen ); 			/* Parameter7 */
	pushString( buffer, size, lpFileName ); 		/* Parameter8 */
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2 (errorcode?): %ld (0x%08lx)\n", _lasterror, _lasterror );
	result = ( HANDLE ) getLong( sock, &size );
	DBG_printf( "long 3 (HANDLE): %ld (0x%08lx)\n", result, result );

	if ( size > 0 )
	{
		flushbuffer( sock );
	}

	return result;
}

STDAPI_( BOOL ) CeReadFile( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped )
{
	long size = BUFSIZE;
	long lng;
	BOOL result;
	/* CEDB_FIND_DATA *ptr; */

	DBG_printf( "CeReadFile( hFile = 0x%08X, nNumberOfBytesToRead = 0x%08X )\n",
	            hFile, nNumberOfBytesToRead );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x06 ); 		/* Command */
	pushLong( buffer, size, ( long ) hFile ); 	/* Parameter1 */
	pushLong( buffer, size, 0x01 ); 		/* Parameter2 */
	pushLong( buffer, size, nNumberOfBytesToRead ); /* Parameter3 */
	pushLong( buffer, size, 0x00 ); 		/* Parameter4 */
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2 (errorcode?): %ld (0x%08lx)\n", _lasterror, _lasterror );
	result = ( BOOL ) getLong( sock, &size );
	DBG_printf( "long 3 (BOOL): %ld (0x%08lx)\n", result, result );
	lng = getLong( sock, &size );
	DBG_printf( "long 4 (size): %ld (0x%08lx)\n", lng, lng );

	if ( lpNumberOfBytesRead )
	{
		*lpNumberOfBytesRead = lng;
	}
	if ( ( lpBuffer ) && ( lng > 0 ) )
	{
		getbufferchunk( sock, &size, lpBuffer, lng );
	}

	if ( size > 0 )
	{
		DBG_printf( "Size != 0 : size=%d\n", size );
		flushbuffer( sock );
	}

	return result;
}

STDAPI_( BOOL ) CeWriteFile( HANDLE hFile, LPVOID lpBuffer,
			     DWORD nNumberOfBytesToWrite,
			     LPDWORD lpNumberOfBytesWritten, 
			     LPOVERLAPPED lpOverlapped )
{
	long size = BUFSIZE;
	long lng;
	BOOL result;
	unsigned long buflen;

	
	/* CEDB_FIND_DATA *ptr; */

	DBG_printf( "CeWriteFile( hFile = 0x%08X, nNumberOfBytesToWrite = 0x%08X )\n",
	            hFile, nNumberOfBytesToWrite );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x07 ); 			/* Command */
	pushLong( buffer, size, ( long ) hFile ); 		/* Parameter1 */
	pushLong( buffer, size, 0x01 ); 			/* Parameter2 */
	pushLong( buffer, size, nNumberOfBytesToWrite ); 	/* Parameter3 */
	
	buflen=_getbufferlen(buffer);
	_setbufferlen( buffer, buflen + nNumberOfBytesToWrite + 4);
	
	(void) safe_write(sock, (void *) buffer, buflen + 4);

	(void) safe_write(sock, lpBuffer, nNumberOfBytesToWrite);
	lng=0;
	(void) safe_write(sock, (void *) &lng, sizeof(lng)); /*  ??? continue ??? */
	
	size = getbufferlen( sock );

	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2 (errorcode?): %ld (0x%08lx)\n", _lasterror, _lasterror );

	result = ( BOOL ) getLong( sock, &size );
	DBG_printf( "long 3 (BOOL): %ld (0x%08lx)\n", result, result );
	lng = getLong( sock, &size );
	DBG_printf( "long 4 (size): %ld (0x%08lx)\n", lng, lng );

	if ( lpNumberOfBytesWritten )
	{
		*lpNumberOfBytesWritten = lng;
	}
	if ( size > 0 )
	{
		flushbuffer( sock );
	}

	return result;
}

/* ================================================================================================================= */
/* ================================================================================================================= */
/*  RAPI - Databases */
/* ================================================================================================================= */
/* ================================================================================================================= */


STDAPI_( BOOL ) CeFindAllDatabases( DWORD dwDbaseType, WORD wFlags, LPWORD cFindData, LPLPCEDB_FIND_DATA ppFindData )
{
	long size = BUFSIZE;
	long lng;
	WCHAR * str;
	long stlen = 0;
	long i, j;
	WORD wrd;
	CEDB_FIND_DATA *ptr;

	DBG_printf( "CeFindAllDatabases( dwDbaseType = 0x%08X, wFlags = 0x%04X, cFindData = 0x%08X, ppFindData = 0x%08X )\n",
	            dwDbaseType, wFlags, cFindData, ppFindData );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x2C ); 	/* Command */
	pushLong( buffer, size, dwDbaseType ); 	/* Parameter1 : */
	pushShort( buffer, size, wFlags ); 	/* Parameter2 : Flags ? */
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );
	lng = getLong( sock, &size );
	DBG_printf( "long 1 (errorcode?): %ld (0x%08lx)\n", lng, lng );
	wrd = getShort( sock, &size );
	DBG_printf( "word 1 : %d (0x%08lx)\n", wrd, wrd );
	if ( cFindData )
	{
		*cFindData = wrd;
	}
	if ( ppFindData && ( wrd > 0 ) )
	{
		DBG_printf( "Avant allocation\n" );
		*ppFindData = ( LPCEDB_FIND_DATA ) calloc( wrd, sizeof( CEDB_FIND_DATA ) );
		DBG_printf( "Après allocation : *ppFindData = %08X\n", *ppFindData );
		if ( ( *ppFindData ) )
		{
			for ( i = 0; i < wrd; i++ )
			{
				ptr = &( ( *ppFindData ) [ i ] );
				DBG_printf( "i=%d : ptr=%08X\n", i, ptr );
				if ( wFlags & FAD_OID )
				{
					ptr->OidDb = getLong( sock, &size );
					DBG_printf( "OidDb : %ld (0x%08lx)\n", ptr->OidDb, ptr->OidDb );
				}
				if ( wFlags & FAD_NAME )
				{
					stlen = getLong( sock, &size );
					DBG_printf( "string size : %ld (0x%08lx)\n", stlen, stlen );
				}
				if ( wFlags & FAD_FLAGS )
				{
					ptr->DbInfo.dwFlags = getLong( sock, &size );
					DBG_printf( "dwFlags : %ld (0x%08lx)\n", ptr->DbInfo.dwFlags, ptr->DbInfo.dwFlags );
				}
				if ( wFlags & FAD_NAME )
				{
					str = getString( sock, &size, stlen );
					memcpy( ptr->DbInfo.szDbaseName, str, sizeof( WCHAR ) * ( CEDB_MAXDBASENAMELEN > ( stlen ) ? CEDB_MAXDBASENAMELEN : ( stlen ) ) );
					DBG_printf( "ptr->DbInfo.szDbaseName : %s\n", ptr->DbInfo.szDbaseName );
				}
				if ( wFlags & FAD_TYPE )
				{
					ptr->DbInfo.dwDbaseType = getLong( sock, &size );
					DBG_printf( "dwDbaseType : %ld (0x%08lx)\n", ptr->DbInfo.dwDbaseType, ptr->DbInfo.dwDbaseType );
				}
				if ( wFlags & FAD_NUM_RECORDS )
				{
					ptr->DbInfo.wNumRecords = getShort( sock, &size );
					DBG_printf( "wNumRecords : %ld (0x%08lx)\n", ptr->DbInfo.wNumRecords, ptr->DbInfo.wNumRecords );
				}
				if ( wFlags & FAD_NUM_SORT_ORDER )
				{
					ptr->DbInfo.wNumSortOrder = getShort( sock, &size );
					DBG_printf( "wNumSortOrder : %ld (0x%08lx)\n", ptr->DbInfo.wNumSortOrder, ptr->DbInfo.wNumSortOrder );
				}
				if ( wFlags & FAD_SIZE )
				{
					ptr->DbInfo.dwSize = getLong( sock, &size );
					DBG_printf( "dwSize : %ld (0x%08lx)\n", ptr->DbInfo.dwSize, ptr->DbInfo.dwSize );
				}
				if ( wFlags & FAD_LAST_MODIFIED )
				{
					getFileTime( sock, &size, &( ptr->DbInfo.ftLastModified ) );
					DBG_printf( "ftLastModified : %ld (0x%08lx)\n", ptr->DbInfo.ftLastModified, ptr->DbInfo.ftLastModified );
				}
				if ( wFlags & FAD_SORT_SPECS )
				{
					for ( j = 0; j < CEDB_MAXSORTORDER; j++ )
					{
						getbufferchunk( sock, &size, &( ( ptr->DbInfo.rgSortSpecs ) [ j ] ), sizeof( SORTORDERSPEC ) );
						DBG_printf( "sortOrder[%d] : %ld (0x%08lx)\n", j, ( ptr->DbInfo.rgSortSpecs ) [ j ].propid, ( ptr->DbInfo.rgSortSpecs ) [ j ].propid );
						DBG_printf( "sortOrder[%d] : %ld (0x%08lx)\n", j, ( ptr->DbInfo.rgSortSpecs ) [ j ].dwFlags, ( ptr->DbInfo.rgSortSpecs ) [ j ].dwFlags );
					}
				}
			}
		}
	}
	return TRUE;
}

STDAPI_( HANDLE ) CeOpenDatabase( PCEOID poid, LPWSTR lpszName, CEPROPID propid, DWORD dwFlags, HWND hwndNotify )
{
	long size = BUFSIZE;
	long lng;
	HANDLE result;

	DBG_printf( "CeOpenDatabase( poid = 0x%08X, lpszName = 0x%08X, propid = 0x%08X, dwFlags = 0x%08X, hwndNotify = 0x%08X )\n",
	            poid ? ( (void*)*poid ) : NULL, lpszName, propid, dwFlags, hwndNotify );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x0E ); 	/* Command */
	pushLong( buffer, size, poid ? ( ( long ) ( *poid ) ) : ( ( long ) NULL ) ); 		/* Parameter1 : */
	pushLong( buffer, size, propid ); 	/* Parameter2 : */
	pushLong( buffer, size, dwFlags ); 	/* Parameter3 : */
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );
	lng = getLong( sock, &size );
	DBG_printf( "long 1 (errorcode?): %ld (0x%08lx)\n", lng, lng );
	lng = getLong( sock, &size );
	DBG_printf( "long 2 (errorcode?): %ld (0x%08lx)\n", lng, lng );
	_lasterror = lng;
	result = ( HANDLE ) getLong( sock, &size );
	DBG_printf( "long 3 (errorcode?): %ld (0x%08lx)\n", result, result );

	return result;
}

STDAPI_( CEOID ) CeReadRecordProps( HANDLE hDbase, DWORD dwFlags, LPWORD lpcPropID, CEPROPID* rgPropID, LPBYTE* lplpBuffer, LPDWORD lpcbBuffer )
{
	long size = BUFSIZE;
	long lng;
	long index;
	long i;
	WORD wrd;
	CEPROPVAL *ptr;
	LPBYTE ptrbyte;
	CEOID result;

	DBG_printf( "CeReadRecordProps( hDbase = 0x%08X, dwFlags = 0x%08X, lpcPropID = 0x%08X, rgPropID = 0x%08X, lplpBuffer = 0x%08X, lpcbBuffer = 0x%08X )\n",
	            hDbase, dwFlags, lpcPropID, rgPropID, lplpBuffer, lpcbBuffer );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x10 ); 		/* Command */
	pushLong( buffer, size, ( long ) hDbase ); 	/* Parameter1 : */
	pushLong( buffer, size, dwFlags ); 		/* Parameter2 : Flags ? */
	pushLong( buffer, size, 0 ); 			/* Parameter3 */
	pushLong( buffer, size, 0 ); 			/* Parameter4 */
	pushShort( buffer, size, 0 ); 			/* Parameter5 */
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );
	lng = getLong( sock, &size );
	DBG_printf( "long 1 (errorcode?): %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2: lasterror %ld (0x%08lx)\n", _lasterror, _lasterror );
	result = getLong( sock, &size );
	DBG_printf( "long 3: CEOID %ld (0x%08lx)\n", result, result );
	lng = getLong( sock, &size );
	DBG_printf( "long 4: (taille buffer) %ld (0x%08lx)\n", lng, lng );
	if ( lpcbBuffer && ( ( *lpcbBuffer ) == 0 ) )
	{
		*lpcbBuffer = lng;
	}
	wrd = getShort( sock, &size );
	DBG_printf( "word 1: (nbprops) %ld (0x%08lx)\n", wrd, wrd );
	if ( !rgPropID )
	{
		if ( lpcPropID )
		{
			*lpcPropID = wrd;
		}
	}

	/*	if( cFindData )
		{
			*cFindData = wrd;
		}*/
	if ( lplpBuffer && ( wrd > 0 ) )
	{
		if ( *lplpBuffer == NULL )
		{
			DBG_printf( "Avant allocation\n" );
			*lplpBuffer = ( LPBYTE ) calloc( 1, lng );
			DBG_printf( "Après allocation : *ppFindData = %08X\n", *lplpBuffer );
		}
		ptr = ( CEPROPVAL * ) ( *lplpBuffer );
		for ( i = 0; i < wrd; i++ )
		{
			DBG_printf( "i=%d : ptr=%08X\n", i, ptr );
			getbufferchunk( sock, &size, &( ptr[ i ] ), sizeof( CEPROPVAL ) );
			DBG_printf( "propval: propid %ld (0x%08lx) \n", ptr[ i ].propid, ptr[ i ].propid );
			DBG_printf( "propval: wLenData %d (0x%04x) \n", ptr[ i ].wLenData, ptr[ i ].wLenData );
			DBG_printf( "propval: wFlags %d (0x%04x) \n", ptr[ i ].wFlags, ptr[ i ].wFlags );
			switch ( ( ptr[ i ].propid ) & 0xFFFF )
			{
				case CEVT_BLOB:
					ptr[ i ].val.blob.lpb = ( LPBYTE ) ( ( DWORD ) ptr + ( DWORD ) ptr[ i ].val.blob.lpb );
					DBG_printf( "propval: BLOB (size : %d, ptr %-8X)\n", ptr[ i ].val.blob.dwCount, ptr[ i ].val.blob.lpb );
					break;
				case CEVT_LPWSTR:
					ptr[ i ].val.lpwstr = ( LPWSTR ) ( ( DWORD ) ptr + ( DWORD ) ptr[ i ].val.lpwstr );
					DBG_printf( "propval: LPWSTR %ld (0x%08lx) \n", ptr[ i ].val.lpwstr, ptr[ i ].val.lpwstr );
					break;
				case CEVT_I2:
					DBG_printf( "propval: I2 %d (0x%04x) \n", ptr[ i ].val.iVal, ptr[ i ].val.iVal );
					break;
				case CEVT_UI2:
					DBG_printf( "propval: UI2 %d (0x%04x) \n", ptr[ i ].val.uiVal, ptr[ i ].val.uiVal );
					break;
				case CEVT_I4:
					DBG_printf( "propval: I4 %ld (0x%08lx) \n", ptr[ i ].val.lVal, ptr[ i ].val.lVal );
					break;
				case CEVT_UI4:
					DBG_printf( "propval: UI4 %ld (0x%08lx) \n", ptr[ i ].val.ulVal, ptr[ i ].val.ulVal );
					break;
				case CEVT_BOOL:
					DBG_printf( "propval: BOOL %s\n", ptr[ i ].val.boolVal ? "true" : "false" );
					break;
				case CEVT_FILETIME:
					DBG_printf( "propval: FILETIME 0x%-8x\n", ptr[ i ].val.filetime );
					break;
				case CEVT_R8:
					DBG_printf( "propval: R8 %d ", ptr[ i ].val.dblVal );
					break;
				default:
					break;
			}
			/* 	                DBG_printf( "propval: sz:%d %ld (0x%08lx) \n", sizeof(CEVALUNION), valunion, valunion ); */
		}

		ptrbyte = ( LPBYTE ) & ( ptr[ wrd ] );
		index = ( long ) ( lng - ( ptrbyte - ( *lplpBuffer ) ) );
		DBG_printf( " getchunk : index = %d, size = %d\n", index, size );
		getbufferchunk( sock, &size, ptrbyte, ( ( long ) ptrbyte > index ) ? index : ( long ) ptrbyte );
	}
	return result;
}

/* ================================================================================================================= */
/* ================================================================================================================= */
/*  RAPI - Processes */
/* ================================================================================================================= */
/* ================================================================================================================= */

STDAPI_( BOOL ) CeCreateProcess( LPCWSTR lpApplicationName,
				 LPCWSTR lpCommandLine, 
				 LPSECURITY_ATTRIBUTES lpProcessAttributes,
				 LPSECURITY_ATTRIBUTES lpThreadAttributes,
				 BOOL bInheritHandles, 
				 DWORD dwCreationFlags,
                                 LPVOID lpEnvironment, 
				 LPWSTR lpCurrentDirectory, 
				 LPSTARTUPINFO lpStartupInfo,
                                 LPPROCESS_INFORMATION lpProcessInformation )
{
	long size = BUFSIZE;
	long lng;
	long stlen;
	BOOL result = FALSE;

	DBG_printf( "CeCreateProcess( lpApplication = 0x%08X, lpCommandLine = 0x%08X )\n",
	            lpApplicationName, lpCommandLine );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x19 ); 	/* Command */
	if(lpApplicationName!=NULL)
	{
		pushLong( buffer, size, 0x01 );
		stlen = wcslen( lpApplicationName );
		pushLong( buffer, size, (stlen+1)*2 );
		pushLong( buffer, size, 0x01 );
		pushString( buffer, size, lpApplicationName );
	}
	else
	{
		pushLong( buffer, size, 0x00 );
	}
	if(lpCommandLine!=NULL)
	{
		pushLong( buffer, size, 0x01 );
		stlen = wcslen( lpCommandLine );
		pushLong( buffer, size, (stlen+1)*2 );
		pushLong( buffer, size, 0x01 );
		pushString( buffer, size, lpCommandLine );
	}
	else
	{
		pushLong( buffer, size, 0x00 );
	}
	
	pushLong( buffer, size, 0x00 ); /* Parameter3 : lpProcessAttibutes ? */
	pushLong( buffer, size, 0x00 ); /* Parameter4 : lpThreadAttributes ? */
	pushLong( buffer, size, 0x00 ); /* Parameter5 : bInheritHandles ? */
	pushLong( buffer, size, dwCreationFlags ); 
	pushLong( buffer, size, 0x00 ); /* Parameter7 : lpEnvironment ? */
	pushLong( buffer, size, 0x00 ); /* Parameter8 : lpCurrentDirectory ? */
	pushLong( buffer, size, 0x00 ); /* Parameter9 : lpStartupInfo ? */
	pushLong( buffer, size, 0x00 ); /* Parametera : lpProcessInformation ? */

	DBG_printbuf( buffer );

	sendbuffer( sock, buffer );

	size = getbufferlen( sock );
	lng = getLong( sock, &size );
	DBG_printf( "long 1 (errorcode?): %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2: lasterror %ld (0x%08lx)\n", _lasterror, _lasterror );

	result = getLong( sock, &size );
	DBG_printf( "long 3: %ld (0x%08lx)\n", result, result );
	if(result)
	{
		lng = getLong( sock, &size );
		DBG_printf( "long 4: %ld (0x%08lx)\n", lng, lng );
		lng = getLong( sock, &size );
		DBG_printf( "long 5: %ld (0x%08lx)\n", lng, lng );
		lng = getLong( sock, &size );
		DBG_printf( "long 6: %ld (0x%08lx)\n", lng, lng );
		lng = getLong( sock, &size );
		DBG_printf( "long 7: %ld (0x%08lx)\n", lng, lng );
		lng = getLong( sock, &size );
		DBG_printf( "long 8: %ld (0x%08lx)\n", lng, lng );
		lng = getLong( sock, &size );
		DBG_printf( "long 9: %ld (0x%08lx)\n", lng, lng );
		lng = getLong( sock, &size );
		DBG_printf( "long a: %ld (0x%08lx)\n", lng, lng );
	}
	else
	{
		lng = getLong( sock, &size );
		DBG_printf( "long 4: %ld (0x%08lx)\n", lng, lng );
	}

	if ( size > 0 )
	{
		flushbuffer( sock );
	}

	return result;
}

/* ================================================================================================================= */
/* ================================================================================================================= */
/*  RAPI - General */
/* ================================================================================================================= */
/* ================================================================================================================= */

STDAPI_( BOOL ) CeCloseHandle( HANDLE hObject )
{
	long size = BUFSIZE;
	long lng;

	DBG_printf( "CeCloseHandle()\n" );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x08 ); 		/* Command */
	pushLong( buffer, size, ( long ) hObject ); 	/* Parameter1 : */
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );
	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2 (lasterror): %ld (0x%08lx)\n", _lasterror, _lasterror );
	lng = getLong( sock, &size );
	DBG_printf( "long 3 : %ld (0x%08lx)\n", lng, lng );

	return lng;
}

STDAPI_( DWORD ) CeGetLastError( void )
{
	return _lasterror;
}

/* ================================================================================================================= */
/* ================================================================================================================= */
/*  RAPI - Library */
/* ================================================================================================================= */
/* ================================================================================================================= */

char * findhost( void );

char * findhost( void )
{
	char * result = NULL;
	struct stat st;
	int res;
	FILE * f;
	char line[ 1024 ];
	char * ptrstart;
	char * ptrstop;

	DBG_printf( "Before stat...\n" );
	res = stat( INFOFILE, &st );
	DBG_printf( "stat(), result = %d...\n", res );
	if ( res != -1 )
	{
		f = fopen( INFOFILE, "r" );
		DBG_printf( "fopen(), result = 0x%08X...\n", f );
		if ( f )
		{
			while ( ( !feof( f ) ) && ( result == NULL ) )
			{
				line[ 0 ] = '\0';
				if ( fgets( line, 1024, f ) != NULL )
				{
					DBG_printf( "fscanf(), line = '%s'...\n", line );
					if ( line[ 0 ] != '#' )
					{
						if ( ( ptrstart = strstr( line, "device" ) ) != NULL )
						{
							ptrstart += 7;
							for ( ptrstop = ptrstart; ( ( *ptrstop ) != 0 ) && ( ( *ptrstop ) != '{' ); ptrstop++ );
							( *ptrstop ) = 0;
							result = strdup( ptrstart );
							DBG_printf( "strdup(), result = '%s'...\n", result );
						}
					}
				}
			}
			fclose( f );
		}
	}
	return result;
}

STDAPI_( HRESULT ) CeRapiInit()
{
	char * hostname;

	if ( sock == 0 )
	{
		hostname = findhost();
		if ( hostname )
		{
			DBG_printf( "CeRapiInit(%s)\n", hostname );
			sock = SockOpen( hostname, 990 );
			if ( sock > 0 )
			{
				DBG_printf( "CeRapiInit(%s) ok\n", hostname );
				free( hostname );
                                buffer = (rapibuffer *)malloc( 4 + BUFSIZE );
				return E_SUCCESS;
			}
			else
			{
				DBG_printf( "CeRapiInit(%s) Nok\n", hostname );
				free( hostname );
				return E_FAIL;
			}
			free( hostname );
		}
		else
		{
			DBG_printf( "No CE Device found !?!\n" );
			return E_FAIL;
		}
	}
	else
	{
		DBG_printf( "CeRapiInit() already done\n" );
		return CERAPI_E_ALREADYINITIALIZED;
	}
}
/* STDAPI CeRapiInitEx (RAPIINIT*); */

STDAPI_( HRESULT ) CeRapiUninit()
{
	HRESULT result = FALSE;
        if( buffer )
        {
                free( buffer );
                buffer = NULL;
        }
	if ( sock > 0 )
	{
		result = (close( sock )==0) ? TRUE : FALSE;
		DBG_printf( "CeRapiUninit() %s\n", (result ? "ok" : "failed") );
	}
	else
	{
		DBG_printf( "CeRapiUninit() no need\n" );
	}
	return result;
}

STDAPI_( HRESULT ) CeRapiFreeBuffer( LPVOID Buffer )
{
	if ( Buffer )
	{
		free( Buffer );
	}
	return ( HRESULT ) NULL;
}

STDAPI_(BOOL) CeGetVersionEx(LPCEOSVERSIONINFO lpVersion)
{
	BOOL result = FALSE;
	long size = BUFSIZE;
	LONG lng;
	
	initBuf(buffer, size);
	
	pushLong(buffer, size, 0x3B); 	/* Command */
	pushParameter(size, lpVersion, lpVersion->dwOSVersionInfoSize, 0);
	
	sendbuffer( sock, buffer );
	size = getbufferlen( sock );

	/*
	 * The return package looks like this
	 *
	 * Offset  Size  Value
	 * 00      4     0
	 * 04      4     0
	 * 08      4     1
	 * 0c      4     n = real size of buffer
	 * 10      n     first n bytes of struct
	 */
	
	lng = getLong(sock, &size); 
	_lasterror = getLong(sock, &size);

	if (0 == lng)
	{
		result = getLong(sock, &size);
		if (1 == result)
		{
		 	long real_length = getLong(sock, &size);
			getbufferchunk(sock, &size, lpVersion, real_length);
		}
	}
	else
	{
		DBG_printf("Warning: expected 0 but got %i=0x%x\n", lng, lng);
	}
	
	if ( size > 0 )
	{
		DBG_printf( "size : %d\n", size );
		flushbuffer( sock );
	}	
	
	return result;
}

STDAPI_(BOOL) CeCreateDirectory(LPCWSTR lpDirName, LPSECURITY_ATTRIBUTES lpSecAttr)
{
	BOOL result = FALSE;
	long size = BUFSIZE;
	LONG lng;
	
	initBuf(buffer, size);
	
	pushLong(buffer, size, 0x17); 	/* Command */
	pushParameter(size, (void*)lpDirName, (wcslen(lpDirName) + 1) * sizeof(WCHAR), 1);
	pushLong(buffer, size, 0); 	/* lpSecAttr not used */
	
	/*DBG_printbuf( buffer );*/
	sendbuffer( sock, buffer );
	size = getbufferlen( sock );

	/*
	 * The return package looks like this
	 *
	 * Offset  Size  Value
	 * 00      4     0
	 * 04      4     error code if the value below is 0
	 * 08      4     0/1
	*/

	lng = getLong(sock, &size); 
	_lasterror = getLong(sock, &size);

	if (0 == lng)
	{
		result = getLong(sock, &size);
	}
	else
	{
		DBG_printf("Warning: expected 0 but got %i=0x%x\n", lng, lng);
	}
	
	if ( size > 0 )
	{
		DBG_printf( "size : %d\n", size );
		flushbuffer( sock );
	}	
	
	return result;
}

STDAPI_(BOOL) CeRemoveDirectory(LPCWSTR lpPathName)
{
	BOOL result = FALSE;
	long size = BUFSIZE;
	LONG lng;
	
	initBuf(buffer, size);
	
	pushLong(buffer, size, 0x18); 	/* Command */
	pushParameter(size, (void*)lpPathName, (wcslen(lpPathName) + 1) * sizeof(WCHAR), 1);
	
	/*DBG_printbuf( buffer );*/
	sendbuffer( sock, buffer );
	size = getbufferlen( sock );

	/*
	 * The return package looks like this
	 *
	 * Offset  Size  Value
	 * 00      4     0
	 * 04      4     error code if the value below is 0
	 * 08      4     0/1
	*/

	lng = getLong(sock, &size); 
	_lasterror = getLong(sock, &size);

	if (0 == lng)
	{
		result = getLong(sock, &size);
	}
	else
	{
		DBG_printf("Warning: expected 0 but got %i=0x%x\n", lng, lng);
	}
	
	if ( size > 0 )
	{
		DBG_printf( "size : %d\n", size );
		flushbuffer( sock );
	}	
	
	return result;
}

STDAPI_(BOOL) CeDeleteFile(LPCWSTR lpFileName)
{
	BOOL result = FALSE;
	long size = BUFSIZE;
	LONG lng;
	
	initBuf(buffer, size);
	
	pushLong(buffer, size, 0x1c); 	/* Command */
	pushParameter(size, (void*)lpFileName, (wcslen(lpFileName) + 1) * sizeof(WCHAR), 1);
	
	/*DBG_printbuf( buffer );*/
	sendbuffer( sock, buffer );
	size = getbufferlen( sock );

	/*
	 * The return package looks like this
	 *
	 * Offset  Size  Value
	 * 00      4     0
	 * 04      4     error code if the value below is 0
	 * 08      4     0/1
	*/

	lng = getLong(sock, &size); 
	DBG_printf("long 1 : %ld (0x%08lx)\n", lng, lng);
	_lasterror = getLong(sock, &size);
	DBG_printf("long 2 : %ld (0x%08lx)\n", _lasterror, _lasterror);

	if (0 == lng)
	{
		result = getLong(sock, &size);
		DBG_printf("long 3 : %ld (0x%08lx)\n", result, result);
	}
	else
	{
		DBG_printf("Warning: expected 0 but got %i=0x%x\n", lng, lng);
	}
	
	if ( size > 0 )
	{
		DBG_printf( "size : %d\n", size );
		flushbuffer( sock );
	}	
	
	return result;
}

STDAPI_( BOOL ) CeMoveFile( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName )
{
	BOOL result = FALSE;
	long size = BUFSIZE;
	LONG lng;
	
	initBuf(buffer, size);
	
	pushLong(buffer, size, 0x1a); 	/* Command */
	pushParameter(size, (void*)lpExistingFileName, (wcslen(lpExistingFileName) + 1) * sizeof(WCHAR), 1);
	pushParameter(size, (void*)lpNewFileName, (wcslen(lpNewFileName) + 1) * sizeof(WCHAR), 1);
	
	/*DBG_printbuf( buffer );*/
	sendbuffer( sock, buffer );
	size = getbufferlen( sock );

	/*
	 * The return package looks like this
	 *
	 * Offset  Size  Value
	 * 00      4     0
	 * 04      4     error code if the value below is 0
	 * 08      4     0/1
	*/

	lng = getLong(sock, &size); 
	DBG_printf("long 1 : %ld (0x%08lx)\n", lng, lng);
	_lasterror = getLong(sock, &size);
	DBG_printf("long 2 : %ld (0x%08lx)\n", _lasterror, _lasterror);

	if (0 == lng)
	{
		result = getLong(sock, &size);
		DBG_printf("long 3 : %ld (0x%08lx)\n", result, result);
	}
	else
	{
		DBG_printf("Warning: expected 0 but got %i=0x%x\n", lng, lng);
	}
	
	if ( size > 0 )
	{
		DBG_printf( "size : %d\n", size );
		flushbuffer( sock );
	}	
	
	return result;
}

