#define _BSD_SOURCE 1
#include "rapi.h"
#include "rapi_context.h"
#include "synce_log.h"
#include <stdlib.h>
#include <string.h>

struct _RapiConnection
{
  RapiContext* context;
};

RapiConnection* rapi_connection_create(const char* path)
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

    if (path)
      connection->context->path = strdup(path);
  }

  return connection;
}

/** Select what connection is used for RAPI calls */
void rapi_connection_select(RapiConnection* connection)
{
  if (connection)
  {
    synce_debug("Selecting context with path '%s'", 
        connection->context->path);
    rapi_context_set(connection->context);
  }
  else
    rapi_context_set(NULL);
}

/** Destroy RAPI connection - use this after CeRapiUninit() */
void rapi_connection_destroy(RapiConnection* connection)
{
  if (connection)
    free(connection);
}

