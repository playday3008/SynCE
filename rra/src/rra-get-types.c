/* $Id$ */
#define _BSD_SOURCE 1
#include "syncmgr.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	int result = 1;
	HRESULT hr;
  RRA_SyncMgr* syncmgr = NULL;
	RRA_SyncMgrType* object_types = NULL;
	size_t object_type_count = 0;
	unsigned i;

/*	synce_log_set_level(0);*/
	
	hr = CeRapiInit();
	if (FAILED(hr))
		goto exit;

	syncmgr = rra_syncmgr_new();

	if (!rra_syncmgr_connect(syncmgr))
	{
		fprintf(stderr, "Connection failed\n");
		goto exit;
	}

  object_type_count = rra_syncmgr_get_type_count(syncmgr);

  object_types = rra_syncmgr_get_types(syncmgr);

	if (!object_types)
	{
		fprintf(stderr, "Failed to get object types\n");
		goto exit;
	}

	printf("ID        COUNT     SIZE      MODIFIED                  NAME 1                         NAME 2\n");
	for (i = 0; i < object_type_count; i++)
	{
		char* modified = NULL;
		if (object_types[i].modified)
		{
			modified = strdup(ctime(&object_types[i].modified));
			/* remove trailing newline */
			modified[strlen(modified)-1] = '\0';
		}
			
		printf("%08x  %8i  %8i  %s  %-30s %-30s\n",
				object_types[i].id,
				object_types[i].count,
				object_types[i].total_size,
				modified ? modified : "                        ",
				object_types[i].name1,
				object_types[i].name2
        );

		if (modified)
			free(modified);
	}
	
exit:
	rra_syncmgr_destroy(syncmgr);
	
	CeRapiUninit();
	return result;
}
