/* $Id$ */
#ifndef __synce_h__
#define __synce_h__

/* 
 * Include some files 
 */
#include <stdint.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifndef NULL
#include <stdlib.h>
#endif


/*
 * Get data types for compatibility with Microsoft Windows
 */
#include "synce_types.h"


/*
 * Functions provided by libsynce
 */

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * FILETIME conversion
 */

#include <time.h>

void filetime_from_unix_time(time_t unix_time, FILETIME *filetime);
time_t filetime_to_unix_time(const FILETIME *filetime);


/*
 * Wide string handling
 */

char* wstr_to_ascii(LPCWSTR unicode);

LPWSTR wstr_from_ascii(const char* ascii);

void wstr_free_string(void* str);

size_t wstr_strlen(LPCWSTR unicode);

LPWSTR wstr_strcpy(LPWSTR dest, LPCWSTR src);

bool wstr_append(LPWSTR dest, LPCWSTR src, size_t max_dest_length);

bool wstr_equal(LPWSTR a, LPWSTR b);


#ifdef __cplusplus
}
#endif


/*
 * Function names for compatibility with Microsoft Windows
 */

#define wcslen(a)    wstr_strlen(a)
#define wcscpy(a,b)  wstr_strcpy(a,b)


/*
 * Endian conversions
 */

#include <sys/types.h>

#ifdef HAVE_ENDIAN_H
#include <endian.h>
#elif HAVE_MACHINE_ENDIAN_H 
#include <machine/endian.h>
#endif

#if !defined(htole32) || !defined(htole16) || !defined(letoh16) || !defined(letoh32)

/* define host-to-little-endian and little-endian-to-host macros */
#ifdef WORDS_BIGENDIAN

/* byte swapping */
#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#elif HAVE_SYS_BYTESWAP_H
#include <sys/byteswap.h>
#else
#define IMPLEMENT_BSWAP_XX  1
uint16_t bswap_16(uint16_t x);
uint32_t bswap_32(uint32_t x);
#endif

/* Use bswap_xx */

#define htole16(x)		bswap_16(x)
#define htole32(x)		bswap_32(x)
#define letoh16(x)    bswap_16(x)
#define letoh32(x)    bswap_32(x)

#else

/* Empty macros */

#define htole16(x)		(x)
#define htole32(x)		(x)
#define letoh16(x)    (x)
#define letoh32(x)    (x)

#endif
#endif


/*
 * Configuration directory and file name stuff
 */

bool synce_get_directory(char** path);
bool synce_get_connection_filename(char** filename);


#endif

