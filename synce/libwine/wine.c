/***************************************************************************
 *   Copyright (C) 2001 by Ludovic LANGE                                   *
 *   ludovic.lange@free.fr                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************
 * Some parts are :                                                        *
 * Copyright (c) 1993-2000 the Wine project authors (see the file AUTHORS  *
 * for a complete list)                                                    *
 ***************************************************************************/

#include <windows.h>

#ifdef WINDOWS
#define _WIN_wcstombs wcstombs
#define _WIN_mbstowcs mbstowcs
#else
#include <iconv.h>

size_t _WIN_wcstombs( char *s, const TCHAR *pwcs, size_t n )
{
	iconv_t cd;
	char *iptr, *optr;
	size_t ibl, obl;
	size_t result = 0;

	cd = iconv_open( "latin1", "UCS-2" );

	iptr = ( char * ) pwcs;
	ibl = n * sizeof( _WIN_WCHAR );
	obl = n;
	optr = s;
	result = iconv( cd, &iptr, &ibl, &optr, &obl );

	iconv_close( cd );

	s[ n ] = '\0';

	return ( ibl == 0 ) ? n : ( size_t ) - 1;
}

//Stolen from Wine
int _WIN_wprintf( const WCHAR * format, ... )
{
	unsigned int written = 0;
	const WCHAR *iter = format;
	char printbuf[ 1024 ];
	int len = 1024;
	WCHAR storebuf[ 2048 ];
	WCHAR * str = storebuf;
	char bufa[ 256 ], fmtbufa[ 64 ], *fmta;
	va_list valist;
	va_start( valist, format );

	//  TRACE("(%d,%s)\n",len,debugstr_w(format));

	while ( *iter )
	{
		while ( *iter && *iter != ( WCHAR ) L'%' )
		{
			if ( written++ >= len )
				return -1;
			*str++ = *iter++;
		}
		if ( *iter == ( WCHAR ) L'%' )
		{
			fmta = fmtbufa;
			*fmta++ = *iter++;
			while ( *iter == ( WCHAR ) L'0' ||
			        *iter == ( WCHAR ) L'+' ||
			        *iter == ( WCHAR ) L'-' ||
			        *iter == ( WCHAR ) L' ' ||
			        *iter == ( WCHAR ) L'0' ||
			        *iter == ( WCHAR ) L'*' ||
			        *iter == ( WCHAR ) L'#' )
			{
				if ( *iter == ( WCHAR ) L'*' )
				{
					char * buffiter = bufa;
					int fieldlen = va_arg( valist, int );
					sprintf( buffiter, "%d", fieldlen );
					while ( *buffiter )
						* fmta++ = *buffiter++;
				}
				else
					*fmta++ = *iter;
				iter++;
			}

			while ( isdigit( *iter ) )
				* fmta++ = *iter++;

			if ( *iter == ( WCHAR ) L'.' )
			{
				*fmta++ = *iter++;
				if ( *iter == ( WCHAR ) L'*' )
				{
					char * buffiter = bufa;
					int fieldlen = va_arg( valist, int );
					sprintf( buffiter, "%d", fieldlen );
					while ( *buffiter )
						* fmta++ = *buffiter++;
				}
				else
					while ( isdigit( *iter ) )
						* fmta++ = *iter++;
			}
			if ( *iter == ( WCHAR ) L'h' ||
			        *iter == ( WCHAR ) L'l' )
			{
				*fmta++ = *iter++;
				*fmta++ = *iter++;
			}

			switch ( *iter )
			{
				case ( WCHAR ) L's':
					{
						static const WCHAR none[] = { '(', 'n', 'u', 'l', 'l', ')', 0 };
						const WCHAR *wstr = va_arg( valist, const WCHAR * );
						const WCHAR *striter = wstr ? wstr : none;
						while ( *striter )
						{
							if ( written++ >= len )
								return -1;
							*str++ = *striter++;
						}
						iter++;
						break;
					}

				case ( WCHAR ) L'c':
					if ( written++ >= len )
						return -1;
					*str++ = ( WCHAR ) va_arg( valist, int );
					iter++;
					break;

				default:
					{
						/* For non wc types, use system sprintf and append to wide char output */
						/* FIXME: for unrecognised types, should ignore % when printing */
						char *bufaiter = bufa;
						if ( *iter == ( WCHAR ) L'p' )
							sprintf( bufaiter, "%08lX", va_arg( valist, long ) );
						else
						{
							*fmta++ = *iter;
							*fmta = '\0';
							if ( *iter == ( WCHAR ) L'f' )
								sprintf( bufaiter, fmtbufa, va_arg( valist, double ) );
							else
								sprintf( bufaiter, fmtbufa, va_arg( valist, void * ) );
						}
						while ( *bufaiter )
						{
							if ( written++ >= len )
								return -1;
							*str++ = *bufaiter++;
						}
						iter++;
						break;
					}
			}
		}
	}
	if ( written >= len )
		return -1;
	*str++ = ( WCHAR ) L'\0';

	_WIN_wcstombs( printbuf, storebuf, len );
	printf( "%s", printbuf );
	va_end( valist );
	return ( int ) written;
}

size_t _WIN_mbstowcs( _WIN_WCHAR * strDestination, char * strSource, size_t n )
{
	iconv_t cd;
	char *iptr, *optr;
	size_t ibl, obl;
	size_t result = 0;

	cd = iconv_open( "UCS-2", "latin1" );

	iptr = strSource;
	ibl = n;
	obl = n * sizeof( _WIN_WCHAR );
	optr = ( char * ) strDestination;
	result = iconv( cd, &iptr, &ibl, &optr, &obl );

	iconv_close( cd );

	strDestination[ n ] = ( _WIN_WCHAR ) 0;

	return ( ibl == 0 ) ? n : ( size_t ) - 1;
}

size_t _WIN_wcslen( const _WIN_WCHAR * str )
{
	size_t result = 0;
	const _WIN_WCHAR * ptr;
	if ( str )
	{
		for ( ptr = str; ( *ptr ) != 0x00; ptr++ )
		{
			result ++;
		}
	}
	return result;
}

//Stolen from Wine !
unsigned int strlenW( const WCHAR * str )
{
	const WCHAR * s = str;
	while ( *s ) s++;
	return s -str;
}

//Stolen from Wine !
WCHAR * strcpyW( WCHAR * dst, const WCHAR * src )
{
	WCHAR * p = dst;
	while ( ( *p++ = *src++ ) );
	return dst;
}

//Stolen from Wine !
WCHAR * strcatW( WCHAR *dst, const WCHAR *src )
{
	strcpyW( dst + strlenW( dst ), src );
	return dst;
}

//Stolen from Wine !
/***********************************************************************
*           DOSFS_FileTimeToUnixTime
*
* Convert a FILETIME format to Unix time.
* If not NULL, 'remainder' contains the fractional part of the filetime,
* in the range of [0..9999999] (even if time_t is negative).
*/
time_t DOSFS_FileTimeToUnixTime( const FILETIME *filetime, DWORD *remainder )
{
	/* Read the comment in the function DOSFS_UnixTimeToFileTime. */
#if USE_LONG_LONG

	long long int t = filetime->dwHighDateTime;
	t <<= 32;
	t += ( UINT ) filetime->dwLowDateTime;
	t -= 116444736000000000LL;
	if ( t < 0 )
	{
		if ( remainder ) * remainder = 9999999 - ( -t - 1 ) % 10000000;
		return -1 - ( ( -t - 1 ) / 10000000 );
	}
	else
	{
		if ( remainder ) * remainder = t % 10000000;
		return t / 10000000;
	}

#else  /* ISO version */

UINT a0;                       /* 16 bit, low    bits */
UINT a1;                       /* 16 bit, medium bits */
UINT a2;                       /* 32 bit, high   bits */
UINT r;                        /* remainder of division */
unsigned int carry;            /* carry bit for subtraction */
int negative;                  /* whether a represents a negative value */

/* Copy the time values to a2/a1/a0 */
a2 = ( UINT ) filetime->dwHighDateTime;
a1 = ( ( UINT ) filetime->dwLowDateTime ) >> 16;
a0 = ( ( UINT ) filetime->dwLowDateTime ) & 0xffff;

/* Subtract the time difference */
if ( a0 >= 32768 ) a0 -= 32768 , carry = 0;
else a0 += ( 1 << 16 ) - 32768 , carry = 1;

if ( a1 >= 54590 + carry ) a1 -= 54590 + carry, carry = 0;
else a1 += ( 1 << 16 ) - 54590 - carry, carry = 1;

a2 -= 27111902 + carry;

/* If a is negative, replace a by (-1-a) */
negative = ( a2 >= ( ( UINT ) 1 ) << 31 );
if ( negative )
{
	/* Set a to -a - 1 (a is a2/a1/a0) */
	a0 = 0xffff - a0;
	a1 = 0xffff - a1;
	a2 = ~a2;
}
/* Divide a by 10000000 (a = a2/a1/a0), put the rest into r.
Split the divisor into 10000 * 1000 which are both less than 0xffff. */
a1 += ( a2 % 10000 ) << 16;
a2 /= 10000;
a0 += ( a1 % 10000 ) << 16;
a1 /= 10000;
r = a0 % 10000;
a0 /= 10000;

a1 += ( a2 % 1000 ) << 16;
a2 /= 1000;
a0 += ( a1 % 1000 ) << 16;
a1 /= 1000;
r += ( a0 % 1000 ) * 10000;
a0 /= 1000;

/* If a was negative, replace a by (-1-a) and r by (9999999 - r) */
if ( negative )
{
	/* Set a to -a - 1 (a is a2/a1/a0) */
	a0 = 0xffff - a0;
	a1 = 0xffff - a1;
	a2 = ~a2;

	r = 9999999 - r;
}

if ( remainder ) * remainder = r;

/* Do not replace this by << 32, it gives a compiler warning and it does
not work. */
return ( ( ( ( time_t ) a2 ) << 16 ) << 16 ) + ( a1 << 16 ) + a0;
#endif
}

//Stolen from Wine !
BOOL FileTimeToDosDateTime( const FILETIME * lpFileTime, LPWORD lpFatDate, LPWORD lpFatTime )
{
	time_t unixtime = DOSFS_FileTimeToUnixTime( lpFileTime, NULL );
	struct tm *tm = localtime( &unixtime );
	if ( lpFatTime )
		* lpFatTime = ( ( tm->tm_hour ) << 11 ) + ( tm->tm_min << 5 ) + ( tm->tm_sec / 2 );
	if ( lpFatDate )
		* lpFatDate = ( ( tm->tm_year - 80 ) << 9 ) + ( ( tm->tm_mon + 1 ) << 5 )
		              + tm->tm_mday;
	return 1;
}

#endif  /* ifndef( WINDOWS )*/
