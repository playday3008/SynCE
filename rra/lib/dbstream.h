#ifndef __dbstream_h__
#define __dbstream_h__

#ifdef __cplusplus
extern "C"
{
#endif

bool dbstream_to_propvals(
		uint8_t* stream,
		uint32_t count,
		CEPROPVAL* propval);

#ifdef __cplusplus
}
#endif

#endif

