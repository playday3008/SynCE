/*
 * Extract data from .rsrc section of PE file
 */
#define _BSD_SOURCE 1
#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif
#include "liborange_internal.h"

#include <stdlib.h>
#include "liborange_log.h"

#include "pe.h"

#define VERBOSE 0

typedef int8_t CHAR;

typedef struct _IMAGE_RESOURCE_DIRECTORY {
  DWORD Characteristics;
  DWORD TimeDateStamp;
  WORD MajorVersion;
  WORD MinorVersion;
  WORD NumberOfNamedEntries;
  WORD NumberOfIdEntries;
} IMAGE_RESOURCE_DIRECTORY,*PIMAGE_RESOURCE_DIRECTORY;

typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
  DWORD Name;
  DWORD OffsetToData;
} IMAGE_RESOURCE_DIRECTORY_ENTRY, *PIMAGE_RESOURCE_DIRECTORY_ENTRY;

typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
  DWORD OffsetToData;
  DWORD Size;
  DWORD CodePage;
  DWORD Reserved;
} IMAGE_RESOURCE_DATA_ENTRY,*PIMAGE_RESOURCE_DATA_ENTRY;

static void read_resource_directory(
    FILE* input, 
    IMAGE_RESOURCE_DIRECTORY* resourceDirectory)
{
  resourceDirectory->Characteristics = pe_read32(input);
  resourceDirectory->TimeDateStamp= pe_read32(input);
  resourceDirectory->MajorVersion = pe_read16(input);
  resourceDirectory->MinorVersion = pe_read16(input);
  resourceDirectory->NumberOfNamedEntries = pe_read16(input);
  resourceDirectory->NumberOfIdEntries = pe_read16(input);
}

static void read_resource_directory_entry(
    FILE* input,
    IMAGE_RESOURCE_DIRECTORY_ENTRY* directoryEntry)
{
  directoryEntry->Name = pe_read32(input);
  directoryEntry->OffsetToData = pe_read32(input);
}

static char* read_resource_directory_string(
    FILE* input)
{
  unsigned length = pe_read16(input);
  unsigned i;
  char* result = malloc(length+1);

#if VERBOSE
  synce_trace("String length: %i", length);
#endif
  for (i = 0; i < length; ++i)
  {
    result[i] = pe_read16(input);
  }
  result[i] = '\0';

  return result;
}

static void read_resource_data_entry(
    FILE* input,
    IMAGE_RESOURCE_DATA_ENTRY* dataEntry)
{
  dataEntry->OffsetToData = pe_read32(input);
  dataEntry->Size = pe_read32(input);
  dataEntry->CodePage = pe_read32(input);
  dataEntry->Reserved = pe_read32(input);
}

static bool extract_resource_data(
    FILE* input, 
    unsigned rsrcVirtualOffset,
    unsigned rsrcOffset, 
    unsigned dataOffset, 
    const char* prefix,
    const char* output_directory)
{
  char name[1024];
  IMAGE_RESOURCE_DATA_ENTRY data_entry;
  
#if VERBOSE
  synce_trace("Data entry offset: %08x", dataOffset); 
#endif

  fseek(input, rsrcOffset + dataOffset, SEEK_SET);

  read_resource_data_entry(input, &data_entry);

#if VERBOSE
  synce_trace("%08x %08x %08x %08x", 
      data_entry.OffsetToData,
      data_entry.Size,
      data_entry.CodePage,
      data_entry.Reserved);

  synce_trace("Data offset: %08x", 
      data_entry.OffsetToData - rsrcVirtualOffset + rsrcOffset);
#endif

  snprintf(name, sizeof(name), "%s-%08x", prefix, data_entry.CodePage);
#if VERBOSE
  synce_trace("Resource: %s", name);
#endif

  fseek(
      input, 
      data_entry.OffsetToData - rsrcVirtualOffset + rsrcOffset,
      SEEK_SET);
  if (!orange_copy(input, data_entry.Size, output_directory, name))
  {
    synce_error("Failed to copy %08x bytes to %s/%s", 
        data_entry.Size, output_directory, name);
    return false;
  }

  return true;
}

static void extract_resource_directory(
    FILE* input, 
    unsigned rsrcVirtualOffset,
    unsigned rsrcOffset, 
    unsigned directoryOffset, 
    const char* prefix,
    const char* output_directory)
{
  IMAGE_RESOURCE_DIRECTORY directory;
  IMAGE_RESOURCE_DIRECTORY_ENTRY* entries;
  unsigned entry_count;
  unsigned i;

  fseek(input, rsrcOffset + directoryOffset, SEEK_SET);
  read_resource_directory(input, &directory);

#if VERBOSE
  synce_trace("named entries: %i", directory.NumberOfNamedEntries);
  synce_trace("id entries: %i", directory.NumberOfIdEntries);
#endif

  entries = calloc(
      directory.NumberOfNamedEntries + directory.NumberOfIdEntries,
      sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY)
      );

  entry_count = directory.NumberOfNamedEntries + directory.NumberOfIdEntries;

  /* read all data first */
  for (i = 0; i < entry_count; ++i)
  {
    read_resource_directory_entry(input, &entries[i]);
  }

  /* synce_trace("End of resource directory: %08x", ftell(input) - offset); */

  /* now we can can functions that fseek to other places */
  for (i = 0; i < entry_count; ++i)
  {
    char name[1024];
    
    if (entries[i].Name & 0x80000000)
    {
      char *entry_name;
      fseek(input, rsrcOffset + (entries[i].Name & ~0x80000000), SEEK_SET);
      entry_name = read_resource_directory_string(input);
      
      snprintf(name, sizeof(name), "%s-%s", prefix, entry_name);
    }
    else
    {
      snprintf(name, sizeof(name), "%s-%08x", prefix, entries[i].Name);
    }
    
    /*synce_trace("Orange name: %s", name);*/

    if (entries[i].OffsetToData & 0x80000000)
    {
#if VERBOSE
      synce_trace("Subdirectory offset: %08x", entries[i].OffsetToData & ~0x80000000);
#endif
      extract_resource_directory(
          input, 
          rsrcVirtualOffset,
          rsrcOffset, 
          entries[i].OffsetToData & ~0x80000000, 
          name,
          output_directory);
    }
    else
    {
      if (!extract_resource_data(
          input,
          rsrcVirtualOffset,
          rsrcOffset,
          entries[i].OffsetToData,
          name,
          output_directory))
        break;
    }
  }

  free(entries);
}

/* Will probably succeed for all PE files */
bool orange_extract_rsrc(
    const char* input_filename,
    const char* output_directory)
{
  bool success = false;
  FILE *input = fopen(input_filename, "r");
  uint32_t file_offset;
  uint32_t virtual_offset;
  uint32_t piggy_back_offset;
 
  if (!input)
    goto exit;

  if (!pe_size(input, &piggy_back_offset))
    goto exit;

  fseek(input, 0, SEEK_END);
  if (piggy_back_offset != (unsigned)ftell(input))
  {
    synce_debug("There are %08x bytes piggy-backed at offset %08x in %s.",
        ftell(input) - piggy_back_offset, 
        piggy_back_offset,
        input_filename);
  }

  if (!pe_rsrc_offset(input, &file_offset, &virtual_offset))
    goto exit;

#if VERBOSE
  synce_trace("File offset: %08x  Virtual offset:  %08x", 
      file_offset, virtual_offset);
#endif

  if (!orange_make_sure_directory_exists(output_directory))
    goto exit;

  extract_resource_directory(
      input, 
      virtual_offset, 
      file_offset, 
      0, 
      "rsrc",
      output_directory);

  success = true;

exit:
  FCLOSE(input)
  return success;
}
