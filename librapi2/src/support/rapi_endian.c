/* $Id$ */
#include "rapi_endian.h"

#if IMPLEMENT_BSWAP_XX

/*
 * Written by Manuel Bouyer <bouyer@netbsd.org>.
 * Public domain.
 */

u_int16_t bswap_16(u_int16_t x)
{
	return ((x << 8) & 0xff00) | ((x >> 8) & 0x00ff);
}
	
u_int32_t bswap_32(u_int32_t x)
{
	return	((x << 24) & 0xff000000 ) |
		((x <<  8) & 0x00ff0000 ) |
		((x >>  8) & 0x0000ff00 ) |
		((x >> 24) & 0x000000ff );
}

#endif

