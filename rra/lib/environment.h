/* $Id$ */
#ifndef __environment_h__
#define __environment_h__

void* environment_push_timezone(const char* name);
void  environment_pop_timezone(void* handle);

#endif
