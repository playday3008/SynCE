#define _GNU_SOURCE 1
#include "file.h"
#include <synce.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "internal.h"


bool rra_file_unpack(
		const uint8_t *data,
		size_t data_size,
		DWORD *ftype,
		char **path,
		uint8_t **file_content,
		size_t *file_size)
{
  DWORD filetype = 0;
  size_t pos = 0;
  char *tmp_path, *parsepath = NULL;
  uint8_t *tmp_content = NULL;

  /* first 4 bytes are a little-endian unsigned int
   * containing the file type */
  filetype = letoh32(*((DWORD*)data));

  /* next comes the path and file name in UCS-16, find
   * a 2 byte NULL ending the file name */
  pos = 4;
  while (pos < data_size) {
    if ( (*(WORD*)(data+pos)) == 0 )
      break;
    pos += 2;
  }
  tmp_path = wstr_to_current((LPWSTR)(data+4));

  /* replace back slashes in the path
   * with forward slashes */
  parsepath = tmp_path;
  while (*parsepath) {
    if ('\\' == *parsepath)
      *parsepath = '/';

    parsepath++;
  }

  pos += 2;
  if ((data_size - pos) > 0) {
    tmp_content = malloc(data_size - pos);
    memcpy(tmp_content, data+pos, data_size - pos);
  }

  *ftype = filetype;
  *path = tmp_path;
  *file_content = tmp_content;
  *file_size = data_size - pos;

  return TRUE;
}


bool rra_file_pack(
		DWORD ftype,
		const char* path, 
		const uint8_t* file_content,
		size_t file_size,
		uint8_t** data,
		size_t* data_size)
{

  char *tmppath = strdup(path);
  char *parsepath = tmppath;
  while (*parsepath) {
    if ('/' == *parsepath)
      *parsepath = '\\';

    parsepath++;
  }

  LPWSTR wide_path = wstr_from_current(tmppath);
  free(tmppath);

  size_t size = sizeof(DWORD) + ((wstrlen(wide_path) + 1) * sizeof(WCHAR)) + file_size;
  uint8_t *tmp_data = malloc(size);

  (*(DWORD*)tmp_data) = htole32(ftype);
  memcpy(tmp_data+sizeof(DWORD), wide_path, ((wstrlen(wide_path) + 1) * sizeof(WCHAR)));

  memcpy(tmp_data+sizeof(DWORD)+((wstrlen(wide_path) + 1) * sizeof(WCHAR)), file_content, file_size);
  wstr_free_string(wide_path);

  *data = tmp_data;
  *data_size = size;

  return TRUE;
}

