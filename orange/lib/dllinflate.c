/* $Id$ */
#include "liborange_internal.h"

#include "liborange_log.h"
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pe.h"

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
  uint32_t resources_virtual_address;
  uint32_t resources_raw_address;
  uint32_t data_virtual_address;
  uint32_t data_raw_address;
  uint32_t data_size;
  FILE* input = fopen(input_filename, "r");
  int error;

  if (!input)
  {
    synce_error("Failed to open file for reading: '%s'", input_filename);
    goto exit;
  }

  *input_buffer = NULL;
  *input_size   = 0;

  /*
     Find resource section
   */

  if (!pe_rsrc_offset(input, &resources_raw_address, &resources_virtual_address))
  {
    synce_debug("pe_rsrc_offset failed");
    goto exit;
  }

  /*
     Find resource entry to find resource data
   */

  /* this move could be more elegant :-) */
  error = fseek(input, resources_raw_address + 0x138, SEEK_SET);
  if (error)
  {
    /*synce_debug("fseek to %08x failed", resources_raw_address + 0x138);*/
    goto exit;
  }

  fread(&data_virtual_address, 1, sizeof(data_virtual_address), input);
  fread(&data_size,            1, sizeof(data_size),            input);

  LETOH32(data_virtual_address);
  LETOH32(data_size);

  /*
     Get data
    */
  
  data_raw_address = data_virtual_address - resources_virtual_address + resources_raw_address;

 
  fseek(input, data_raw_address, SEEK_SET);

  *input_size   = data_size;
  *input_buffer = (Byte*)malloc(*input_size);

  if (!*input_buffer)
  {
    /* this probably means that this is not a DllInflate file */
    /*synce_error("Failed to allocate %li bytes", *input_size);*/
    goto exit;
  }

  synce_trace("Getting 0x%08x (%i) bytes from offset 0x%08x (%i)",
      data_size, data_size, data_raw_address, data_raw_address);

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

