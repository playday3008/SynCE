/* $Id$ */
#define _BSD_SOURCE 1
#include "synce.h"
#include "synce_log.h"
#include "config/config.h"
#include <stdlib.h>
#include <string.h>

#define FREE(x)     if(x) free(x)

char *STRDUP(const char* str)
{ 
  return str ? strdup(str) : NULL;
}

SynceInfo* synce_info_new(const char* filename)
{
  SynceInfo* result = calloc(1, sizeof(SynceInfo));
  bool success = false;
  char* connection_filename;
 	struct configFile* config = NULL;
 
  if (filename)
    connection_filename = strdup(filename);
  else
    synce_get_connection_filename(&connection_filename);

	config = readConfigFile(connection_filename);
	if (!config)
	{
		synce_error("unable to open file: %s", connection_filename);
		goto exit;
	}

  result->dccm_pid        = getConfigInt(config, "dccm",   "pid");

  result->key             = getConfigInt(config, "device", "key");
  result->os_version      = getConfigInt(config, "device", "os_version");
  result->build_number    = getConfigInt(config, "device", "build_number");
  result->processor_type  = getConfigInt(config, "device", "processor_type");
  result->partner_id_1    = getConfigInt(config, "device", "partner_id_1");
  result->partner_id_2    = getConfigInt(config, "device", "partner_id_2");

  result->ip        = STRDUP(getConfigString(config, "device", "ip"));
  result->password  = STRDUP(getConfigString(config, "device", "password"));
  result->name      = STRDUP(getConfigString(config, "device", "name"));
  result->os_name   = STRDUP(getConfigString(config, "device", "os_name"));
  result->model     = STRDUP(getConfigString(config, "device", "model"));

  success = true;
  
exit:
  FREE(connection_filename);

  if (config)
    unloadConfigFile(config);

  if (success)
    return result;
  else 
  {
    synce_info_destroy(result);
    return NULL;
  }
}

void synce_info_destroy(SynceInfo* info)
{
  if (info)
  {
    FREE(info->ip);
    FREE(info->password);
    FREE(info->name);
    FREE(info->os_name);
    FREE(info->model);
    free(info);
  }
}

