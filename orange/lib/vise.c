/* $Id$ */
#define _BSD_SOURCE 1
#include "liborange_internal.h"
#include <ctype.h>
#include <synce_log.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define VERBOSE 0
#define DEBUG_ZLIB 0

/*
   Installer VISE - http://www.mindvision.com
 */

typedef struct
{
  char* filename;
  unsigned compressed_size;
  uint8_t* buffer;
} FileEntry;

#ifdef DEBUG_ZLIB
extern int z_verbose;
#endif


#define OUTPUT_BUFFER_SIZE 0x8000

static bool orange_decompress_to_file(uint8_t* input_buffer, size_t input_size, size_t expected_output_size, const char* output_filename)/*{{{*/
{
  bool success = false;
  Byte* output_buffer = malloc(OUTPUT_BUFFER_SIZE);
  bool decompress = true;
  uint8_t* fixed_buffer = NULL;

  if (!output_buffer)
  {
    synce_error("Failed to allocate %i bytes", OUTPUT_BUFFER_SIZE);
    goto exit;
  }

  while (decompress)
  {
    FILE* output = fopen(output_filename, "w");
    z_stream stream;
    int error;

    decompress = false;
    
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

      bytes_to_write = OUTPUT_BUFFER_SIZE - stream.avail_out;

      if (bytes_to_write != fwrite(output_buffer, 1, bytes_to_write, output))
      {
        synce_error("Failed to write %i bytes to output file '%s'", 
            bytes_to_write, output_filename);
        goto exit;
      }

      if (error < Z_OK)
      {

        /*
           Sometimes the decompression fails because of "incorrect stored block
           lengths". It seems like there is a zero byte to much!
         */
           
        unsigned offset;
        offset = input_size - stream.avail_in;

        synce_trace("offset=%08x: %02x %02x %02x %02x %02x",
            offset-4,
            input_buffer[offset-4],
            input_buffer[offset-3],
            input_buffer[offset-2],
            input_buffer[offset-1],
            input_buffer[offset-0]);

        if (0 == input_buffer[offset-4] &&
            input_buffer[offset-1] == (~input_buffer[offset-3] & 0xff) &&
            input_buffer[offset-2] == (~input_buffer[offset-0] & 0xff))
        {
          synce_warning("Fixing incorrect stored block length and trying again...");
          
          input_size--;
          memmove(input_buffer + offset - 4, input_buffer + offset - 3, input_size - offset + 3);

          decompress = true;
          goto exit;
        }

        synce_error("inflate failed with error %i '%s' avail_in=%08x, total_out=%08x", 
            error, stream.msg, stream.avail_in, stream.total_out);
         goto exit;
      }

    }
  
    if (expected_output_size == (size_t)-1)
      success = (input_size == stream.total_in) || (input_size == stream.total_in+1);
    else
      success = stream.total_out == expected_output_size;

exit:
    FCLOSE(output);
  }

  FREE(fixed_buffer);
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
    if (!isprint(*p) && !iscntrl(*p))
      abort();

  return result;
}/*}}}*/

static void orange_correct_compressed_vise_buffer(uint8_t* buffer, size_t compressed_size)/*{{{*/
{
  unsigned k;

#if 0
  synce_trace("Last two bytes in buffer: %02x %02x",
      buffer[compressed_size-2], buffer[compressed_size-1]);
#endif

  /* who the hell thought up this obfuscation? */
  for (k = 0; k < compressed_size; k+=2)
  {
    uint8_t tmp = buffer[k];
    buffer[k] = buffer[k+1];
    buffer[k+1] = tmp;
  }
}/*}}}*/

static bool orange_read_vise_files(/*{{{*/
    FILE* input_file, 
    const char* output_directory)
{
  bool success = false;
  int file_count;
  int j;
  unsigned unknown;

#if VERBOSE
  synce_trace("Offset: %08lx", ftell(input_file));
#endif

  file_count = 
    orange_read_byte(input_file) |
    orange_read_byte(input_file) << 8;

#if VERBOSE
  synce_trace("File count: %04x", file_count);
#endif

  for (j = 0; j < file_count; j++)
  {
    unsigned filename_size;
#if VERBOSE
    synce_trace("Offset: %08lx", ftell(input_file));
#endif
    filename_size = orange_read_byte(input_file);

    if (filename_size)
    {
      FileEntry entry;
      char output_filename[256];
#if VERBOSE
      synce_trace("Filename size: %02x", filename_size);
#endif

      entry.filename = malloc(filename_size + 1);
      if (!entry.filename)
        goto exit;

      if (filename_size != fread(entry.filename, 1, filename_size, input_file))
        goto exit;

      entry.filename[filename_size] = '\0';

      unknown = orange_read32(input_file);
#if VERBOSE
      synce_trace("Unknown 1: %08x", unknown);
#endif

      unknown = orange_read32(input_file);
#if VERBOSE
      if (unknown != 0)
        synce_trace("Unknown 2 not zero but %08x", unknown);
#endif

      unknown = orange_read32(input_file);
#if VERBOSE
      if (unknown != 0)
        synce_trace("Unknown 3 not zero but %08x", unknown);
#endif

      entry.compressed_size = orange_read32(input_file);

#if VERBOSE
      synce_trace("%s %08x", entry.filename, entry.compressed_size);
#endif

      entry.buffer = malloc(entry.compressed_size);
      if (!entry.buffer)
        goto exit;

      if (entry.compressed_size & 1)
        synce_trace("File size not even!");

      if (entry.compressed_size != fread(entry.buffer, 1, entry.compressed_size, input_file))
        goto exit;

      orange_correct_compressed_vise_buffer(entry.buffer, entry.compressed_size);

      snprintf(output_filename, sizeof(output_filename), "%s/%s", 
          output_directory, entry.filename);
      if (!orange_decompress_to_file(entry.buffer, entry.compressed_size, (size_t)-1, output_filename))
        goto exit;

      FREE(entry.buffer);
      FREE(entry.filename);
    }
    else
    {
      goto exit;
    }
  } /* for() */

  success = true;

exit:
  return success;
}/*}}}*/

static bool orange_read_vise_data1(/*{{{*/
    FILE* input_file)
{
  bool success = false;
  char* str;
  int j;
  int key_value_pairs;
  unsigned unknown;

#if VERBOSE
  synce_trace("Offset: %08lx", ftell(input_file));
#endif

  unknown =
    orange_read32(input_file);
#if VERBOSE
  synce_trace("Unknown: %08x", unknown);
#endif

  for (j = 0; j < 2; j++)
  {
    str = orange_read_vise_string(input_file, 2);
#if VERBOSE
    synce_trace("'%s'", str);
#endif
    FREE(str);
  }

  unknown =
    orange_read_byte(input_file) |
    orange_read_byte(input_file) << 8;

#if VERBOSE
  synce_trace("Unknown: %04x", unknown);
#endif

  for (j = 0; j < 2; j++)
  {
    str = orange_read_vise_string(input_file, 2);
#if VERBOSE
    synce_trace("'%s'", str);
#endif
    FREE(str);
  }

  unknown =
    orange_read_byte(input_file) |
    orange_read_byte(input_file) << 8;
#if VERBOSE
  synce_trace("Unknown: %04x", unknown);
#endif

  unknown = orange_read32(input_file);
#if VERBOSE
  synce_trace("Unknown: %08x", unknown);
#endif

  /* skip block */
#if 0
  {
    char output_filename[256];
    char*buffer = malloc(unknown);

    fread(buffer, 1, unknown, input_file);

    snprintf(output_filename, sizeof(output_filename), "%s/unknown", 
        output_directory);
    if (!orange_decompress_to_file(buffer, unknown, output_filename))
      synce_trace("unknown data not compressed");
    free(buffer);
  }
#else
  fseek(input_file, unknown, SEEK_CUR);
#endif

  unknown = orange_read_byte(input_file);
#if VERBOSE
  synce_trace("Unknown: %02x", unknown);
#endif

  unknown = orange_read32(input_file);
#if VERBOSE
  synce_trace("Unknown: %08x", unknown);
#endif

  key_value_pairs = orange_read32(input_file);

  for (j = 0; j < key_value_pairs; j++)
  {
    char* key;
    char* value;

    key   = orange_read_vise_string(input_file, 2);
    value = orange_read_vise_string(input_file, 2);

#if VERBOSE
    synce_trace("'%s'='%s'", key, value);
#endif

    FREE(key);
    FREE(value);
  }

  /* skip block */
  fseek(input_file, 8, SEEK_CUR);

  success = true;

  return success;
}/*}}}*/

static bool orange_read_vise_data2(/*{{{*/
    FILE* input_file, 
    unsigned base_offset,
    const char* output_directory)
{
  int i;
  int file_count = 0;
  int object_count =
    orange_read_byte(input_file) |
    orange_read_byte(input_file) << 8 |
    orange_read_byte(input_file) << 16 |
    orange_read_byte(input_file) << 24;

  for (i = 0; i < object_count; i++)
  {
    int type;

#if VERBOSE
    synce_trace("Offset: %08lx", ftell(input_file));
#endif

    type =
      orange_read_byte(input_file) |
      orange_read_byte(input_file) << 8;

#if VERBOSE
    synce_trace("Type 0x%04x", type);
#endif

    switch (type)
    {
      case 0x02:
        {/*{{{*/
          char* filename;
          char* directory;
          size_t original_size;
          size_t compressed_size;
          unsigned offset;
          char* p;

          fseek(input_file, 0x65, SEEK_CUR);

          filename = orange_read_vise_string(input_file, 2);

          fseek(input_file, 0x4, SEEK_CUR);

          original_size =
            orange_read_byte(input_file) |
            orange_read_byte(input_file) << 8 |
            orange_read_byte(input_file) << 16 |
            orange_read_byte(input_file) << 24;

          compressed_size =
            orange_read_byte(input_file) |
            orange_read_byte(input_file) << 8 |
            orange_read_byte(input_file) << 16 |
            orange_read_byte(input_file) << 24;

          fseek(input_file, 0x4, SEEK_CUR);

          offset =
            orange_read_byte(input_file) |
            orange_read_byte(input_file) << 8 |
            orange_read_byte(input_file) << 16 |
            orange_read_byte(input_file) << 24;

          fseek(input_file, 0x25, SEEK_CUR);

          directory = orange_read_vise_string(input_file, 2);

          fseek(input_file, 0x28, SEEK_CUR);

          for (p = directory; *p != '\0'; p++)
            if (*p == '\\')
              *p = '/';

          synce_trace("offset %08x uncompressed size %08x compressed size %08x file '%s/%s' ", 
              offset, original_size, compressed_size, directory, filename);

          {
            uint8_t* buffer = malloc(compressed_size);
            if (buffer)
            {
              long old_offset = ftell(input_file);
              
              fseek(input_file, base_offset + offset, SEEK_SET);

              if (compressed_size == fread(buffer, 1, compressed_size, input_file))
              {
                char output_filename[256];

                /* create directory */
                snprintf(output_filename, sizeof(output_filename), "%s/%s",
                    output_directory, directory);
                orange_make_sure_directory_exists(output_filename);
                
                /* decompress file */
                snprintf(output_filename, sizeof(output_filename), "%s/%s/%s",
                    output_directory, directory, filename);

                if (compressed_size & 1)
                  synce_trace("File size not even!");

                orange_correct_compressed_vise_buffer(buffer, compressed_size);
                if (orange_decompress_to_file(buffer, compressed_size, original_size, output_filename))
                  file_count++;
              }

              fseek(input_file, old_offset, SEEK_SET);
           
              free(buffer);   
            }
            else
            {
              synce_error("Failed to allocate %i bytes", compressed_size);
            }
          }
          
          FREE(filename);
          FREE(directory);
        }/*}}}*/
        break;

      case 0x05:
        {/*{{{*/
          char* str;
          int unknown;
          int j;

          fseek(input_file, 0x66, SEEK_CUR);

#if 0
          unknown =
            orange_read_byte(input_file) |
            orange_read_byte(input_file) << 8;

          synce_trace("Unknown = %04x", unknown);
#endif

          str = orange_read_vise_string(input_file, 2);
#if VERBOSE
          synce_trace("'%s'", str);
#endif
          FREE(str);

          unknown = orange_read_byte(input_file);
          if (2 != unknown)
          {
            synce_trace("Unknown = %02x", unknown);
            abort();
          }

          for (j = 0; j < 3; j++)
          {
            str = orange_read_vise_string(input_file, 2);
#if VERBOSE
            synce_trace("'%s'", str);
#endif
            FREE(str);
          }

          fseek(input_file, 0x5, SEEK_CUR);
        }/*}}}*/
        break;

      case 0x10:
        {/*{{{*/
          char* key;
          char* value;
          int unknown;

          fseek(input_file, 0x66, SEEK_CUR);

          key   = orange_read_vise_string(input_file, 2);

          unknown = orange_read_byte(input_file);
          if (unknown)
          {
            synce_trace("Unknown = %02x", unknown);
            abort();
          }

          value = orange_read_vise_string(input_file, 2);

          fseek(input_file, 0x7, SEEK_CUR);

#if VERBOSE
          synce_trace("'%s'='%s'", key, value);
#endif

          FREE(key);
          FREE(value);
        }/*}}}*/
        break;

      case 0x11:
        {/*{{{*/
          int j;
          fseek(input_file, 0x65, SEEK_CUR);

          for (j = 0; j < 2; j++)
          {
            char* str = orange_read_vise_string(input_file, 2);
#if VERBOSE
            synce_trace("'%s'", str);
#endif
            FREE(str);
          }

          fseek(input_file, 0x5, SEEK_CUR);
        }/*}}}*/
        break;

      case 0x1f:
        fseek(input_file, 0x65, SEEK_CUR);
        break;

      case 0x24:
        {/*{{{*/
          int j;
          int string_count = 4;
          int unknown;

          fseek(input_file, 0x65, SEEK_CUR);

          unknown = 
            orange_read_byte(input_file) |
            orange_read_byte(input_file) << 8;

          switch (unknown)
          {
            case 1+2:
              string_count = 5;
              break;

            case 1+8:
              string_count = 6;
              break;

            case 1+4+8:
              string_count = 1;
              break;

            default:
              synce_trace("Unknown = %02x", unknown);
              abort();
          }

          fseek(input_file, 4, SEEK_CUR);

          for (j = 0; j < string_count; j++)
          {
            char* str = orange_read_vise_string(input_file, 2);
#if VERBOSE
            synce_trace("'%s'", str);
#endif
            FREE(str);
          }
        }/*}}}*/
        break;

      default:
        synce_trace("Unknown type: %04x", type);
        abort();
    }
  }

  synce_trace("Number of successfully extracted files: %i", file_count);

  return file_count > 0;
}/*}}}*/

bool orange_extract_vise(
    const char* input_filename, 
    const char* output_directory)
{
  bool success = false;
  FILE* input_file = fopen(input_filename, "r");
  unsigned base_offset;

#if DEBUG_ZLIB
  z_verbose = 1;
#endif

  if (!input_file)
    goto exit;

  fseek(input_file, -8, SEEK_END);

  if (orange_read_byte(input_file) != 'E' ||
      orange_read_byte(input_file) != 'S' ||
      orange_read_byte(input_file) != 'I' ||
      orange_read_byte(input_file) != 'V')
    goto exit;

  synce_trace("ESIV end signature found");

  base_offset = orange_read32(input_file);
  
  fseek(input_file, base_offset, SEEK_SET);

  if (orange_read_byte(input_file) != 'E' ||
      orange_read_byte(input_file) != 'S' ||
      orange_read_byte(input_file) != 'I' ||
      orange_read_byte(input_file) != 'V')
    goto exit;

  synce_trace("ESIV start signature found");

  /* skip unknown stuff */
  fseek(input_file, 0x3f, SEEK_CUR);

  orange_read_vise_files(input_file, output_directory);
  orange_read_vise_data1(input_file);
  orange_read_vise_files(input_file, output_directory);

#if VERBOSE
  synce_trace("Offset: %08lx", ftell(input_file));
#endif
  {
    int i;
    int setup_type_count =
      orange_read_byte(input_file) |
      orange_read_byte(input_file) << 8;

    for (i = 0; i < setup_type_count; i++)
    {
      char* str = orange_read_vise_string(input_file, 2);
#if VERBOSE
      synce_trace("Setyp type %i: %s", i, str);
#endif
      fseek(input_file, 0x55, SEEK_CUR);
      FREE(str);
    }
  }

#if VERBOSE
  synce_trace("Offset: %08lx", ftell(input_file));
#endif
  
  success = orange_read_vise_data2(input_file, base_offset, output_directory);

#if VERBOSE
  synce_trace("Offset: %08lx", ftell(input_file));
#endif

exit:
  FCLOSE(input_file);
  return success;
}

