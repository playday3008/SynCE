/* $Id$ */
#ifndef __little_endian_h__
#define __little_endian_h__

/*
 * This file will make sure that the functions/macros htole16 and htole32 
 * are available.
 */

#ifdef HAVE_CONFIG_H
#include "rapi_config.h"
#else
#error HAVE_CONFIG_H not defined.
#endif

/* needed first */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/* try to get host-to-little-endian and little-endian-to-host macros */
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
#define IMPLEMENT_BSWAP_XX
u_int16_t bswap_16(u_int16_t x);
u_int32_t bswap_32(u_int32_t x);
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

#endif

