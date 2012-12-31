#ifndef __rrac_h__
#define __rrac_h__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <rapi2.h>

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

bool rrac_connect(RRAC* rrac, IRAPISession *session);

void rrac_disconnect(RRAC* rrac);

bool rrac_is_connected(RRAC* rrac);

/** Get file descriptor for use with select() or poll() */
int rrac_get_event_descriptor(RRAC* self);

/** See if there is an event pending */
bool rrac_event_pending(RRAC* self);

/** Wait for an event */
bool rrac_event_wait(RRAC* self, int timeoutInSeconds, bool* gotEvent);


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
		uint32_t* object_type_count);

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

struct _SyncCommand;
typedef struct _SyncCommand SyncCommand;

#define SYNC_COMMAND_ERROR              0x006e
#define SYNC_COMMAND_NOTIFY             0x0069
#define SYNC_COMMAND_NEGOTIATION        0x0065

#define SYNC_COMMAND_NOTIFY_UPDATE      0x00000000
#define SYNC_COMMAND_NOTIFY_PARTNERS    0x02000000
#define SYNC_COMMAND_NOTIFY_IDS_4       0x04000000
#define SYNC_COMMAND_NOTIFY_IDS_6       0x06000000
#define SYNC_COMMAND_NOTIFY_INVALID     0xffffffff

/* Destroy SyncCommand object */
void sync_command_destroy(SyncCommand* self);

/* Get command code from SyncCommand object */
uint16_t sync_command_code(SyncCommand* self);

/* Get notify code from SyncCommand object if command code is SYNC_COMMAND_NOTIFY */
uint32_t sync_command_notify_code(SyncCommand* self);

typedef struct
{
	uint32_t  current;
	uint32_t  ids[2];
} SyncPartners;

/** Get SyncParners structure if notify code is SYNC_COMMAND_NOTIFY_PARTNERS */
bool sync_command_notify_partners(SyncCommand* self, SyncPartners* partners);

typedef struct
{
  uint32_t  notify_code;
  uint32_t  type;
  uint32_t  total;
  uint32_t  deleted;
  uint32_t  unchanged;
  uint32_t  changed;
} SyncNotifyHeader;

/** Get total and changed ID count if notify code is SYNC_COMMAND_NOTIFY_IDS_[46] */
bool sync_command_notify_header(SyncCommand* self, SyncNotifyHeader* header);

/** Get IDs if notify code is SYNC_COMMAND_NOTIFY_IDS_[46] */
bool sync_command_notify_ids(SyncCommand* self, uint32_t* ids);

typedef struct
{
	uint32_t    type_id;
	uint32_t    old_id;
	uint32_t    new_id;
	uint32_t    flags;
} SyncNegotiation;

/** Get SYNC_COMMAND_NEGOTIATION data */
bool sync_command_negotiation_get(
    SyncCommand* self, 
    SyncNegotiation* negotiation);

SyncCommand* rrac_recv_command(RRAC* self);

#endif

