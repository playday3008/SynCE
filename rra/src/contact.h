#ifndef __contact_h__
#define __contact_h__

#include <rapi.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define CONTACT_OID_UNKNOWN 0xffffffff
	
bool contact_to_vcard(
		uint32_t oid, 
		CEPROPVAL* pFields, 
		uint32_t count, 
		char** ppVcard);

void contact_free_vcard(char* vcard);

#ifdef __cplusplus
}
#endif


#endif

