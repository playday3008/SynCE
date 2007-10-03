/* $Id$ */
#define _BSD_SOURCE 1
#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif
#include "liborange_internal.h"
#include <ctype.h>
#include "liborange_log.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define VERBOSE 1
#define ZLIB_DEBUG 0
#define ZLIB_HACK 0

/*
   Installer VISE - http://www.mindvision.com
 */

typedef struct
{
  char* filename;
  unsigned compressed_size;
  uint8_t* buffer;
} FileEntry;

#ifdef ZLIB_DEBUG
extern int z_verbose;
#endif

#if ZLIB_HACK
extern int z_visehack;
#endif

#define OUTPUT_BUFFER_SIZE 0x8000

static bool orange_decompress_to_file(uint8_t* input_buffer, size_t input_size, size_t expected_output_size, const char* output_filename)/*{{{*/
{
  bool success = false;
  Byte* output_buffer = malloc(OUTPUT_BUFFER_SIZE);
  bool decompress = true;
  uint8_t* fixed_buffer = NULL;
#if 0
  unsigned last_fix_offset = (unsigned)-1;
#endif

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

      synce_trace("Offset = %08x", input_size - stream.avail_in);

      error = inflate(&stream, Z_SYNC_FLUSH/*Z_NO_FLUSH*/);

      bytes_to_write = OUTPUT_BUFFER_SIZE - stream.avail_out;

      if (bytes_to_write != fwrite(output_buffer, 1, bytes_to_write, output))
      {
        synce_error("Failed to write %i bytes to output file '%s'", 
            bytes_to_write, output_filename);
        goto exit;
      }

      if (error < Z_OK)
      {
        unsigned offset;
        offset = input_size - stream.avail_in;
        synce_error("inflate failed with error %i '%s' offset=%08x, total_out=%08x", 
            error, stream.msg, offset, stream.total_out);

#if 0
        bool remove_byte = false;
        int index = 0;

        synce_trace("offset=%08x: %02x %02x %02x %02x (%02x %02x) (%02x %02x) [%02x] %02x %02x %02x %02x",
            offset,
            input_buffer[offset-8],
            input_buffer[offset-7],
            input_buffer[offset-6],
            input_buffer[offset-5],
            input_buffer[offset-4],
            input_buffer[offset-3],
            input_buffer[offset-2],
            input_buffer[offset-1],
            input_buffer[offset-0],
            input_buffer[offset+1],
            input_buffer[offset+2],
            input_buffer[offset+3],
            input_buffer[offset+4]
            );

        /*
           Sometimes the decompression fails because of "incorrect stored block
           lengths". It seems like there is a zero byte to much!
         */

        if (strcmp(stream.msg, "invalid stored block lengths") == 0)
        {
          if (0 == input_buffer[offset-4] &&
              input_buffer[offset-1] == (~input_buffer[offset-3] & 0xff) &&
              input_buffer[offset-2] == (~input_buffer[offset-0] & 0xff))
          {
            remove_byte = true;
            index = -4;
            
#if 0
            synce_warning("Removing zero byte and trying again...");

            input_size--;
            memmove(input_buffer + offset - 4, input_buffer + offset - 3, input_size - offset + 3);

            last_fix_offset = offset;
            decompress = true;
            goto exit;
#endif
          }
          else if (last_fix_offset != offset)
          {
#if 0
            uint8_t* old_fixed_buffer = fixed_buffer;
            /* oh, a spare byte, thanks! */
            synce_warning("Inserting stored block of size 1...");

            input_size += 5;
            fixed_buffer = malloc(input_size);

            memcpy(fixed_buffer, input_buffer, offset-5);
            memcpy(fixed_buffer + offset + 1, input_buffer + offset - 4, input_size - offset);

            fixed_buffer[offset-5] = 0;   /* type for stored block */
            fixed_buffer[offset-4] = 1;   /* low size byte */
            fixed_buffer[offset-3] = 0;   /* high size byte */
            fixed_buffer[offset-2] = ~fixed_buffer[offset-4] & 0xff;
            fixed_buffer[offset-1] = ~fixed_buffer[offset-3] & 0xff;
            fixed_buffer[offset  ] = input_buffer[offset-5];    /* byte value */

            FREE(old_fixed_buffer);
            input_buffer = fixed_buffer;

            synce_trace("offset=%08x: %02x %02x (%02x %02x) (%02x %02x) [%02x] %02x %02x %02x %02x",
                offset,
                input_buffer[offset-6],
                input_buffer[offset-5],
                input_buffer[offset-4],
                input_buffer[offset-3],
                input_buffer[offset-2],
                input_buffer[offset-1],
                input_buffer[offset-0],
                input_buffer[offset+1],
                input_buffer[offset+2],
                input_buffer[offset+3],
                input_buffer[offset+4]
                );
#else
            remove_byte = true;
            index = -5;
#endif
          }
        }
        else if (strcmp(stream.msg, "invalid block type") == 0)
        {
          remove_byte = true;
          index = -1;
        }
        else if (strcmp(stream.msg, "oversubscribed dynamic bit lengths tree") == 0)
        {
          remove_byte = true;
          index = -7;
        }

        if (remove_byte)
        {
          synce_warning("Removing byte %02x and trying again...", input_buffer[offset+index]);

          input_size--;
          memmove(input_buffer + offset + index, input_buffer + offset + index + 1, input_size - offset - index - 1);

          last_fix_offset = offset;

          decompress = true;
          goto exit;
        }
#endif

        goto exit;
      }
    }
  
    if (expected_output_size == (size_t)-1)
      success = (input_size == stream.total_in) || (input_size == stream.total_in+1);
    else
      success = stream.total_out == expected_output_size;

exit:
    inflateEnd(&stream);
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

                if (compressed_size & 1)
                  synce_trace("File size not even!");

                orange_correct_compressed_vise_buffer(buffer, compressed_size);

                /* save compressed data for debugging */
                snprintf(output_filename, sizeof(output_filename), "%s/%s.compressed",
                    directory, filename);
                orange_write(buffer, compressed_size, output_directory, output_filename);
                
                /* decompress file */
                snprintf(output_filename, sizeof(output_filename), "%s/%s/%s",
                    output_directory, directory, filename);

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
          int string_count = -1;
          int values[6];

          fseek(input_file, 0x65, SEEK_CUR);

          for (j = 0; j < 6; j++)
            values[j] = orange_read_byte(input_file);

          switch (values[0])
          {
            case 1+2:
              switch (values[4])
              {
                case 0:
                  string_count = 5;
                  break;
                case 1:
                  string_count = 1;
                  break;
                default:
                  synce_trace("values[4] = %02x", values[4]);
                  abort();
              }
              break;

            case 2+4:
              string_count = 3;
              break;

            case 1+8:
              string_count = 6;
              break;

            case 1+4+8:
              string_count = 1;
              break;

            case 16:
              /* this is not correct */
              string_count = 1;
              break;

            default:
              synce_trace("values[0] = %02x", values[0]);
              abort();
          }

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

      case 0x25:
        fseek(input_file, 0x67, SEEK_CUR);
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

#if ZLIB_DEBUG
  z_verbose = 1;
#endif
#if ZLIB_HACK
  z_visehack = 1;
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
#if ZLIB_HACK
  z_visehack = 0;
#endif
  FCLOSE(input_file);
  return success;
}

