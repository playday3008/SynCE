/* $Id$ */
#include "rapi_wstr.h"
#include <stdlib.h>

#if HAVE_ICONV_H
#include <iconv.h>
#endif

#if HAVE_DMALLOC_H
#include "dmalloc.h"
#endif

#define rapi_wstr_WIDE   "UNICODELITTLE"
#define rapi_wstr_ASCII  "ISO_8859-1"

#define INVALID_ICONV_HANDLE ((iconv_t)(-1))

char* rapi_wstr_to_ascii(LPCWSTR inbuf)
{
	size_t length = rapi_wstr_string_length(inbuf);
	size_t inbytesleft = length * 2, outbytesleft = length;
	char* outbuf = malloc(outbytesleft+sizeof(char));
  char* outbuf_iterator = outbuf;
  ICONV_CONST char* inbuf_iterator = (ICONV_CONST char*)inbuf;
	size_t result;
	iconv_t cd = INVALID_ICONV_HANDLE;

	if (!inbuf)
		return NULL;
	
  cd = iconv_open(rapi_wstr_ASCII, rapi_wstr_WIDE);
	if (INVALID_ICONV_HANDLE == cd)
		return false;

  result = iconv(cd, &inbuf_iterator, &inbytesleft, &outbuf_iterator, &outbytesleft);
  iconv_close(cd);

  if ((size_t)-1 == result)
	{
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
		return NULL;
	
	cd = iconv_open(rapi_wstr_WIDE, rapi_wstr_ASCII);
	if (INVALID_ICONV_HANDLE == cd)
		return false;

	result = iconv(cd, &inbuf_iterator, &inbytesleft, (char**)&outbuf_iterator, &outbytesleft);
	iconv_close(cd);

	if ((size_t)-1 == result)
	{
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

size_t rapi_wstr_string_length(LPCWSTR unicode)
{
	unsigned length = 0;

	if (!unicode)
		return 0;
	
	while (*unicode++)
		length++;
	return length;
}


