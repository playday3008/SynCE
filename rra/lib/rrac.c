/* $Id$ */
#include "rrac.h"
#include "rrac_packet.h"
#include <synce_log.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#define LETOH16(x)  x = letoh16(x)
#define LETOH32(x)  x = letoh32(x)
#define HTOLE16(x)  x = htole16(x)
#define HTOLE32(x)  x = htole32(x)


#define DUMP_PACKETS 0

#if DUMP_PACKETS
#define DUMP(desc,data,len) dump(desc, data, len)

static void dump(const char *desc, void* data, size_t len)/*{{{*/
{
	uint8_t* buf = (uint8_t*)data;
	size_t i, j;
	char hex[8 * 3 + 1];
	char chr[8 + 1];

	printf("%s (%d bytes):\n", desc, len);
	for (i = 0; i < len + 7; i += 8) {
		for (j = 0; j < 8; j++) 
			if (j + i >= len) {
				hex[3*j+0] = ' ';
				hex[3*j+1] = ' ';
				hex[3*j+2] = ' ';
				chr[j] = ' ';
			} else {
				uint8_t c = buf[j + i];
				const char *hexchr = "0123456789abcdef";
				hex[3*j+0] = hexchr[(c >> 4) & 0xf];
				hex[3*j+1] = hexchr[c & 0xf];
				hex[3*j+2] = ' ';
				if (c > ' ' && c <= '~')
					chr[j] = c;
				else
					chr[j] = '.';
			}
		hex[8*3] = '\0';
		chr[8] = '\0';
		fprintf(stderr, "  %04x: %s %s\n", i, hex, chr);
	}
}/*}}}*/

#else
#define DUMP(desc,data,len)
#endif


static bool rrac_recv_any(SynceSocket* socket, CommandHeader* header, uint8_t** data)/*{{{*/
{
	bool success = false;

	*data = NULL;

	if (!synce_socket_read(socket, header, sizeof(CommandHeader)))
	{
		synce_error("Failed to read command header");
		goto exit;
	}

	LETOH16( header->command    );
	LETOH16( header->size       );

	synce_trace("Received command %08x", header->command);

	DUMP("packet header", header, sizeof(CommandHeader));

	*data = (uint8_t*)malloc(header->size);

	if (!synce_socket_read(socket, *data, header->size))
	{
		synce_error("Failed to read data");
		goto exit;
	}

	DUMP("packet data", *data, header->size);

	success = true;

exit:
	if (!success)
	{
		if (*data)
		{
			free(*data);
			*data = NULL;
		}
	}

	return success;
}/*}}}*/

/*
 * Be a little clever when reading packets
 */
bool rrac_expect(SynceSocket* socket, uint32_t command, uint8_t** data, size_t* size)/*{{{*/
{
	bool success = false;
	CommandHeader header;

	*data = NULL;
	
	for (;;)
	{
		if (*data)
			free(*data);
		
		if (!rrac_recv_any(socket, &header, data))
		{
			synce_error("Failed to receive packet");
			break;
		}

		if (command == header.command)
		{
			*size = header.size;
			success = true;
		}
		else if (0x69 == header.command)
		{
			Subheader_69* subheader = (Subheader_69*)*data;
			if (0 == subheader->subcommand)
			{
				synce_trace("Some object was updated");
				continue;
			}
		}
		else if (0x6e == header.command)
		{
			Packet_6e* packet = (Packet_6e*)*data;
			LETOH32(packet->type_id);
			LETOH32(packet->object_id);
			LETOH32(packet->hr);
			LETOH32(packet->unknown);
			synce_trace("Error: type=%08x, object=%08x, hr=%08x, unknown=%08x",
					packet->type_id, packet->object_id, packet->hr, packet->unknown);
		}
		else
		{
			synce_trace("Unexpected packet: command=%08x, size=%08x", 
					header.command, header.size);
		}
		break;
	}

	if (!success && *data)
	{
		free(*data);
		*data = NULL;
	}

	return success;
}/*}}}*/

bool rrac_expect_reply(SynceSocket* socket, uint32_t reply_to, uint8_t** data, size_t* size)/*{{{*/
{
	bool success = false;
	Subheader_6c* subheader = NULL;

	synce_trace("Expecting reply to command %08x", reply_to);
	
	*data = NULL;

	if (!rrac_expect(socket, 0x6c, data, size))
	{
		synce_error("Failed to receive expected packet");
		goto exit;
	}

	subheader = (Subheader_6c*)*data;
	LETOH32( subheader->reply_to );

	if (reply_to != subheader->reply_to)
	{
		synce_error("Unexpected reply message");
		goto exit;
	}

	success = true;

exit:
	if (!success && *data)
	{
		free(*data);
		*data = NULL;
	}

	return success;
}/*}}}*/

bool rrac_send_65( /*{{{*/
		SynceSocket* socket, 
		uint32_t type_id, 
		uint32_t object_id1, 
		uint32_t object_id2, 
		uint32_t flags)
{
	bool success = false;
	Command_65 packet;

	packet.command       = htole16(0x65);
	packet.size          = htole16(sizeof(packet) - 4);
	packet.type_id       = htole32(type_id);
	packet.object_ids[0] = htole32(object_id1);
	packet.object_ids[1] = htole32(object_id2);
	packet.unknown       = htole32(flags);

	DUMP("command 65", &packet, sizeof(packet));

	if (!synce_socket_write(socket, &packet, sizeof(packet)))
	{
		printf("Failed to send packet");
		goto exit;
	}

	success = true;

exit:
	return success;
}/*}}}*/

bool rrac_recv_65(/*{{{*/
		SynceSocket* socket, 
		uint32_t* type_id, 
		uint32_t* object_id1, 
		uint32_t* object_id2,
		uint32_t* flags)
{
	bool success = false;
	Packet_65* packet = NULL;
	uint8_t* data = NULL;
	size_t size = 0;

	if (!rrac_expect(socket, 0x65, &data, &size))
	{
		synce_error("Failed to receive expected packet");
		goto exit;
	}

	packet = (Packet_65*)data;

	if (sizeof(Packet_65) != size)
	{
		synce_error("Unexpected packet format");
		goto exit;
	}

	LETOH32(packet->type_id);
	LETOH32(packet->object_ids[0]);
	LETOH32(packet->object_ids[1]);
	LETOH32(packet->flags);

	if (type_id)    *type_id    = packet->type_id;
	if (object_id1) *object_id1 = packet->object_ids[0];
	if (object_id2) *object_id2 = packet->object_ids[1];
	if (flags)      *flags      = packet->flags;
	
	success = true;

exit:
	if (data)
		free(data);
	return success;
}/*}}}*/

bool rrac_send_66(/*{{{*/
		SynceSocket* socket, 
		uint32_t type_id, 
		uint32_t object_id, 
		uint32_t flags)
{
	Command_66 command;

	command.header.command    = htole16(0x66);
	command.header.size       = htole16(sizeof(command.packet));
	
	command.packet.unknown    = 0;
	command.packet.type_id    = htole32(type_id);
	command.packet.object_id  = htole32(object_id);
	command.packet.flags      = htole32(flags);
	
	DUMP("command 66", &command, sizeof(command));
	return synce_socket_write(socket, &command, sizeof(command));
}	/*}}}*/

bool rrac_send_67(SynceSocket* socket, uint32_t type_id, uint32_t* ids, size_t count)/*{{{*/
{
	uint8_t* packet = NULL;
	Command_67_Header* header;
	bool success = false;
	size_t size = sizeof(Command_67_Header) + count * sizeof(uint32_t);
	uint32_t *packet_ids;
	int i;

	packet = (uint8_t*)malloc(size);
	header = (Command_67_Header*)packet;
	
	header->command      = htole16(0x67);
	header->size         = htole16(size - 4);
	header->unknown      = 0;
	header->type_id      = htole32(type_id);
	header->count        = htole32(count);

	packet_ids = (uint32_t*)(packet + sizeof (Command_67_Header));
	for (i = 0; i < count; i++)
	{
		packet_ids[i] = htole32 (ids[i]);
	}

	DUMP("packet 67", packet, size);
	success = synce_socket_write(socket, packet, size);

	free(packet);
	return success;
}/*}}}*/

bool rrac_send_6f(SynceSocket* socket, uint32_t subcommand)/*{{{*/
{	
	Command_6f packet;
	packet.command     = htole16(0x6f);
	packet.size        = htole16(sizeof(Command_6f) - 4);
	packet.subcommand  = htole32(subcommand);

	DUMP("command 6f", &packet, sizeof(packet));
	return synce_socket_write(socket, &packet, sizeof(packet));
}/*}}}*/

bool rrac_recv_reply_6f_6(SynceSocket* socket)/*{{{*/
{
	bool success = false;
	Command_6c_Reply_6f_6 packet;

	if (!synce_socket_read(socket, &packet, sizeof(packet)))
	{
		synce_error("Failed to read command packet");
		goto exit;
	}

	DUMP("reply packet", &packet, sizeof(packet));

	LETOH16( packet.command    );
	LETOH16( packet.size       );
	LETOH32( packet.reply_to   );

	if (packet.command  != 0x6c ||
			packet.size     != (sizeof(packet) - 4) ||
			packet.reply_to != 0x6f)
	{
		synce_error("Unexpected command or packet format");
		goto exit;
	}

	success = true;
	
exit:
	return success;
}/*}}}*/

bool rrac_recv_reply_6f_10(SynceSocket* socket)/*{{{*/
{
	bool success = false;
	uint8_t* data = NULL;
	size_t size = 0;
	
	if (!rrac_expect_reply(socket, 0x6f, &data, &size))
	{
		synce_error("Failed to receive reply packet");
		goto exit;
	}

	/* care about data? */

exit:
	if (data)
		free(data);

	return success;	
}/*}}}*/

bool rrac_recv_reply_6f_c1(/*{{{*/
		SynceSocket* socket,
		RawObjectType** object_type_array,
		size_t* object_type_count)
{
	bool success = false;
	uint8_t* data = NULL;
	size_t size = 0;
	size_t array_size = 0;
	unsigned i;
	Command_6c_Reply_6f_c1_Header* header = NULL;
	
	if (!rrac_expect_reply(socket, 0x6f, &data, &size))
	{
		synce_error("Failed to receive reply packet");
		goto exit;
	}

	header = (Command_6c_Reply_6f_c1_Header*)data;
	LETOH32( header->type_count );

	array_size = header->type_count * sizeof(RawObjectType);
	*object_type_array = rrac_alloc(array_size);
	*object_type_count = header->type_count;

	memcpy(*object_type_array, data + sizeof(Command_6c_Reply_6f_c1_Header),
		array_size);

	for (i = 0; i < *object_type_count; i++)
	{
		LETOH32( (*object_type_array)[i].id         );
		LETOH32( (*object_type_array)[i].count      );
		LETOH32( (*object_type_array)[i].total_size );
	}

	success = true;

exit:
	if (data)
		free(data);

	return success;	
}/*}}}*/

bool rrac_send_70_2(SynceSocket* socket, uint32_t subsubcommand)/*{{{*/
{
	Command_70_2 packet;

	packet.command     = htole16(0x70);
	packet.size        = htole16(sizeof(packet) - 4);
	packet.size2       = htole32(sizeof(packet) - 8);
	packet.unknown1    = htole32(0xf0000001);
	packet.subcommand  = htole32(2);
	memset(packet.empty1, 0, sizeof(packet.empty1));

	switch (subsubcommand)
	{
		case 1: 
			packet.unknown2 = htole32(0x80000003); 
			break;
			
		case 2: 
			packet.unknown2 = 0; 
			break;

		default:
			synce_error("Unknown subsubcommand");
			return false;
	}

	packet.subsubcommand = htole32(subsubcommand);
	memset(packet.empty2, 0, sizeof(packet.empty2));

	DUMP("packet 70:2", &packet, sizeof(packet));
	return synce_socket_write(socket, &packet, sizeof(packet));
}/*}}}*/

bool rrac_send_70_3(SynceSocket* socket, uint32_t* ids, size_t count)/*{{{*/
{
	bool success = false;
	uint8_t* packet = NULL;
	Command_70_3_Header* header = NULL;
	uint32_t* packet_ids = NULL;
	size_t size = sizeof(Command_70_3_Header) + count * sizeof(uint32_t);
	unsigned i;

	packet     = (uint8_t*)malloc(size);
	header     = (Command_70_3_Header*)packet;
	packet_ids = (uint32_t*)(packet + sizeof(Command_70_3_Header));
	
	header->command      = htole16(0x70);
	header->size         = htole16(size - 4);
	header->size2        = htole32(size - 8);
	header->unknown1     = htole32(0xf0000001);
	header->subcommand   = htole32(3);
	header->unknown2[0]  = htole32(2);
	header->unknown2[1]  = 0;
	header->unknown2[2]  = 0;
	header->unknown2[3]  = 0;
	header->count        = htole32(count);

	for (i = 0; i < count; i++)
		packet_ids[i] = htole32(ids[i]);

	DUMP("packet 70:3", packet, size);
	success = synce_socket_write(socket, packet, size);

	free(packet);
	return success;
}/*}}}*/

bool rrac_recv_reply_70(SynceSocket* socket)/*{{{*/
{
	bool success = false;
	uint8_t* data = NULL;
	size_t size = 0;
	
	if (!rrac_expect_reply(socket, 0x70, &data, &size))
	{
		synce_error("Failed to receive reply packet");
		goto exit;
	}

	if (sizeof(Command_6c_Reply_70_Header) != size)
		synce_warning("Unexpected packet size: %08x", size);
	
	success = true;

exit:
	if (data)
		free(data);

	return success;
}/*}}}*/

bool rrac_recv_69_2(SynceSocket* socket)/*{{{*/
{
	bool success = false;
	Command_69_2 packet;

	if (!synce_socket_read(socket, &packet, sizeof(packet)))
	{
		synce_error("Failed to read packet");
		goto exit;
	}

	DUMP("packet 69", &packet, sizeof(packet));

	LETOH16( packet.command    );
	LETOH16( packet.size       );
	LETOH32( packet.subcommand );
	
	if (packet.command    != 0x69 ||
			packet.size       != (sizeof(packet) - 4) ||
			packet.subcommand != 0x02000000)
	{
		synce_error("Unexpected command");
		goto exit;
	}

	/* XXX: care about packet contents? */

	success = true;

exit:
	return success;
}/*}}}*/

bool rrac_recv_69_not_2(/*{{{*/
		SynceSocket* socket,
		uint32_t* subcommand,
		uint32_t* type_id,
		uint32_t* some_count,
		uint32_t** ids,
		uint32_t* id_count)
{
	bool success = false;
	uint8_t* data = NULL;
	size_t size = 0;
	Subheader_69_X* subheader = NULL;
	uint32_t *packet_ids;
	int i;

	if (!ids)
	{
		synce_error("id array parameter is NULL");
		goto exit;
	}

	if (!rrac_expect(socket, 0x69, &data, &size))
	{
		synce_error("Failed to read command header");
		goto exit;
	}

	if (size < sizeof(Subheader_69_X))
	{
		synce_error("Unexpected packet format");
		goto exit;
	}

	subheader = (Subheader_69_X*)data;

	LETOH32( subheader->subcommand );
	LETOH32( subheader->type_id    );
	LETOH32( subheader->some_count );
	LETOH32( subheader->array_size );

	synce_trace("subcommand = %08x", subheader->subcommand);
	
	switch (subheader->subcommand)
	{
		case 0x00000000:
		case 0x04000000:
		case 0x06000000:
			break;

		default:
			synce_error("Unexpected subcommand");
	}
	
	if (subheader->array_size)
	{
		if (subheader->array_size % 4)
		{
			synce_error("Unexpected array size");
			goto exit;
		}
		
		*ids = malloc(subheader->array_size);
		packet_ids = (uint32_t *)(data + sizeof (Subheader_69_X));
		for (i = 0; i < subheader->array_size / 4; i++)
		{
			(*ids)[i] = letoh32 (packet_ids[i]);
		}

		/*DUMP("packet 69 ids", *ids, subheader->array_size);*/
	}

	if (subcommand)  *subcommand = subheader->subcommand;
	if (type_id)     *type_id    = subheader->type_id;
	if (some_count)  *some_count = subheader->some_count;
	if (id_count)    *id_count   = subheader->array_size / 4;

	success = true;

exit:
	if (data)
		free(data);
	return success;
}/*}}}*/

bool rrac_recv_data(/*{{{*/
		SynceSocket* socket,
		uint32_t* object_id,
		uint32_t* type_id,
		uint8_t** data, 
		size_t* size)
{
	bool success = false;
	DataHeader header;
	ChunkHeader chunk_header;
	size_t total_size = 0;

	if (!synce_socket_read(socket, &header, sizeof(header)))
	{
		synce_error("Failed to read data header");
		goto exit;
	}
	
	DUMP("data header", &header, sizeof(header));

	LETOH32(header.object_id);
	LETOH32(header.type_id);

	if (object_id)  *object_id  = header.object_id;
	if (type_id)    *type_id    = header.type_id;

	if (OBJECT_ID_STOP == header.object_id)
	{
		/* end of data marker */
		success = true;
		goto exit;
	}

	if (!data)
	{
		synce_error("Data parameter is NULL");
		goto exit;
	}

	*data = NULL;

	do
	{
		size_t aligned_size;
		
		if (!synce_socket_read(socket, &chunk_header, sizeof(chunk_header)))
		{
			synce_error("Failed to read chunk header");
			goto exit;
		}
		
		DUMP("chunk header", &chunk_header, sizeof(chunk_header));

		LETOH16(chunk_header.size);
		LETOH16(chunk_header.stuff);

		aligned_size = (chunk_header.size + 3) & ~3;
		*data = realloc(*data, total_size + aligned_size);

		if (!synce_socket_read(socket, *data + total_size, aligned_size))
		{
			synce_error("Failed to read data");
			goto exit;
		}

		DUMP("data", *data + total_size, aligned_size < 0x100 ? aligned_size : 0x100);
	
		total_size += chunk_header.size;

	} while (!(chunk_header.stuff & 0x8000));

	if (size)
		*size = total_size;

	success = true;

exit:
	return success;
}/*}}}*/

bool rrac_send_data(/*{{{*/
		SynceSocket* socket,
		uint32_t object_id,
		uint32_t type_id,
		uint32_t flags,
		uint8_t* data, 
		size_t size)
{
	bool success = false;
	DataHeader header;
	ChunkHeader chunk_header;
	size_t offset = 0;
	size_t bytes_left = size;

	synce_trace("object_id=0x%x, type_id=0x%x, data size=0x%x", object_id, type_id, size);

	header.object_id = htole32(object_id);
	header.type_id   = htole32(type_id);
	header.flags     = htole32(flags); /* maybe the RSF_ flags in cesync.h */

	if (!synce_socket_write(socket, &header, sizeof(header)))
	{
		synce_error("Failed to write data header");
		goto exit;
	}

	DUMP("data header", &header, sizeof(header));

	if (OBJECT_ID_STOP == object_id)
	{
		success = true;
		goto exit;
	}
	
	while (bytes_left)
	{
		size_t chunk_size = MIN(bytes_left, CHUNK_MAX_SIZE);
		size_t aligned_size = (chunk_size + 3) & ~3;
		
		chunk_header.size = htole16(chunk_size);
		bytes_left -= chunk_size;

		if (bytes_left > 0)
			chunk_header.stuff = htole16(offset);
		else
			chunk_header.stuff = htole16(0xffa0);
	
		DUMP("chunk header", &chunk_header, sizeof(chunk_header));

		if (!synce_socket_write(socket, &chunk_header, sizeof(chunk_header)))
		{
			synce_error("Failed to write chunk header");
			goto exit;
		}
		
		DUMP("data", data + offset, chunk_size);
		if (!synce_socket_write(socket, data + offset, chunk_size))
		{
			synce_error("Failed to write chunk data");
			goto exit;
		}
		
		if (aligned_size > chunk_size)
		{
			char pad[3] = {0,0,0};
			synce_trace("Writing %i bytes padding", aligned_size - chunk_size);
			if (!synce_socket_write(socket, pad, aligned_size - chunk_size))
			{
				synce_error("Failed to write padding");
				goto exit;
			}
		}

		offset += chunk_size;
	}
	
	success = true;

exit:
	return success;
}/*}}}*/


