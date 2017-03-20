/* $Id$ */
#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif
#include "liborange_internal.h"
#include "liborange_log.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define VERBOSE 0

typedef struct
{
  unsigned offset;
  unsigned size;
  unsigned filename_size;
  unsigned unknown3;
  unsigned unknown4;
  char* filename;
} FileEntry;

/*
   .apk files used by TomTom products
 */

static void ugly_copy(FILE* output_file, size_t offset, size_t size)/*{{{*/
{
  uint8_t* buffer = malloc(size);
  size_t bytes_copied;

#if VERBOSE
  fprintf(stderr, "Copy %08x bytes from offset %08x to offset %08lx\n",
      size, offset, ftell(output_file));
#endif

/*  fflush(output_file);*/
  fseek(output_file, offset, SEEK_SET);
  
  bytes_copied = fread(buffer, 1, size, output_file);
  
  fseek(output_file, 0, SEEK_END);

  if (size != bytes_copied)
  {
    fprintf(stderr, "Copy %08x bytes from offset %08x to offset %08lx failed\n",
        (int) size, (int) offset, ftell(output_file));
    abort(); 
  }
  
  
  bytes_copied = fwrite(buffer, 1, size, output_file);
  assert(size == bytes_copied);
}/*}}}*/

bool orange_extract_apk(/*{{{*/
    const char* input_filename,
    const char* output_directory)
{
  bool success = false;
  FILE* input_file = fopen(input_filename, "r");
  FILE* output_file = NULL;
  size_t uncompressed_size;
  uint8_t magic_byte;
  uint8_t current_byte;
  size_t bytes_written = 0;
  char output_filename[256];
  const char* basename;
  char* p;

  if (!input_file)
    goto exit;

  basename = strrchr(input_filename, '/');
  if (basename)
    basename++;
  else
    basename = input_filename;

  snprintf(output_filename, sizeof(output_filename), "%s/%s", output_directory, basename);

  p = strrchr(output_filename, '.');
  if (p && p > strrchr(output_filename, '/'))
    strcat(p, ".arh");

  output_file = fopen(output_filename, "w+");
  if (!output_file)
    goto exit;

  if (orange_read_byte(input_file) != 'A' ||
      orange_read_byte(input_file) != 'R' ||
      orange_read_byte(input_file) != 'P' ||
      orange_read_byte(input_file) != 'K')
    goto exit;

  uncompressed_size = 
    orange_read_byte(input_file) |
    (orange_read_byte(input_file) << 8) |
    (orange_read_byte(input_file) << 16) |
    (orange_read_byte(input_file) << 24);

  synce_trace("ARPK signature found");

  synce_trace("uncompressed size: %08x (%i)", uncompressed_size, uncompressed_size);

  magic_byte = orange_read_byte(input_file);

#if VERBOSE
  fprintf(stderr, "Block start\n");
#endif

  while (bytes_written < uncompressed_size)
  {
    unsigned count;

    current_byte = orange_read_byte(input_file);
  
    if (magic_byte == current_byte)
    {
      unsigned offset;

#if VERBOSE
      fprintf(stderr, "Block stop (offset %08lx)\n", ftell(output_file));
#endif

      current_byte = orange_read_byte(input_file);

      if (magic_byte == current_byte)
      {
        count = 1;
      }
      else if (current_byte <= 9)
      {
        size_t offset_bytes = current_byte % 5;
        size_t size_bytes   = current_byte / 5;

        offset = orange_read_byte(input_file);

        if (offset_bytes > 1)
          offset |= orange_read_byte(input_file) << 8;
        if (offset_bytes > 2)
          offset |= orange_read_byte(input_file) << 16;
        if (offset_bytes > 3)
          offset |= orange_read_byte(input_file) << 24;
         
        count = orange_read_byte(input_file);
        
        if (size_bytes > 0)
          count |= orange_read_byte(input_file) << 8;
        if (size_bytes > 1)
          abort();
        
        ugly_copy(output_file, offset, count);
        bytes_written += count;
        count = 0;
      }
      else
      {
        count = current_byte - 5;
        current_byte = orange_read_byte(input_file);
#if VERBOSE
        fprintf(stderr, "Byte %02x repeated %02x times at offset %08lx\n", current_byte, count, ftell(output_file));
#endif
      }
    }
    else
    {
      count = 1;
    }

    while (count--)
    {
      orange_write_byte(output_file, current_byte);
      bytes_written++;
    }
  } /* for() */

  success = (bytes_written == uncompressed_size);

  if (success)
    synce_trace("Wrote '%s'", output_filename);

exit:
  if (!success && output_file)  
    unlink(output_filename);
  FCLOSE(input_file);
  FCLOSE(output_file);
  return success;
}/*}}}*/

bool orange_extract_arh(/*{{{*/
    const char* input_filename,
    const char* output_directory)
{
  bool success = false;
  FILE* input_file = fopen(input_filename, "r");
  size_t uncompressed_size;
  unsigned count = 0;
  unsigned i;
  FileEntry* entries = NULL;
  char* buffer = NULL;
  size_t buffer_size = 0;
  
  if (!input_file)
    goto exit;

  if (orange_read_byte(input_file) != 'T' ||
      orange_read_byte(input_file) != 'O' ||
      orange_read_byte(input_file) != 'M' ||
      orange_read_byte(input_file) != 'A')
    goto exit;

  synce_trace("Found TOMA signature");

  uncompressed_size = orange_read32(input_file);
  count = orange_read32(input_file);

  entries = calloc(count, sizeof(FileEntry));
  
  for (i = 0; i < count; i++)
  {
    entries[i].offset         = orange_read32(input_file);
    entries[i].size           = orange_read32(input_file);
    entries[i].filename_size  = orange_read32(input_file);
    entries[i].unknown3       = orange_read32(input_file);
    entries[i].unknown4       = orange_read32(input_file);
  }

  synce_trace("File list offset: %08lx", ftell(input_file));

  for (i = 0; i < count; i++)
  {
    char* p;
    
    entries[i].filename = malloc(entries[i].filename_size);
    if (entries[i].filename_size != fread(entries[i].filename, 1, entries[i].filename_size, input_file))
      goto exit;

    for (p = entries[i].filename; *p; p++)
      if (*p == '\\')
        *p = '/';
  }

  for (i = 0; i < count; i++)
  {
    fseek(input_file, entries[i].offset, SEEK_SET);

    if (buffer_size < entries[i].size)
    {
      buffer_size = entries[i].size;
      buffer = realloc(buffer, buffer_size);
      if (!buffer)
        goto exit;
    }

    if (entries[i].size != fread(buffer, 1, entries[i].size, input_file))
    {
      goto exit;
    }

    synce_trace("Writing '%s'", entries[i].filename);
    orange_write((const uint8_t*)buffer, entries[i].size, output_directory, entries[i].filename);
  }

  success = true;

exit:
  FREE(buffer);
  if (entries)
  {
    for (i = 0; i < count; i++)
    {
      FREE(entries[i].filename);
    }
    free(entries);
  }
  FCLOSE(input_file);
  return success;
}
