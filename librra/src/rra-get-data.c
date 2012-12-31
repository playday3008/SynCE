/* $Id$ */
#define _BSD_SOURCE 1
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
			"\t%s [-d LEVEL] TYPE-ID OBJECT-ID FILENAME\n"
			"\n"
			"\t-d LEVEL          Set debug log level\n"
			"\t                  0 - No logging\n"
			"\t                  1 - Errors only (default)\n"
			"\t                  2 - Errors and warnings\n"
			"\t                  3 - Errors, warnings and info\n"
			"\t                  4 - Everything\n"
			"\tTYPE-ID           RRA type-id of the object to get\n"
			"\tOBJECT-ID         RRA object-id of the object to get\n"
			"\tFILENAME          The destination filename\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** typeid, char** objectid, char** filename)
{
	int c;
	int arg_count;
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

	arg_count = argc - optind;
	if (arg_count != 3) {
		fprintf(stderr, "%s: You need to specify type id, object id and destination file name on command line\n\n", argv[0]);
		show_usage(argv[0]);
		return false;
	}

	*typeid = strdup(argv[optind++]);
       	*objectid = strdup(argv[optind++]);
       	*filename = strdup(argv[optind++]);

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
	char* type_id_str = NULL;
	char* object_id_str = NULL;
	uint32_t type_id = 0;
	uint32_t object_id = 0;
	char* filename = NULL;
	uint8_t* data = NULL;
	size_t data_size = 0;
	FILE* file = NULL;
	const RRA_SyncMgrType* type = NULL;
	
	if (!handle_parameters(argc, argv, &type_id_str, &object_id_str, &filename))
		goto exit;

	object_id   = strtol(object_id_str, NULL, 16);
	
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

	if (fwrite(data, data_size, 1, file) != 1)
	{
		fprintf(stderr, "Failed to write data to file '%s'\n", filename);
		goto exit;
	}
	
	result = 0;
	
	rra_syncmgr_disconnect(syncmgr);
exit:
	if (file)
		fclose(file);

	if (data)
		free(data);

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
