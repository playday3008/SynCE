/* $Id$ */
#include <rapi.h>
#include <synce_log.h>
#include <librra.h>
#include <string.h>
#include "multisync_plugin.h"

uint32_t synce_object_type_to_id(sync_object_type object_type)/*{{{*/
{
	uint32_t type_id = 0;
	
	switch (object_type)
	{
		case SYNC_OBJECT_TYPE_CALENDAR:  
			type_id = RRA_OBJECT_TYPE_APPOINTMENT; 
			break;
		case SYNC_OBJECT_TYPE_PHONEBOOK: 
			type_id = RRA_OBJECT_TYPE_CONTACT; 
			break;
		case SYNC_OBJECT_TYPE_TODO: 		 
			type_id = RRA_OBJECT_TYPE_TASK; 
			break;
		default:
			break;
	}

	return type_id;
}/*}}}*/

/**
 * Create connection object
 */
SynceConnection* sync_connect(sync_pair* handle, connection_type type) /*{{{*/
{
	SynceConnection* sc = g_new0(SynceConnection, 1);

	synce_trace("here");

	sc->handle = handle;
	sc->type   = type;

	sync_set_requestdone(sc->handle);
	return sc;
}/*}}}*/

typedef struct
{
	SynceConnection* sc;
	int newdbs;
} GetChangesParameters;

static gboolean get_changes_proxy(GetChangesParameters* p)
{
	synce_get_changes(p->sc, p->newdbs);
	return FALSE;
}

/**
 * Retrieve list of changes
 */
void get_changes(SynceConnection* sc, int newdbs)/*{{{*/
{
	GetChangesParameters* p = g_new0(GetChangesParameters, 1);

	synce_trace("here");

	p->sc     = sc;
	p->newdbs = newdbs;
	
	g_idle_add((GSourceFunc)get_changes_proxy, p);
}/*}}}*/

/**
 * Modify object
 */
void syncobj_modify(/*{{{*/
		SynceConnection* sc,
		const char* object,
		const char* uid,
		sync_object_type object_type,
		char* result_uid,
		int* result_uid_length)
{
	bool success = false;
	
	/* RAPI data */
	HRESULT hr;
	
	/* RRA data */
	RRA* rra                   = NULL;
	uint32_t object_id         = 0;
	uint32_t new_object_id     = 0;
	uint32_t type_id           = 0;
	uint8_t* data              = NULL;
	size_t data_size           = 0;

	synce_trace("uid='%s', type=%i", uid, object_type);

	type_id = synce_object_type_to_id(object_type);
	if (!type_id)
	{
			synce_error("Unknown object type");
			goto exit;
	}
	
	if (uid)
		object_id = strtol(uid, NULL, 16);
	else
		object_id = RRA_CONTACT_ID_UNKNOWN;
	
	hr = CeRapiInit();
	if (FAILED(hr))
	{
		synce_error("Failed to initialize RAPI");
		goto exit;
	}

	rra = rra_new();

	if (!rra_connect(rra))
	{
		synce_error("Connection failed");
		goto exit;
	}

	switch (object_type)
	{
		case SYNC_OBJECT_TYPE_PHONEBOOK: 
			if (!rra_contact_from_vcard(
						uid ? RRA_CONTACT_UPDATE : RRA_CONTACT_NEW,
						object,
						NULL,
						&data,
						&data_size))
			{
				synce_error("Failed to create data");
				goto exit;
			}
			break;

		default:
			synce_error("Unknown object type");
			goto exit;
	}

	if (!data_size)
	{
		synce_error("Empty object");
		goto exit;
	}

	if (!rra_object_put(
				rra,
				type_id,
				object_id,
				uid ? 0x40 : 2,
				data,
				data_size,
				&new_object_id))
	{
		synce_error("Failed to add/update object");
		goto exit;
	}

	if (!uid)
	{
		snprintf(result_uid, *result_uid_length, "%08x", new_object_id);
		*result_uid_length = strlen(result_uid);
	}

	success = true;

exit:
	rra_free(rra);
	CeRapiUninit();

	if (data)
		free(data);

	if (success)
		sync_set_requestdone(sc->handle);
	else
		sync_set_requestfailed(sc->handle);
}/*}}}*/

/**
 * Delete object
 */
void syncobj_delete(/*{{{*/
		SynceConnection* sc,
		const char* uid,
		sync_object_type object_type,
		int soft_delete)
{
	bool success = false;
	
	/* RAPI data */
	HRESULT hr;
	
	/* RRA data */
	RRA* rra                   = NULL;
	uint32_t object_id         = 0;
	uint32_t type_id           = 0;

	synce_trace("uid='%s', type=%i", uid, object_type);

	type_id = synce_object_type_to_id(object_type);
	if (!type_id)
	{
			synce_error("Unknown object type");
			goto exit;
	}
	
	if (uid)
		object_id = strtol(uid, NULL, 16);
	else
		object_id = RRA_CONTACT_ID_UNKNOWN;
	
	hr = CeRapiInit();
	if (FAILED(hr))
	{
		synce_error("Failed to initialize RAPI");
		goto exit;
	}

	rra = rra_new();

	if (!rra_connect(rra))
	{
		synce_error("Connection failed");
		goto exit;
	}

	if (!rra_object_delete(
				rra,
				type_id,
				object_id))
	{
		synce_error("Failed to delete object");
		goto exit;
	}

	success = true;

exit:
	rra_free(rra);
	CeRapiUninit();

	if (success)
		sync_set_requestdone(sc->handle);
	else
		sync_set_requestfailed(sc->handle);
}/*}}}*/

gboolean always_connected()
{
	return TRUE;
}

const char* short_name()
{
	return "spfms";
}

const char* long_name()
{
	return "SynCE Plugin";
}

int object_types()
{
	return SYNC_OBJECT_TYPE_PHONEBOOK;
}

void plugin_init()
{
	synce_trace("here");
}

const char* plugin_info()
{
	return "This plugin allows synchronization with a device running Windows CE or Pocket PC.";
}

