/* $Id$ */
#define _BSD_SOURCE 1
#include "liborange_internal.h"
#include <synce_log.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
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

bool orange_write(const uint8_t* output_buffer, size_t output_size, const char* output_directory, const char* basename)/*{{{*/
{
  bool success = false;
  char filename[256];
  FILE* output = NULL;
  char*p;

  /* allow basename to contain path components... */
  
  snprintf(filename, sizeof(filename), "%s/%s", output_directory, basename);
  p = strrchr(filename, '/');
  assert(p);
  *p = '\0';

  if (!orange_make_sure_directory_exists(filename))
    goto exit;

  snprintf(filename, sizeof(filename), "%s/%s", output_directory, basename);

  output = fopen(filename, "w");
  if (!output)
  {
    synce_error("Failed to open file for writing: '%s'", filename);
    goto exit;
  }

  if (output_size != fwrite(output_buffer, 1, output_size, output))
  {
    synce_error("Failed to write %i bytes to file '%s'", output_size, filename);
    goto exit;
  }

  success = true;

exit:
  FCLOSE(output);
  return success;
}/*}}}*/


