/* $Id$ */
#ifndef __little_endian_h__
#define __little_endian_h__

/*
 * This file will make sure that the functions/macros htole16 and htole32 
 * are available.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#error No config.h available.
#endif

/* needed first */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/* byte swapping */
#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#elif HAVE_SYS_BYTESWAP_H
#include <sys/byteswap.h>
#endif

/* endianness */
#ifdef HAVE_ENDIAN_H
#include <endian.h>
#elif HAVE_MACHINE_ENDIAN_H 
#include <machine/endian.h>
#endif

/* define host-to-little-endian and little-endian-to-host macros */
#if !defined(htole32) || !defined(htole16) || !defined(letoh16) || !defined(letoh32)
#ifdef WORDS_BIGENDIAN
#define htole16(x)		bswap_16(x)
#define htole32(x)		bswap_32(x)
#define letoh16(x)    bswap_16(x)
#define letoh32(x)    bswap_32(x)
#else
#define htole16(x)		(x)
#define htole32(x)		(x)
#define letoh16(x)    (x)
#define letoh32(x)    (x)
#endif
#endif

#endif

