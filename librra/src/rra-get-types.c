/* $Id$ */
#include "syncmgr.h"
#include <rapi2.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-d LEVEL]\n"
			"\n"
			"\t-d LEVEL          Set debug log level\n"
			"\t                  0 - No logging\n"
			"\t                  1 - Errors only (default)\n"
			"\t                  2 - Errors and warnings\n"
			"\t                  3 - Errors, warnings and info\n"
			"\t                  4 - Everything\n",
			name);
}

static bool handle_parameters(int argc, char** argv)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_ERROR;

	while ((c = getopt(argc, argv, "d:")) != -1)
	{
		switch (c)
		{
			case 'd':
				log_level = atoi(optarg);
				break;
			
			case 'h':
			default:
				show_usage(argv[0]);
				return false;
		}
	}

	synce_log_set_level(log_level);
				
	return true;
}

int main(int argc, char** argv)
{
	int result = 1;
	IRAPIDesktop *desktop = NULL;
	IRAPIEnumDevices *enumdev = NULL;
	IRAPIDevice *device = NULL;
	IRAPISession *session = NULL;
	HRESULT hr;
	RRA_SyncMgr* syncmgr = NULL;
	const RRA_SyncMgrType* object_types = NULL;
	size_t object_type_count = 0;
	unsigned i;

	if (!handle_parameters(argc, argv))
		goto exit;
	

        if (FAILED(hr = IRAPIDesktop_Get(&desktop)))
        {
          fprintf(stderr, "%s: failed to initialise RAPI: %d: %s\n", 
                  argv[0], hr, synce_strerror_from_hresult(hr));
          goto exit;
        }

        if (FAILED(hr = IRAPIDesktop_EnumDevices(desktop, &enumdev)))
        {
          fprintf(stderr, "%s: failed to get connected devices: %d: %s\n", 
                  argv[0], hr, synce_strerror_from_hresult(hr));
          goto exit;
        }

        if (FAILED(hr = IRAPIEnumDevices_Next(enumdev, &device)))
        {
          fprintf(stderr, "%s: Could not find device: %08x: %s\n", 
                  argv[0], hr, synce_strerror_from_hresult(hr));
          device = NULL;
          goto exit;
        }

        IRAPIDevice_AddRef(device);
        IRAPIEnumDevices_Release(enumdev);
        enumdev = NULL;

        if (FAILED(hr = IRAPIDevice_CreateSession(device, &session)))
        {
          fprintf(stderr, "%s: Could not create a session to device: %08x: %s\n", 
                  argv[0], hr, synce_strerror_from_hresult(hr));
          goto exit;
        }

        if (FAILED(hr = IRAPISession_CeRapiInit(session)))
        {
          fprintf(stderr, "%s: Unable to initialize connection to device: %08x: %s\n", 
                  argv[0], hr, synce_strerror_from_hresult(hr));
          goto exit;
        }

	syncmgr = rra_syncmgr_new();

	if (!rra_syncmgr_connect(syncmgr, session))
	{
		fprintf(stderr, "RRA connection failed\n");
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

	rra_syncmgr_disconnect(syncmgr);
exit:
	rra_syncmgr_destroy(syncmgr);
	
        if (session)
        {
          IRAPISession_CeRapiUninit(session);
          IRAPISession_Release(session);
        }

        if (device) IRAPIDevice_Release(device);
        if (enumdev) IRAPIEnumDevices_Release(enumdev);
        if (desktop) IRAPIDesktop_Release(desktop);
	return result;
}
