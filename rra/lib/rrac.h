#ifndef __rrac_h__
#define __rrac_h__

#include <synce_socket.h>

#define RRAC_PORT 5678

#define OBJECT_ID_STOP 0xffffffff

typedef struct
{
	uint32_t offset_00;
	WCHAR name1[100];
	WCHAR name2[80];
	uint32_t id;
	uint32_t count;
	uint32_t total_size;
	FILETIME filetime;
} RawObjectType;

bool rrac_send_65(
		SynceSocket* socket, 
		uint32_t type_id, 
		uint32_t object_id1, 
		uint32_t object_id2, 
		uint32_t flags);

bool rrac_recv_65(
		SynceSocket* socket, 
		uint32_t* type_id, 
		uint32_t* object_id1, 
		uint32_t* object_id2,
		uint32_t* flags);

bool rrac_send_66(
		SynceSocket* socket, 
		uint32_t type_id, 
		uint32_t object_id, 
		uint32_t flags);

bool rrac_send_67(SynceSocket* socket, uint32_t type_id, uint32_t* ids, size_t count);

bool rrac_send_6f(SynceSocket* socket, uint32_t subcommand);

bool rrac_recv_reply_6f_6(SynceSocket* socket);
bool rrac_recv_reply_6f_10(SynceSocket* socket);
bool rrac_recv_reply_6f_c1(
		SynceSocket* socket,
		RawObjectType** object_type_array,
		size_t* object_type_count);

bool rrac_send_70_2(SynceSocket* socket, uint32_t subsubcommand);

bool rrac_send_70_3(SynceSocket* socket, uint32_t* ids, size_t count);

bool rrac_recv_reply_70(SynceSocket* socket);

bool rrac_recv_69_2(SynceSocket* socket);

bool rrac_recv_69_not_2(
		SynceSocket* socket,
		uint32_t* subcommand,
		uint32_t* type_id,
		uint32_t* some_count,
		uint32_t** ids,
		uint32_t* id_count);

bool rrac_recv_data(
		SynceSocket* socket,
		uint32_t* object_id,
		uint32_t* type_id,
		uint8_t** data, 
		size_t* size);

bool rrac_send_data(
		SynceSocket* socket,
		uint32_t object_id,
		uint32_t type_id,
		uint32_t flags,
		uint8_t* data, 
		size_t size);

#define rrac_alloc(n) malloc(n)
#define rrac_free(p) if (p) free(p)


#endif

