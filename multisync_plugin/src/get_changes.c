/* $Id$ */
#include "multisync_plugin.h"
#include <synce_log.h>
#include <rapi.h>

/**
 * Get changed objects
 */
static GList* synce_get_changed_objects(/*{{{*/
		SynceConnection* sc,
		sync_object_type object_type,
		GList* changes,
		bool everything)
{
	bool success = false;
	int id;
	
	/* RRA data */
	ObjectIdArray* object_ids  = NULL;
	uint32_t type_id           = 0;
	uint8_t* data              = NULL;
	size_t data_size           = 0;
	char* object_string        = NULL;
	uint32_t* deleted_ids      = NULL;
	size_t deleted_count       = 0;

	synce_trace("here");

	if (!sc)
	{
		synce_error("No connection");
		goto exit;
	}

	type_id = synce_object_type_to_id(sc->rra, object_type);
	if (!type_id)
	{
			synce_error("Unknown object type: %i", object_type);
			goto exit;
	}
	
	if (!rra_get_object_ids(sc->rra, type_id, &object_ids))
	{
		synce_error("Failed to get object ids");
		goto exit;
	}

	/* Select start ID */
	if (everything || sc->get_all)
		id = 0;
	else
		id = object_ids->unchanged;

	for (; id < (object_ids->unchanged + object_ids->changed); id++)
	{
		changed_object* change = NULL;
	
		synce_trace("id %08x", object_ids->ids[id]);
		
		if (!rra_object_get(
					sc->rra, 
					type_id, 
					object_ids->ids[id], 
					&data, 
					&data_size))
		{
			synce_error("Failed to get object");
			goto exit;
		}

		switch (object_type)
		{
			case SYNC_OBJECT_TYPE_CALENDAR: 
				if (!rra_appointment_to_vevent(
							object_ids->ids[id],
							data,
							data_size,
							&object_string,
							RRA_APPOINTMENT_UTF8))
				{
					fprintf(stderr, "Failed to create vCard\n");
					goto exit;
				}
				break;

			case SYNC_OBJECT_TYPE_PHONEBOOK: 
				if (!rra_contact_to_vcard(
							object_ids->ids[id],
							data,
							data_size,
							&object_string,
							RRA_CONTACT_VERSION_2_1 | RRA_CONTACT_UTF8))
				{
					fprintf(stderr, "Failed to create vCard\n");
					goto exit;
				}
				break;

			case SYNC_OBJECT_TYPE_TODO: 
				if (!rra_task_to_vtodo(
							object_ids->ids[id],
							data,
							data_size,
							&object_string,
							RRA_TASK_UTF8))
				{
					fprintf(stderr, "Failed to create vCard\n");
					goto exit;
				}
				break;

			default:
				synce_error("Unknown object type");
				goto exit;
		}

		change = g_new0(changed_object, 1);

		change->comp        = g_strdup(object_string);
		change->uid         = g_strdup_printf("%08x", object_ids->ids[id]);
		change->change_type = SYNC_OBJ_MODIFIED;
		change->object_type = object_type;

		changes = g_list_append(changes, change);
	
		free(data);
		free(object_string);
	}

	if (!rra_get_deleted_object_ids(
				sc->rra,
				type_id,
				object_ids,
				&deleted_ids,
				&deleted_count))
	{
		synce_error("Failed to get deleted object ids");
		goto exit;
	}
	
	for (id = 0; id < deleted_count; id++)
	{
		changed_object* change = g_new0(changed_object, 1);

		change->comp        = NULL;
		change->uid         = g_strdup_printf("%08x", deleted_ids[id]);
		change->change_type = SYNC_OBJ_HARDDELETED;
		change->object_type = object_type;

		changes = g_list_append(changes, change);
	}

	success = true;

exit:
	rra_free_object_ids(object_ids);
	rra_free_deleted_object_ids(deleted_ids);

	return changes;
}/*}}}*/

void synce_get_changes(SynceConnection* sc, int newdbs)/*{{{*/
{
	/* RAPI data */
	HRESULT hr;

	change_info* result = g_new0(change_info, 1);
	
	synce_trace("newdbs = %i", newdbs);

  synce_connection_load_state(sc);

	hr = CeRapiInit();
	if (FAILED(hr))
	{
		synce_error("Failed to initialize RAPI");
		goto exit;
	}

	sc->rra = rra_new();

	if (!rra_connect(sc->rra))
	{
		synce_error("Connection failed");
		goto exit;
	}
	
	result->newdbs  = newdbs;

	result->changes = synce_get_changed_objects(
			sc, 
			SYNC_OBJECT_TYPE_CALENDAR,
			result->changes, 
			newdbs & SYNC_OBJECT_TYPE_CALENDAR);

  /* XXX: disconnect and connect again to make it work?! */
	rra_free(sc->rra);
	sc->rra = rra_new();

	if (!rra_connect(sc->rra))
	{
		synce_error("Connection failed");
		goto exit;
	}
	
	result->changes = synce_get_changed_objects(
			sc, 
			SYNC_OBJECT_TYPE_PHONEBOOK,
			result->changes, 
			newdbs & SYNC_OBJECT_TYPE_PHONEBOOK);

  /* XXX: disconnect and connect again to make it work?! */
  rra_free(sc->rra);
  sc->rra = rra_new();

  if (!rra_connect(sc->rra))
  {
    synce_error("Connection failed");
    goto exit;
  }

  result->changes = synce_get_changed_objects(
      sc, 
      SYNC_OBJECT_TYPE_TODO,
      result->changes, 
      newdbs & SYNC_OBJECT_TYPE_TODO);
  
  sc->get_all = false;
  synce_connection_save_state(sc);

exit:
	sync_set_requestdata(result, sc->handle);

	rra_free(sc->rra);
	sc->rra = NULL;

	CeRapiUninit();
}/*}}}*/


