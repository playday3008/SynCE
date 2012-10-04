/* $Id$ */
#include "internal.h"
#include <string.h>
#include <ctype.h>

/* XXX: poorly tested version for libc without GNU extensions */
char *rra_strcasestr(const char *haystack, const char *needle)
{
  for(;;)
  {
    char *lower = strchr(haystack, tolower(needle[0]));
    char *upper = strchr(haystack, toupper(needle[0]));

    if (lower && (!upper || (lower < upper)))
    {
      if (0 == strncasecmp(lower, needle, strlen(needle)))
	return lower;

      haystack = lower + 1;
    }
    else if (upper)
    {
      if (0 == strncasecmp(upper, needle, strlen(needle)))
	return upper;

      haystack = upper + 1;
    }
    else
    {
      return NULL;
    }
  }
}

