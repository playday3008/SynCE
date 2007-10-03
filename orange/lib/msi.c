#define _BSD_SOURCE 1
#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif
#include "liborange_internal.h"
#include <synce_log.h>
#include <libole2/libole2.h>
#include <stdio.h>
#include <string.h>

#define ROOT_DIRECTORY "/"

static void write_file(
    MsOle* ole,
    int index,
    const char* name,
    const char* output_directory,
    const char* basename)
{
  MsOleErr error;
  MsOleStream* stream = NULL;
  MsOlePos bytes_to_write = 0;
  MsOlePos bytes_written = 0;
  MsOlePos bytes_left = 0;
  char output_filename[256];
  FILE* output = NULL;

  /* Open stream */

  error = ms_ole_stream_open(&stream, ole, ROOT_DIRECTORY, name, 'r');
  if (error != MS_OLE_ERR_OK)
    goto exit;

  /* Open output file */

  snprintf(output_filename, sizeof(output_filename), "%s/%s.%04x", output_directory, basename, index);

  output = fopen(output_filename, "w");
  if (!output)
    goto exit;

  /* Dump stream */

  for (bytes_left = stream->size; bytes_left; bytes_left -= bytes_written)
  {
    bytes_to_write = MIN(32, bytes_left);
    guint8* ptr = stream->read_ptr(stream, bytes_to_write);

    if (ptr)
    {
      bytes_written = fwrite(ptr, 1, bytes_to_write, output);
      if (bytes_written == 0)
      {
        synce_error("Failed to write to file");
        goto exit;
      }
    }
    else
    {
      synce_error("Failed to read from OLE stream");
      goto exit;
    }
  }

exit:
  if (stream)
    ms_ole_stream_close(&stream);
  if (output)
    fclose(output);
}

bool orange_extract_msi(
    const char* input_filename,
    const char* output_directory)
{
  bool success = false;
  MsOle* ole = NULL;
  MsOleErr error;
  char** names = NULL;
  int i;
  const char* basename;

  synce_trace("here");

  basename = strrchr(input_filename, '/');
  if (basename)
    basename++;
  else
    basename = input_filename;
 
  error = ms_ole_open(&ole, input_filename);
  if (error != MS_OLE_ERR_OK)
  {
    goto exit;
  }

  error = ms_ole_directory(&names, ole, ROOT_DIRECTORY);
  if (error != MS_OLE_ERR_OK)
  {
    goto exit;
  }

  for (i = 0; names[i] != NULL; i++)
  {
    MsOleStat stat;
    
    error = ms_ole_stat(&stat, ole, ROOT_DIRECTORY, names[i]);
    if (error != MS_OLE_ERR_OK)
      continue;
    
    if (stat.type != MsOleStreamT)
      continue;

    write_file(ole, i, names[i], output_directory, basename);
  }
  
  success = true;
  
exit:
  /* XXX: free names? */
  if (ole)
    ms_ole_destroy(&ole);
  return success;
}
