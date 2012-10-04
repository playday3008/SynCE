/* $Id$ */
#define _GNU_SOURCE 1
#include "strv.h"
#include <synce_log.h>
#include <stdlib.h>
#include <string.h>
#include "internal.h"

char** strsplit(const char* source, int separator)/*{{{*/
{
  int i;
  int count = 0;
  const char* p = NULL;
  char** result = NULL;
  size_t length = 0;

  if (!source)
    return NULL;

  for (p = source; *p; p++)
    if (separator == *p)
      count++;

  result = malloc((count + 2) * sizeof(char*));

  for (p = source, i = 0; i < count; i++)
  {
    length = strchr(p, separator) - p;
    result[i] = strndup(p, length);
    p += length + 1;
  }

  result[i++] = strdup(p);

  result[i] = NULL;
  return result;
}/*}}}*/

void strv_dump(char** strv)/*{{{*/
{
	char** pp;

	for (pp = strv; *pp; pp++)
		synce_trace("'%s'", *pp);
}/*}}}*/

void strv_free(char** strv)/*{{{*/
{
	char** pp;

	for (pp = strv; *pp; pp++)
		free(*pp);

	free(strv);
}/*}}}*/


