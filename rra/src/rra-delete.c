/* $Id$ */
#define _BSD_SOURCE 1
#include "../lib/syncmgr.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	int result = 1;
	HRESULT hr;
	RRA_SyncMgr* syncmgr = NULL;
	const char* type_id_str = NULL;
	uint32_t type_id = 0;
	uint32_t object_id = 0;
	RRA_SyncMgrType* type = NULL;
  int i;
	
	/* synce_log_set_level(0); */

	if (argc < 3)
	{
		fprintf(stderr, "Syntax: %s TYPE-ID OBJECT-ID\n",
				argv[0]);
		goto exit;
	}

	type_id_str = argv[1];
	
	hr = CeRapiInit();
	if (FAILED(hr))
		goto exit;

	syncmgr = rra_syncmgr_new();

	if (!rra_syncmgr_connect(syncmgr))
	{
		fprintf(stderr, "Connection failed\n");
		goto exit;
	}

  type = rra_syncmgr_type_from_name(syncmgr, type_id_str);
  if (type)
    type_id = type->id;
  else
    type_id = strtol(type_id_str, NULL, 16);

  for (i = 2; argv[i] != NULL; i++) 
  {
    object_id = strtol(argv[i], NULL, 16);

    if (!rra_syncmgr_delete_object(syncmgr, type_id, object_id))
    {
      fprintf(stderr, "Failed to delete object %08x\n", object_id);
      goto exit;
    }
  }

	result = 0;
	
exit:
	rra_syncmgr_destroy(syncmgr);
	
	CeRapiUninit();
	return result;
}
