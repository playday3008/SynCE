/* $Id$ */
#define _BSD_SOURCE 1
#define _SVID_SOURCE 1
#include "librra.h"
#include "rrac.h"
#include "timezone.h"
#include <rapi.h>
#include <synce_ini.h>
#include <synce_log.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define SEND_COMMAND_6F_6   1
#define SEND_COMMAND_6F_10  0
#define SEND_COMMAND_70_2   0
#define SEND_COMMAND_70_3   1

struct _RRA
{
  RRAC*         rrac;

	ObjectType*   object_types;
	size_t        object_type_count;

	HKEY          partners_key;

  bool                  has_tzi;
  TimeZoneInformation   tzi;
  char*                 time_zone_id;
};

static const char* PARTNERS =
	"Software\\Microsoft\\Windows CE Services\\Partners";

static const char* CURRENT_PARTNER  = "PCur";
static const char* PARTNER_ID       = "PId";
static const char* PARTNER_NAME     = "PName";

static const char* RRA_DIRECTORY    = "rra";

static const char* PARTERSHIP_FILENAME = "partner";
static const char* PARTERSHIP_SECTION  = "partnership";

RRA* rra_new()/*{{{*/
{
	RRA* rra = (RRA*)calloc(1, sizeof(RRA));
  rra->rrac = rrac_new();
  return rra;
}/*}}}*/

void rra_free(RRA* rra)/*{{{*/
{
	if (rra)
	{
		rra_disconnect(rra);

		if (rra->object_types)
		{
			free(rra->object_types);
			rra->object_types = NULL;
		}

    if (rra->time_zone_id)
      free(rra->time_zone_id);

    rrac_free(rra->rrac);
		free(rra);
	}
}/*}}}*/

bool rra_connect(RRA* rra)/*{{{*/
{
  if (rra && rra->rrac)
    return rrac_connect(rra->rrac);
  else 
  {
    synce_error("Bad RRA object");
    return false;
  }
}/*}}}*/

void rra_disconnect(RRA* rra)/*{{{*/
{
	if (rra)
	{
		if (rra->partners_key)
		{
			CeRegCloseKey(rra->partners_key);
			rra->partners_key = 0;
		}

    rrac_disconnect(rra->rrac);
	}
}/*}}}*/

bool rra_get_object_types(RRA* rra, /*{{{*/
                          ObjectType** object_types,
                          size_t* object_type_count)
{
	bool success = false;
	RawObjectType* raw_object_types = NULL;

	if (!rra->object_types)
	{
		unsigned i = 0;

		if (!rrac_send_6f(rra->rrac, 0x3c1))
		{
			synce_error("Failed to send command 6f");
			goto exit;
		}

		if (!rrac_recv_reply_6f_c1(
					rra->rrac, 
					&raw_object_types, 
					&rra->object_type_count))
		{
			synce_error("Failed to receive reply");
			goto exit;
		}

    rra->object_types = malloc(rra->object_type_count * sizeof(ObjectType));

    for (i = 0; i < rra->object_type_count; i++)
    {
      char* ascii = NULL;

      rra->object_types[i].id          = raw_object_types[i].id;
      rra->object_types[i].count       = raw_object_types[i].count;
      rra->object_types[i].total_size  = raw_object_types[i].total_size;
      rra->object_types[i].modified    = 
        filetime_to_unix_time(&raw_object_types[i].filetime);

      ascii = wstr_to_ascii(raw_object_types[i].name1);
      strcpy(rra->object_types[i].name, ascii);
      wstr_free_string(ascii);
    }
  }

	if (object_types)	
		*object_types       = rra->object_types;
	if (object_type_count) 
		*object_type_count  = rra->object_type_count;

	success = true;

exit:
	rrac_free(raw_object_types);
	return success;
}/*}}}*/

uint32_t rra_type_id_from_name(/*{{{*/
    RRA* rra,
    const char* name)
{
  uint32_t result = 0;
  size_t i;
  
  if (!rra_get_object_types(rra, NULL, NULL))
  {
    synce_error("rra_get_object_types failed");
    goto exit;
  }

  for (i = 0; i < rra->object_type_count; i++)
  {
    if (0 == strcasecmp(name, rra->object_types[i].name))
    {
      result = rra->object_types[i].id;
      break;
    }
  }

exit:
  return result;
}/*}}}*/

#if 0
bool rra_get_changes(/*{{{*/
    RRA* rra, 
    uint32_t* object_type_ids, 
    size_t object_type_count, 
    NotificationFunc func,
    void* cookie)
{
  bool success = false;
	unsigned i, ignored_count;
	uint32_t* ignored_ids = NULL;
  ObjectType* object_types = NULL;
	size_t total_object_type_count = 0;
  ObjectIdArray object_id_array;
  uint32_t recv_subcommand = 0;

	if (!rra_get_object_types(rra, &object_types, &total_object_type_count))
	{
		synce_error("Failed to get object types");
		goto exit;
	}

	/*
	 * Ignore object types
	 */

	ignored_ids = malloc(object_type_count * sizeof(uint32_t));

  /* O(2) loop but with so few entries I don't care */
	for (i = 0, ignored_count = 0; i < total_object_type_count; i++)
	{
    unsigned j;

    for (j = 0; j < object_type_count; j++)
    {
      if (object_types[i].id == object_type_ids[j])
        break;
    }

    if (j == object_type_count)
    {
      ignored_ids[ignored_count++] = object_types[i].id;
    }
  }

#if SEND_COMMAND_70_3
	rrac_send_70_3(rra->rrac, ignored_ids, ignored_count);

	if (!rrac_recv_reply_70(rra->rrac))
	{
		goto exit;
	}
#endif
  
	/*
	 * Receive this first
	 */

	if (!rrac_recv_69_2(rra->rrac))
	{
		synce_trace("rrac_recv_69_2 failed");
		goto exit;
	}

	/*
	 * Receive object ids
	 */

	while (recv_subcommand != 0x06000000)
	{
    uint32_t recv_type_id;
    uint32_t recv_some_count;
    uint32_t* recv_ids;
    uint32_t recv_id_count;

		if (!rrac_recv_69_not_2(
					rra->rrac, 
					&recv_subcommand,
					&recv_type_id,
					&recv_some_count,
					&recv_ids,
					&recv_id_count))
		{
			synce_trace("rrac_recv_69_not_2 failed");
			goto exit;
		}

		if (0x04000000 == recv_subcommand ||
				0x06000000 == recv_subcommand)
		{
			unsigned j = 0;

			synce_trace("subcommand %08x, changed count=%i, total count=%i",
					recv_subcommand, recv_some_count, recv_id_count);

			if (recv_id_count)
			{
				synce_trace("unchanged ids:");
				for (i = 0; i < (recv_id_count-recv_some_count); i++, j++)
				{
					synce_trace("id[%i] = %08x", j, recv_ids[j]);
				}

				synce_trace("changed ids:");
				for (i = 0; i < recv_some_count; i++, j++)
				{
					synce_trace("id[%i] = %08x", j, recv_ids[j]);
				}

        object_id_array.ids       = recv_ids;
				object_id_array.unchanged = recv_id_count - recv_some_count;
				object_id_array.changed   = recv_some_count;
        func(recv_type_id, &object_id_array, cookie);
			}
		}
		else
		{
			synce_trace("unexpected subcommand %08x, some_count=%i, id_count=%i",
					recv_subcommand, recv_some_count, recv_id_count);

			for (i = 0; i < recv_id_count; i++)
			{
				synce_trace("id[%i] = %08x", i, recv_ids[i]);
			}
		}

    rrac_free(recv_ids);
  }

  success = true;
  
exit:
	if (ignored_ids)
		free(ignored_ids);
  return success;
}/*}}}*/
#endif

bool rra_get_object_ids(RRA* rra,/*{{{*/
                        uint32_t object_type_id,
                        ObjectIdArray** object_id_array)
{
	bool success = false;
	unsigned i, ignored_count;
	uint32_t* ignored_ids = NULL;
  ObjectType* object_types = NULL;
	size_t object_type_count = 0;
	uint32_t recv_subcommand = 0;
	uint32_t recv_type_id;
	uint32_t recv_some_count;
	uint32_t* recv_ids;
	uint32_t recv_id_count;

	*object_id_array = NULL;

	if (!rra_get_object_types(rra, &object_types, &object_type_count))
	{
		synce_error("Failed to get object types");
		goto exit;
	}

	/*
	 * Ignore object types
	 */

	ignored_ids = malloc(object_type_count * sizeof(uint32_t));

	for (i = 0, ignored_count = 0; i < object_type_count; i++)
	{
		if (object_types[i].id != object_type_id)
		{
			ignored_ids[ignored_count++] = object_types[i].id;
		}
	}

	if (i == ignored_count)
	{
		synce_error("Unknown object type");
		goto exit;
	}
	
#if SEND_COMMAND_70_3
	rrac_send_70_3(rra->rrac, ignored_ids, ignored_count);

	if (!rrac_recv_reply_70(rra->rrac))
	{
		goto exit;
	}
#endif

	/*
	 * test
	 */

#if SEND_COMMAND_6F_10
	if (!rrac_send_6f(rra->rrac, 0x10))
	{
		goto exit;
	}
#endif

	/*
	 * Receive object ids
	 */

	if (!rrac_recv_69_2(rra->rrac))
	{
		synce_trace("rrac_recv_69_2 failed");
		goto exit;
	}
					
	*object_id_array = calloc(1, sizeof(ObjectIdArray));

	while (recv_subcommand != 0x06000000)
	{
		unsigned i;
		
		if (!rrac_recv_69_not_2(
					rra->rrac, 
					&recv_subcommand,
					&recv_type_id,
					&recv_some_count,
					&recv_ids,
					&recv_id_count))
		{
			synce_trace("rrac_recv_69_not_2 failed");
			goto exit;
		}

		if (recv_type_id != object_type_id)
		{
			synce_warning("recv_type_id = %08x but object_type_id = %08x",
					recv_type_id, object_type_id );

			/* try next message */
			continue;
		}

		if (0x04000000 == recv_subcommand ||
				0x06000000 == recv_subcommand)
		{
			unsigned j = 0;

			synce_trace("subcommand %08x, changed count=%i, total count=%i",
					recv_subcommand, recv_some_count, recv_id_count);

			if (recv_id_count)
			{
				synce_trace("unchanged ids:");
				for (i = 0; i < (recv_id_count-recv_some_count); i++, j++)
				{
					synce_trace("id[%i] = %08x", j, recv_ids[j]);
				}

				synce_trace("changed ids:");
				for (i = 0; i < recv_some_count; i++, j++)
				{
					synce_trace("id[%i] = %08x", j, recv_ids[j]);
				}

				if ((**object_id_array).ids)
				{
					synce_warning("Already have an array of ids!");
				}
				else
				{
					(**object_id_array).ids        = recv_ids;
					(**object_id_array).unchanged  = recv_id_count - recv_some_count;
					(**object_id_array).changed    = recv_some_count;
				}
			}
		}
		else
		{
			synce_trace("subcommand %08x, some_count=%i, id_count=%i",
					recv_subcommand, recv_some_count, recv_id_count);

			for (i = 0; i < recv_id_count; i++)
			{
				synce_trace("id[%i] = %08x", i, recv_ids[i]);
			}
		}
		
	}

#if SEND_COMMAND_6F_10
	if (!rrac_recv_reply_6f_10(rra->rrac))
	{
		goto exit;
	}
#endif

#if 0 && SEND_COMMAND_6F_6
	if (!rrac_send_6f(rra->rrac, 6))
	{
		goto exit;
	}

	if (!rrac_recv_reply_6f_6(rra->rrac))
	{
		goto exit;
	}
#endif
	
	success = true;

exit:
	if (ignored_ids)
		free(ignored_ids);

	if (!success && *object_id_array)
	{
		free(*object_id_array);
		*object_id_array = NULL;
	}

	return success;
}/*}}}*/

void rra_free_object_ids(ObjectIdArray* object_id_array)/*{{{*/
{
	if (object_id_array)
	{
		if (object_id_array->ids)
			free(object_id_array->ids);
		free(object_id_array);
	}
}/*}}}*/


typedef struct _Uint32Vector
{
	uint32_t* items;
	size_t used;
	size_t size;
} Uint32Vector;

static void uint32vector_enlarge(Uint32Vector* v, size_t size)
{
	if (v->size < size)
	{
		size_t new_size = v->size ? v->size : 2;

		while (new_size < size)
			new_size <<= 1;

		v->items = realloc(v->items, sizeof(uint32_t) * new_size);
		if (!v->items)
		{
			synce_error("Failed to allocate space for %i elements - crashing!", new_size);
		}

		v->size  = new_size;
	}
}

Uint32Vector* uint32vector_new()
{
	return (Uint32Vector*)calloc(1, sizeof(Uint32Vector));
}

void uint32vector_destroy(Uint32Vector* v, bool free_items)
{
	if (v)
	{
		if (free_items && v->items)
			free(v->items);
		free(v);
	}
}

Uint32Vector* uint32vector_add(Uint32Vector* v, uint32_t value)
{
	uint32vector_enlarge(v, v->used + 1);
	v->items[v->used++] = value;
	return v;
}

static int uint32vector_compare(const void* a, const void* b)
{
	return *(const uint32_t*)a - *(const uint32_t*)b;
}

void uint32vector_sort(Uint32Vector* v)
{
	qsort(v->items, v->used, sizeof(uint32_t), uint32vector_compare);
}

void uint32vector_dump(Uint32Vector* v)
{
	unsigned i;
	for (i = 0; i < v->used; i++)
		synce_trace("%i: %08x", i, v->items[i]);
}



/*
 * This function can be extended to find out which objects are new too.
 * 
 * Maybe not be the most optimal way to do this, but it will do for now.
 */
bool rra_get_deleted_object_ids(RRA* rra,/*{{{*/
                                uint32_t object_type_id,
		                            ObjectIdArray* object_id_array,
																uint32_t** deleted_id_array,
																size_t* deleted_count)
{
	bool success = false;
	char* directory = NULL;
	uint32_t index = 0;
	uint32_t partner_id = 0;
	char filename[256];
	FILE* file = NULL;
	unsigned previous, current;
	Uint32Vector* previous_ids = uint32vector_new();
	Uint32Vector* current_ids  = uint32vector_new();
	Uint32Vector* deleted_ids  = uint32vector_new();

	if (!rra_partner_get_current(rra, &index))
	{
		synce_error("Failed to get current partner index");
		goto exit;
	}

	/* XXX: maybe the partner ID should be stored in RRA struct? */
	if (!rra_partner_get_id(rra, index, &partner_id))
	{
		synce_error("Failed to get current partner ID");
		goto exit;
	}

	if (!synce_get_subdirectory(RRA_DIRECTORY, &directory))
	{
		synce_error("Failed to get rra directory path");
		goto exit;
	}

	snprintf(filename, sizeof(filename), "%s/partner-%08x-type-%08x", directory,
			partner_id, object_type_id);


	/*
	 * Read previous list of IDs to vector
	 */
	
	file = fopen(filename, "r");
	if (file)
	{
		char buffer[16];
		while (fgets(buffer, sizeof(buffer), file))
		{
			uint32vector_add(previous_ids, strtol(buffer, NULL, 16));
		}

		/* uint32vector_dump(previous_ids); */
		uint32vector_sort(previous_ids);
		/* uint32vector_dump(previous_ids); */
		fclose(file);
	}

	/*
	 * Take obejcts from array and put in vector
	 */

	for (current = 0; 
			 current < (object_id_array->unchanged + object_id_array->changed); 
			 current++)
	{
		uint32vector_add(current_ids, object_id_array->ids[current]);
	}
	
	/* uint32vector_dump(current_ids); */
	uint32vector_sort(current_ids);
	/* uint32vector_dump(current_ids); */

	/*
	 * Iterate both vectors and see what is missing from the previous
	 */

	for (current = 0, previous = 0;
			 current < current_ids->used && previous < previous_ids->used; )
	{
		/* synce_trace("current id: %08x    previous id: %08x", 
				current_ids->items[current], previous_ids->items[previous]); */

		if (current_ids->items[current] > previous_ids->items[previous])
		{
			/* deleted item */
			uint32vector_add(deleted_ids, previous_ids->items[previous]);
			/* synce_trace("deleted item: %08x", previous_ids->items[previous]); */
			previous++;
		}
		else if (current_ids->items[current] < previous_ids->items[previous])
		{
			/* new item */
			current++;
		}
		else
		{
			/* the IDs are equal */
			current++;
			previous++;
		}
	}

	/*
	 * Check if there are any IDs left at the end of the vector
	 */

	for (; previous < previous_ids->used; previous++)
	{
		uint32vector_add(deleted_ids, previous_ids->items[previous]);
		/* synce_trace("deleted item: %08x", previous_ids->items[previous]); */
	}

	/*
	 * Save current ID list
	 */

	file = fopen(filename, "w");
	if (!file)
	{
		synce_error("Failed to open '%s' for writing.", filename);
		goto exit;
	}
	
	if (file)
	{
		for (current = 0; current < current_ids->used; current++)
		{
			char buffer[16];
			snprintf(buffer, sizeof(buffer), "%08x\n", current_ids->items[current]);
			fwrite(buffer, strlen(buffer), 1, file);
		}
				
		fclose(file);
	}

	/*
	 * Return deleted ID array
	 */

	*deleted_id_array = deleted_ids->items;
	*deleted_count    = deleted_ids->used;

	success = true;
	
exit:
	if (directory)
		free(directory);

	uint32vector_destroy(current_ids,  true);
	uint32vector_destroy(previous_ids, true);
	uint32vector_destroy(deleted_ids,  !success);

	return success;
}/*}}}*/

bool rra_object_get(RRA* rra, /*{{{*/
                    uint32_t type_id,
                    uint32_t object_id,
                    uint8_t** data,
                    size_t* data_size)
{
	bool success = false;
	uint32_t recv_object_id;
	uint32_t recv_type_id;

#if SEND_COMMAND_70_2
	/* Lock data? */
	if (!rrac_send_70_2(rra->rrac, 1))
	{
		goto exit;
	}

	if (!rrac_recv_reply_70(rra->rrac))
	{
		goto exit;
	}
#endif
	
	/* Ask for object data */
	if (!rrac_send_67(rra->rrac, type_id, &object_id, 1))
	{
		goto exit;
	}

	/* Receive object data */
	if (!rrac_recv_data(rra->rrac, &recv_object_id, &recv_type_id, data, data_size))
	{
		goto exit;
	}

	/* Received end-of-data object */
	if (!rrac_recv_data(rra->rrac, NULL, NULL, NULL, NULL))
	{
		synce_error("rrac_recv_data failed");
		goto exit;
	}
	
	/* This command will mark the packet as unchanged */	
	if (!rrac_send_65(
				rra->rrac, 
				type_id, 
				recv_object_id, 
				recv_object_id,
				0))
	{
		synce_error("Failed to send command 65");
		goto exit;
	}
	
#if SEND_COMMAND_70_2
	/* Unlock data? */
	if (!rrac_send_70_2(rra->rrac, 2))
	{
		goto exit;
	}
#endif

#if SEND_COMMAND_6F_6
	if (!rrac_send_6f(rra->rrac, 6))
	{
		synce_error("rrac_send_6f failed");
		goto exit;
	}
#endif

#if SEND_COMMAND_70_2
	if (!rrac_recv_reply_70(rra->rrac))
	{
		goto exit;
	}
#endif

#if SEND_COMMAND_6F_6
	if (!rrac_recv_reply_6f_6(rra->rrac))
	{
		synce_error("rrac_recv_reply_6f_6 failed");
		goto exit;
	}
#endif

	if (type_id   != recv_type_id ||
			object_id != recv_object_id)
	{
		synce_error("Unexpected object received");
		goto exit;
	}
	
	success = true;

exit:
	return success;
}/*}}}*/

void rra_object_free_data(uint8_t* data);

bool rra_object_put(RRA* rra,  /*{{{*/
                    uint32_t type_id,
                    uint32_t object_id,
										uint32_t flags,
										uint8_t* data,
                    size_t data_size,
										uint32_t* new_object_id)
{
	bool success = false;
	uint32_t recv_type_id;
	uint32_t recv_object_id1;
	uint32_t recv_object_id2;
	uint32_t recv_flags;

#if SEND_COMMAND_70_2
	/* Lock data? */
	if (!rrac_send_70_2(rra->rrac, 1))
	{
		goto exit;
	}

	if (!rrac_recv_reply_70(rra->rrac))
	{
		goto exit;
	}
#endif
	
	if (!rrac_send_data(
				rra->rrac, 
				object_id, 
				type_id, 
				flags, 
				data, 
				data_size))
	{
		synce_error("Failed to send data");
		goto exit;	
	}

	if (!rrac_send_data(rra->rrac, OBJECT_ID_STOP, type_id, 0, NULL, 0))
	{
		synce_error("Failed to send stop entry");
		goto exit;	
	}

	if (!rrac_recv_65(
				rra->rrac, 
				&recv_type_id, 
				&recv_object_id1, 
				&recv_object_id2,
				&recv_flags))
	{
		synce_error("Failed to receive command 65");
		goto exit;
	}

	if (recv_type_id != type_id || recv_object_id1 != object_id)
	{
		synce_error("Unexpected type or object id");
		goto exit;
	}

	if (recv_flags != 2)
	{
		synce_warning("Unexpected flags: %08x", recv_flags);
	}

#if 1
	if (recv_object_id1 != recv_object_id2)
	{
		if (!rrac_send_65(
					rra->rrac, 
					type_id, 
					recv_object_id2, 
					recv_object_id2,
					0x08000000))
		{
			synce_error("Failed to send command 65");
			goto exit;
		}
	}
#endif

#if SEND_COMMAND_70_2
	/* Unlock data? */
	if (!rrac_send_70_2(rra->rrac, 2))
	{
		goto exit;
	}
#endif

#if SEND_COMMAND_6F_6
	if (!rrac_send_6f(rra->rrac, 6))
	{
		goto exit;
	}
#endif

#if SEND_COMMAND_70_2
	if (!rrac_recv_reply_70(rra->rrac))
	{
		goto exit;
	}
#endif

#if SEND_COMMAND_6F_6
	if (!rrac_recv_reply_6f_6(rra->rrac))
	{
		goto exit;
	}
#endif

	if (new_object_id)
		*new_object_id = recv_object_id2;

	success = true;

exit:
	return success;
}/*}}}*/

bool rra_object_new(RRA* rra,  /*{{{*/
                    uint32_t type_id,
                    uint8_t* data,
                    size_t data_size,
									  uint32_t* new_object_id)
{
	uint32_t object_id = (uint32_t)time(NULL);

	return rra_object_put(
			rra, 
			type_id,
			object_id,
			2,
			data,
			data_size,
			new_object_id);
}/*}}}*/

bool rra_object_update(RRA* rra,  /*{{{*/
                       uint32_t type_id,
                       uint32_t object_id,
                       uint8_t* data,
                       size_t data_size)
{
	uint32_t new_object_id;
	bool success = rra_object_put(
			rra, 
			type_id,
			object_id,
			0x40,
			data,
			data_size,
			&new_object_id);

	if (success && object_id != new_object_id)
	{
		synce_error("Update of object with id %08x changed the id to %08x",
				object_id, new_object_id);
		success = false;
	}

	return success;
}/*}}}*/

bool rra_object_delete(RRA* rra, uint32_t type_id, uint32_t object_id)/*{{{*/
{
	bool success = false;
	uint32_t recv_type_id;
	uint32_t recv_object_id1;
	uint32_t recv_object_id2;
	uint32_t recv_flags;
	uint32_t index = 1;

	if (!rrac_send_66(rra->rrac, type_id, object_id, index))
	{
		synce_error("Failed to senmd command 66");
		goto exit;
	}

	if (!rrac_recv_65(
				rra->rrac, 
				&recv_type_id, 
				&recv_object_id1,
				&recv_object_id2,
				&recv_flags))
	{
		synce_error("Failed to receive command 65");
		goto exit;
	}

	if (recv_object_id1 != recv_object_id2)
	{
		synce_error("Unexpected object ids");
		goto exit;
	}

	if (recv_flags != (index | 0x80000000))
	{
		synce_warning("Unexpected flags: %08x", recv_flags);
	}

	success = true;

exit:
	return success;
}/*}}}*/

#if 0
bool rra_lock(RRA* rra)
{
	return 
		rrac_send_70_2(rra->rrac, 1) &&
		rrac_recv_reply_70(rra->rrac);
}

bool rra_unlock(RRA* rra)
{
	return 
		rrac_send_70_2(rra->rrac, 2) &&
		rrac_recv_reply_70(rra->rrac);
}
#endif

static bool rra_partner_init(RRA* rra)
{
	if (!rra->partners_key)
	{
		return rapi_reg_create_key(
				HKEY_LOCAL_MACHINE, 
				PARTNERS, 
				&rra->partners_key);
	}

	return true;
}

static bool rra_partner_create_key(RRA* rra, uint32_t index, HKEY* partner_key)
{
	char name[MAX_PATH];
	snprintf(name, sizeof(name), "%s\\P%i", PARTNERS, index);
	return rapi_reg_create_key(HKEY_LOCAL_MACHINE, name, partner_key);
}

static bool rra_partner_open_key(RRA* rra, uint32_t index, HKEY* partner_key)
{
	char name[MAX_PATH];
	snprintf(name, sizeof(name), "%s\\P%i", PARTNERS, index);
	return rapi_reg_open_key(HKEY_LOCAL_MACHINE, name, partner_key);
}

bool rra_partner_set_current(RRA* rra, uint32_t index)
{
	return 
		(index == 1 || index == 2) &&
		rra_partner_init(rra) &&
		rapi_reg_set_dword(rra->partners_key, CURRENT_PARTNER, index);
}

bool rra_partner_get_current(RRA* rra, uint32_t* index)
{
	return 
		rra_partner_init(rra) &&
		rapi_reg_query_dword(rra->partners_key, CURRENT_PARTNER, index);
}

bool rra_partner_set_id(RRA* rra, uint32_t index, uint32_t id)
{
	HKEY partner_key = 0;

	bool success = 
		(index == 1 || index == 2) &&
		rra_partner_init(rra) &&
		rra_partner_create_key(rra, index, &partner_key) &&
		rapi_reg_set_dword(partner_key, PARTNER_ID, id);

	if (partner_key)
		CeRegCloseKey(partner_key);

	return success;
}

bool rra_partner_get_id(RRA* rra, uint32_t index, uint32_t* id)
{
	HKEY partner_key = 0;

	bool success = 
		(index == 1 || index == 2) &&
		rra_partner_init(rra) &&
		rra_partner_open_key(rra, index, &partner_key) &&
		rapi_reg_query_dword(partner_key, PARTNER_ID, id);

	if (partner_key)
		CeRegCloseKey(partner_key);

	return success;
}

bool rra_partner_set_name(RRA* rra, uint32_t index, const char* name)
{
	HKEY partner_key = 0;
	
	bool success =
		(index == 1 || index == 2) &&
		rra_partner_init(rra) &&
		rra_partner_open_key(rra, index, &partner_key) &&
		rapi_reg_set_string(partner_key, PARTNER_NAME, name);
	
	if (partner_key)
		CeRegCloseKey(partner_key);
	
	return success;
}

bool rra_partner_get_name(RRA* rra, uint32_t index, char** name)
{
	HKEY partner_key = 0;

	bool success = 
		(index == 1 || index == 2) &&
		rra_partner_init(rra) &&
		rra_partner_open_key(rra, index, &partner_key) &&
		rapi_reg_query_string(partner_key, PARTNER_NAME, name);

	if (partner_key)
		CeRegCloseKey(partner_key);

	return success;	
}

static char* rra_partner_get_filename(uint32_t id)
{
  char* path = NULL;
  char filename[256];

  if (!synce_get_subdirectory(RRA_DIRECTORY, &path))
    return NULL;

  snprintf(filename, sizeof(filename), "%s/%s-%08x", path, PARTERSHIP_FILENAME, id);

  free(path);

  return strdup(filename);
}

bool rra_partner_replace(RRA* rra, uint32_t index)
{
  bool success = false;
  char hostname[256];
  uint32_t other_id = 0;
  uint32_t id = 0;
  SynceInfo* info = synce_info_new(NULL);
  char* filename = NULL;
  char* p;

  if (!info)
  {
    goto exit;
  }
  
  if (index != 1 && index != 2)
  {
    synce_error("Bad index: %i", index);
    goto exit;
  }

  if (0 != gethostname(hostname, sizeof(hostname)))
  {
    synce_error("Failed to get hostname");
    goto exit;
  }

  /* only use the part of the hostname before the first '.' char */
  for (p = hostname; *p != '\0'; p++)
    if ('.' == *p) 
    {
      *p = '\0';
      break;
    }

  if (!rra_partner_get_id(rra, 3-index, &other_id))
    other_id = 0;

  srandom(time(NULL));

  for (;;)
  {
    char* filename = NULL;
    struct stat dummy;
    
    /* better id generation? */
    id = (uint32_t)random();

    filename = rra_partner_get_filename(id);
    
    if (0 == stat(filename, &dummy))
      id = 0;       /* file exists, try another id */

    free(filename);

    if (0 == id || 0xffffffff == id || other_id == id)
      continue;

    break;
  }

  success = 
    rra_partner_set_id(rra, index, id) &&
    rra_partner_set_name(rra, index, hostname);

  if (success)
  {
    FILE* file = NULL;

    if (!(filename = rra_partner_get_filename(id)))
    {
      synce_error("Failed to get filename for partner id %08x", id);
      goto exit;
    }
    
    if ((file = fopen(filename, "w")) == NULL)
    {
      synce_error("Failed to open file for writing: %s", filename);
      goto exit;
    }

    fprintf(file,
        "[device]\n"
        "name=%s\n"
        "\n"
        "[%s]\n"
        "%s=%i\n"
        "%s=%i\n"
        "%s=%s\n"
        ,
        info->name,
        PARTERSHIP_SECTION,
        CURRENT_PARTNER,  index,
        PARTNER_ID,       id,
        PARTNER_NAME,     hostname);

    fclose(file);
  }

exit:
  synce_info_destroy(info);
  if (filename)
    free(filename);
  return success;
}

bool rra_partner_create(RRA* rra, uint32_t* index)
{
  bool success = false;
  int i;
  uint32_t ids[2];
  SynceIni* ini = NULL;

  for (i = 0; i < 2; i++)
  {
    if (!rra_partner_get_id(rra, i+1, &ids[i]))
      ids[i] = 0;

    /* see if we have a partnership file */
    if (ids[i])
    {
      char* filename = rra_partner_get_filename(ids[i]);

      if (!filename)
      {
        synce_error("Failed to get filename for partner id %08x", ids[i]);
        goto exit;
      }

      ini = synce_ini_new(filename);
      free(filename);

      if (ini)
      {
        const char* local_name = synce_ini_get_string(ini, PARTERSHIP_SECTION, PARTNER_NAME);
        char* remote_name = NULL;

        /* verify that the hostnames match */
        if (local_name &&
            rra_partner_get_name(rra, i+1, &remote_name) &&
            remote_name &&
            0 == strcmp(local_name, remote_name))
        {
          free(remote_name);
          *index = i+1;
          success = true;
          goto exit;
        }
        else
        {
          synce_trace("Local host name '%s' and remote host name '%s' do not match", 
              local_name, remote_name);
        }
      }
      else
      {
        synce_trace("Partnership file not found for ID %08x", ids[i]);
      }
    }
    else
    {
      synce_trace("Partnership slot %i is empty on device", i+1);
    }
  
    /* prepare for next ini file */
    synce_ini_destroy(ini);
    ini = NULL;
  }

  /* If we get here, we have no partnership with the device.
     We try to create a partnership on an empty slot */

  for (i = 0; i < 2; i++)
  {
    if (0 == ids[i])
    {
      if (rra_partner_replace(rra, i+1))
      {
        *index = i+1;
        success = true;
        goto exit;
      }
    }
  }

  synce_error("Partnership not found and there are no empty partner slots on device.");

exit:
  synce_ini_destroy(ini);
  if (success)
    success = rra_partner_set_current(rra, *index);
  return success;
}

TimeZoneInformation* rra_get_time_zone_information(RRA* rra)
{
  if (!rra)
    return NULL;

  if (rra->has_tzi)
    return &rra->tzi;

  if (time_zone_get_information(&rra->tzi))
  {
    rra->has_tzi = true;

    rra->time_zone_id = NULL;
    time_zone_get_id(&rra->tzi, &rra->time_zone_id);
    
    return &rra->tzi;
  }

  return NULL;
}

