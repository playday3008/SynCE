/* $Id$ */

#include "librra.h"
#include <stdint.h>
#include <synce.h>

bool time_zone_get_information(TimeZoneInformation* tzi);
void time_zone_get_id(TimeZoneInformation* tzi, char** id);
#define time_zone_free_id(id)  if (id) free(id)

time_t time_zone_convert_to_utc(TimeZoneInformation* tzi, time_t unix_time);

struct _Generator;

bool time_zone_generate_vtimezone(struct _Generator* generator, TimeZoneInformation* tzi);

