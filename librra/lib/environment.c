/* $Id$ */
#include "environment.h"
#include <stdlib.h>
#include <string.h>
#include <synce.h>

void* environment_push_timezone(const char* name)
{
  char* old_tz = getenv("TZ");
  
  if (old_tz)
    old_tz = strdup(old_tz);
  setenv("TZ", "UTC", true);
  
  return old_tz;
}

void environment_pop_timezone(void* handle)
{
  char* old_tz = (char*)handle;

  if (old_tz)
  {
    setenv("TZ", old_tz, true);
    free(old_tz);
  }
  else
    unsetenv("TZ");
}

