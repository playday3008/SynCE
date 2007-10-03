#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif
#include "liborange_internal.h"

#include <stdio.h>
#include <string.h>

#include "pe.h"

                        /* deadbeef -- very funny */
#define SIGNATURE       "\x00\x00\x00\x00\xef\xbe\xad\xdeNullsoft"
#define SIGNATURE_SIZE  16

bool orange_is_nullsoft_installer(const char* input_filename)
{
  bool success = false;
  uint32_t offset;
  FILE* input_file = NULL;
  int error;
  char signature[SIGNATURE_SIZE+1];
  unsigned bytes;

#if VERBOSE
  synce_trace("here");
#endif

  input_file = fopen(input_filename, "r");

  if (!pe_size(input_file, &offset))
  {
#if VERBOSE
    synce_trace("pe_size failed");
#endif
    goto exit;
  }

  error = fseek(input_file, offset, SEEK_SET);
  if (error)
  {
#if VERBOSE
    synce_trace("fseek failed");
#endif
    goto exit;
  }

  bytes = fread(signature, 1, SIGNATURE_SIZE, input_file);
  if (bytes != SIGNATURE_SIZE)
  {
#if VERBOSE
    synce_trace("fread failed");
#endif
    goto exit;
  }

  signature[SIGNATURE_SIZE] = '\0';

  if (strcmp(signature, SIGNATURE) != 0)
  {
#if VERBOSE
    synce_trace("signature comparison failed");
#endif
    goto exit;
  }

  success = true;

exit:
  FCLOSE(input_file);
  return success;
}

