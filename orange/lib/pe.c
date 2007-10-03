#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif
#include "pe.h"
#include "liborange_internal.h"
#include "liborange_log.h"
#include <stdlib.h>
#include <string.h>

#define IMAGE_DOS_SIGNATURE   0x5a4d
#define IMAGE_NT_SIGNATURE    0x00004550

typedef struct _IMAGE_FILE_HEADER {
  WORD    Machine;         
  WORD    NumberOfSections;
  DWORD   TimeDateStamp;
  DWORD   PointerToSymbolTable;           
  DWORD   NumberOfSymbols; 
  WORD    SizeOfOptionalHeader;
  WORD    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct _IMAGE_DATA_DIRECTORY {
  DWORD   VirtualAddress;     /* RVA of the data */
  DWORD   Size;               /* Size of the data */
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_OPTIONAL_HEADER {
  WORD Magic;
  BYTE MajorLinkerVersion;
  BYTE MinorLinkerVersion;
  DWORD SizeOfCode;
  DWORD SizeOfInitializedData;
  DWORD SizeOfUninitializedData;
  DWORD AddressOfEntryPoint;
  DWORD BaseOfCode;
  DWORD BaseOfData;
  DWORD ImageBase;
  DWORD SectionAlignment;
  DWORD FileAlignment;
  WORD MajorOperatingSystemVersion;
  WORD MinorOperatingSystemVersion;
  WORD MajorImageVersion;
  WORD MinorImageVersion;
  WORD MajorSubsystemVersion;
  WORD MinorSubsystemVersion;
  DWORD Win32VersionValue;
  DWORD SizeOfImage;
  DWORD SizeOfHeaders;
  DWORD CheckSum;
  WORD Subsystem;
  WORD DllCharacteristics;
  DWORD SizeOfStackReserve;
  DWORD SizeOfStackCommit;
  DWORD SizeOfHeapReserve;
  DWORD SizeOfHeapCommit;
  DWORD LoaderFlags;
  DWORD NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER, *PIMAGE_OPTIONAL_HEADER;

#define IMAGE_SIZEOF_SHORT_NAME 8

typedef struct _IMAGE_SECTION_HEADER {
  BYTE Name[IMAGE_SIZEOF_SHORT_NAME];
  union {
    DWORD PhysicalAddress;
    DWORD VirtualSize;
  } Misc;
  DWORD VirtualAddress;
  DWORD SizeOfRawData;
  DWORD PointerToRawData;
  DWORD PointerToRelocations;
  DWORD PointerToLinenumbers;
  WORD NumberOfRelocations;
  WORD NumberOfLinenumbers;
  DWORD Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define SIZEOF_IMAGE_SECTION_HEADER 40

uint16_t pe_read16(FILE *input)
{
  return 
    orange_read_byte(input) |
    (orange_read_byte(input) << 8);
}

uint32_t pe_read32(FILE *input)
{
  return 
    orange_read_byte(input) |
    (orange_read_byte(input) << 8) |
    (orange_read_byte(input) << 16) |
    (orange_read_byte(input) << 24);
}

static bool pe_nt_headers_offset(FILE *input, uint32_t* result)
{
  uint16_t e_lfanew;
    
  fseek(input, 0, SEEK_SET);
  
  if (pe_read16(input) != IMAGE_DOS_SIGNATURE)
    return false;

  /* Skip things we don't care about */
  fseek(input, 0x3a, SEEK_CUR);
  
  e_lfanew = pe_read16(input);

  *result = e_lfanew;
  return true;
}

static void read_file_header(FILE* input, IMAGE_FILE_HEADER* fileHeader)
{
  fileHeader->Machine              = pe_read16(input);
  fileHeader->NumberOfSections     = pe_read16(input);
  fileHeader->TimeDateStamp        = pe_read32(input);
  fileHeader->PointerToSymbolTable = pe_read32(input);
  fileHeader->NumberOfSymbols      = pe_read32(input); 
  fileHeader->SizeOfOptionalHeader = pe_read16(input);
  fileHeader->Characteristics      = pe_read16(input);
}

static void read_optional_header(FILE* input, IMAGE_OPTIONAL_HEADER* optionalHeader, size_t size)
{
  int i;
  long start = ftell(input);
  int count;

  optionalHeader->Magic = pe_read16(input);
  optionalHeader->MajorLinkerVersion = orange_read_byte(input);
  optionalHeader->MinorLinkerVersion = orange_read_byte(input);
  optionalHeader->SizeOfCode = pe_read32(input);
  optionalHeader->SizeOfInitializedData = pe_read32(input);
  optionalHeader->SizeOfUninitializedData = pe_read32(input);
  optionalHeader->AddressOfEntryPoint = pe_read32(input);
  optionalHeader->BaseOfCode = pe_read32(input);
  optionalHeader->BaseOfData = pe_read32(input);
  optionalHeader->ImageBase = pe_read32(input);
  optionalHeader->SectionAlignment = pe_read32(input);
  optionalHeader->FileAlignment = pe_read32(input);
  optionalHeader->MajorOperatingSystemVersion = pe_read16(input);
  optionalHeader->MinorOperatingSystemVersion = pe_read16(input);
  optionalHeader->MajorImageVersion = pe_read16(input);
  optionalHeader->MinorImageVersion = pe_read16(input);
  optionalHeader->MajorSubsystemVersion = pe_read16(input);
  optionalHeader->MinorSubsystemVersion = pe_read16(input);
  optionalHeader->Win32VersionValue = pe_read32(input);
  optionalHeader->SizeOfImage = pe_read32(input);
  optionalHeader->SizeOfHeaders = pe_read32(input);
  optionalHeader->CheckSum = pe_read32(input);
  optionalHeader->Subsystem = pe_read16(input);
  optionalHeader->DllCharacteristics = pe_read16(input);
  optionalHeader->SizeOfStackReserve = pe_read32(input);
  optionalHeader->SizeOfStackCommit = pe_read32(input);
  optionalHeader->SizeOfHeapReserve = pe_read32(input);
  optionalHeader->SizeOfHeapCommit = pe_read32(input);
  optionalHeader->LoaderFlags = pe_read32(input);
  optionalHeader->NumberOfRvaAndSizes = pe_read32(input);

  /* Calculate number of data directory entries */
  count = (size - (ftell(input) - start)) / (2 * sizeof(uint32_t));

  /*synce_debug("Data directory entry count: %i", count);*/

  for (i = 0; i < count; i++)
  {
    optionalHeader->DataDirectory[i].VirtualAddress = pe_read32(input);
    optionalHeader->DataDirectory[i].Size = pe_read32(input);
  }
}

static void read_section_header(FILE* input, IMAGE_SECTION_HEADER* sectionHeader)
{
  fread(sectionHeader->Name, 1, IMAGE_SIZEOF_SHORT_NAME, input);
  sectionHeader->Misc.VirtualSize = pe_read32(input);
  sectionHeader->VirtualAddress = pe_read32(input);
  sectionHeader->SizeOfRawData = pe_read32(input);
  sectionHeader->PointerToRawData = pe_read32(input);
  sectionHeader->PointerToRelocations = pe_read32(input);
  sectionHeader->PointerToLinenumbers = pe_read32(input);
  sectionHeader->NumberOfRelocations = pe_read16(input);
  sectionHeader->NumberOfLinenumbers = pe_read16(input);
  sectionHeader->Characteristics = pe_read32(input);
}

static void read_section_headers(FILE* input, IMAGE_SECTION_HEADER* sectionHeaders, unsigned count)
{
  unsigned i;
  for (i = 0; i < count; i++)
  {
    read_section_header(input, &sectionHeaders[i]);
    /*synce_trace("Section %i: %s", i, sectionHeaders[i].Name);*/
  }
}

static bool pe_read_headers(
    FILE *input,
    IMAGE_FILE_HEADER* fileHeader,
    IMAGE_OPTIONAL_HEADER** optionalHeader,
    IMAGE_SECTION_HEADER** sectionHeaders)
{
  uint32_t nt_headers_offset;
  if (!pe_nt_headers_offset(input, &nt_headers_offset))
    return false;

  /* Signature */
  fseek(input, nt_headers_offset, SEEK_SET);
  if (pe_read32(input) != IMAGE_NT_SIGNATURE)
    return false;

  /* IMAGE_FILE_HEADER */
  read_file_header(input, fileHeader);

  /* IMAGE_OPTIONAL_HEADER */
  *optionalHeader = calloc(1, fileHeader->SizeOfOptionalHeader);
  read_optional_header(
      input, *optionalHeader, fileHeader->SizeOfOptionalHeader);

  /* IMAGE_SECTION_HEADER */
  *sectionHeaders = calloc(
      fileHeader->NumberOfSections, SIZEOF_IMAGE_SECTION_HEADER);
  read_section_headers(input, *sectionHeaders, fileHeader->NumberOfSections);

  return true;
}

    
bool pe_find_section(FILE* input, const char *name, uint32_t *fileOffset, uint32_t *virtualOffset)
{
  bool success = false;
  unsigned i;
  IMAGE_FILE_HEADER file_header;
  IMAGE_OPTIONAL_HEADER* optional_header;
  IMAGE_SECTION_HEADER* section_headers;

  if (!pe_read_headers(input, &file_header, &optional_header, &section_headers))
    return false;
  
  for (i = 0; i < file_header.NumberOfSections; i++)
  {
    if (strncmp((const char*)section_headers[i].Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0)
    {
      if (fileOffset)
        *fileOffset    = section_headers[i].PointerToRawData;
      if (virtualOffset)
        *virtualOffset = section_headers[i].VirtualAddress;
      success = true;
      goto exit;
    }
  }
 
exit: 
  free(optional_header);
  free(section_headers);
  return success;
}


bool pe_rsrc_offset(FILE *input, uint32_t* fileOffset, uint32_t* virtualOffset)
{
  return pe_find_section(input, ".rsrc", fileOffset, virtualOffset);
}

bool pe_size(FILE *input, uint32_t* result)
{
  unsigned i;
  IMAGE_FILE_HEADER file_header;
  IMAGE_OPTIONAL_HEADER* optional_header;
  IMAGE_SECTION_HEADER* section_headers;
  uint32_t max_offset = 0;

  if (!pe_read_headers(input, &file_header, &optional_header, &section_headers))
    return false;
  
  for (i = 0; i < file_header.NumberOfSections; i++)
  {
    if (max_offset <= section_headers[i].PointerToRawData)
    {
      max_offset = 
        section_headers[i].PointerToRawData +
        section_headers[i].SizeOfRawData;
    }
  }

  free(optional_header);
  free(section_headers);

  *result = max_offset;

  return true;
}

