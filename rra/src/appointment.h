/* $Id$ */
#ifndef __appointment_h__
#define __appointment_h__

#include <rapi.h>

bool appointment_to_vcal(
	uint32_t id,
	CEPROPVAL* pFields,
	uint32_t count,
	char** ppVcal);

#define APPOINTMENT_OID_UNKNOWN 0xffffffff


#endif

