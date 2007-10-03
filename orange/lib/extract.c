/* $Id$ */
#define _BSD_SOURCE 1
#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif
#include "liborange_internal.h"
#if WITH_LIBUNSHIELD
#include <libunshield.h>
#endif
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

bool orange_extract_is_cab(/*{{{*/
    const char* input_filename,
    const char* output_directory)
{
  bool success = false;

#if WITH_LIBUNSHIELD
  Unshield* unshield = NULL;
  int i;
  int count;

  unshield = unshield_open(input_filename);
  if (!unshield)
    goto exit;
  
  count = unshield_file_count(unshield);

  if (count < 0)
    goto exit;

  if (!orange_make_sure_directory_exists(output_directory))
    goto exit;
  
  for (i = 0; i < count; i++)
  {
    char filename[256];
    char* p;

    if (unshield_file_is_valid(unshield, i))
    {
      snprintf(filename, sizeof(filename), "%s/%s", 
          output_directory, unshield_file_name(unshield, i));

      for (p = filename; *p != '\0'; p++)
        if (!isprint(*p))
          *p = '_';

      unshield_file_save(unshield, i, filename);
    }
  }

  success = true;
  
exit:
  unshield_close(unshield);
#endif

  return success;
}/*}}}*/

static bool orange_extract(const char* command)
{
  int status = system(command);

  return -1 != status && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool orange_extract_ms_cab(
    const char* input_filename,
    const char* output_directory)
{
  char command[1024];

  snprintf(command, sizeof(command), "cabextract -q -d \"%s\" \"%s\" 2>/dev/null",
      output_directory, input_filename);
  
  return orange_extract(command);
}

bool orange_extract_rar(
    const char* input_filename,
    const char* output_directory)
{
  bool success = false;
  return success;
}

bool orange_extract_zip(
    const char* input_filename,
    const char* output_directory)
{
  char command[1024];

  snprintf(command, sizeof(command), "unzip -o -qq -d \"%s\" \"%s\" 2>/dev/null",
      output_directory, input_filename);
  
  return orange_extract(command);
}

