/* $Id$ */
#define _BSD_SOURCE 1
#include "syncmgr.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	int result = 1;
	HRESULT hr;
	RRA_SyncMgr* syncmgr = NULL;
  const char* type_id_str = NULL;
  uint32_t type_id = 0;
	uint32_t object_id = 0;
	const char* filename = NULL;
	uint8_t* data = NULL;
	size_t data_size = 0;
	FILE* file = NULL;
	RRA_SyncMgrType* type = NULL;
	
/*	synce_log_set_level(0);*/

	if (argc < 4)
	{
		fprintf(stderr, "Syntax: %s TYPE-ID OBJECT-ID FILENAME\n",
				argv[0]);
		goto exit;
	}

	type_id_str = argv[1];
  object_id   = strtol(argv[2], NULL, 16);
	filename    = argv[3];
	
	hr = CeRapiInit();
	if (FAILED(hr))
		goto exit;

	syncmgr = rra_syncmgr_new();

	if (!rra_syncmgr_connect(syncmgr, NULL))
	{
		fprintf(stderr, "Connection failed\n");
		goto exit;
	}

  type = rra_syncmgr_type_from_name(syncmgr, type_id_str);
  if (type)
    type_id = type->id;
  else
    type_id = strtol(type_id_str, NULL, 16);

  if (!type_id)
  {
 		fprintf(stderr, "Invalid type ID '%s'\n", type_id_str);
		goto exit;
  }

	if (!rra_syncmgr_get_single_object(syncmgr, type_id, object_id, &data, &data_size))
	{
		fprintf(stderr, "Failed to get object\n");
		goto exit;
	}

	file = fopen(filename, "w");
	if (!file)
	{
		fprintf(stderr, "Failed to open file '%s'\n", filename);
		goto exit;
	}
	
	fwrite(data, data_size, 1, file);

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
