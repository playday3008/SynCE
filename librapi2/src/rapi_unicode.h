/* $Id$ */
#ifndef __rapi_unicode_h__
#define __rapi_unicode_h__

#include "rapi_internal.h"

/**
 * A unicode character
 */
typedef u_int16_t uchar;

/**
 * Convert a string from UCS2 to iso8859-1
 */
char* rapi_unicode_to_ascii(const uchar* unicode);

/**
 * Convert a string from iso8859-1 to UCS2
 */
uchar* rapi_unicode_from_ascii(const char* ascii);

/**
 * Free a string returned by a conversion function
 */
void rapi_unicode_free_string(void* str);

/**
 * Return size of ascii string as unicode
 */
size_t rapi_unicode_string_length(const uchar* unicode);

#endif

