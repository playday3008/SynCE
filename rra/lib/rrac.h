#ifndef __rrac_h__
#define __rrac_h__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define OBJECT_ID_STOP 0xffffffff

struct _RRAC;
typedef struct _RRAC RRAC;

typedef bool (*Command69Callback)(
    uint32_t subcommand, 
    uint8_t* data, 
    size_t size, 
    void* cookie);

RRAC* rrac_new();

void rrac_destroy(RRAC* rrac);

bool rrac_set_command_69_callback(
    RRAC* rrac,
    Command69Callback callback, 
    void* cookie);

bool rrac_connect(RRAC* rrac);

void rrac_disconnect(RRAC* rrac);

#include <synce.h>

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
		RRAC* rrac, 
		uint32_t type_id, 
		uint32_t object_id1, 
		uint32_t object_id2, 
		uint32_t flags);

bool rrac_recv_65(
		RRAC* rrac, 
		uint32_t* type_id, 
		uint32_t* object_id1, 
		uint32_t* object_id2,
		uint32_t* flags);

bool rrac_send_66(
		RRAC* rrac, 
		uint32_t type_id, 
		uint32_t object_id, 
		uint32_t flags);

bool rrac_send_67(RRAC* rrac, uint32_t type_id, uint32_t* ids, size_t count);

bool rrac_send_6f(RRAC* rrac, uint32_t subcommand);

bool rrac_recv_reply_6f_6(RRAC* rrac);
bool rrac_recv_reply_6f_10(RRAC* rrac);
bool rrac_recv_reply_6f_c1(
		RRAC* rrac,
		RawObjectType** object_type_array,
		size_t* object_type_count);

bool rrac_send_70_2(RRAC* rrac, uint32_t subsubcommand);

bool rrac_send_70_3(RRAC* rrac, uint32_t* ids, size_t count);

bool rrac_recv_reply_70(RRAC* rrac);

bool rrac_recv_69_2(RRAC* rrac);

bool rrac_recv_69_not_2(
		RRAC* rrac,
		uint32_t* subcommand,
		uint32_t* type_id,
		uint32_t* some_count,
		uint32_t** ids,
		uint32_t* id_count);

bool rrac_recv_data(
		RRAC* rrac,
		uint32_t* object_id,
		uint32_t* type_id,
		uint8_t** data, 
		size_t* size);

bool rrac_send_data(
		RRAC* rrac,
		uint32_t object_id,
		uint32_t type_id,
		uint32_t flags,
		uint8_t* data, 
		size_t size);

#define rrac_alloc(n) malloc(n)
#define rrac_free(p) if (p) free(p)


#endif

