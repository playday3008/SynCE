/* $Id$ */
#define _BSD_SOURCE 1
#include "liborange_internal.h"
#include "liborange_log.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 0x1000

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

bool orange_copy(
    FILE* input_file, 
    size_t size, 
    const char* output_directory, 
    const char* filename)
{
  bool success = false;
  char output_filename[0x200];
  FILE* output_file = NULL;
  unsigned bytes;
  size_t bytes_to_transfer = 0;
  size_t bytes_left = 0;
  uint8_t buffer[BUFFER_SIZE];

  snprintf(output_filename, sizeof(output_filename), "%s/%s", output_directory, filename);
  output_file = fopen(output_filename, "w");
  if (!output_file)
    goto exit;

  for (bytes_left = size; bytes_left; bytes_left -= bytes_to_transfer)
  {
    size_t bytes_written = 0;
    bytes_to_transfer = MIN(BUFFER_SIZE, bytes_left);

    bytes = fread(buffer, 1, bytes_to_transfer, input_file);
    if (bytes != bytes_to_transfer)
    {
      synce_error("Failed to read from file");
      goto exit;
    }

    bytes_written = fwrite(buffer, 1, bytes_to_transfer, output_file);
    if (bytes_written != bytes_to_transfer)
    {
      synce_error("Failed to write to file");
      goto exit;
    }
  }

  success = true;

exit:
  FCLOSE(output_file);
  return success;
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
  assert(p != NULL);
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

uint8_t orange_read_byte(FILE* input_file)/*{{{*/
{
  uint8_t byte;
  if (sizeof(byte) != fread(&byte, 1, sizeof(byte), input_file))
    byte = 0;
#if VERBOSE
  fprintf(stderr, "%02x ", byte);
#endif
  return byte;
}/*}}}*/

uint32_t orange_read32(FILE* input_file)
{
  return orange_read_byte(input_file) |
    (orange_read_byte(input_file) << 8) |
    (orange_read_byte(input_file) << 16) |
    (orange_read_byte(input_file) << 24);
}

bool orange_write_byte(FILE* output_file, uint8_t byte)/*{{{*/
{
  return sizeof(byte) == fwrite(&byte, 1, sizeof(byte), output_file);
}/*}}}*/


