/* $Id$ */
#ifndef __multisync_plugin_h__
#define __multisync_plugin_h__

#include <multisync.h>
#include <librra.h>

typedef struct 
{
	client_connection  client;
	sync_pair*         handle;
	connection_type 	 type;
	RRA*               rra;
  bool               get_all;
} SynceConnection;

SynceConnection* synce_connection_new(sync_pair* handle, connection_type type);
void synce_connection_destroy(SynceConnection* sc);

void synce_connection_load_state(SynceConnection* sc);
void synce_connection_save_state(SynceConnection* sc);

void synce_get_changes(SynceConnection* sc, int newdbs);
uint32_t synce_object_type_to_id(RRA* rra, sync_object_type object_type);

#endif
