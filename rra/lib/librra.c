/* $Id$ */
#include "librra.h"
#include "rrac.h"
#include <rapi.h>
#include <synce_log.h>
#include <string.h>

#define SEND_COMMAND_6F_6   1
#define SEND_COMMAND_6F_10  0
#define SEND_COMMAND_70_2   1

struct _RRA
{
	SynceSocket*  server;
	SynceSocket*  cmd_channel;
	SynceSocket*  data_channel;

	ObjectType*   object_types;
	size_t        object_type_count;
};

RRA* rra_new()/*{{{*/
{
	return (RRA*)calloc(1, sizeof(RRA));
}/*}}}*/

void rra_free(RRA* rra)/*{{{*/
{
	rra_disconnect(rra);
	free(rra);
}/*}}}*/

bool rra_connect(RRA* rra)/*{{{*/
{
	rra->server = synce_socket_new();

	if (!synce_socket_listen(rra->server, NULL, RRAC_PORT))
		goto fail;

	/* TODO: error handling! */
	CeStartReplication();

	rra->cmd_channel   = synce_socket_accept(rra->server, NULL);
	rra->data_channel  = synce_socket_accept(rra->server, NULL);

	return true;

fail:
	rra_disconnect(rra);
	return false;
}/*}}}*/

void rra_disconnect(RRA* rra)/*{{{*/
{
	if (rra)
	{
		synce_socket_free(rra->data_channel);
		synce_socket_free(rra->cmd_channel);
		synce_socket_free(rra->server);
		
		if (rra->object_types)
			free(rra->object_types);
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

		if (!rrac_send_6f(rra->cmd_channel, 0xc1))
		{
			synce_error("Failed to send command 6f");
			goto exit;
		}

		if (!rrac_recv_reply_6f_c1(
					rra->cmd_channel, 
					&raw_object_types, 
					&rra->object_type_count))
		{
			synce_error("Failed to receive reply");
			goto exit;
		}

		if (object_types)
		{
			rra->object_types = malloc(rra->object_type_count * sizeof(ObjectType));

			for (i = 0; i < rra->object_type_count; i++)
			{
				rra->object_types[i].id          = raw_object_types[i].id;
				rra->object_types[i].count       = raw_object_types[i].count;
				rra->object_types[i].total_size  = raw_object_types[i].total_size;
				rra->object_types[i].modified    = 
					filetime_to_unix_time(&raw_object_types[i].filetime);

				char* ascii = wstr_to_ascii(raw_object_types[i].name1);
				strcpy(rra->object_types[i].name, ascii);
				wstr_free_string(ascii);
			}
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
	
	rrac_send_70_3(rra->cmd_channel, ignored_ids, ignored_count);

	if (!rrac_recv_reply_70(rra->cmd_channel))
	{
		goto exit;
	}

	/*
	 * test
	 */

#if SEND_COMMAND_6F_10
	if (!rrac_send_6f(rra->cmd_channel, 0x10))
	{
		goto exit;
	}
#endif

	/*
	 * Receive object ids
	 */

	if (!rrac_recv_69_2(rra->cmd_channel))
	{
		goto exit;
	}

	while (recv_subcommand != 0x06000000)
	{
		unsigned i;
		
		if (!rrac_recv_69_not_2(
					rra->cmd_channel, 
					&recv_subcommand,
					&recv_type_id,
					&recv_some_count,
					&recv_ids,
					&recv_id_count))
		{
			goto exit;
		}

		if (recv_type_id != object_type_id)
		{
			goto exit;
		}

		if (0x04000000 == recv_subcommand)
		{
			unsigned j = 0;

			synce_trace("subcommand %08x, changed count=%i, total count=%i",
					recv_subcommand, recv_some_count, recv_id_count);

			synce_trace("unchanged ids:");
			for (i = 0; i < (recv_id_count-recv_some_count); i++, j++)
			{
				synce_trace("id[%i] = %08x", j, recv_ids[j]);
			}
			
			synce_trace("changed ids:");
			for (i = 0; i < recv_some_count; i++, j++)
			{
				synce_trace("id[%i] = %08x", j, recv_ids[j]);
#if 0
				rrac_send_65(rra->cmd_channel, object_type_id, recv_ids[j], recv_ids[j]);
#endif
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
	if (!rrac_recv_reply_6f_10(rra->cmd_channel))
	{
		goto exit;
	}
#endif

#if 0 && SEND_COMMAND_6F_6
	if (!rrac_send_6f(rra->cmd_channel, 6))
	{
		goto exit;
	}

	if (!rrac_recv_reply_6f_6(rra->cmd_channel))
	{
		goto exit;
	}
#endif
	
	/* success = true; */

exit:
	if (ignored_ids)
		free(ignored_ids);

	return success;
}/*}}}*/

void rra_free_object_ids(ObjectIdArray* object_id_array);

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
	if (!rrac_send_70_2(rra->cmd_channel, 1))
	{
		goto exit;
	}

	if (!rrac_recv_reply_70(rra->cmd_channel))
	{
		goto exit;
	}
#endif
	
	/* Ask for object data */
	if (!rrac_send_67(rra->cmd_channel, type_id, &object_id, 1))
	{
		goto exit;
	}

	/* Receive object data */
	if (!rrac_recv_data(rra->data_channel, &recv_object_id, &recv_type_id, data, data_size))
	{
		goto exit;
	}

	/* Received end-of-data object */
	if (!rrac_recv_data(rra->data_channel, NULL, NULL, NULL, NULL))
	{
		goto exit;
	}
	
	/* This command will mark the packet as unchanged */	
	if (!rrac_send_65(
				rra->cmd_channel, 
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
	if (!rrac_send_70_2(rra->cmd_channel, 2))
	{
		goto exit;
	}
#endif

#if SEND_COMMAND_6F_6
	if (!rrac_send_6f(rra->cmd_channel, 6))
	{
		goto exit;
	}
#endif

#if SEND_COMMAND_70_2
	if (!rrac_recv_reply_70(rra->cmd_channel))
	{
		goto exit;
	}
#endif

#if SEND_COMMAND_6F_6
	if (!rrac_recv_reply_6f_6(rra->cmd_channel))
	{
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
	if (!rrac_send_70_2(rra->cmd_channel, 1))
	{
		goto exit;
	}

	if (!rrac_recv_reply_70(rra->cmd_channel))
	{
		goto exit;
	}
#endif
	
	if (!rrac_send_data(
				rra->data_channel, 
				object_id, 
				type_id, 
				flags, 
				data, 
				data_size))
	{
		synce_error("Failed to send data");
		goto exit;	
	}

	if (!rrac_send_data(rra->data_channel, OBJECT_ID_STOP, type_id, 0, NULL, 0))
	{
		synce_error("Failed to send stop entry");
		goto exit;	
	}

	if (!rrac_recv_65(
				rra->cmd_channel, 
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
					rra->cmd_channel, 
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
	if (!rrac_send_70_2(rra->cmd_channel, 2))
	{
		goto exit;
	}
#endif

#if SEND_COMMAND_6F_6
	if (!rrac_send_6f(rra->cmd_channel, 6))
	{
		goto exit;
	}
#endif

#if SEND_COMMAND_70_2
	if (!rrac_recv_reply_70(rra->cmd_channel))
	{
		goto exit;
	}
#endif

#if SEND_COMMAND_6F_6
	if (!rrac_recv_reply_6f_6(rra->cmd_channel))
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

bool rra_object_add(RRA* rra,  /*{{{*/
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

bool rra_object_delete(RRA* rra, uint32_t type_id, uint32_t object_id)
{
	bool success = false;
	uint32_t recv_type_id;
	uint32_t recv_object_id1;
	uint32_t recv_object_id2;
	uint32_t recv_flags;
	uint32_t index = 1;

	if (!rrac_send_66(rra->cmd_channel, type_id, object_id, index))
	{
		synce_error("Failed to senmd command 66");
		goto exit;
	}

	if (!rrac_recv_65(
				rra->cmd_channel, 
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
}

#if 0
bool rra_lock(RRA* rra)
{
	return 
		rrac_send_70_2(rra->cmd_channel, 1) &&
		rrac_recv_reply_70(rra->cmd_channel);
}

bool rra_unlock(RRA* rra)
{
	return 
		rrac_send_70_2(rra->cmd_channel, 2) &&
		rrac_recv_reply_70(rra->cmd_channel);
}
#endif

