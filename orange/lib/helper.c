/* $Id$ */
#define _BSD_SOURCE 1
#include "liborange_internal.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

bool orange_make_sure_directory_exists(const char* directory)/*{{{*/
{
  struct stat dir_stat;
  const char* p = directory;

  while (p && *p)
  {
    if ('/' == *p)
      p++;
    else if (0 == strncmp(p, "./", 2))
      p+=2;
    else if (0 == strncmp(p, "../", 3))
      p+=3;
    else
    {
      char* current = strdup(directory);
      const char* slash = strchr(p, '/');

      if (slash)
        current[slash-directory] = '\0';

      if (stat(current, &dir_stat) < 0)
      {
        if (mkdir(current, 0700) < 0)
        {
          fprintf(stderr, "Failed to create directory %s\n", directory);
          return false;
        }
      }

      p = slash;
    }
  }

  return true;
}/*}}}*/

long orange_fsize(FILE* file)
{
  long result;
  long previous = ftell(file);
  fseek(file, 0L, SEEK_END);
  result = ftell(file);
  fseek(file, previous, SEEK_SET);
  return result;
}


