/* $Id$ */
#define _BSD_SOURCE 1
#include "../lib/syncmgr.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long fsize(FILE *stream)
{
  long offset, size;
  offset = ftell(stream);
  fseek(stream, 0, SEEK_END);
  size = ftell(stream);
  fseek(stream, offset, SEEK_SET);
  return size;
}

int main(int argc, char** argv)
{
  int result = 1;
  HRESULT hr;
  RRA_SyncMgr* syncmgr = NULL;
  const char* type_id_str = NULL;
  uint32_t type_id = 0;
  uint32_t object_id = 0;
  uint32_t flags = 0;
  uint32_t new_object_id = 0;
  const char* filename = NULL;
  uint8_t* data = NULL;
  size_t data_size = 0;
  FILE* file = NULL;
	RRA_SyncMgrType* type = NULL;

  /* synce_log_set_level(0); */

  if (argc < 5)
  {
    fprintf(stderr, 
        "Syntax:\n"
        "\n"
        "\t%s TYPE-ID OBJECT-ID FLAGS FILENAME\n"
        "\n"
        "The value for FLAGS is normally one of these:\n"
        "\n"
        "\t0x02   New object\n"
        "\t0x40   Update object\n"
        ,
        argv[0]);
    goto exit;
  }

  type_id_str = argv[1];
  object_id   = strtol(argv[2], NULL, 16);
  flags       = strtol(argv[3], NULL, 16);
  filename    = argv[4];

  hr = CeRapiInit();
  if (FAILED(hr))
    goto exit;

  syncmgr = rra_syncmgr_new();

  if (!rra_syncmgr_connect(syncmgr, NULL))
  {
    fprintf(stderr, "Connection failed\n");
    goto exit;
  }

  file = fopen(filename, "r");
  if (!file)
  {
    fprintf(stderr, "Failed to open file '%s'\n", filename);
    goto exit;
  }

  data_size = fsize(file);

  if (!data_size)
  {
    fprintf(stderr, "File is empty");
    goto exit;
  }

  data = (uint8_t*)malloc(data_size);

  fread(data, data_size, 1, file);

  type = rra_syncmgr_type_from_name(syncmgr, type_id_str);
  if (type)
    type_id = type->id;
  else
    type_id = strtol(type_id_str, NULL, 16);

  if (!rra_syncmgr_put_single_object(syncmgr, type_id, object_id, flags, data, data_size, &new_object_id))
  {
    fprintf(stderr, "Failed to put object\n");
    goto exit;
  }

  printf("New object id: %08x\n", new_object_id);

  result = 0;

exit:
  if (file)
    fclose(file);

  if (data)
    free(data);

  rra_syncmgr_destroy(syncmgr);

  CeRapiUninit();
  return result;
}
