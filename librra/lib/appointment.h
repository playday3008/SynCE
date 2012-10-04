/* $Id$ */
#ifndef __appointment_h__
#define __appointment_h__

#include <synce.h>

struct _RRA_Timezone;

#define RRA_APPOINTMENT_ID_UNKNOWN  0

#define RRA_APPOINTMENT_NEW     		0x1
#define RRA_APPOINTMENT_UPDATE  		0x2
#define RRA_APPOINTMENT_COMMAND_MASK		0xf

#define RRA_APPOINTMENT_ISO8859_1		0x10
#define RRA_APPOINTMENT_UTF8			0x20
#define RRA_APPOINTMENT_CHARSET_MASK		0xf0

#ifndef SWIG
bool rra_appointment_to_vevent(
    uint32_t id,
    const uint8_t* data,
    size_t data_size,
    char** vevent,
    uint32_t flags,
    struct _RRA_Timezone* tzi,
    const char *codepage);

bool rra_appointment_from_vevent(
    const char* vevent,
    uint32_t* id,
    uint8_t** data,
    size_t* data_size,
    uint32_t flags,
    struct _RRA_Timezone* tzi,
    const char *codepage);

#define rra_appointment_free_vevent(p)  if (p) free(p)
#define rra_appointment_free_data(p)    if (p) free(p)

#endif /* SWIG */

#endif
