/* $Id$ */
#define _BSD_SOURCE 1
#include <rapi.h>
#include <synce_log.h>
#include "librra.h"
#include <stdio.h>
#include <time.h>

int main(int argc, char** argv)
{
	int result = 1;
	RRA* rra = NULL;
	DWORD current_partner;
	HRESULT hr;
	int i;

	hr = CeRapiInit();
	if (FAILED(hr))
	{
		synce_error("Failed to initialize RAPI");
		goto exit;
	}

	rra = rra_new();

	if (!rra_partner_get_current(rra, &current_partner))
	{
		fprintf(stderr, "Failed to get current partner index");
		goto exit;
	}

	printf("Current partner index: %i\n", current_partner);

	for (i = 1; i <= 2; i++)
	{
		uint32_t id = 0;
		char* name = NULL;
		
		if (rra_partner_get_id(rra, i, &id))
		{
			printf("Partner %i id:    0x%08x\n", i, id);
		}

		if (rra_partner_get_name(rra, i, &name))
		{
			printf("Partner %i name:  \"%s\"\n", i, name);
			rra_partner_free_name(name);
		}
	}

	result = 0;
	
exit:
	rra_free(rra);
	CeRapiUninit();
	return result;
}

