/* $Id$ */
#ifndef __rapi_wstr_h__
#define __rapi_wstr_h__

#include "rapi_internal.h"

/**
 * Convert a string from UCS2 to iso8859-1
 */
char* rapi_wstr_to_ascii(LPCWSTR unicode);

/**
 * Convert a string from iso8859-1 to UCS2
 */
LPWSTR rapi_wstr_from_ascii(const char* ascii);

/**
 * Free a string returned by a conversion function
 */
void rapi_wstr_free_string(void* str);

/**
 * Return size of ascii string as unicode
 */
size_t rapi_wstr_string_length(LPCWSTR unicode);

#endif

