/* $Id$ */
#include <rapi.h>
#include <synce_log.h>
#include <librra.h>
#include <string.h>
#include "multisync_plugin.h"

uint32_t synce_object_type_to_id(RRA* rra, sync_object_type object_type)/*{{{*/
{
	uint32_t type_id = 0;
	
	switch (object_type)
	{
		case SYNC_OBJECT_TYPE_CALENDAR:  
			type_id = rra_type_id_from_name(rra, RRA_TYPE_NAME_APPOINTMENT); 
			break;
		case SYNC_OBJECT_TYPE_PHONEBOOK: 
			type_id = rra_type_id_from_name(rra, RRA_TYPE_NAME_CONTACT); 
			break;
		case SYNC_OBJECT_TYPE_TODO: 		 
			type_id = rra_type_id_from_name(rra, RRA_TYPE_NAME_TASK); 
			break;
		default:
			break;
	}

	return type_id;
}/*}}}*/

SynceConnection* synce_connection_new(sync_pair* handle, connection_type type)
{
	SynceConnection* sc = g_new0(SynceConnection, 1);

	sc->handle = handle;
	sc->type   = type;

/*	sync_set_requestdone(sc->handle);*/
	return sc;
}

void synce_connection_destroy(SynceConnection* sc)
{
  if (sc) 
  {
    rra_free(sc->rra);
    g_free(sc);
  }
}

static const char * STATE_FILE = "synce";

static char * get_state_filename(SynceConnection* sc)
{
  return g_strdup_printf(
      "%s/%s_%s", 
      sc->handle->datapath, 
      (sc->type == CONNECTION_TYPE_LOCAL ? "local" : "remote"), 
      STATE_FILE);
}

void synce_connection_load_state(SynceConnection* sc)
{
  char *filename;
  FILE *f;
  char line[256];

  filename = get_state_filename(sc);

  if ((f = fopen(filename, "r"))) 
  {
    while (fgets(line, 256, f)) 
    {
      char prop[128], data[256];
      if (sscanf(line, "%128s = %256[^\n]", prop, data) == 2)
      {
        if (!strcasecmp(prop, "get_all"))
          sc->get_all = (0 == strcasecmp(data, "yes"));
      }
    }
    
    fclose(f);
  }
  
  g_free(filename);
}
  
void synce_connection_save_state(SynceConnection* sc)
{
  char *filename;
  FILE *f;

  filename = get_state_filename(sc);

  if ((f = fopen(filename, "w"))) 
  {
    fprintf(f, "get_all = %s\n", sc->get_all ? "yes" : "no");
    fclose(f);
  }
  
  g_free(filename);
}
  
/**
 * Create connection object
 */
SynceConnection* sync_connect(sync_pair* handle, connection_type type) /*{{{*/
{
	SynceConnection* sc = synce_connection_new(handle, type);
	
  synce_trace("here");
	
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

  type_id = synce_object_type_to_id(rra, object_type);
  if (!type_id)
  {
    synce_error("unknown object type: %i", object_type);
    goto exit;
  }

  switch (object_type)
  {
    case SYNC_OBJECT_TYPE_CALENDAR: 
      if (!rra_appointment_from_vevent(
						object,
						NULL,
						&data,
						&data_size,
						(uid ? RRA_APPOINTMENT_UPDATE : RRA_APPOINTMENT_NEW) | RRA_APPOINTMENT_UTF8))
			{
				synce_error("Failed to create data");
				goto exit;
			}
			break;

		case SYNC_OBJECT_TYPE_PHONEBOOK: 
			if (!rra_contact_from_vcard(
						object,
						NULL,
						&data,
						&data_size,
						(uid ? RRA_CONTACT_UPDATE : RRA_CONTACT_NEW) | RRA_CONTACT_UTF8))
			{
				synce_error("Failed to create data");
				goto exit;
			}
			break;

		case SYNC_OBJECT_TYPE_TODO: 
			if (!rra_task_from_vtodo(
						object,
						NULL,
						&data,
						&data_size,
						(uid ? RRA_CONTACT_UPDATE : RRA_CONTACT_NEW) | RRA_CONTACT_UTF8))
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

	type_id = synce_object_type_to_id(rra, object_type);
	if (!type_id)
	{
			synce_error("Unknown object type: %i", object_type);
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
	return "synce-plugin";
}

const char* long_name()
{
	return "SynCE Plugin";
}

int object_types()
{
	return SYNC_OBJECT_TYPE_CALENDAR|SYNC_OBJECT_TYPE_PHONEBOOK|SYNC_OBJECT_TYPE_TODO;
}

void plugin_init()
{
	synce_trace("here");
}

const char* plugin_info()
{
	return "This plugin allows synchronization with a device running Windows CE or Pocket PC.";
}

int plugin_API_version(void) {
    return(2);
}
