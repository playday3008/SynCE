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
	uint32_t type_id = 0;
	uint32_t object_id = 0;
	
	/* synce_log_set_level(0); */

	if (argc < 3)
	{
		fprintf(stderr, "Syntax: %s TYPE-ID OBJECT-ID\n",
				argv[0]);
		goto exit;
	}

	type_id    = strtol(argv[1], NULL, 16);
	object_id  = strtol(argv[2], NULL, 16);
	
	hr = CeRapiInit();
	if (FAILED(hr))
		goto exit;

	rra = rra_new();

	if (!rra_connect(rra))
	{
		fprintf(stderr, "Connection failed\n");
		goto exit;
	}

	if (!rra_object_delete(rra, type_id, object_id))
	{
		fprintf(stderr, "Failed to delete object\n");
		goto exit;
	}

	result = 0;
	
exit:
	rra_free(rra);
	
	CeRapiUninit();
	return result;
}
