/* $Id$ */
#include "rapi_wstr.h"
#include "rapi_internal.h"
#include <stdlib.h>
#include <string.h>

#if HAVE_ICONV_H
#include <iconv.h>
#endif

#if HAVE_DMALLOC_H
#include "dmalloc.h"
#endif

#define RAPI_WSTR_DEBUG 1

#if RAPI_WSTR_DEBUG
#define rapi_wstr_trace(args...)    rapi_trace(args)
#define rapi_wstr_warning(args...)  rapi_warning(args)
#define rapi_wstr_error(args...)    rapi_error(args)
#else
#define rapi_wstr_trace(args...)
#define rapi_wstr_warning(args...)
#define rapi_wstr_error(args...)
#endif


#define rapi_wstr_WIDE   "UNICODELITTLE"
#define rapi_wstr_ASCII  "ISO_8859-1"

#define INVALID_ICONV_HANDLE ((iconv_t)(-1))

char* rapi_wstr_to_ascii(LPCWSTR inbuf)
{
	size_t length = rapi_wstr_strlen(inbuf);
	size_t inbytesleft = length * 2, outbytesleft = length;
	char* outbuf = malloc(outbytesleft+sizeof(char));
  char* outbuf_iterator = outbuf;
  ICONV_CONST char* inbuf_iterator = (ICONV_CONST char*)inbuf;
	size_t result;
	iconv_t cd = INVALID_ICONV_HANDLE;

	if (!inbuf)
	{
		rapi_wstr_error("inbuf is NULL");
		return NULL;
	}
	
  cd = iconv_open(rapi_wstr_ASCII, rapi_wstr_WIDE);
	if (INVALID_ICONV_HANDLE == cd)
	{
		rapi_wstr_error("iconv_open failed");
		return false;
	}

  result = iconv(cd, &inbuf_iterator, &inbytesleft, &outbuf_iterator, &outbytesleft);
  iconv_close(cd);

  if ((size_t)-1 == result)
	{
		rapi_wstr_error("iconv failed: inbytesleft=%i, outbytesleft=%i", inbytesleft, outbytesleft);
		/* it would be nice to use rapi_trace_wstr here, but that would cause recursion */
		rapi_wstr_free_string(outbuf);
		return NULL;
	}
    
	outbuf[length] = 0;

  return outbuf;
}

LPWSTR rapi_wstr_from_ascii(const char* inbuf)
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
		rapi_wstr_error("inbuf is NULL");
		return NULL;
	}
	
	cd = iconv_open(rapi_wstr_WIDE, rapi_wstr_ASCII);
	if (INVALID_ICONV_HANDLE == cd)
	{
		rapi_wstr_error("iconv_open failed");
		return false;
	}

	result = iconv(cd, &inbuf_iterator, &inbytesleft, (char**)&outbuf_iterator, &outbytesleft);
	iconv_close(cd);

	if ((size_t)-1 == result)
	{
		rapi_wstr_error("iconv failed: inbytesleft=%i, outbytesleft=%i, inbuf=\"%s\"", 
				inbytesleft, outbytesleft, inbuf);
		rapi_wstr_free_string(outbuf);
		return NULL;
	}

	outbuf[length] = 0;

	return outbuf;
}

void rapi_wstr_free_string(void* str)
{
	if (str)
		free(str);
}

size_t rapi_wstr_strlen(LPCWSTR unicode)
{
	unsigned length = 0;

	if (!unicode)
		return 0;
	
	while (*unicode++)
		length++;
	return length;
}

LPWSTR rapi_wstr_strcpy(LPWSTR dest, LPCWSTR src)
{
	LPWSTR p = dest;

	while (*src)
		*p++ = *src++;

	*p = 0;

	return dest;
}

bool rapi_wstr_append(LPWSTR dest, LPCWSTR src, size_t max_dest_length)
{
	size_t dest_length = rapi_wstr_strlen(dest);
	size_t src_length  = rapi_wstr_strlen(src);

	rapi_wstr_trace("dest=%p, dest_length=%i, src=%p, src_length=%i, max_dest_length=%i",
			dest, dest_length, src, src_length, max_dest_length);
	
	if (!dest)
	{
		rapi_wstr_error("dest is NULL");
		return false;
	}
	
	if (!src)
	{
		rapi_wstr_error("dest is NULL");
		return false;
	}
	
	if ( (dest_length + src_length + 1) > max_dest_length)
	{
		rapi_wstr_warning("append failed: dest_length=%i, src_length=%i, max_dest_length=%i",
				dest_length, src_length, max_dest_length);
		return false;
	}

	memcpy(
			dest + dest_length, /* don't multiply by sizeof(WCHAR) */
			src,
			(src_length + 1) * sizeof(WCHAR)); /* copy terminating zero char too */

	return true;
}


