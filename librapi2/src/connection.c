#define _BSD_SOURCE 1
#include "rapi.h"
#include "rapi_context.h"
#include "synce_log.h"
#include <stdlib.h>
#include <string.h>

struct _RapiConnection
{
  RapiContext* context;
  /* if the RapiConnection owns the SynceInfo object, it's here */
  SynceInfo* owned_info;
};

RapiConnection* rapi_connection_from_name(const char* device_name)
{
  SynceInfo* info = synce_info_new(device_name);
  if (info == NULL)
    return NULL;
  RapiConnection* result = rapi_connection_from_info(info);
  if (result)
    result->owned_info = info;

  return result;
}

RapiConnection* rapi_connection_from_info(SynceInfo* info)
{
  RapiConnection* connection =
    (RapiConnection*)calloc(1, sizeof(RapiConnection));

  if (connection)
  {
    connection->context = rapi_context_new();
    if (!connection->context)
    {
      synce_error("Failed to create RapiContext object");
      free(connection);
      return NULL;
    }

    connection->context->info = info;
  }

  return connection;
}

/** Select what connection is used for RAPI calls */
void rapi_connection_select(RapiConnection* connection)
{
  if (connection)
  {
    rapi_context_set(connection->context);
  }
  else
    rapi_context_set(NULL);
}

/** Destroy RAPI connection - use this after CeRapiUninit() */
void rapi_connection_destroy(RapiConnection* connection)
{
  if (connection)
  {
    synce_info_destroy(connection->owned_info);
    free(connection);
  }
}

