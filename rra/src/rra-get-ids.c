/* $Id$ */
#define _BSD_SOURCE 1
#include "syncmgr.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool callback (
    SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie)
{
  const char* event_str;
  unsigned i;

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

	for (i = 0; i < count; i++)
		printf("%08x   %08x  %s\n", type, ids[i], event_str);

  return true;
}

int main(int argc, char** argv)
{
	int result = 1;
	HRESULT hr;
	SyncMgr* syncmgr = NULL;
  const char* type_id_str = NULL;
	uint32_t type_id = 0;
  int i;
	/*uint32_t* deleted_ids = NULL;
	size_t deleted_count = 0;*/
	
/*	synce_log_set_level(0);*/

	if (argc < 2)
	{
		fprintf(stderr, "Missing object type id on command line\n");
		goto exit;
	}

	type_id_str = argv[1];
	
	hr = CeRapiInit();
	if (FAILED(hr))
		goto exit;

	syncmgr = syncmgr_new();

	if (!syncmgr_connect(syncmgr))
	{
		fprintf(stderr, "Connection failed\n");
		goto exit;
	}

  for (i = 1; i < argc; i++)
  {
    type_id_str = argv[i];

    type_id = syncmgr_type_from_name(syncmgr, type_id_str);
    if (type_id == 0xffffffff)
      type_id = strtol(type_id_str, NULL, 16);

    syncmgr_subscribe(syncmgr, type_id, callback, NULL);
  }
    
  if (!syncmgr_start_events(syncmgr))
  {
 		fprintf(stderr, "Failed to start events\n");
		goto exit;
  }

  /* Process all events triggered by syncmgr_start_events */
  while (syncmgr_event_wait(syncmgr, 3))
  {
    syncmgr_handle_event(syncmgr);
  }

exit:
	syncmgr_destroy(syncmgr);
	
	CeRapiUninit();
	return result;
}
