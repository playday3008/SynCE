#ifndef __contact_h__
#define __contact_h__

#include <rapi.h>

#ifdef __cplusplus
extern "C"
{
#endif

bool contact_to_vcard(
		uint32_t oid, 
		CEPROPVAL* pFields, 
		uint32_t count, 
		char** ppVcard);

#ifdef __cplusplus
}
#endif


#endif

