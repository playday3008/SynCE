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

bool dbstream_from_propvals(
		CEPROPVAL* propval,
		uint32_t count,
		uint8_t** result,
		size_t* result_size);

#define dbstream_free_propvals(p)  if(p) free(p)
#define dbstream_free_stream(p)    if(p) free(p)

#ifdef __cplusplus
}
#endif

#endif

