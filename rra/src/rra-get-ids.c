/* $Id$ */
#define _BSD_SOURCE 1
#include "librra.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	int result = 1;
	HRESULT hr;
	RRA* rra = NULL;
  const char* type_id_str = NULL;
	uint32_t type_id = 0;
	ObjectIdArray* object_ids = NULL;
	unsigned i, id = 0;
	uint32_t* deleted_ids = NULL;
	size_t deleted_count = 0;
	
	synce_log_set_level(0);

	if (argc < 2)
	{
		fprintf(stderr, "Missing object type id on command line\n");
		goto exit;
	}

	type_id_str = argv[1];
	
	hr = CeRapiInit();
	if (FAILED(hr))
		goto exit;

	rra = rra_new();

	if (!rra_connect(rra))
	{
		fprintf(stderr, "Connection failed\n");
		goto exit;
	}

  type_id = rra_type_id_from_name(rra, type_id_str);
  if (!type_id)
    type_id = strtol(type_id_str, NULL, 16);

	if (!rra_get_object_ids(rra, type_id, &object_ids))
	{
		fprintf(stderr, "Failed to get object ids\n");
		goto exit;
	}

	for (i = 0; i < object_ids->unchanged; i++)
		printf("%08x  Unchanged\n", object_ids->ids[id++]);

	for (i = 0; i < object_ids->changed; i++)
		printf("%08x  Changed\n", object_ids->ids[id++]);

	if (!rra_get_deleted_object_ids(
				rra, 
				type_id, 
				object_ids, 
				&deleted_ids, 
				&deleted_count))
	{
		fprintf(stderr, "Failed to get deleted object ids\n");
		goto exit;
	}

	for (id = 0; id < deleted_count; id++)
		printf("%08x  Deleted\n", deleted_ids[id]);

exit:
	rra_free_object_ids(object_ids);
	rra_free_deleted_object_ids(deleted_ids);
	rra_free(rra);
	
	CeRapiUninit();
	return result;
}
