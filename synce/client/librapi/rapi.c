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

#include "chunk.h"
#include "rapi.h"

/*
 * Global Variables ! Yuck !
 */
int sock = 0;
DWORD _lasterror = 0;
#define BUFSIZE 16384
//unsigned char buffer[ BUFSIZE ];
rapibuffer * buffer = NULL;


//=================================================================================================================
//=================================================================================================================
// RAPI - Global
//=================================================================================================================
//=================================================================================================================

STDAPI_( DWORD ) CeGetSpecialFolderPath( int nFolder, DWORD nBufferLength, LPWSTR lpBuffer )
{
	long size = BUFSIZE;
	long lng;
	WCHAR * str;

	initBuf( buffer, size );
	pushLong( buffer, size, 0x44 ); 	//Command
	pushLong( buffer, size, nFolder ); 	//Parameter1 : the folder
	pushLong( buffer, size, nBufferLength ); 	//Parameter2 : Buffer size that'll get the string
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	lng = getLong( sock, &size );
	DBG_printf( "long 2 : %ld (0x%08lx)\n", lng, lng );
	lng = getLong( sock, &size );
	DBG_printf( "string size : %ld (0x%08lx)\n", lng, lng );
	str = getString( sock, &size, 2 * lng );
	DBG_printf( "string1 : %s\n", str );
	if ( lpBuffer )
	{
		memcpy( lpBuffer, str, MIN( ( 2 * lng ), nBufferLength ) );
	}
	return 2*lng;
}

//=================================================================================================================
//=================================================================================================================
// RAPI - Registry
//=================================================================================================================
//=================================================================================================================

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
	long index;
	long errcode;

	DBG_printf( "CeRegCreatKeyEx( hKey = 0x%08X, lpszSubKey = 0x%08X, Reserved = 0x%08X, lpszClass = 0x%08X, ulOptions = 0x%08X, samDesired = 0x%08X, lpSecurityAttributes = 0x%08X, phkResult = 0x%08X, lpdwDisposition = 0x%08X )\n",
	            hKey, lpszSubKey, Reserved, lpszClass, ulOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x20 ); 	//Command
	pushLong( buffer, size, hKey ); 		//Parameter1 :
	pushLong( buffer, size, 0x01 ); 	//Parameter2 :
	pushLong( buffer, size, 1 + wcslen( lpszSubKey ) ); 	//Parameter3 :
	pushString( buffer, size, lpszSubKey ); 	//Parameter4 : the path
	pushLong( buffer, size, 0x01 ); 	//Parameter2 :
	pushLong( buffer, size, 0x01 ); 	//Parameter2 :
	pushShort( buffer, size, 0x00 ); 	//Parameter2 :

	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	errcode = getLong( sock, &index );
	DBG_printf( "errpresent : %ld (0x%08lx)\n", errcode, errcode );
	if ( errcode != 0 )
	{
		errcode = getLong( sock, &index );
		DBG_printf( "errcode : %ld (0x%08lx)\n", errcode, errcode );
	}
	else
	{
		lng = getLong( sock, &index );
		DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
		lng = getLong( sock, &index );
		DBG_printf( "long 2 : %ld (0x%08lx)\n", lng, lng );
		*phkResult = ( HKEY ) getLong( sock, &index );
		DBG_printf( "pHkey : %ld (0x%08lx)\n", *phkResult, *phkResult );
		*lpdwDisposition = ( DWORD ) getLong( sock, &index );
		DBG_printf( "lpdwDisposition : %ld (0x%08lx)\n", *lpdwDisposition, *lpdwDisposition );
	}
	return TRUE;
}

STDAPI_( LONG ) CeRegOpenKeyEx( HKEY hKey, LPCWSTR lpszSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult )
{
	long size = BUFSIZE;
	long errcode;

	DBG_printf( "CeRegOpenKeyEx( hKey = 0x%08X, lpszSubKey = 0x%08X, ulOptions = 0x%08X, samDesired = 0x%08X, phkResult = 0x%08X )\n",
	            hKey, lpszSubKey, ulOptions, samDesired, phkResult );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x1E ); 	//Command
	pushLong( buffer, size, hKey ); 		//Parameter1 :
	pushLong( buffer, size, 0x01 ); 	//Parameter2 :
	pushLong( buffer, size, 1 + wcslen( lpszSubKey ) ); 	//Parameter3 :
	pushString( buffer, size, lpszSubKey ); 	//Parameter4 : the path
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
	long index;

	DBG_printf( "CeRegCloseKey( hKey = 0x%08X )\n",
	            hKey );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x21 ); 	//Command
	pushLong( buffer, size, hKey ); 		//Parameter1 :

	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );
	size = getbufferlen( sock );

	lng = getLong( sock, &index );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	lng = getLong( sock, &index );
	DBG_printf( "long 2 : %ld (0x%08lx)\n", lng, lng );
	lng = getLong( sock, &index );
	DBG_printf( "long 3 : %ld (0x%08lx)\n", lng, lng );
	return TRUE;
}

STDAPI_( LONG ) CeRegQueryInfoKey( HKEY hKey, LPWSTR lpClass, LPDWORD lpcbClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen, LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime )
{
	long size = BUFSIZE;
	long lng;
	int i;
	long index;
	long cSubKeys;

	DBG_printf( "CeRegQueryInfoKey( hKey = 0x%08X, lpClass = 0x%08X, lpcbClass = 0x%08X, lpReserved = 0x%08X, lpcSubKeys = 0x%08X, lpcbMaxSubKeyLen = 0x%08X, lpcbMaxClassLen = 0x%08X, lpcValues = 0x%08X, lpcbMaxValueNameLen = 0x%08X, lpcbMaxValueLen = 0x%08X, lpcbSecurityDescriptor = 0x%08X, lpftLastWriteTime = 0x%08X )\n",
	            hKey, lpClass, lpcbClass, lpReserved, lpcSubKeys, lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x25 ); 	//Command
	pushLong( buffer, size, hKey ); 	//Parameter1 :
	pushLong( buffer, size, 0x00 ); 	//Parameter2 :
	pushLong( buffer, size, 0x00 ); 	//Parameter :

	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x01 ); 	//Parameter :
	pushLong( buffer, size, 0x04 ); 	//Parameter :

	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x01 ); 	//Parameter :
	pushLong( buffer, size, 0x04 ); 	//Parameter :

	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x01 ); 	//Parameter :
	pushLong( buffer, size, 0x04 ); 	//Parameter :

	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x01 ); 	//Parameter :
	pushLong( buffer, size, 0x04 ); 	//Parameter :

	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x00 ); 	//Parameter :

	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

        size = getbufferlen( sock );

	for ( i = 0; i < 8; i++ )
	{
		lng = getLong( sock, &index );
		DBG_printf( "long %d : %ld (0x%08lx)\n", i, lng, lng );
	}
	cSubKeys = getLong( sock, &index );
	DBG_printf( "cSubKeys : %ld (0x%08lx)\n", cSubKeys, cSubKeys );
	if ( lpcSubKeys != NULL )
	{
		*( lpcSubKeys ) = cSubKeys;
	}
	for ( i = 9; i < 25; i++ )
	{
		lng = getLong( sock, &index );
		DBG_printf( "long %d : %ld (0x%08lx)\n", i, lng, lng );
	}
	return TRUE;
}

STDAPI_( LONG ) CeRegEnumValue( HKEY hKey, DWORD dwIndex, LPWSTR lpszValueName, LPDWORD lpcbValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData )
{
	long size = BUFSIZE;

	DBG_printf( "CeRegEnumValue( hKey = 0x%08X, dwIndex = 0x%08X, lpszValueName = 0x%08X, lpcbValueName = 0x%08X, lpReserved = 0x%08X, lpType = 0x%08X, lpDate = 0x%08X, lpcbDate = 0x%08X )\n",
	            hKey, dwIndex, lpszValueName, ( *lpcbValueName ), lpReserved, ( *lpType ), ( *lpData ), ( *lpcbData ) );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x23 ); 	//Command
	pushLong( buffer, size, hKey ); 	//Parameter1 :
	pushLong( buffer, size, dwIndex ); 	//Parameter2 :
	pushLong( buffer, size, 0x01 ); 	//Parameter :

	pushLong( buffer, size, 0x0202 ); 	//Parameter :

	pushLong( buffer, size, 0x00 ); 	//Parameter2 :
	pushLong( buffer, size, 0x01 ); 	//Parameter :
	pushLong( buffer, size, 0x04 ); 	//Parameter :
	pushLong( buffer, size, 0x01 ); 	//Parameter :

	pushLong( buffer, size, ( *lpcbValueName ) ); 	//Parameter :
	pushLong( buffer, size, 0x00 ); 	//Parameter2 :
	pushLong( buffer, size, 0x01 ); 	//Parameter :
	pushLong( buffer, size, 0x04 ); 	//Parameter :

	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x01 ); 	//Parameter :
	pushLong( buffer, size, 0x0400 ); 	//Parameter :

	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x01 ); 	//Parameter :
	pushLong( buffer, size, 0x04 ); 	//Parameter :

	pushLong( buffer, size, 0x01 ); 	//Parameter :
	pushLong( buffer, size, 0x0400 ); 	//Parameter :

	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	return TRUE;
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
	pushLong( buffer, size, 0x1F ); 	//Command
	pushLong( buffer, size, hKey ); 	//Parameter1 :
	pushLong( buffer, size, dwIndex ); 	//Parameter2 :
	pushLong( buffer, size, 0x01 ); 	//Parameter :
	pushLong( buffer, size, 0x0202 ); //Parameter
	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x01 ); 	//Parameter :
	pushLong( buffer, size, 0x04 ); 	//Parameter :
	pushLong( buffer, size, 0x01 ); 	//Parameter :
	pushLong( buffer, size, lpcbName ? ( *lpcbName ) : 0 ); 	//Parameter :
	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x00 ); 	//Parameter :
	pushLong( buffer, size, 0x00 ); 	//Parameter :
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

//=================================================================================================================
//=================================================================================================================
// RAPI - Tests
//=================================================================================================================
//=================================================================================================================

/*DWORD CeTestSec1()
{
	long size=BUFSIZE;
	long lng;
	int result;
	long index;
 
	initBuf( buffer, size );
	pushLong( buffer, size, 0x27 );	//Command
	pushLong( buffer, size, 0x80000002 );		//Parameter1 :
	pushLong( buffer, size, 0x01 );	//Parameter2 :
	pushLong( buffer, size, 0x20 );	//Parameter3 :
	pushString( buffer, size, "Comm\\SecurityProviders\\SCHANNEL" );	//Parameter4 : the path
	pushLong( buffer, size, 0x01 );	//Parameter2 :
	pushLong( buffer, size, 0x01 );	//Parameter2 :
	pushShort( buffer, size, 0x00 );	//Parameter2 :
 
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );
	result = readbuffer( sock, buffer, size );
	DBG_printbuf( buffer );
	index = 0;
	return 2*lng;
} */

/*DWORD CeTest0()
{
	long size=BUFSIZE;
	long lng;
	int result;
	long index;
	
	initBuf( buffer, size );
	pushLong( buffer, size, 0x1E );	//Command
	pushLong( buffer, size, 0x80000000 );		//Parameter1 :
	pushLong( buffer, size, 0x01 );	//Parameter2 :
	pushLong( buffer, size, 0x27 );	//Parameter3 :
	pushString( buffer, size, "{000214A0-0000-0000-C000-000000000046}" );	//Parameter3 : the path
 
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );
	result = readbuffer( sock, buffer, size );
	DBG_printbuf( buffer );
	index = 0;
	return 2*lng;
} */

//=================================================================================================================
//=================================================================================================================
// RAPI - Files
//=================================================================================================================
//=================================================================================================================

STDAPI_( BOOL ) CeFindAllFiles( LPCWSTR szPath, DWORD dwFlags, LPDWORD lpdwFoundCount, LPLPCE_FIND_DATA ppFindDataArray )
{
	long size = BUFSIZE;
	long lng;
	WCHAR * str;
	long stlen = 0;
	long i;
	CE_FIND_DATA *ptr;

	initBuf( buffer, size );
	pushLong( buffer, size, 0x09 ); 	//Command
	pushLong( buffer, size, 0x01 ); 		//Parameter1 :
	pushLong( buffer, size, 1 + wcslen( szPath ) ); 	//Parameter2 :
	pushString( buffer, size, szPath ); 	//Parameter3 : the path
	pushLong( buffer, size, dwFlags ); 	//Parameter4 : Flags ?
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
					//					stlen = popLong( buffer, &index );
					stlen = getLong( sock, &size );
					DBG_printf( "string size : %ld (0x%08lx)\n", stlen, stlen );
				}
				if ( dwFlags & FAF_ATTRIBUTES )
				{
					//					ptr->dwFileAttributes = popLong( buffer, &index );
					ptr->dwFileAttributes = getLong( sock, &size );
					DBG_printf( "ptr->dwFileAttributes : %ld (0x%08lx)\n", ptr->dwFileAttributes, ptr->dwFileAttributes );
				}
				if ( dwFlags & FAF_CREATION_TIME )
				{
					//					ptr->ftCreationTime.dwLowDateTime = popLong( buffer, &index );
					ptr->ftCreationTime.dwLowDateTime = getLong( sock, &size );
					DBG_printf( "ptr->ftCreationTime.dwLowDateTime : %ld (0x%08lx)\n", ptr->ftCreationTime.dwLowDateTime, ptr->ftCreationTime.dwLowDateTime );
					//					ptr->ftCreationTime.dwHighDateTime = popLong( buffer, &index );
					ptr->ftCreationTime.dwHighDateTime = getLong( sock, &size );
					DBG_printf( "ptr->ftCreationTime.dwHighDateTime : %ld (0x%08lx)\n", ptr->ftCreationTime.dwHighDateTime, ptr->ftCreationTime.dwHighDateTime );
				}
				if ( dwFlags & FAF_LASTACCESS_TIME )
				{
					//					ptr->ftLastAccessTime.dwLowDateTime = popLong( buffer, &index );
					ptr->ftLastAccessTime.dwLowDateTime = getLong( sock, &size );
					DBG_printf( "ptr->ftLastAccessTime.dwLowDateTime : %ld (0x%08lx)\n", ptr->ftLastAccessTime.dwLowDateTime, ptr->ftLastAccessTime.dwLowDateTime );
					//					ptr->ftLastAccessTime.dwHighDateTime = popLong( buffer, &index );
					ptr->ftLastAccessTime.dwHighDateTime = getLong( sock, &size );
					DBG_printf( "ptr->ftLastAccessTime.dwHighDateTime : %ld (0x%08lx)\n", ptr->ftLastAccessTime.dwHighDateTime, ptr->ftLastAccessTime.dwHighDateTime );
				}
				if ( dwFlags & FAF_LASTWRITE_TIME )
				{
					//					ptr->ftLastWriteTime.dwLowDateTime = popLong( buffer, &index );
					ptr->ftLastWriteTime.dwLowDateTime = getLong( sock, &size );
					DBG_printf( "ptr->ftLastWriteTime.dwLowDateTime : %ld (0x%08lx)\n", ptr->ftLastWriteTime.dwLowDateTime, ptr->ftLastWriteTime.dwLowDateTime );
					//					ptr->ftLastWriteTime.dwHighDateTime = popLong( buffer, &index );
					ptr->ftLastWriteTime.dwHighDateTime = getLong( sock, &size );
					DBG_printf( "ptr->ftLastWriteTime.dwHighDateTime : %ld (0x%08lx)\n", ptr->ftLastWriteTime.dwHighDateTime, ptr->ftLastWriteTime.dwHighDateTime );
				}
				if ( dwFlags & FAF_SIZE_HIGH )
				{
					//					ptr->nFileSizeHigh = popLong( buffer, &index );
					ptr->nFileSizeHigh = getLong( sock, &size );
					DBG_printf( "ptr->nFileSizeHigh : %ld (0x%08lx)\n", ptr->nFileSizeHigh, ptr->nFileSizeHigh );
				}
				if ( dwFlags & FAF_SIZE_LOW )
				{
					//					ptr->nFileSizeLow = popLong( buffer, &index );
					ptr->nFileSizeLow = getLong( sock, &size );
					DBG_printf( "ptr->nFileSizeLow : %ld (0x%08lx)\n", ptr->nFileSizeLow, ptr->nFileSizeLow );
				}
				if ( dwFlags & FAF_OID )
				{
					//					ptr->dwOID = popLong( buffer, &index );
					ptr->dwOID = getLong( sock, &size );
					DBG_printf( "ptr->dwOID : %ld (0x%08lx)\n", ptr->dwOID, ptr->dwOID );
				}
				if ( dwFlags & FAF_NAME )
				{
					//					str = popString( buffer, 2*stlen, &index );
					str = getString( sock, &size, stlen );
					//strncpy( ptr->cFileName, str, MAX_PATH );
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
	//CEDB_FIND_DATA *ptr;

	DBG_printf( "CeFindFirstFile( lpFileName = 0x%08X, lpFileFindData = 0x%08X )\n",
	            lpFileName, lpFileFindData );

	initBuf( buffer, size );
        DBG_printf(" line : %d\n", __LINE__ );
	pushLong( buffer, size, 0x00 ); 	//Command
        DBG_printf(" line : %d\n", __LINE__ );
	pushLong( buffer, size, 0x01 ); 	//Parameter1 :
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
	pushLong( buffer, size, 1 + stlen ); 	//Parameter2 : Flags ?
	pushString( buffer, size, lpFileName ); 	//Parameter4 : the path
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2 (code erreur?): %ld (0x%08lx)\n", _lasterror, _lasterror );
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
	//CEDB_FIND_DATA *ptr;

	DBG_printf( "CeFindNextFile( hFindFile = 0x%08X, lpFileFindData = 0x%08X )\n",
	            hFindFile, lpFileFindData );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x01 ); 	//Command
	pushLong( buffer, size, ( long ) hFindFile ); 	//Parameter1 :
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2 (code erreur?): %ld (0x%08lx)\n", _lasterror, _lasterror );
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
	pushLong( buffer, size, 0x02 ); 	//Command
	pushLong( buffer, size, ( long ) hFindFile ); 		//Parameter1 :
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
	pushLong( buffer, size, 0x05 ); 	//Command
	pushLong( buffer, size, dwDesiredAccess ); 	//Parameter
	pushLong( buffer, size, dwShareMode ); 	//Parameter
	pushLong( buffer, size, dwCreationDisposition ); 	//Parameter
	pushLong( buffer, size, dwFlagsAndAttributes ); 	//Parameter
	pushLong( buffer, size, ( long ) hTemplateFile ); 	//Parameter
	pushLong( buffer, size, 1 ); 	//Parameter
	stlen = wcslen( lpFileName );
	pushLong( buffer, size, 1 + stlen ); 	//Parameter
	pushString( buffer, size, lpFileName ); 	//Parameter
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2 (code erreur?): %ld (0x%08lx)\n", _lasterror, _lasterror );
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
	//CEDB_FIND_DATA *ptr;

	DBG_printf( "CeReadFile( hFile = 0x%08X, nNumberOfBytesToRead = 0x%08X )\n",
	            hFile, nNumberOfBytesToRead );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x06 ); 	//Command
	pushLong( buffer, size, ( long ) hFile ); 	//Parameter
	pushLong( buffer, size, 0x01 ); 	//Parameter
	pushLong( buffer, size, nNumberOfBytesToRead ); 	//Parameter
	pushLong( buffer, size, 0x00 ); 	//Parameter
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );

	lng = getLong( sock, &size );
	DBG_printf( "long 1 : %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2 (code erreur?): %ld (0x%08lx)\n", _lasterror, _lasterror );
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


//=================================================================================================================
//=================================================================================================================
// RAPI - Databases
//=================================================================================================================
//=================================================================================================================


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
	pushLong( buffer, size, 0x2C ); 	//Command
	pushLong( buffer, size, dwDbaseType ); 	//Parameter1 :
	pushShort( buffer, size, wFlags ); 	//Parameter2 : Flags ?
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );
	lng = getLong( sock, &size );
	DBG_printf( "long 1 (code erreur?): %ld (0x%08lx)\n", lng, lng );
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
	pushLong( buffer, size, 0x0E ); 	//Command
	pushLong( buffer, size, poid ? ( ( long ) ( *poid ) ) : ( ( long ) NULL ) ); 		//Parameter1 :
	pushLong( buffer, size, propid ); 	//Parameter2 :
	pushLong( buffer, size, dwFlags ); 	//Parameter3 :
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );
	lng = getLong( sock, &size );
	DBG_printf( "long 1 (code erreur?): %ld (0x%08lx)\n", lng, lng );
	lng = getLong( sock, &size );
	DBG_printf( "long 2 (code erreur?): %ld (0x%08lx)\n", lng, lng );
	_lasterror = lng;
	result = ( HANDLE ) getLong( sock, &size );
	DBG_printf( "long 3 (code erreur?): %ld (0x%08lx)\n", result, result );

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
	pushLong( buffer, size, 0x10 ); 	//Command
	pushLong( buffer, size, ( long ) hDbase ); 	//Parameter1 :
	pushLong( buffer, size, dwFlags ); 	//Parameter2 : Flags ?
	pushLong( buffer, size, 0 ); 	//Parameter3
	pushLong( buffer, size, 0 ); 	//Parameter4
	pushShort( buffer, size, 0 ); 	//Parameter5
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );
	lng = getLong( sock, &size );
	DBG_printf( "long 1 (code erreur?): %ld (0x%08lx)\n", lng, lng );
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
			//	                DBG_printf( "propval: sz:%d %ld (0x%08lx) \n", sizeof(CEVALUNION), valunion, valunion );
		}

		ptrbyte = ( LPBYTE ) & ( ptr[ wrd ] );
		index = ( long ) ( lng - ( ptrbyte - ( *lplpBuffer ) ) );
		DBG_printf( " getchunk : index = %d, size = %d\n", index, size );
		getbufferchunk( sock, &size, ptrbyte, ( ( long ) ptrbyte > index ) ? index : ( long ) ptrbyte );
	}
	return result;
}

//=================================================================================================================
//=================================================================================================================
// RAPI - Processes
//=================================================================================================================
//=================================================================================================================

STDAPI_( BOOL ) CeCreateProcess( LPCWSTR lpApplicationName, LPCWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                 LPSECURITY_ATTRIBUTES lpThreadAttribute, BOOL bInheritHandles, DWORD dwCreationFlags,
                                 LPVOID lpEnvironment, LPWSTR lpCurrentDirectory, LPSTARTUPINFO lpStartupInfo,
                                 LPPROCESS_INFORMATION lpProcessInformation )
{
	long size = BUFSIZE;
	long lng;
	long stlen;
	BOOL result = FALSE;

	DBG_printf( "CeCreateProcess( lpApplication = 0x%08X, lpCommandLine = 0x%08X )\n",
	            lpApplicationName, lpCommandLine );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x19 ); 	//Command
	pushLong( buffer, size, 0x01 ); 	//Parameter1 :
	stlen = wcslen( lpApplicationName );
	pushLong( buffer, size, 1 + stlen ); 	//Parameter2
	pushLong( buffer, size, 0x01 ); 	//Parameter1 :
	pushString( buffer, size, lpApplicationName ); 	//Parameter5
	pushLong( buffer, size, 0x00 ); 	//Parameter1 :
	pushLong( buffer, size, 0x00 ); 	//Parameter1 :
	pushLong( buffer, size, 0x00 ); 	//Parameter1 :
	pushLong( buffer, size, 0x00 ); 	//Parameter1 :
	pushLong( buffer, size, 0x00 ); 	//Parameter1 :
	pushLong( buffer, size, 0x00 ); 	//Parameter1 :
	pushLong( buffer, size, 0x00 ); 	//Parameter1 :
	pushLong( buffer, size, 0x00 ); 	//Parameter1 :

	pushLong( buffer, size, 0x01 ); 	//Parameter1 :
	pushLong( buffer, size, 0x01 ); 	//Parameter1 :
	pushLong( buffer, size, 0x00 ); 	//Parameter1 :
	DBG_printbuf( buffer );
	sendbuffer( sock, buffer );

	size = getbufferlen( sock );
	lng = getLong( sock, &size );
	DBG_printf( "long 1 (code erreur?): %ld (0x%08lx)\n", lng, lng );
	_lasterror = getLong( sock, &size );
	DBG_printf( "long 2: lasterror %ld (0x%08lx)\n", _lasterror, _lasterror );

	result = getLong( sock, &size );
	DBG_printf( "long 3: %ld (0x%08lx)\n", result, result );
	result = getLong( sock, &size );
	DBG_printf( "long 3: %ld (0x%08lx)\n", result, result );
	result = getLong( sock, &size );
	DBG_printf( "long 3: %ld (0x%08lx)\n", result, result );
	result = getLong( sock, &size );
	DBG_printf( "long 3: %ld (0x%08lx)\n", result, result );
	result = getLong( sock, &size );
	DBG_printf( "long 3: %ld (0x%08lx)\n", result, result );
	result = getLong( sock, &size );
	DBG_printf( "long 3: %ld (0x%08lx)\n", result, result );
	result = getLong( sock, &size );
	DBG_printf( "long 3: %ld (0x%08lx)\n", result, result );
	result = getLong( sock, &size );
	DBG_printf( "long 3: %ld (0x%08lx)\n", result, result );

	return result;
}

//=================================================================================================================
//=================================================================================================================
// RAPI - General
//=================================================================================================================
//=================================================================================================================

STDAPI_( BOOL ) CeCloseHandle( HANDLE hObject )
{
	long size = BUFSIZE;
	long lng;

	DBG_printf( "CeCloseHandle()\n" );

	initBuf( buffer, size );
	pushLong( buffer, size, 0x08 ); 	//Command
	pushLong( buffer, size, ( long ) hObject ); 		//Parameter1 :
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

//=================================================================================================================
//=================================================================================================================
// RAPI - Library
//=================================================================================================================
//=================================================================================================================

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

#define E_SUCCESS ERROR_SUCCESS
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
//STDAPI CeRapiInitEx (RAPIINIT*);

STDAPI_( HRESULT ) CeRapiUninit()
{
	if ( sock > 0 )
	{
		DBG_printf( "CeRapiUninit() ok\n" );
		return TRUE;
	}
	else
	{
		DBG_printf( "CeRapiUninit() no need\n" );
		return FALSE;
	}
        if( buffer )
        {
                free( buffer );
                buffer = NULL;
        }
}

STDAPI_( HRESULT ) CeRapiFreeBuffer( LPVOID Buffer )
{
	if ( Buffer )
	{
		free( Buffer );
	}
	return ( HRESULT ) NULL;
}
