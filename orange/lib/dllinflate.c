/* $Id$ */
#include "liborange_internal.h"
#include <synce_log.h>
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUTPUT_BUFFER_SIZE 0x8000

#define SECTION_HEADER_OFFSET 0x1e0

/**
  Behave similar to the DllInflate function in inflate.dll
 */
static bool DllInflate(const uint8_t* input_buffer, size_t input_size, const char* output_filename)/*{{{*/
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

  error = inflateInit_(&stream, ZLIB_VERSION, sizeof(z_stream));
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

  success = (input_size == stream.total_in);

exit:
  FCLOSE(output);
  FREE(output_buffer);
  return success;
}/*}}}*/

static bool get_compressed_data(const char* input_filename, uint8_t** input_buffer, size_t* input_size)/*{{{*/
{
  bool success = false;
  char name[8];
  uint32_t resources_virtual_address;
  uint32_t resources_raw_address;
  uint32_t resources_raw_size;
  uint32_t data_virtual_address;
  uint32_t data_raw_address;
  uint32_t data_size;
  FILE* input = fopen(input_filename, "r");

  if (!input)
  {
    synce_error("Failed to open file for reading: '%s'", input_filename);
    goto exit;
  }

  *input_buffer = NULL;
  *input_size   = 0;

  /*
     Read PE header to find resource section
   */

  fseek(input, SECTION_HEADER_OFFSET, SEEK_SET);
  fread(name, 1, sizeof(name), input);
  if (0 != strcmp(name, ".rsrc"))
  {
#if 0
    synce_error("Unexpected input file format");
#endif
    goto exit;
  }
 
  fseek(input, 4, SEEK_CUR);

  fread(&resources_virtual_address, 1, sizeof(resources_virtual_address), input);
  fread(&resources_raw_size,        1, sizeof(resources_raw_size),        input);
  fread(&resources_raw_address,     1, sizeof(resources_raw_address),     input);

  LETOH32(resources_virtual_address);
  LETOH32(resources_raw_size);
  LETOH32(resources_raw_address);

  /*
     Find resource entry to find resource data
   */

  /* this move could be more elegant :-) */
  fseek(input, resources_raw_address + 0x138, SEEK_SET);

  fread(&data_virtual_address, 1, sizeof(data_virtual_address), input);
  fread(&data_size,            1, sizeof(data_size),            input);

  LETOH32(data_virtual_address);
  LETOH32(data_size);

  /*
     Get data
    */
  
  data_raw_address = data_virtual_address - resources_virtual_address + resources_raw_address;

  synce_trace("Getting 0x%08x (%i) bytes from offset 0x%08x (%i)",
      data_size, data_size, data_raw_address, data_raw_address);
  
  fseek(input, data_raw_address, SEEK_SET);

  *input_size   = data_size;
  *input_buffer = (Byte*)malloc(*input_size);

  if (!*input_buffer)
  {
    synce_error("Failed to allocate %li bytes", *input_size);
    goto exit;
  }

  /* read data */
  if (*input_size != fread(*input_buffer, 1, *input_size, input))
  {
    synce_error("Failed to read %li bytes", *input_size);
    goto exit;
  }

  success = true;

exit:
  if (!success)
  {
    FREE(*input_buffer);
  }
 
  FCLOSE(input);
  return success;
}/*}}}*/

bool orange_dllinflate(/*{{{*/
    const char* input_filename, 
    const char* output_filename)
{
  bool success = false;
  uint8_t* input_buffer = NULL;
  size_t input_size;

  if (!get_compressed_data(input_filename, &input_buffer, &input_size))
  {
#if 0
    synce_error("Failed to get compressed data");
#endif
    goto exit;
  }

  if (!DllInflate(input_buffer, input_size, output_filename))
  {
    synce_error("Failed to decompress data");
    goto exit;
  }

  success = true;

exit:
  FREE(input_buffer);
  return success;
}/*}}}*/

