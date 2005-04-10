#define _BSD_SOURCE 1
#include "liborange_internal.h"
#include "liborange.h"
#include <synce_log.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

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
  size_t offset;
  FILE* input_file = NULL;
  int error;
  char signature[SIGNATURE_SIZE+1];
  unsigned count; 
  unsigned flags;
  unsigned bytes;
  unsigned size;
  unsigned i;

  synce_trace("here");

  /* hard-coded offset for now... should really be calculated from PE header */
  offset = 0x37000;

  input_file = fopen(input_filename, "r");

  error = fseek(input_file, offset, SEEK_SET);
  if (error)
  {
    synce_trace("fseek failed");
    goto exit;
  }

  bytes = fread(signature, 1, SIGNATURE_SIZE, input_file);
  if (bytes != SIGNATURE_SIZE)
  {
    synce_trace("fread failed");
    goto exit;
  }

  signature[SIGNATURE_SIZE] = '\0';

  if (strcmp(signature, SIGNATURE) != 0)
  {
    synce_trace("signature comparison failed");
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
        break;
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
