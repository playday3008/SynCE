/* $Id$ */
#define _BSD_SOURCE 1
#include "liborange_internal.h"
#include <ctype.h>
#include <synce_log.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

/*
   Installer VISE - http://www.mindvision.com
 */

typedef struct
{
  char* filename;
  unsigned compressed_size;
  uint8_t* buffer;
} FileEntry;



#define OUTPUT_BUFFER_SIZE 0x8000

/* almost the same as DllInflate in dllinflate.c, only different initialization of zlib */
static bool orange_decompress_to_file(const uint8_t* input_buffer, size_t input_size, const char* output_filename)/*{{{*/
{
  bool success = false;
  z_stream stream;
  int error;
  Byte* output_buffer = malloc(OUTPUT_BUFFER_SIZE);
  FILE* output = fopen(output_filename, "w");

  if (!output_buffer)
  {
    synce_error("Failed to allocate %i bytes", OUTPUT_BUFFER_SIZE);
    goto exit;
  }

  if (!output)
  {
    synce_error("Failed to open file for writing: '%s'", output_filename);
    goto exit;
  }

  stream.next_in  = (Byte*)input_buffer;
  stream.avail_in = input_size;
  
  stream.zalloc = NULL;
  stream.zfree  = NULL;

  error = inflateInit2(&stream, -MAX_WBITS);
  if (Z_OK != error)
  {
    synce_error("inflateInit failed with error %i", error);
    goto exit;
  }

  while (error != Z_STREAM_END)
  {
    uInt bytes_to_write;

    stream.next_out   = output_buffer;
    stream.avail_out  = OUTPUT_BUFFER_SIZE;

    error = inflate(&stream, Z_NO_FLUSH);

    if (error < Z_OK)
    {
      synce_error("inflate failed with error %i", error);
      goto exit;
    }

    bytes_to_write = OUTPUT_BUFFER_SIZE - stream.avail_out;

    if (bytes_to_write != fwrite(output_buffer, 1, bytes_to_write, output))
    {
      synce_error("Failed to write %i bytes to output file '%s'", 
          bytes_to_write, output_filename);
      goto exit;
    }
  }

  success = (input_size == stream.total_in) || (input_size == stream.total_in+1);

exit:
  FCLOSE(output);
  FREE(output_buffer);
  return success;
}/*}}}*/

static char* orange_read_vise_string(FILE* input_file, int size_bytes)/*{{{*/
{
  unsigned size = 0;
  char* result = NULL;
  char*p;

  size = orange_read_byte(input_file);
  
  if (size_bytes >= 2)
    size |= orange_read_byte(input_file) << 8;
  
  if (size_bytes >= 3)
    size |= orange_read_byte(input_file) << 16;
  
  if (size_bytes >= 4)
    size |= orange_read_byte(input_file) << 24;

  if (!size)
    return strdup("");

  result = malloc(size + 1);
  if (!result)
    return NULL;
  
  if (size != fread(result, 1, size, input_file))
  {
    free(result);
    return NULL;
  }

  result[size] = '\0';

  for (p = result; *p != '\0'; p++)
    if (!isprint(*p))
      abort();

  return result;
}/*}}}*/

bool orange_extract_vise(
    const char* input_filename, 
    const char* output_directory)
{
  bool success = false;
  FILE* input_file = fopen(input_filename, "r");
  int i;
  unsigned start_offset;

  if (!input_file)
    goto exit;

  fseek(input_file, -8, SEEK_END);

  if (orange_read_byte(input_file) != 'E' ||
      orange_read_byte(input_file) != 'S' ||
      orange_read_byte(input_file) != 'I' ||
      orange_read_byte(input_file) != 'V')
    goto exit;

  synce_trace("ESIV end signature found");

  start_offset = orange_read32(input_file);
  
  fseek(input_file, start_offset, SEEK_SET);

  if (orange_read_byte(input_file) != 'E' ||
      orange_read_byte(input_file) != 'S' ||
      orange_read_byte(input_file) != 'I' ||
      orange_read_byte(input_file) != 'V')
    goto exit;

  synce_trace("ESIV start signature found");

  /* skip unknown stuff */
  fseek(input_file, 0x3f, SEEK_CUR);

  for (i = 0; i < 2; i++)
  {
    unsigned unknown;
    int file_count;
    int j;

    synce_trace("Offset: %08lx", ftell(input_file));

    file_count = 
      orange_read_byte(input_file) |
      orange_read_byte(input_file) << 8;

    synce_trace("File count: %04x", file_count);

    for (j = 0; j < file_count; j++)
    {
      unsigned filename_size;
      synce_trace("Offset: %08lx", ftell(input_file));
      filename_size = orange_read_byte(input_file);

      if (filename_size)
      {
        FileEntry entry;
        unsigned k;
        char output_filename[256];
        synce_trace("Filename size: %02x", filename_size);

        entry.filename = malloc(filename_size + 1);
        if (!entry.filename)
          goto exit;

        if (filename_size != fread(entry.filename, 1, filename_size, input_file))
          goto exit;

        entry.filename[filename_size] = '\0';

        unknown = orange_read32(input_file);
        synce_trace("Unknown 1: %08x", unknown);

        unknown = orange_read32(input_file);
        if (unknown != 0)
          synce_trace("Unknown 2 not zero but %08x", unknown);

        unknown = orange_read32(input_file);
        if (unknown != 0)
          synce_trace("Unknown 3 not zero but %08x", unknown);

        entry.compressed_size = orange_read32(input_file);

        synce_trace("%s %08x", entry.filename, entry.compressed_size);

        entry.buffer = malloc(entry.compressed_size);
        if (!entry.buffer)
          goto exit;

        if (entry.compressed_size & 1)
          synce_trace("File size not even!");

        if (entry.compressed_size != fread(entry.buffer, 1, entry.compressed_size, input_file))
          goto exit;

        /* who the hell thought up this obfuscation? */
        for (k = 0; k < entry.compressed_size; k+=2)
        {
          uint8_t tmp = entry.buffer[k];
          entry.buffer[k] = entry.buffer[k+1];
          entry.buffer[k+1] = tmp;
        }

        snprintf(output_filename, sizeof(output_filename), "%s/%s", 
            output_directory, entry.filename);
        if (!orange_decompress_to_file(entry.buffer, entry.compressed_size, output_filename))
          goto exit;

        FREE(entry.buffer);
        FREE(entry.filename);
      }
      else
      {
        goto exit;
      }
    } /* for() */

    synce_trace("End of file data");

    {
      char* str;
      int j;
      int key_value_pairs;

      synce_trace("Offset: %08lx", ftell(input_file));
      
      unknown =
        orange_read32(input_file);
      synce_trace("Unknown: %08x", unknown);

      str = orange_read_vise_string(input_file, 2);
      synce_trace("'%s'", str);
      FREE(str);

      str = orange_read_vise_string(input_file, 2);
      synce_trace("'%s'", str);
      FREE(str);

      unknown =
        orange_read_byte(input_file) |
        orange_read_byte(input_file) << 8;
      synce_trace("Unknown: %04x", unknown);

      str = orange_read_vise_string(input_file, 2);
      synce_trace("'%s'", str);
      FREE(str);

      str = orange_read_vise_string(input_file, 2);
      synce_trace("'%s'", str);
      FREE(str);

      unknown =
        orange_read_byte(input_file) |
        orange_read_byte(input_file) << 8;
      synce_trace("Unknown: %04x", unknown);

      unknown = orange_read32(input_file);
      synce_trace("Unknown: %08x", unknown);

      /* skip block */
      fseek(input_file, unknown, SEEK_CUR);

      unknown = orange_read_byte(input_file);
      synce_trace("Unknown: %02x", unknown);

      unknown = orange_read32(input_file);
      synce_trace("Unknown: %08x", unknown);

      key_value_pairs = orange_read32(input_file);

      for (j = 0; j < key_value_pairs; j++)
      {
        char* key;
        char* value;
        
        key   = orange_read_vise_string(input_file, 2);
        value = orange_read_vise_string(input_file, 2);

        synce_trace("'%s'='%s'", key, value);
        
        FREE(key);
        FREE(value);
      }

      /* skip block */
      fseek(input_file, 8, SEEK_CUR);
    }
  }

exit:
  FCLOSE(input_file);
  return success;
}

