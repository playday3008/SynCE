/* $Id$ */
#ifndef __rapi_filetime_h__
#define __rapi_filetime_h__

#include "rapi_types.h"

#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

void rapi_filetime_from_unix_time(time_t unix_time, FILETIME *filetime);
time_t rapi_filetime_to_unix_time(const FILETIME *filetime);

#ifdef __cplusplus
}
#endif


#endif 

