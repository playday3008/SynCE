/* $Id$ */
#include "synce.h"
#include "synce_log.h"
#include "synce_config.h"
#include <stdlib.h>
#include <string.h>

#if HAVE_ICONV_H
#include <iconv.h>
#endif

#if HAVE_DMALLOC_H
#include "dmalloc.h"
#endif

#define wstr_DEBUG 1

#if wstr_DEBUG
#define wstr_trace(args...)    synce_trace(args)
#define wstr_warning(args...)  synce_warning(args)
#define wstr_error(args...)    synce_error(args)
#else
#define wstr_trace(args...)
#define wstr_warning(args...)
#define wstr_error(args...)
#endif


#define wstr_WIDE   "UNICODELITTLE"
#define wstr_ASCII  "ISO_8859-1"
#define wstr_UTF8   "UTF-8"

#define INVALID_ICONV_HANDLE ((iconv_t)(-1))

/**
 * Convert a string from UCS2 to some other code
 */
static char* wstr_to_x(LPCWSTR inbuf, const char* code, size_t multiplier)
{
	size_t length = wstr_strlen(inbuf);
	size_t inbytesleft = length * 2, outbytesleft = length * multiplier;
	char* outbuf = malloc(outbytesleft+sizeof(char));
  char* outbuf_iterator = outbuf;
  ICONV_CONST char* inbuf_iterator = (ICONV_CONST char*)inbuf;
	size_t result;
	iconv_t cd = INVALID_ICONV_HANDLE;

	if (!inbuf)
	{
		wstr_error("inbuf is NULL");
		return NULL;
	}
	
  cd = iconv_open(code, wstr_WIDE);
	if (INVALID_ICONV_HANDLE == cd)
	{
		wstr_error("iconv_open failed");
		return NULL;
	}

  result = iconv(cd, &inbuf_iterator, &inbytesleft, &outbuf_iterator, &outbytesleft);
  iconv_close(cd);

  if ((size_t)-1 == result)
	{
		wstr_error("iconv failed: inbytesleft=%i, outbytesleft=%i", inbytesleft, outbytesleft);
		/* it would be nice to use rapi_trace_wstr here, but that would cause recursion */
		wstr_free_string(outbuf);
		return NULL;
	}
    
	outbuf[length] = 0;

  return outbuf;
}

/**
 * Convert a string from UCS2 to iso8859-1
 */
char* wstr_to_ascii(LPCWSTR unicode)
{
	return wstr_to_x(unicode, wstr_ASCII, 1);
}

/*
 * Convert a string from UCS2 to UTF8
 */
char* wstr_to_utf8(LPCWSTR unicode)
{
	return wstr_to_x(unicode, wstr_UTF8, 2);
}


/**
 * Convert a string from iso8859-1 to UCS2
 */
static LPWSTR wstr_from_x(const char* inbuf, const char* code)
{
	size_t length = strlen(inbuf);
	size_t inbytesleft = length, outbytesleft = (length+1)* 2;
	ICONV_CONST char * inbuf_iterator = (ICONV_CONST char*)inbuf;
	LPWSTR outbuf = malloc(outbytesleft+sizeof(WCHAR));
	LPWSTR outbuf_iterator = outbuf;
	size_t result;
	iconv_t cd = INVALID_ICONV_HANDLE;

	if (!inbuf)
	{
		wstr_error("inbuf is NULL");
		return NULL;
	}
	
	cd = iconv_open(wstr_WIDE, code);
	if (INVALID_ICONV_HANDLE == cd)
	{
		wstr_error("iconv_open failed");
		return NULL;
	}

	result = iconv(cd, &inbuf_iterator, &inbytesleft, (char**)&outbuf_iterator, &outbytesleft);
	iconv_close(cd);

	if ((size_t)-1 == result)
	{
		wstr_error("iconv failed: inbytesleft=%i, outbytesleft=%i, inbuf=\"%s\"", 
				inbytesleft, outbytesleft, inbuf);
		wstr_free_string(outbuf);
		return NULL;
	}

	*outbuf_iterator = '\0';

	return outbuf;
}

/**
 * Convert a string from iso8859-1 to UCS2
 */
LPWSTR wstr_from_ascii(const char* inbuf)
{
	return wstr_from_x(inbuf, wstr_ASCII);
}

/**
 * Convert a string from UTF8 to UCS2
 */
LPWSTR wstr_from_utf8(const char* inbuf)
{
	return wstr_from_x(inbuf, wstr_UTF8);
}

/**
 * Free a string returned by a conversion function
 */
void wstr_free_string(void* str)
{
	if (str)
		free(str);
}

/**
 * Return size of ascii string as unicode
 */
size_t wstrlen(LPCWSTR unicode)
{
	unsigned length = 0;

	if (!unicode)
		return 0;
	
	while (*unicode++)
		length++;
	return length;
}

/**
 * Copy strings
 */
LPWSTR wstrcpy(LPWSTR dest, LPCWSTR src)
{
	LPWSTR p = dest;

	while (*src)
		*p++ = *src++;

	*p = 0;

	return dest;
}

/**
 * Append unicode strings, return success
 */
bool wstr_append(LPWSTR dest, LPCWSTR src, size_t max_dest_length)
{
	size_t dest_length = wstr_strlen(dest);
	size_t src_length  = wstr_strlen(src);

	wstr_trace("dest=%p, dest_length=%i, src=%p, src_length=%i, max_dest_length=%i",
			dest, dest_length, src, src_length, max_dest_length);
	
	if (!dest)
	{
		wstr_error("dest is NULL");
		return false;
	}
	
	if (!src)
	{
		wstr_error("dest is NULL");
		return false;
	}
	
	if ( (dest_length + src_length + 1) > max_dest_length)
	{
		wstr_warning("append failed: dest_length=%i, src_length=%i, max_dest_length=%i",
				dest_length, src_length, max_dest_length);
		return false;
	}

	memcpy(
			dest + dest_length, /* don't multiply by sizeof(WCHAR) */
			src,
			(src_length + 1) * sizeof(WCHAR)); /* copy terminating zero char too */

	return true;
}

/**
 * Compare strings
 */
bool wstr_equal(LPWSTR a, LPWSTR b)
{
	for (; *a == *b && *a; a++, b++)
		;

	return *a == *b;
}

LPWSTR wstrdup(LPCWSTR string)
{
	LPWSTR result = NULL;

	if (string)
	{
		size_t size = (wstrlen(string) + 1) * sizeof(WCHAR);
		result = malloc(size);

		if (result)
		{
			memcpy(result, string, size);
		}
	}

	return result;
}
