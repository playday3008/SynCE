#ifndef __librra_h__
#define __librra_h__


#ifdef __cplusplus
extern "C"
{
#endif

#include <synce.h>

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

bool rra_object_add(RRA* rra,  
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


bool rra_partner_set_current(RRA* rra, uint32_t index);
bool rra_partner_get_current(RRA* rra, uint32_t* index);

bool rra_partner_set_id(RRA* rra, uint32_t index, uint32_t id);
bool rra_partner_get_id(RRA* rra, uint32_t index, uint32_t* id);

bool rra_partner_set_name(RRA* rra, uint32_t index, const char* name);
bool rra_partner_get_name(RRA* rra, uint32_t index, char** name);

#define rra_partner_free_name(name)  rapi_reg_free_string(name)




#if 0
bool rra_lock(RRA* rra);
bool rra_unlock(RRA* rra);
#endif

#ifdef __cplusplus
}
#endif

#endif

