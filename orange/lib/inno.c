/* $Id$ */
#define _BSD_SOURCE 1
#include "inno.h"
#include "liborange_internal.h"
#include <synce_log.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h> /* for MIN() */
#include <zlib.h>

/*
   Inno Setup - comes with source code :-)

   www.innosetup.com
 */

#define VERBOSE 0

#if VERBOSE
static void dump(const char *desc, void* data, size_t len)/*{{{*/
{
	uint8_t* buf = (uint8_t*)data;
	size_t i, j;
	char hex[8 * 3 + 1];
	char chr[8 + 1];

	synce_trace("%s (%d bytes):", desc, len);
	for (i = 0; i < len + 7; i += 8) {
		for (j = 0; j < 8; j++) 
			if (j + i >= len) {
				hex[3*j+0] = ' ';
				hex[3*j+1] = ' ';
				hex[3*j+2] = ' ';
				chr[j] = ' ';
			} else {
				uint8_t c = buf[j + i];
				const char *hexchr = "0123456789abcdef";
				hex[3*j+0] = hexchr[(c >> 4) & 0xf];
				hex[3*j+1] = hexchr[c & 0xf];
				hex[3*j+2] = ' ';
				if (c > ' ' && c <= '~')
					chr[j] = c;
				else
					chr[j] = '.';
			}
		hex[8*3] = '\0';
		chr[8] = '\0';
		synce_trace("  %04x: %s %s", i, hex, chr);
	}
}/*}}}*/

#define DUMP(a,b,c) dump(a,b,c)
#else
#define DUMP(a,b,c)
#endif


int EntryStrings[] = { SetupTypeEntryStrings,
    SetupComponentEntryStrings, SetupTaskEntryStrings, SetupDirEntryStrings,
    SetupFileEntryStrings, SetupFileLocationEntryStrings, SetupIconEntryStrings,
    SetupIniEntryStrings, SetupRegistryEntryStrings, SetupDeleteEntryStrings,
    SetupDeleteEntryStrings, SetupRunEntryStrings, SetupRunEntryStrings };

static uint8_t SetupLdrOffsetTableID[] = 
{
  'r','D','l','P','t','S','0','2',0x87,0x65,0x56,0x78
};



#define OUTPUT_BUFFER_SIZE 0x8000

#define DATA_OFFSET 0xf000

#define FILE_SIGNATURE_SIZE    6


#if 0
static uint8_t FILE_SIGNATURE[FILE_SIGNATURE_SIZE] = 
{0x7a, 0x6c, 0x62, 0x1a, 0x78, 0xda};
#endif

#if 0
static bool orange_inno_decompress(/*{{{*/
    const uint8_t* input_buffer, 
    size_t input_max_size,
    const char* output_filename,
    size_t* input_used)
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
  stream.avail_in = input_max_size;
  
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

  *input_used = stream.total_in;

  success = *input_used != 0;

exit:
  FCLOSE(output);
  FREE(output_buffer);
  return success;
}/*}}}*/
#endif

static void InitStream(z_stream* strm)/*{{{*/
{
  memset(strm, 0, sizeof(z_stream));
  strm->zalloc = NULL;
  strm->zfree  = NULL;
}/*}}}*/

uint32_t GetCRC32 (void* Buf, size_t BufSize)/*{{{*/
{
  return crc32(crc32(0, NULL, 0), (const Bytef*)Buf, BufSize);
}/*}}}*/

static bool InflateBlockReadBegin(FILE* F, TDeflateBlockReadData* Data)/*{{{*/
{
  bool success = false;
  uint32_t HdrCRC;
  TNewBlockDeflateHeader Hdr;
  
  memset(Data, 0, sizeof(TDeflateBlockReadData));
  Data->F = F;
  Data->StartPos = ftell(F);

  /* TODO: check file pos */

  fread(&HdrCRC,  1, sizeof(HdrCRC),  F);
  fread(&Hdr,     1, sizeof(Hdr),     F);

  LETOH32(HdrCRC);
  LETOH32(Hdr.CompressedSize);
  LETOH32(Hdr.UncompressedSize);

  if (HdrCRC != GetCRC32(&Hdr, sizeof(Hdr)))
  {
    synce_trace("Invalid block header CRC32");
    goto exit;
  }
  
  synce_trace("Block size: compressed = %08x, uncompressed = %08x",
      Hdr.CompressedSize, Hdr.UncompressedSize);

  if (Hdr.CompressedSize != (uint32_t)-1)
  {
    Data->Compressed = true;
    Data->InBytesLeft = Hdr.CompressedSize;
  }
  else
    Data->InBytesLeft = Hdr.UncompressedSize;

  InitStream(&Data->strm);
  if (Data->Compressed)
  {
    int error = inflateInit_(&Data->strm, zlib_version, sizeof(z_stream));
    if (Z_OK != error)
    {
      synce_error("inflateInit failed with error %i", error);
      goto exit;
    }
  }

  Data->strm.next_out = Data->OutBuffer;
  Data->strm.avail_out = sizeof(Data->OutBuffer);

  success = true;

exit:
  return success;
}/*}}}*/

static bool InflateBlockRead(TDeflateBlockReadData* Data, void* Buf, size_t Count)/*{{{*/
{
  bool success = false;
  void* B = Buf;
  bool Finished = false;
  size_t Left = Count;
  size_t OutCount = 0;
  long Len = 0;
  uint32_t CRC;
  int Res;

  while (Left != 0)
  {
    if (Data->strm.avail_in == 0)
    {
      if (Data->InBytesLeft == 0)
        Finished = true;
      else
      {
        Len = Data->InBytesLeft;
        if (Len > sizeof(Data->InBuffer))
          Len = sizeof(Data->InBuffer);

        /* TODO: check file pos */
        fread(&CRC, 1, sizeof(CRC),  Data->F);
        LETOH32(CRC);
        fread(Data->InBuffer, 1, Len, Data->F);
        
        Data->InBytesLeft -= Len;
        Data->strm.next_in = Data->InBuffer;
        Data->strm.avail_in = Len;

        if (CRC != GetCRC32(Data->InBuffer, Len))
        {
          synce_error("Block CRC32 error");
          goto exit;
        }
      }
    }

    if ((Data->strm.avail_out != 0) &&
        (Data->strm.next_out - &Data->OutBuffer[Data->OutBufferStart] < Left))
    {
      if (Data->NoMoreData)
      {
        synce_error("No more data. Left = %08x", Left);
        abort();
        goto exit;
      }

      if (Data->Compressed)
      {
#if VERBOSE
        synce_trace("Inflating...");
#endif
        Res = inflate(&Data->strm, Finished ? Z_FINISH : Z_NO_FLUSH);

        switch (Res)
        {
          case Z_OK:
            break;

          case Z_STREAM_END:
            Data->NoMoreData = true;
            break;

          default:
            synce_error("zlib error: %i", Res);
            goto exit;
        }
      }
      else
      {
        memcpy(Data->strm.next_out, Data->strm.next_in, OutCount);
        Data->strm.avail_in   -= OutCount;
        Data->strm.next_in    += OutCount;
        Data->strm.total_in   += OutCount;
        Data->strm.avail_out  -= OutCount;
        Data->strm.next_out   += OutCount;
        Data->strm.total_out  += OutCount;
      }
    }

    if (Data->strm.next_out > &Data->OutBuffer[Data->OutBufferStart])
    {
      OutCount = Data->strm.next_out - &Data->OutBuffer[Data->OutBufferStart];
      if (OutCount > Left)
        OutCount = Left;
      
      memcpy(B, &Data->OutBuffer[Data->OutBufferStart], OutCount);
      Left            -= OutCount;
      B               += OutCount;
      Data->OutBufferStart  += OutCount;

      if (Data->OutBufferStart == sizeof(Data->OutBuffer))
      {
        Data->strm.next_out   = Data->OutBuffer;
        Data->strm.avail_out  = sizeof(Data->OutBuffer);
        Data->OutBufferStart  = 0;
      }
    }
  }

  success = true;

exit:
  return success;
}/*}}}*/

static void InflateBlockReadEnd(TDeflateBlockReadData* Data)/*{{{*/
{
  if (Data->Compressed)
    inflateEnd(&Data->strm);
}/*}}}*/

static void SEInflateBlockRead(/*{{{*/
    TDeflateBlockReadData* Data, 
    void* Buf, 
    size_t Count, 
    int NumStrings)
{
  void* P;
  int I;
  int Len;
  char* S;

  P = Buf;
  for (I = 0; I < NumStrings; I++)
  {
    InflateBlockRead(Data, &Len, sizeof(Len));
    S = malloc(Len+1);
    if (Len)
    {
      InflateBlockRead(Data, S, Len);
    }
    S[Len] = '\0';

#if VERBOSE
    synce_trace("%2i: '%s'", I, S);
#endif

    *(char**)P = S;
    (uint8_t*)P += sizeof(char*);
  }

  Count -= NumStrings * sizeof(char*);
  InflateBlockRead(Data, P, Count);
  DUMP("Data after strings", P, Count);
}/*}}}*/

static void FreeStrings(/*{{{*/
    void* Buf, 
    int NumStrings)
{
  if (Buf)
  {
    void* P;
    int I;
    char* S;

    P = Buf;
    for (I = 0; I < NumStrings; I++)
    {
      S = *(char**)P;
      FREE(S);
      (uint8_t*)P += sizeof(char*);
    }
  }
}/*}}}*/

static bool SkipFile(TDeflateBlockReadData* Data)/*{{{*/
{
  bool success = false;
  uint32_t BytesLeft;
  uint8_t Buf[8192];
  size_t Bytes;

  if (!InflateBlockRead(Data, &BytesLeft, sizeof(BytesLeft)))
    goto exit;

  LETOH32(BytesLeft);

#if VERBOSE
  synce_trace("Skipping %08x bytes of file data", BytesLeft);
#endif

  while (BytesLeft > 0)
  {
    Bytes = MIN(BytesLeft, sizeof(Buf));
    if (!InflateBlockRead(Data, Buf, Bytes))
    {
      synce_error("InflateBlockRead failed");
      goto exit;
    }
#if 0
    DUMP("File data", Buf, Bytes);
#endif
    BytesLeft -= Bytes;
  }

  success = true;

exit:
  return success;
}/*}}}*/

static void ReadEntries(/*{{{*/
    TDeflateBlockReadData* Data, 
    TEntryType EntryType, 
    size_t Count, 
    size_t Size,
    void* Buf)
{
  int I;
  void* P;

  P = Buf;
  for (I = 0; I < Count; I++)
  {
    SEInflateBlockRead(Data, P, Size, EntryStrings[EntryType]);

#if VERBOSE
    if (EntryType == seFile)
    {
      synce_trace("LocationEntry = %08x", 
          ((TSetupFileEntry*)P)->LocationEntry);
    }
#endif
    
    (uint8_t*)P += Size;
  }
}/*}}}*/

static void FreeEntries(
    TEntryType EntryType, 
    size_t Count, 
    size_t Size,
    void* Buf)
{
  if (Buf)
  {
    int I;
    void* P;

    P = Buf;
    for (I = 0; I < Count; I++)
    {
      FreeStrings(P, EntryStrings[EntryType]);
      (uint8_t*)P += Size;
    }

    free(Buf);
  }
}

static bool orange_get_inno_offset_table(FILE* SetupFile, TSetupLdrOffsetTable* OffsetTable)/*{{{*/
{
  bool success = false;
  size_t SizeOfFile;
  TSetupLdrExeHeader ExeHeader;

  SizeOfFile = FSIZE(SetupFile);

  /*
     Handle ExeHeader
   */

  fseek(SetupFile, SetupLdrExeHeaderOffset, SEEK_SET);
  
  if (sizeof(TSetupLdrExeHeader) != fread(&ExeHeader, 1, sizeof(ExeHeader), SetupFile))
  {
    goto exit;
  }

  LETOH32(ExeHeader.ID);
  LETOH32(ExeHeader.OffsetTableOffset);
  LETOH32(ExeHeader.NotOffsetTableOffset);

  if ((ExeHeader.ID != SetupLdrExeHeaderID) ||
      (ExeHeader.OffsetTableOffset != ~ExeHeader.NotOffsetTableOffset) ||
      (ExeHeader.OffsetTableOffset + sizeof(OffsetTable) > SizeOfFile))
  {
    synce_trace("Not a valid Inno Setup file");
    goto exit;
  }

  /*
     Handle OffsetTable
   */

  fseek(SetupFile, ExeHeader.OffsetTableOffset, SEEK_SET);

  if (sizeof(TSetupLdrOffsetTable) != fread(OffsetTable, 1, sizeof(TSetupLdrOffsetTable), SetupFile))
  {
    goto exit;
  }

  if (0 != memcmp(OffsetTable->ID, SetupLdrOffsetTableID, 12))
  {
    synce_trace("Not a valid Inno Setup file");
    goto exit;
  }

  /* TODO: convert more */
  LETOH32(OffsetTable->Offset0);
  LETOH32(OffsetTable->Offset1);

#if VERBOSE
  synce_trace("OffsetTable.Offset0 = %08x", OffsetTable->Offset0);
  synce_trace("OffsetTable.Offset1 = %08x", OffsetTable->Offset1);
#endif

  success = true;

exit:
  return success;
}/*}}}*/

static bool orange_get_inno_setup_data(/*{{{*/
    FILE* SetupFile, 
    TSetupHeader* SetupHeader,
    TSetupFileEntry** FileEntries,
    TSetupFileLocationEntry** FileLocationEntries)
{
  bool success = false;
  int version[3];
  TDeflateBlockReadData Data;
  TSetupLangOptions LangOptions;
  TSetupRunEntry* RunEntries = NULL;
  char TestID[SETUP_ID_SIZE];
  int i;
  
  if (sizeof(TestID) != fread(TestID, 1, sizeof(TestID), SetupFile))
  {
    goto exit;
  }

  if (3 != sscanf(TestID, SETUP_ID_FORMAT, version+0, version+1, version+2))
  {
    goto exit;
  }

  synce_trace("Inno Setup version: %i.%i.%i", version[0], version[1], version[2]);

#if VERBOSE
  synce_trace("Offset: %08x", ftell(SetupFile));
#endif

  if (!InflateBlockReadBegin(SetupFile, &Data))
  {
    synce_error("InflateBlockReadBegin failed");
    goto exit;
  }

  SEInflateBlockRead(&Data, SetupHeader, SETUP_HEADER_SIZE, SetupHeaderStrings);

  LETOH32(SetupHeader->NumTypeEntries);
  LETOH32(SetupHeader->NumComponentEntries);
  LETOH32(SetupHeader->NumTaskEntries);
  LETOH32(SetupHeader->NumDirEntries);
  LETOH32(SetupHeader->NumFileEntries);
  LETOH32(SetupHeader->NumIconEntries);
  LETOH32(SetupHeader->NumIniEntries);
  LETOH32(SetupHeader->NumRegistryEntries);
  LETOH32(SetupHeader->NumInstallDeleteEntries);
  LETOH32(SetupHeader->NumUninstallDeleteEntries);
  LETOH32(SetupHeader->NumRunEntries);
  LETOH32(SetupHeader->NumUninstallRunEntries);

#if VERBOSE
  synce_trace("NumFileEntries: %i", SetupHeader->NumFileEntries);
#endif
  
  SEInflateBlockRead(&Data, &LangOptions, sizeof(TSetupLangOptions),  SetupLangOptionsStrings);

  SkipFile(&Data);   /* WizardImage */
  SkipFile(&Data);   /* WizardSmallImage */

  /* TODO: SkipFile() if bzip is used */

 
  /* can't handle these yet */
  assert(0 == SetupHeader->NumTypeEntries);
  assert(0 == SetupHeader->NumComponentEntries);
  assert(0 == SetupHeader->NumTaskEntries);
  assert(0 == SetupHeader->NumDirEntries);

/*  synce_trace("Reading file entries");*/
  *FileEntries = calloc(SetupHeader->NumFileEntries, sizeof(TSetupFileEntry));
  ReadEntries(&Data, seFile, SetupHeader->NumFileEntries, sizeof(TSetupFileEntry), *FileEntries);

  /* can't handle these yet */
  assert(0 == SetupHeader->NumIconEntries);
  assert(0 == SetupHeader->NumIniEntries);
  assert(0 == SetupHeader->NumRegistryEntries);
  assert(0 == SetupHeader->NumInstallDeleteEntries);
  assert(0 == SetupHeader->NumUninstallDeleteEntries);

  RunEntries = calloc(SetupHeader->NumRunEntries, sizeof(TSetupRunEntry));
  ReadEntries(&Data, seRun, SetupHeader->NumRunEntries, sizeof(TSetupRunEntry), RunEntries);

  assert(0 == SetupHeader->NumUninstallRunEntries);
  
  InflateBlockReadEnd(&Data);

  /*
     File location entries
   */

  if (!InflateBlockReadBegin(SetupFile, &Data))
  {
    synce_error("InflateBlockReadBegin failed");
    goto exit;
  }

  synce_trace("Reading file location entries");
  *FileLocationEntries = calloc(SetupHeader->NumFileLocationEntries, sizeof(TSetupFileLocationEntry));

  for (i = 0; i < SetupHeader->NumFileLocationEntries; i++)
  {
    TSetupFileLocationEntry* entry = (*FileLocationEntries) + i;
    InflateBlockRead(&Data, entry,  sizeof(TSetupFileLocationEntry));

    LETOH32(entry->FirstDisk);
    LETOH32(entry->LastDisk);
    LETOH32(entry->StartOffset);
    LETOH32(entry->OriginalSize);
    LETOH32(entry->CompressedSize);
    LETOH32(entry->Adler);
  }

  InflateBlockReadEnd(&Data);

  success = true;

exit:
  FreeEntries(seRun,  SetupHeader->NumRunEntries,  sizeof(TSetupRunEntry),  RunEntries);
  return success;
}/*}}}*/

bool orange_extract_inno(
    const char* input_filename, 
    const char* output_directory)
{
  bool success = false;
  FILE* SetupFile = fopen(input_filename, "r");
  int i;
#if 0
  uint8_t* input_buffer = NULL;
  size_t input_size;
  uint8_t* p;
  int count = 0;
  char filename[256];
#endif
 
  TSetupLdrOffsetTable OffsetTable;
  TSetupHeader SetupHeader;
  TSetupFileEntry* FileEntries = NULL;
  TSetupFileLocationEntry* FileLocationEntries = NULL;

  if (!SetupFile)
  {
    synce_error("Failed to open file for reading: '%s'", input_filename);
    goto exit;
  }

  if (!orange_get_inno_offset_table(SetupFile, &OffsetTable))
  {
    /* Not an Inno Setup executable */
    goto exit;
  }

  /*
     Setup data
   */

  fseek(SetupFile, OffsetTable.Offset0, SEEK_SET);

  if (!orange_get_inno_setup_data(SetupFile,
        &SetupHeader, &FileEntries, &FileLocationEntries))
  {
    goto exit;
  }

  for (i = 0; i < SetupHeader.NumFileEntries; i++)
  {
    int l = FileEntries[i].LocationEntry;
    synce_trace("%08x %08x %08x %s", 
        FileLocationEntries[l].StartOffset,
        FileLocationEntries[l].OriginalSize,
        FileLocationEntries[l].CompressedSize,
        FileEntries[i].DestName);
  }

#if 0
  input_size = FSIZE(SetupFile) - DATA_OFFSET;

  fseek(SetupFile, DATA_OFFSET, SEEK_SET);

  input_buffer = (uint8_t*)malloc(input_size);
  if (!input_buffer)
    goto exit;

  if (input_size != fread(input_buffer, 1, input_size, SetupFile))
  {
    goto exit;
  }

  p = input_buffer;

  while (input_size)
  {
    size_t input_used = 0;
    
    synce_trace("Offset %08x + %08x, %08x bytes left",
        DATA_OFFSET, p - input_buffer, input_size);

    if (memcmp(p, FILE_SIGNATURE, FILE_SIGNATURE_SIZE) != 0)
    {
      synce_trace("Signature mismatch");
      break;
    }

    synce_trace("Found file header");

    snprintf(filename, sizeof(filename), "%s/%08x.bin",
        output_directory, p - input_buffer);

    p           += FILE_SIGNATURE_SIZE;
    input_size  -= FILE_SIGNATURE_SIZE;

    if (!orange_inno_decompress(p, input_size, filename, &input_used))
    {
      synce_trace("Decompression failed");
      break;
    }

    synce_trace("Unknown: %08x", *(uint32_t*)(p + input_used));

    /* CRC32? */
    input_used += 4;

    p           += input_used;
    input_size  -= input_used;

    count++;
  }

  success = (count > 0);
#endif
  
exit:
  FreeEntries(seFile, SetupHeader.NumFileEntries, sizeof(TSetupFileEntry), FileEntries);
  FREE(FileLocationEntries);
#if 0
  FREE(input_buffer);
#endif
  FCLOSE(SetupFile);
  return success;  
}


