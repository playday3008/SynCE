#ifndef __dbstream_h__
#define __dbstream_h__

#include <rapi.h>

#ifdef __cplusplus
extern "C"
{
#endif

bool dbstream_to_propvals(
		const uint8_t* stream,
		uint32_t count,
		CEPROPVAL* propval);

#define dbstream_free_propvals(p) if(p) free(p)

#ifdef __cplusplus
}
#endif

#endif

