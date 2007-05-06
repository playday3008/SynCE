#define _BSD_SOURCE 1
#include "liborange_internal.h"
#include "liborange.h"
#include "pe.h"
#include "liborange_log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <ctype.h>

#define VERBOSE 0

#define SIGNATURE       "InstallShield"
#define SIGNATURE_SIZE  13

#define BUFFER_SIZE 0x100

#define FLAG_OBFUSCATED   2

static void create_key(const char* filename, uint8_t *key)
{
  unsigned i;
  uint8_t seed[4] = { 0x13, 0x35, 0x86, 0x07 };

  for (i = 0; i < strlen(filename); i++)
  {
    key[i] = filename[i] ^ seed[i % sizeof(seed)];
  }
}

static uint8_t decode_byte(uint8_t byte, uint8_t key)
{
  return ~( (byte >> 4 | byte << 4) ^ key );  
}

bool orange_extract_installshield_sfx(
    const char* input_filename,
    const char* output_directory)
{
  bool success = false;
  uint32_t offset;
  FILE* input_file = NULL;
  int error;
  char signature[SIGNATURE_SIZE+1];
  unsigned count; 
  unsigned flags;
  unsigned bytes;
  unsigned size;
  unsigned i;

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

  fseek(input_file, 1, SEEK_CUR);

  count = orange_read32(input_file);
  
  fseek(input_file, 28, SEEK_CUR);

  for (i = 0; i < count; i++)
  {
    char output_filename[0x200];
    FILE* output_file = NULL;
    size_t bytes_to_transfer = 0;
    size_t bytes_left = 0;
    uint8_t buffer[BUFFER_SIZE];
    char filename[0x100];
    uint8_t key[0x100];
    unsigned key_index = 0;
    size_t key_length;
   
    /* Read file metadata */
    
    fread(filename, 1, sizeof(filename), input_file);
    filename[sizeof(filename) - 1] = '\0';

    fseek(input_file, 4, SEEK_CUR);

    flags = orange_read32(input_file);

    fseek(input_file, 4, SEEK_CUR);

    size = orange_read32(input_file);

    synce_trace("File: name=%s, flags=%i, size=%i", filename, flags, size);

    fseek(input_file, 0x28, SEEK_CUR);

    /* Create output file */

    snprintf(output_filename, sizeof(output_filename), "%s/%s", output_directory, filename);
    output_file = fopen(output_filename, "w");
    if (!output_file)
      goto exit;

    /* Create obfuscation key */

    create_key(filename, key);
    key_length = strlen(filename);

    /* Decode file */
    
    for (bytes_left = size; bytes_left; bytes_left -= bytes_to_transfer)
    {
      size_t j;
      size_t bytes_written = 0;
      bytes_to_transfer = MIN(BUFFER_SIZE, bytes_left);

      bytes = fread(buffer, 1, bytes_to_transfer, input_file);
      if (bytes != bytes_to_transfer)
      {
        synce_error("Failed to read from file");
        goto exit;
      }

      if (flags & FLAG_OBFUSCATED)
      {
        for (j = 0; j < bytes_to_transfer; j++, key_index++)
        {
          buffer[j] = decode_byte(buffer[j], key[key_index % key_length]);
        }
      }

      bytes_written = fwrite(buffer, 1, bytes_to_transfer, output_file);
      if (bytes_written != bytes_to_transfer)
      {
        synce_error("Failed to write to file");
        goto exit;
      }
    }

    fclose(output_file);
  }
  
  success = true;
  
exit:
  if (input_file)
    fclose(input_file);
  return success;
}

typedef enum
{
  STRING,
  INTEGER 
} DataType;

typedef int (*ValidatorFunc)(int c);

static char* read_asciiz(FILE* input_file, ValidatorFunc validator)
{
  bool success = false;
  unsigned max_size = 16;
  char* result = (char*)malloc(max_size);
  char c;
  unsigned size = 0;

  for (;;)
  {
    if (sizeof(c) != fread(&c, 1, sizeof(c), input_file))
    {
#if VERBOSE
      synce_trace("End of file, size = %i", size);
#endif
      goto exit;
    }

    result[size] = c;
    
    if (c == '\0')
      break;

    if (!validator(c))
    {
#if VERBOSE
      synce_trace("invalid char: 0x%02x", (int)(unsigned char)c);
#endif
      goto exit;
    }

    size++;

    if (size > max_size)
    {
      max_size *= 2;
      result = (char*)realloc(result, max_size);
    }
  }

  success = true;
  
exit:
  if (success)
    return result;
  free(result);
  return NULL;
}

enum
{
  STRING_FILENAME,
  STRING_PATH,
  STRING_UNKNOWN,
  STRING_COUNT
};

enum
{
  INTEGER_SIZE,
  INTEGER_COUNT
};

#if 0
static bool copy(
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
  if (output_file)
    fclose(output_file);
  return success;
}
#endif

bool orange_extract_installshield_sfx2(
    const char* input_filename,
    const char* output_directory)
{
  bool success = false;
  size_t offset;
  FILE* input_file = NULL;
  int error;
  long file_size = 0;

#if VERBOSE
  synce_trace("here");
#endif

  input_file = fopen(input_filename, "r");

  if (!pe_size(input_file, &offset))
  {
    synce_trace("pe_size failed");
    goto exit;
  }

#if VERBOSE
  synce_trace("offset = %08x", offset);
#endif

  file_size = orange_fsize(input_file);

  if ((unsigned)file_size == offset)
  {
    /* nothing piggybacked */
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

  while (ftell(input_file) < file_size)
  {
    int i;

    char* strings[STRING_COUNT];
    unsigned integers[INTEGER_COUNT];

    for (i = 0; i < STRING_COUNT; i++)
    {
      strings[i] = read_asciiz(input_file, isprint);
      if (!strings[i])
        goto exit;
#if VERBOSE
      synce_trace("strings[%i] = '%s'", i, strings[i]);
#endif
    }

    for (i = 0; i < INTEGER_COUNT; i++)
    {
      char* str = read_asciiz(input_file, isdigit);
      if (!str)
        goto exit;

#if VERBOSE
      synce_trace("integers[%i] = '%s'", i, str);
#endif
      integers[i] = atoi(str);
      free(str);
    }
    
    if (integers[INTEGER_SIZE] == 0)
    {
      synce_trace("size is 0");
      goto exit; 
    }

    synce_trace("Extracting %s (%i bytes)", 
        strings[STRING_FILENAME], 
        integers[INTEGER_SIZE]);

    /* error = fseek(input_file, integers[INTEGER_SIZE], SEEK_CUR); */
    if (!orange_copy(input_file, integers[INTEGER_SIZE], output_directory, strings[STRING_FILENAME]))
    {
      synce_trace("failed to write file: %s", strings[STRING_FILENAME]);
      goto exit; 
    }

    for (i = 0; i < STRING_COUNT; i++)
      free(strings[i]);
  }

  success = true;

exit:
  return success;
}


