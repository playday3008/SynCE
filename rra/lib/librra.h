#ifndef __librra_h__
#define __librra_h__


#ifdef __cplusplus
extern "C"
{
#endif

#include <synce.h>

#define RRA_OBJECT_TYPE_MS_TABLE     0x2710
#define RRA_OBJECT_TYPE_APPOINTMENT  0x2711
#define RRA_OBJECT_TYPE_CONTACT      0x2712
#define RRA_OBJECT_TYPE_TASK         0x2713
#define RRA_OBJECT_TYPE_FILE         0x2714
#define RRA_OBJECT_TYPE_MERLIN_MAIL  0x2715
#define RRA_OBJECT_TYPE_FAVORITE     0x2716
#define RRA_OBJECT_TYPE_INK          0x2717

struct _RRA;
typedef struct _RRA RRA;

typedef struct _ObjectType
{
  uint32_t  id;
  uint32_t  count;        /* number of objects in folder */
  uint32_t  total_size;   /* total size in bytes */
  time_t    modified;     /* 0 or last time any object was modified */
  char      name[100];
} ObjectType;

typedef struct _ObjectIdArray
{
  uint32_t*  ids;         /* changed ids first, then unchanged ids */
  uint32_t   unchanged;   /* number of unchanged ids */
  uint32_t   changed;     /* number of changed ids */
} ObjectIdArray;


/* connection to RRA */

RRA* rra_new();

void rra_free(RRA* rra);

bool rra_connect(RRA* rra);

void rra_disconnect(RRA* rra);


/* get array of object types */

bool rra_get_object_types(RRA* rra, 
                          ObjectType** pp_object_types,
                          size_t* object_type_count);

/* get object ids */

bool rra_get_object_ids(RRA* rra,
                        uint32_t object_type_id,
                        ObjectIdArray** object_id_array);

void rra_free_object_ids(ObjectIdArray* object_id_array);

bool rra_get_deleted_object_ids(RRA* rra,
                                uint32_t object_type_id,
		                            ObjectIdArray* object_id_array,
																uint32_t** deleted_id_array,
																size_t* deleted_count);

#define rra_free_deleted_object_ids(deleted_ids)  if(deleted_ids) free(deleted_ids);


/* get, update, add object data 
   the data buffer must be in little endian format */ 

bool rra_object_get(RRA* rra, 
                    uint32_t type_id,
                    uint32_t object_id,
                    uint8_t** data,
                    size_t* data_size);

void rra_object_free_data(uint8_t* data);

bool rra_object_put(RRA* rra,  
                    uint32_t type_id,
                    uint32_t object_id,
										uint32_t flags,
                    uint8_t* data,
                    size_t data_size,
									  uint32_t* new_object_id);

bool rra_object_new(RRA* rra,  
                    uint32_t type_id,
                    uint8_t* data,
                    size_t data_size,
									  uint32_t* new_object_id);

bool rra_object_update(RRA* rra,  
                       uint32_t type_id,
                       uint32_t object_id,
                       uint8_t* data,
                       size_t data_size);

bool rra_object_delete(RRA* rra, uint32_t type_id, uint32_t object_id);

/* manage partnerships */

bool rra_partner_set_current(RRA* rra, uint32_t index);
bool rra_partner_get_current(RRA* rra, uint32_t* index);

bool rra_partner_set_id(RRA* rra, uint32_t index, uint32_t id);
bool rra_partner_get_id(RRA* rra, uint32_t index, uint32_t* id);

bool rra_partner_set_name(RRA* rra, uint32_t index, const char* name);
bool rra_partner_get_name(RRA* rra, uint32_t index, char** name);

#define rra_partner_free_name(name)  rapi_reg_free_string(name)


/*
 * Convert contact data
 */

#define RRA_CONTACT_ID_UNKNOWN  0

/* flags for rra_contact_(from|to)_vcard() */

#define RRA_CONTACT_NEW     				0x1
#define RRA_CONTACT_UPDATE  				0x2
#define RRA_CONTACT_COMMAND_MASK		0xf

#define RRA_CONTACT_ISO8859_1				0x10
#define RRA_CONTACT_UTF8						0x20
#define RRA_CONTACT_CHARSET_MASK		0xf0

#define RRA_CONTACT_VERSION_UNKNOWN   0x000
#define RRA_CONTACT_VERSION_2_1   		0x100
#define RRA_CONTACT_VERSION_3_0   		0x200
#define RRA_CONTACT_VERSION_MASK			0xf00


bool rra_contact_to_vcard(
		uint32_t id, 
		const uint8_t* data, 
		size_t data_size,
		char** vcard,
		uint32_t flags);

bool rra_contact_from_vcard(
		const char* vcard, 
		uint32_t* id,
		uint8_t** data, 
		size_t* data_size,
		uint32_t flags);

#define rra_contact_free_vcard(p) if (p) free(p)
#define rra_contact_free_data(p)  if (p) free(p)

#if 0
bool rra_dbstream_to_string(
		uint32_t type_id,
		uint32_t object_id,
		const uint8_t* data,
		size_t data_size,
		char** str);

bool rra_dbstream_from_string(
		uint32_t type_id,
		uint32_t object_id,
		char** str,
		uint8_t** data,
		size_t* data_size);
#endif

#if 0
bool rra_lock(RRA* rra);
bool rra_unlock(RRA* rra);
#endif

#ifdef __cplusplus
}
#endif

#endif

