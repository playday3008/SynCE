/* $Id$ */
#ifndef __multisync_plugin_h__
#define __multisync_plugin_h__

#include <syncengine.h>
#include <librra.h>

typedef struct 
{
	client_connection  client;
	sync_pair*         handle;
	connection_type 	 type;
	RRA*               rra;
} SynceConnection;

void synce_get_changes(SynceConnection* sc, int newdbs);
uint32_t synce_object_type_to_id(sync_object_type object_type);

#endif
