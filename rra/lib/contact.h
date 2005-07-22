/* $Id$ */
#ifndef __contact_h__
#define __contact_h__

#include <synce.h>

/*
 * Convert contact data
 */

#define RRA_CONTACT_ID_UNKNOWN  0

/* flags for rra_contact_(from|to)_vcard() */

#define RRA_CONTACT_NEW     				0x1
#define RRA_CONTACT_UPDATE  				0x2
#define RRA_CONTACT_COMMAND_MASK		0xf

#define RRA_CONTACT_ISO8859_1				0x10
#define RRA_CONTACT_UTF8						0x20
#define RRA_CONTACT_CHARSET_MASK		0xf0

#define RRA_CONTACT_VERSION_UNKNOWN   0x000
#define RRA_CONTACT_VERSION_2_1   		0x100
#define RRA_CONTACT_VERSION_3_0   		0x200
#define RRA_CONTACT_VERSION_MASK			0xf00

#ifndef SWIG
bool rra_contact_to_vcard(
		uint32_t id, 
		const uint8_t* data, 
		size_t data_size,
		char** vcard,
		uint32_t flags);

bool rra_contact_from_vcard(
		const char* vcard, 
		uint32_t* id,
		uint8_t** data, 
		size_t* data_size,
		uint32_t flags);
#endif /* SWIG */

struct FieldStrings
{
	char* name;
	char* type;
	char* value;
	bool pref;
};

#define rra_contact_free_vcard(p) if (p) free(p)
#define rra_contact_free_data(p)  if (p) free(p)

#endif
