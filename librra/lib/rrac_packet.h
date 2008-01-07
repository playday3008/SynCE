/* $Id$ */
#ifndef __rrac_packets_h__
#define __rrac_packets_h__

typedef struct _CommandHeader
{
	uint16_t    command;
	uint16_t    size;
} CommandHeader;

typedef struct _Command_65
{
	uint16_t    command;
	uint16_t    size;
	uint32_t    type_id;
	uint32_t    object_ids[2];
	uint32_t    unknown;
} Command_65;

typedef struct _Packet_65
{
	uint32_t    type_id;
	uint32_t    object_ids[2];
	uint32_t    flags;
} Packet_65;

typedef struct _Packet_66
{
	uint32_t    unknown;
	uint32_t    type_id;
	uint32_t    object_id;
	uint32_t    flags;
} Packet_66;

typedef struct _Command_66
{
	CommandHeader header;
	Packet_66 packet;
} Command_66;

typedef struct _Command_67_Header
{
	uint16_t    command;
	uint16_t    size;
	uint32_t    unknown;
	uint32_t    type_id;
	uint32_t    count;
} Command_67_Header;

typedef struct _Subheader_69
{
	uint32_t    subcommand;
} Subheader_69;


typedef struct _Subheader_69_X
{
	uint32_t    subcommand;
	uint32_t    type_id;
	uint32_t    some_count;
	uint32_t    array_size;
} Subheader_69_X;

typedef struct _Command_69_X_Header
{
	uint16_t    command;
	uint16_t    size;
	uint32_t    subcommand;
	uint32_t    type_id;
	uint32_t    some_count;
	uint32_t    array_size;
} Command_69_X_Header;

typedef struct _Command_69_2
{
	uint16_t    command;
	uint16_t    size;
	uint32_t    subcommand;
	uint32_t    unknown[3];
	uint32_t    partner_index;
	uint32_t    partner_ids[2];
} Command_69_2;

#define COMMAND_69_2_SIZE (7*sizeof(uint32_t))

typedef struct _Packet_6e
{
	uint32_t    type_id;
	uint32_t    object_id;
	HRESULT     hr;
	uint32_t    unknown;
} Packet_6e;

typedef struct _Command_6f
{
	uint16_t    command;
	uint16_t    size;
	uint32_t    subcommand;
} Command_6f;

typedef struct _Subheader_6c
{
	uint32_t    reply_to;
} Subheader_6c;

#if 0
typedef struct _Command_6c_Reply_6f_6
{
	uint16_t    command;
	uint16_t    size;
	uint32_t    reply_to;
	uint32_t    unknown[11];
} Command_6c_Reply_6f_6;
#endif

typedef struct _Command_6c_Reply_6f_c1_Header
{
	Subheader_6c	reply_header;
	uint32_t    unknown[7];
	uint32_t		type_count;
} Command_6c_Reply_6f_c1_Header;

typedef struct _Command_6c_Reply_70_Header
{
	Subheader_6c	reply_header;
	uint32_t    unknown[3];
} Command_6c_Reply_70_Header;

typedef struct _Command_70_2
{
	uint16_t command;
	uint16_t size;
	uint32_t size2;
	uint32_t unknown1;
	uint32_t subcommand;
	uint8_t empty1[0xc8];
	uint32_t subsubcommand;
	uint32_t unknown2;
	uint8_t empty2[0x18];
} Command_70_2;

typedef struct _Command_70_3_Header
{
	uint16_t    command;
	uint16_t    size;
	uint32_t    size2;
	uint32_t    unknown1;
	uint32_t    subcommand;
	uint32_t    unknown2[4];
	uint32_t    count;
} Command_70_3_Header;

typedef struct _DataHeader
{
	uint32_t object_id;
	uint32_t type_id;
	uint32_t flags;
} DataHeader;

typedef struct _ChunkHeader
{
	uint16_t size;
	uint16_t stuff;
} ChunkHeader;

#define CHUNK_MAX_SIZE  0x1000

#endif

