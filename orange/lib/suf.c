/* $Id$ */
#define _BSD_SOURCE 1
#include "liborange_internal.h"
#if WITH_LIBDYNAMITE
#include <libdynamite.h>
#endif
#include "liborange_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG 0

#define STUB_SIZE_5      0x8000     /* for MazingoSetup.exe (SetupFactory 5) */
#define STUB_SIZE_6     0x12000     /* for suf60ev.exe      (SetupFactory 6) */
#define SIGNATURE_SIZE        8

#define FILENAME_SIZE_5   16
#define FILENAME_SIZE_6   0x104

#define DATA_FILE     "irsetup.dat"

const uint8_t SIGNATURE[SIGNATURE_SIZE] = {0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7};

typedef struct _Cookie
{
  FILE* input_file;
  FILE* output_file;
} Cookie;

#if WITH_LIBDYNAMITE
static size_t reader(void* buffer, size_t size, void* cookie)
{
  return fread(buffer, 1, size, ((Cookie*)cookie)->input_file);
}

static size_t writer(void* buffer, size_t size, void* cookie)
{
  return fwrite(buffer, 1, size, ((Cookie*)cookie)->output_file);
}
#endif

static int orange_get_setup_factory_version(FILE* input_file)/*{{{*/
{
  int version = 0;
  char signature[SIGNATURE_SIZE];
  
  fseek(input_file, STUB_SIZE_5, SEEK_SET);
  if (SIGNATURE_SIZE != fread(signature, 1, SIGNATURE_SIZE, input_file))
    goto exit;

  if (memcmp(signature, SIGNATURE, SIGNATURE_SIZE) == 0)
  {
    version = 5;
  }
  else
  {
    fseek(input_file, STUB_SIZE_6, SEEK_SET);
    if (SIGNATURE_SIZE != fread(signature, 1, SIGNATURE_SIZE, input_file))
      goto exit;
    
    if (memcmp(signature, SIGNATURE, SIGNATURE_SIZE) == 0)
      version = 6;
    else
      goto exit;
  }

exit:
  return version;
}/*}}}*/

static bool orange_explode(/*{{{*/
    FILE* input_file,
    const char* output_directory,
    const char* filename,
    size_t size)
{
  bool success = false;
  long next_file_offset = ftell(input_file) + size;

#if WITH_LIBDYNAMITE
  DynamiteResult result;
  Cookie cookie;
  char output_filename[256];
  
  if (!orange_make_sure_directory_exists(output_directory))
  {
    synce_error("Failed to create directory: '%s'", output_directory);
    goto exit;
  }

  snprintf(output_filename, sizeof(output_filename), "%s/%s", 
      output_directory, filename);

  cookie.input_file  = input_file;
  cookie.output_file = fopen(output_filename, "w");
  
  if (!cookie.output_file)
  {
    synce_error("Failed to open file for writing: '%s'", output_filename);
    goto exit;
  }

  result = dynamite_explode(reader, writer, &cookie);

  if (DYNAMITE_SUCCESS != result)
  {
    synce_error("Decompression of file '%s' failed with error code %i", 
        filename, result);
    FCLOSE(cookie.output_file);
    abort();
    goto exit;
  }

  success = true;

exit:
  FCLOSE(cookie.output_file);
#if DEBUG
  synce_trace("Current offset: %08x", ftell(input_file));
#endif
#endif

  fseek(input_file, next_file_offset, SEEK_SET);
  return success;
}/*}}}*/

static bool orange_extract_setup_factory_1(/*{{{*/
    FILE* input_file,
    const char* output_directory,
    int version)
{
  bool success = false;
  unsigned i;
  uint32_t count; 
  size_t filename_size = 0;

  switch (version)
  {
    case 5:
      filename_size = FILENAME_SIZE_5;
      break;

    case 6:
      filename_size = FILENAME_SIZE_6;
      break;

    default:
      synce_error("Unsupported Setup Factory version");
      goto exit;
  }

  if (!orange_make_sure_directory_exists(output_directory))
    goto exit;

  fread(&count, 1, sizeof(count), input_file);
  LETOH32(count);

  for (i = 0; i < count; i++)
  {
    uint32_t size = 0;
    uint32_t crc32 = 0;
    char filename[FILENAME_SIZE_6];

    memset(filename, 0, sizeof(filename));

    fread(filename, 1, filename_size, input_file);
    fread(&size,    1, sizeof(size),  input_file);
    fread(&crc32,   1, sizeof(crc32), input_file);

    LETOH32(size);
    LETOH32(crc32);

    if (!orange_explode(input_file, output_directory, filename, size))
      goto exit;
  }

  success = true;

exit:
  return success;
}/*}}}*/

static uint16_t orange_read_uint16(uint8_t**p)/*{{{*/
{
   uint16_t result = LETOH16(*(uint16_t*)*p);
   *p += sizeof(uint16_t);
   return result;
}/*}}}*/

static uint32_t orange_read_uint32(uint8_t**p)/*{{{*/
{
   uint32_t result = LETOH32(*(uint32_t*)*p);
   *p += sizeof(uint32_t);
   return result;
}/*}}}*/

static char *orange_strndup (const char *s, size_t n)/*{{{*/
{
  char *r;

  if (!s)
    return NULL;

  if (strlen (s) < n)
    n = strlen (s);

  r = malloc (n + 1);
  memcpy (r, s, n);
  r[n] = '\0';
  return r;
}/*}}}*/

static char* orange_read_string1(uint8_t**p)/*{{{*/
{
  char* result;
  size_t length = **p;
  (*p)++;
  result = orange_strndup((const char*)*p, length);
  *p += length;
  return result;
}/*}}}*/

static char* orange_read_string2(uint8_t**p)/*{{{*/
{
  char* result;
  size_t length = orange_read_uint16(p);
  result = orange_strndup((const char*)*p, length);
  *p += length;
  return result;
}/*}}}*/

static bool orange_extract_setup_factory_2(/*{{{*/
    FILE* input_file,
    const char* output_directory,
    int version)
{
  bool success = false;
  char data_file_name[256];
  FILE* data_file = NULL;
  size_t data_size;
  uint8_t* data = NULL;
  uint8_t* p = NULL;
  unsigned count;
  unsigned i;
  
  snprintf(data_file_name, sizeof(data_file_name), "%s/%s", output_directory, DATA_FILE);

  data_file = fopen(data_file_name, "r");
  if (!data_file)
  {
    synce_error("Failed to open file for reading: '%s'", data_file_name);
    goto exit;
  }

  data_size = FSIZE(data_file);
  data = malloc(data_size);

  if (!data)
  {
    synce_error("Failed to allocate %u bytes", data_size);
    goto exit;
  }

  if (data_size != fread(data, 1, data_size, data_file))
  {
    synce_error("Failed to read %u bytes from file '%s'", data_size, data_file_name);
    goto exit;
  }

  p = data;

  count = orange_read_uint16(&p);

#if DEBUG
  synce_trace("%u files?", count);
#endif

  {
    unsigned num;
    num = orange_read_uint16(&p);   /* skip 0xffff */
#if DEBUG
    synce_trace("%04x", num);
#endif
    num = orange_read_uint16(&p);   /* skip 0x0003 */
#if DEBUG
    synce_trace("%04x", num);
#endif
  }

  {
    char* classname = orange_read_string2(&p);
#if DEBUG
    synce_trace("Class name: '%s'", classname);
#endif
    FREE(classname);
  }

  for (i = 0; i < count; i++)
  {
    char* str;
    unsigned num;
    size_t size;
    char* basename = NULL;
    char* directory = NULL;
    char output_directory2[256];
    bool is_compressed;
    
    if (6 == version)
    {
      num = orange_read_uint32(&p);
#if DEBUG
      synce_trace("Unknown               : 0x%08x (%i)", num, num);
      if (num != 1)
        abort();
#endif
    }

    str = orange_read_string1(&p);
#if DEBUG
    synce_trace("Full source path      : '%s'", str);
#endif
    FREE(str);

    basename = orange_read_string1(&p);
#if DEBUG
    synce_trace("Basename              : '%s'", basename);
#endif
    FREE(str);

    str = orange_read_string1(&p);
#if DEBUG
    synce_trace("Source directory      : '%s'", str);
#endif
    FREE(str);

    str = orange_read_string1(&p);
#if DEBUG
    synce_trace("Suffix                : '%s'", str);
#endif
    FREE(str);

    p++;

    num = orange_read_uint32(&p);
#if DEBUG
    synce_trace("Decompressed file size: 0x%08x (%i)", num, num);
#endif

    p += 0x26;

    /* typically starts with "%AppDir%" */
    directory = orange_read_string1(&p);
#if DEBUG
    synce_trace("Destination directory : '%s'", directory);
#endif
    FREE(str);

    p += 0x5;

    str = orange_read_string1(&p);
#if DEBUG
    synce_trace("String                : '%s'", str);
#endif
    FREE(str);

    p += 0x9;

    /* typically "None" */
    str = orange_read_string1(&p);
#if DEBUG
    synce_trace("String                : '%s'", str);
#endif
    FREE(str);
    
    p += 0x5;

    is_compressed = *p++;
#if DEBUG
    synce_trace("Is compressed?        : %i", is_compressed);
#endif

    switch (version)
    {
      case 5:
        p += 0x11;
        break;

      case 6:
        p += 0x8;

        str = orange_read_string1(&p);
#if DEBUG
        synce_trace("String                : '%s'", str);
#endif
        FREE(str);

        p += 0x2;
        break;
    }

    size = orange_read_uint32(&p);
#if DEBUG
    synce_trace("Compressed file size  : 0x%08x (%i)", size, size);
#endif
    
    p += 0x2b;

    snprintf(output_directory2, sizeof(output_directory2), "%s/%s", 
        output_directory, directory);
    FREE(directory);

    if (is_compressed)
    {
      if (!orange_explode(input_file, output_directory2, basename, size))
      {
        FREE(basename);
        goto exit;
      }
    }
    else
    {
      uint8_t* data = malloc(size);
      if (!data)
      {
        synce_error("Failed to allocate %u bytes", data_size);
        goto exit;
      }

      if (size != fread(data, 1, size, input_file))
      {
        synce_error("Failed to read %u bytes from inout file", size);
        goto exit;
      }

      if (!orange_write(data, size, output_directory2, basename))
      {
        synce_error("Failed to write %u bytes to file '%s/%s'", size, output_directory2, basename);
        goto exit;
      }

      FREE(data);
    }

    FREE(basename);
  }

  success = true;

exit:
  FREE(data);
  FCLOSE(data_file);
  return success;
}/*}}}*/

bool orange_extract_setup_factory(
    const char* input_filename, 
    const char* output_directory)
{
  bool success = false;
  int version;
  FILE* input_file = fopen(input_filename, "r");

  if (!input_file)
    goto exit;

  version = orange_get_setup_factory_version(input_file);
  if (!version)
  {
#if 0
    synce_error("Unable to detect Setup Factory version");
#endif
    goto exit;
  }

  /*
     First extract the real setup program and its data files
   */

  if (!orange_extract_setup_factory_1(input_file, output_directory, version))
    goto exit;

#if DEBUG
  synce_trace("Part two offset: 0x%08x", ftell(input_file));
#endif

  /*
     Next read the irsetup.dat file to be able to get the installable data
   */

  if (!orange_extract_setup_factory_2(input_file, output_directory, version))
    goto exit;

  success = true;

exit:
  FCLOSE(input_file);
  return success;
}
    

