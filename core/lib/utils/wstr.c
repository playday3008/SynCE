/* $Id$ */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "synce.h"
#include "synce_log.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if HAVE_LOCALE_H
#include <locale.h>
#endif

#if HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#if HAVE_ICONV_H
#include <iconv.h>
#endif

#if HAVE_DMALLOC_H
#include "dmalloc.h"
#endif

/** 
 * @defgroup SynceWstr Wide string (UCS2) handling
 * @ingroup SynceUtils
 * @brief Tools for manipulating and converting strings in wide (UCS2) format
 *
 * @{ 
 */ 

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


#define wstr_WIDE   "ucs-2le"
#define wstr_ASCII  "ISO_8859-1"
#define wstr_UTF8   "UTF-8"

#define INVALID_ICONV_HANDLE ((iconv_t)(-1))

#if HAVE_SETLOCALE && HAVE_NL_LANGINFO
static char* current_codeset = NULL;

static char* get_current_codeset()
{
  if (!current_codeset)
  {
    setlocale(LC_ALL, "");
    current_codeset = strdup(nl_langinfo(CODESET));   /* XXX: memory leak */
  }

  return current_codeset;
}
#endif


/**
 * Convert a string from UCS2 to some other code
 */
static char* wstr_to_x(LPCWSTR inbuf, const char* code)
{
	size_t length = wstr_strlen(inbuf), size = length;
	size_t inbytesleft = length * 2, outbytesleft = size;
	char* outbuf = malloc(outbytesleft+sizeof(char)), *tmp;
  char* outbuf_iterator = outbuf;
  ICONV_CONST char* inbuf_iterator = (ICONV_CONST char*)inbuf;
	iconv_t cd = INVALID_ICONV_HANDLE;

	if (!inbuf)
	{
		wstr_error("inbuf is NULL");
		return NULL;
	}
	
  cd = iconv_open(code, wstr_WIDE);
	if (INVALID_ICONV_HANDLE == cd)
	{
		wstr_error("iconv_open(%s, %s) failed: %s", code, wstr_WIDE, strerror(errno));
		return NULL;
	}
	
	while (iconv(cd, &inbuf_iterator, &inbytesleft,
		     &outbuf_iterator, &outbytesleft) == (size_t)-1) {
		if (errno != E2BIG) {
			wstr_error("iconv failed: inbytesleft=%i, outbytesleft="
				   "%i", inbytesleft, outbytesleft);
			/* it would be nice to use rapi_trace_wstr here, but
			   that would cause recursion */
			wstr_free_string(outbuf);
			return NULL;
		}

		size += length;
		tmp = realloc(outbuf, size + 1);
		if (tmp == NULL) {
			wstr_error("realloc failed");
			free(outbuf);
			return NULL;
		}
		outbytesleft += length;
		outbuf_iterator += tmp - outbuf;
		outbuf = tmp;
	}

  iconv_close(cd);

	*outbuf_iterator = '\0';

  return outbuf;
}

/** @brief Convert string from UCS2 to iso8859-1
 * 
 * This function converts a string from UCS2 encoding to 
 * iso8859-1 (ascii).
 * 
 * @param[in] unicode UCS2 string to convert
 * @return string in ascii encoding
 */ 
char* wstr_to_ascii(LPCWSTR unicode)
{
	return wstr_to_x(unicode, wstr_ASCII);
}

/** @brief Convert string from UCS2 to UTF8
 * 
 * This function converts a string from UCS2 encoding to 
 * UTF8.
 * 
 * @param[in] unicode UCS2 string to convert
 * @return string in UTF8 encoding
 */ 
char* wstr_to_utf8(LPCWSTR unicode)
{
	return wstr_to_x(unicode, wstr_UTF8);
}

#if HAVE_SETLOCALE && HAVE_NL_LANGINFO
/** @brief Convert string from UCS2 to current local charset
 * 
 * This function converts a string from UCS2 encoding to 
 * the charset of the current locale.
 * 
 * @param[in] unicode UCS2 string to convert
 * @return string in current locale encoding
 */ 
char* wstr_to_current(LPCWSTR unicode)
{
  return wstr_to_x(unicode, get_current_codeset());
}
#endif

/**
 * Convert a string from iso8859-1 to UCS2
 */
static LPWSTR wstr_from_x(const char* inbuf, const char* code)
{
	size_t length = strlen(inbuf);
	size_t inbytesleft = length, outbytesleft = (length+1)* 2;
	ICONV_CONST char * inbuf_iterator = (ICONV_CONST char*)inbuf;
	LPWSTR outbuf = malloc(outbytesleft+sizeof(WCHAR));
	char *outbuf_iterator = (char*)outbuf;
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
		wstr_error("iconv_open(%s, %s) failed: %s", code, wstr_WIDE, strerror(errno));
		return NULL;
	}

	result = iconv(cd, &inbuf_iterator, &inbytesleft, &outbuf_iterator, &outbytesleft);
	iconv_close(cd);

	if ((size_t)-1 == result)
	{
		wstr_error("iconv failed: inbytesleft=%i, outbytesleft=%i, inbuf=\"%s\"", 
				inbytesleft, outbytesleft, inbuf);
		wstr_free_string(outbuf);
		return NULL;
	}

	*(LPWSTR)outbuf_iterator = '\0';

	return outbuf;
}

/** @brief Convert string from iso8859-1 to UCS2
 * 
 * This function converts a string from iso8859-1 (ascii)
 * encoding to UCS2.
 * 
 * @param[in] inbuf ascii string to convert
 * @return string in UCS2 encoding
 */ 
LPWSTR wstr_from_ascii(const char* inbuf)
{
	return wstr_from_x(inbuf, wstr_ASCII);
}

/** @brief Convert string from UTF8 to UCS2
 * 
 * This function converts a string from UTF8
 * encoding to UCS2.
 * 
 * @param[in] inbuf UTF8 string to convert
 * @return string in UCS2 encoding
 */ 
LPWSTR wstr_from_utf8(const char* inbuf)
{
	return wstr_from_x(inbuf, wstr_UTF8);
}

#if HAVE_SETLOCALE && HAVE_NL_LANGINFO
/** @brief Convert string from current locale charset to UCS2
 * 
 * This function converts a string from the charset of the
 * current locale to UCS2 encoding.
 * 
 * @param[in] inbuf string to convert
 * @return string in UCS2 encoding
 */ 
LPWSTR wstr_from_current(const char* inbuf)
{
  return wstr_from_x(inbuf, get_current_codeset());
}
#endif

/** @brief Free a string returned by a conversion function
 * 
 * This function frees the memory allocated for a string
 * returned by a conversion function.
 * 
 * @param[in] str string to free
 */ 
void wstr_free_string(void* str)
{
	if (str)
		free(str);
}

/**
 * Return size of ascii string as unicode
 */
/** @brief Determine length of a UCS2 string
 * 
 * This function determines the length of a string in
 * UCS2 encoding, as number of characters.
 * 
 * @param[in] unicode string in UCS2
 * @return length of string in characters
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

/** @brief Copy a UCS2 string
 * 
 * This function copies the UCS2 string. dest must be large enough
 * to receive the string, no bounds checking is performed.
 * 
 * @param[out] dest buffer to receive the copy
 * @param[in] src string to copy
 * @return pointer to the destination string dest
 */
LPWSTR wstrcpy(LPWSTR dest, LPCWSTR src)
{
	LPWSTR p = dest;

	while (*src)
		*p++ = *src++;

	*p = 0;

	return dest;
}

/** @brief Append UCS2 strings
 * 
 * This function appends the UCS2 string src to the
 * UCS2 string dest. dest must be at least max_dest_length
 * in size, in UCS2 characters. No bounds checking is performed.
 * 
 * @param[in,out] dest string to be appended to
 * @param[in] src string to append
 * @param[in] max_dest_length size of the dest buffer in UCS2 characters
 * @return TRUE on success, FALSE on failure
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

/** @brief Compares UCS2 strings
 * 
 * This function compares two UCS2 strings.
 * 
 * @param[in] a first string to compare
 * @param[in] b second string to compare
 * @return TRUE if the strings are identical, FALSE otherwise
 */
bool wstr_equal(LPWSTR a, LPWSTR b)
{
	for (; *a == *b && *a; a++, b++)
		;

	return *a == *b;
}

/** @brief Copy a UCS2 string
 * 
 * This function copies the given string, allocating memory to
 * do so.
 * 
 * @param[in] string the string to copy
 * @return pointer to the new string, or NULL on failure
 */
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

/** @} */
