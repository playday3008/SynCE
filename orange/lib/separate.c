/* $Id$ */
#define _BSD_SOURCE 1
#include "liborange_internal.h"
#include "liborange_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

static uint8_t* orange_memstr(const void *haystack, const char *needle, size_t size)/*{{{*/
{
  const void *next = haystack;
  size_t input_size = size;
  size_t needle_size = strlen(needle);

  for (;;)
  {
    const void *p = memchr(next, needle[0], input_size);

    if (!p)
      break;

    input_size -= (p - next);

    if (input_size < needle_size)
      break;

    if (0 == memcmp(p, needle, needle_size))
      return (uint8_t*)p;

    next = p + 1;
    input_size--;
  }

  return NULL;
}/*}}}*/

#define MSCF_SIGNATURE "MSCF"
#define MSCE_SIGNATURE "MSCE"

#define MSCF_SIZE                   0x08

#define MSCF_MSCE_HEADER            0x24

#define MSCE_HEADER_TO_DATA         0x08

#define MSCE_SIZE                   0x08
#define MSCE_PROCESSOR              0x14

bool orange_get_installable_cab_info2(/*{{{*/
    uint8_t* input_buffer,
    size_t input_size,
    CabInfo* cab_info)
{
  bool success = false;
  uint8_t *msce;
  size_t msce_offset;

  if (!cab_info)
  {
    synce_error("CabInfo parameter is NULL");
    goto exit;
  }

  memset(cab_info, 0, sizeof(CabInfo));

  if (input_size < (MSCF_MSCE_HEADER + sizeof(uint32_t)))
  {
    synce_error("Input buffer is too small");
    goto exit;
  }

  if (0 != strncmp((char*)input_buffer, MSCF_SIGNATURE, strlen(MSCF_SIGNATURE)))
  {
    synce_error("Not a Microsoft Cabinet file");
    goto exit;
  }

  cab_info->size = letoh32(*(uint32_t*)(input_buffer + MSCF_SIZE));
  msce_offset    = letoh32(*(uint32_t*)(input_buffer + MSCF_MSCE_HEADER)) + MSCE_HEADER_TO_DATA;

  if (input_size < (msce_offset + MSCE_PROCESSOR + sizeof(uint32_t)))
  {
    synce_trace("Invalid or not installable");
    goto exit;
  }

  msce = input_buffer + msce_offset;

  if (0 != strncmp((char*)msce, MSCE_SIGNATURE, strlen(MSCE_SIGNATURE)))
  {
    synce_trace("Not installable");
    goto exit;
  }

  cab_info->processor   = letoh32(*(uint32_t*)(msce + MSCE_PROCESSOR));

  success = true;

exit:
  return success;
}/*}}}*/

bool orange_get_installable_cab_info(/*{{{*/
    const char* input_filename,
    CabInfo* cab_info)
{
  bool success = false;
  FILE* input = fopen(input_filename, "r");
  size_t input_size;
  uint8_t* input_buffer = NULL;

  if (!input)
  {
    synce_error("Failed to open file for reading: '%s'", input_filename);
    goto exit;
  }

  /* don't need the whole file */
  input_size    = MIN(FSIZE(input), 0x8000);
  input_buffer  = (uint8_t*)malloc(input_size);

  if (!input_buffer)
  {
    synce_error("Failed to allocate %i bytes", input_size);
    goto exit;
  }

  if (input_size != fread(input_buffer, 1, input_size, input))
  {
    synce_error("Failed to read %i bytes from file '%s'", input_size, input_filename);
    goto exit;
  }

  success = orange_get_installable_cab_info2(input_buffer, input_size, cab_info);

exit:
  FCLOSE(input);
  FREE(input_buffer);
  return success;
}/*}}}*/

bool orange_get_new_installable_cab_info(/*{{{*/
    const char* input_filename,
    CabInfo* cab_info)
{
  bool success = false;
  FILE *file = NULL;
  char command[1024];
  char tmp[1024];
  size_t bytes_read;

  snprintf(command, sizeof(command), "cabextract -q -p -F_setup.xml '%s'", 
      input_filename);

  /* extract _setup.xml file to stdout */
  file = popen(command, "r");
  if (!file)
  {
    synce_debug("Failed to open pipe for reading: %s", command);
    goto exit;
  }

  bytes_read = fread(tmp, 1, sizeof(tmp)-1, file);
  tmp[bytes_read] = '\0';

  synce_trace("%i bytes read: %s", bytes_read, tmp);

  if (bytes_read == 0)
  {
    synce_debug("No _setup.xml in .cab file");
    goto exit;
  }

  synce_debug("Found _setup.xml in .cab file");

  /* TODO: get these */
  cab_info->processor = 0;
  cab_info->size = 0;

  success = true;

exit:
  if (file)
    pclose(file);
  return success;
}/*}}}*/

typedef struct 
{
  const char* output_directory;
  char* basename;  
} SeparationCookie;

static bool orange_separate_callback(/*{{{*/
    const uint8_t* buffer,
    size_t size,
    CabInfo* info,
    void* cookie)
{
  bool success = false;
  SeparationCookie* sc = (SeparationCookie*)cookie;
  char cabfile[256];
  const char* processor_name = NULL;

  switch (info->processor)
  {
    case 0:
      processor_name = "UnspecifiedProcessor";
      break;

    case 2577:
      processor_name = "StrongARM";
      break;

    case 10003:
      processor_name = "HitachiSH3";
      break;

    case 4000:
      processor_name = "MipsR4000";
      break;
  }

  if (processor_name)
    snprintf(cabfile, sizeof(cabfile), "%s.%s.cab", sc->basename, processor_name);
  else
    snprintf(cabfile, sizeof(cabfile), "%s.%i.cab", sc->basename, info->processor);

  if (!orange_write(buffer, size, sc->output_directory, cabfile))
  {
    synce_error("Failed to write Microsoft Cabinet File to directory '%s'", sc->output_directory);
    goto exit;
  }

  synce_trace("Saving %s", cabfile);

  success = true;

exit:
  return success;
}/*}}}*/

bool orange_separate2(/*{{{*/
    uint8_t* input_buffer,
    size_t input_size,
    orange_buffer_callback callback,
    void* cookie)
{
  bool success = false;
  uint8_t* last = NULL;
  uint8_t* mscf = NULL;
  int cab_count = 0;

  last = input_buffer;

  while ( (mscf = orange_memstr(last, MSCF_SIGNATURE, input_size)) )
  {
    CabInfo cab_info;

    input_size -= (mscf - last);

    if (orange_get_installable_cab_info2(mscf, input_size, &cab_info))
    {
      cab_count++;

      if (!callback(mscf, cab_info.size, &cab_info, cookie))
        goto exit;

      input_size -= cab_info.size;
      last = mscf + cab_info.size;
    }
    else
    {
      last = mscf + 1;
      input_size--;
    }
  }

  success = cab_count > 0;

exit:
  return success;
}/*}}}*/

bool orange_separate(/*{{{*/
    const char* input_filename, 
    const char* output_directory)
{
  bool success = false;
  FILE* input = fopen(input_filename, "r");
  size_t input_size;
  uint8_t* input_buffer = NULL;
  char* p = NULL;
  SeparationCookie cookie;
  
  if (!input)
  {
    synce_error("Failed to open file for reading: '%s'", input_filename);
    goto exit;
  }

  input_size    = FSIZE(input);
  input_buffer  = (uint8_t*)malloc(input_size);

  if (!input_buffer)
  {
    synce_error("Failed to allocate %i bytes", input_size);
    goto exit;
  }

  if (input_size != fread(input_buffer, 1, input_size, input))
  {
    synce_error("Failed to read %i bytes from file '%s'", input_size, input_filename);
    goto exit;
  }

  /* create cookie */

  cookie.output_directory = output_directory;

  p = strrchr(input_filename, '/');
  if (p)
    cookie.basename = strdup(p+1);
  else
    cookie.basename = strdup(input_filename);

  p = strrchr(cookie.basename, '.');
  if (p)
    *p = '\0';

  success = orange_separate2(
      input_buffer, 
      input_size, 
      orange_separate_callback, 
      (void*)&cookie);

  FREE(cookie.basename);

exit:
  FCLOSE(input);
  FREE(input_buffer);
  return success;
}/*}}}*/

