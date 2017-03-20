/* $Id$ */
#include "../lib/syncmgr.h"
#include <rapi2.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static bool running = true;

static void ctrl_c_handler(int unused)
{
  running = false;
}

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-d LEVEL] TYPE-ID ...\n"
			"\n"
			"\t-d LEVEL          Set debug log level\n"
			"\t                  0 - No logging\n"
			"\t                  1 - Errors only (default)\n"
			"\t                  2 - Errors and warnings\n"
			"\t                  3 - Errors, warnings and info\n"
			"\t                  4 - Everything\n"
			"\tTYPE-ID ...       Zero or more RRA type-id's to monitor\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char ***type_id_list, uint *type_id_no)
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
				
	*type_id_no = argc - optind;
		
	*type_id_list = &argv[optind];

	return true;
}

static bool callback (
    RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie)
{
  const char* event_str;
  unsigned i;
  time_t now;
  char timestamp[32];

  time(&now);

  synce_trace("event=%i, type=%08x, count=%08x",
      event, type, count);
  
  switch (event)
  {
    case SYNCMGR_TYPE_EVENT_UNCHANGED:
      event_str = "Unchanged";
      break;
    case SYNCMGR_TYPE_EVENT_CHANGED:
      event_str = "Changed";
      break;
    case SYNCMGR_TYPE_EVENT_DELETED:
      event_str = "Deleted";
      break;
    default:
      event_str = "Unknown";
      break;
  }

  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

  printf("      TIMESTAMP            TYPE      ID        EVENT     (%i items)\n", count);

	for (i = 0; i < count; i++)
		printf("%s  %08x  %08x  %s\n", timestamp, type, ids[i], event_str);

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
	char** type_id_list = NULL;
	uint type_id_no = 0;
	char* type_id_str = NULL;
	uint32_t type_id = 0;
	bool got_event = false;
	/*uint32_t* deleted_ids = NULL;
	size_t deleted_count = 0;*/
	
	if (!handle_parameters(argc, argv, &type_id_list, &type_id_no))
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
		fprintf(stderr, "Connection failed\n");
		goto exit;
	}

  if (type_id_no == 0)
  {
    /* Subcribe to all types */
    unsigned i;
    uint32_t count = rra_syncmgr_get_type_count(syncmgr);
    const RRA_SyncMgrType* types = rra_syncmgr_get_types(syncmgr);
    for (i = 0; i < count; i++)
    {
      rra_syncmgr_subscribe(syncmgr, types[i].id, callback, NULL);
    }
  }
  else
  {
    int i;

    for (i = 0; i < type_id_no; i++)
    {
      const RRA_SyncMgrType* type = NULL;

      type_id_str = type_id_list[i];

      type = rra_syncmgr_type_from_name(syncmgr, type_id_str);
      if (type)
        type_id = type->id;
      else
        type_id = strtol(type_id_str, NULL, 16);

      rra_syncmgr_subscribe(syncmgr, type_id, callback, NULL);
    }
  }
    
  if (!rra_syncmgr_start_events(syncmgr))
  {
 		fprintf(stderr, "Failed to start events\n");
		goto exit;
  }

  signal(SIGINT, ctrl_c_handler);
  printf("Press Ctrl+C to cancel subscription.\n");
		
  /* Process all events triggered by rra_syncmgr_start_events */
  while (running)
  {
    if (rra_syncmgr_event_wait(syncmgr, 3, &got_event) && got_event)
      rra_syncmgr_handle_event(syncmgr);
  }
  
  printf("See you later!\n");

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
