/* $Id$ */
#include <syncengine.h>
#include <rapi.h>
#include <synce_log.h>
#include <librra.h>
#include <string.h>

typedef struct 
{
	client_connection  client;
	sync_pair*         handle;
	connection_type 	 type;
} SynceConnection;

/**
 * Create connection object
 */
SynceConnection* sync_connect(sync_pair* handle, connection_type type) 
{
	SynceConnection* sc = g_new0(SynceConnection, 1);

	synce_trace("here");

	sc->handle = handle;
	sc->type   = type;

	sync_set_requestdone(sc->handle);
	return sc;
}

/**
 * Get changed phonebook entries (contacts)
 */
static GList* get_phonebook_changes(
		SynceConnection* sc, 
		GList* changes,
		bool everything)
{
	bool success = false;
	int id;
	
	/* RAPI data */
	HRESULT hr;
	
	/* RRA data */
	RRA* rra                   = NULL;
	ObjectIdArray* object_ids  = NULL;
	uint32_t type_id           = 0x2712;
	uint8_t* data              = NULL;
	size_t data_size           = 0;
	char* vcard                = NULL;


	synce_trace("here");
	
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
	
	if (!rra_get_object_ids(rra, 0x2712, &object_ids))
	{
		synce_error("Failed to get object ids");
		goto exit;
	}

	/* Select start ID */
	if (everything)
		id = 0;
	else
		id = object_ids->unchanged;

	for (; id < (object_ids->unchanged + object_ids->changed); id++)
	{
		changed_object* change = NULL;
	
		synce_trace("id %08x", object_ids->ids[id]);
		
		if (!rra_object_get(rra, type_id, object_ids->ids[id], &data, &data_size))
		{
			synce_error("Failed to get object");
			goto exit;
		}

		if (!rra_contact_to_vcard(
					object_ids->ids[id++],
					data,
					data_size,
					RRA_CONTACT_VCARD_3_0,
					&vcard))
		{
			fprintf(stderr, "Failed to create vCard\n");
			goto exit;
		}

		change = g_new0(changed_object, 1);

		change->comp        = g_strdup(vcard);
		change->uid         = g_strdup_printf("%08x", object_ids->ids[id]);
		change->change_type = SYNC_OBJ_MODIFIED;
		change->object_type = SYNC_OBJECT_TYPE_PHONEBOOK;

		changes = g_list_append(changes, change);
	
		free(data);
		free(vcard);
	}

	success = true;

exit:
	rra_free(rra);
	CeRapiUninit();

	return changes;
}

typedef struct
{
	SynceConnection* sc;
	int newdbs;
} GetChangesParameters;

static gboolean get_changes_real(GetChangesParameters* p)
{
	change_info* result = g_new0(change_info, 1);
	
	synce_trace("here");

	result->newdbs = p->newdbs;

	result->changes = get_phonebook_changes(
			p->sc, 
			result->changes, 
			p->newdbs & SYNC_OBJECT_TYPE_PHONEBOOK);

	sync_set_requestdata(result, p->sc->handle);

	return FALSE;
}

/**
 * Retrieve list of changes
 */
void get_changes(SynceConnection* sc, int newdbs)
{
	GetChangesParameters* p = g_new0(GetChangesParameters, 1);
	p->sc     = sc;
	p->newdbs = newdbs;
	
	synce_trace("here");

	g_idle_add((GSourceFunc)get_changes_real, p);
}

/**
 * Modify object
 */
void syncobj_modify(
		SynceConnection* sc,
		const char* object,
		const char* uid,
		sync_object_type type,
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
	uint32_t type_id           = 0x2712;
	uint8_t* data              = NULL;
	size_t data_size           = 0;

	synce_trace("uid='%s', type=%i", uid, type);

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

	sync_set_requestfailed(sc->handle);
}

/**
 * Delete object
 */
void syncobj_delete(
		SynceConnection* sc,
		const char* uid,
		sync_object_type type,
		int soft_delete)
{
	synce_trace("here");

	sync_set_requestfailed(sc->handle);
}

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
	return SYNC_OBJECT_TYPE_PHONEBOOK;;
}

void plugin_init()
{
	synce_trace("here");
}

const char* plugin_info()
{
	return "This plugin allows synchronization with a device running Windows CE or Pocket PC.";
}

