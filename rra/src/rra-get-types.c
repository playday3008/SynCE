/* $Id$ */
#define _BSD_SOURCE 1
#include "librra.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	int result = 1;
	HRESULT hr;
	RRA* rra = NULL;
	ObjectType* object_types = NULL;
	size_t object_type_count = 0;
	unsigned i;

/*	synce_log_set_level(0);*/
	
	hr = CeRapiInit();
	if (FAILED(hr))
		goto exit;

	rra = rra_new();

	if (!rra_connect(rra))
	{
		fprintf(stderr, "Connection failed\n");
		goto exit;
	}

	if (!rra_get_object_types(rra, &object_types, &object_type_count))
	{
		fprintf(stderr, "Failed to get object types\n");
		goto exit;
	}

	printf("ID        COUNT     SIZE      MODIFIED                  NAME\n");
	for (i = 0; i < object_type_count; i++)
	{
		char* modified = NULL;
		if (object_types[i].modified)
		{
			modified = strdup(ctime(&object_types[i].modified));
			/* remove trailing newline */
			modified[strlen(modified)-1] = '\0';
		}
			
		printf("%08x  %8i  %8i  %s  %s\n",
				object_types[i].id,
				object_types[i].count,
				object_types[i].total_size,
				modified ? modified : "                        ",
				object_types[i].name);

		if (modified)
			free(modified);
	}
	
exit:
	rra_free(rra);
	
	CeRapiUninit();
	return result;
}
