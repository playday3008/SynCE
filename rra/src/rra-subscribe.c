/* $Id$ */
#define _BSD_SOURCE 1
#include "../lib/syncmgr.h"
#include <rapi.h>
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
	HRESULT hr;
	RRA_SyncMgr* syncmgr = NULL;
  const char* type_id_str = NULL;
	uint32_t type_id = 0;
  bool got_event = false;
	/*uint32_t* deleted_ids = NULL;
	size_t deleted_count = 0;*/
	
	synce_log_set_level(0);

	hr = CeRapiInit();
	if (FAILED(hr))
		goto exit;

	syncmgr = rra_syncmgr_new();

	if (!rra_syncmgr_connect(syncmgr))
	{
		fprintf(stderr, "Connection failed\n");
		goto exit;
	}

  if (argc < 2)
  {
    /* Subcribe to all types */
    unsigned i;
    uint32_t count = rra_syncmgr_get_type_count(syncmgr);
    RRA_SyncMgrType* types = rra_syncmgr_get_types(syncmgr);
    for (i = 0; i < count; i++)
    {
      rra_syncmgr_subscribe(syncmgr, types[i].id, callback, NULL);
    }
  }
  else
  {
    int i;

    for (i = 1; i < argc; i++)
    {
      RRA_SyncMgrType* type = NULL;

      type_id_str = argv[i];

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
	
	CeRapiUninit();
	return result;
}
