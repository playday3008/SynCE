#ifndef __librra_h__
#define __librra_h__


#ifdef __cplusplus
extern "C"
{
#endif

#ifndef SWIG
#include <synce.h>
#endif /* SWIG */

#define RRA_TYPE_NAME_APPOINTMENT  "Appointment"
#define RRA_TYPE_NAME_CONTACT      "Contact"
#define RRA_TYPE_NAME_FAVORITE     "Favorite"
#define RRA_TYPE_NAME_FILE         "File"
#define RRA_TYPE_NAME_INBOX        "Inbox"
#define RRA_TYPE_NAME_INK          "Ink"
#define RRA_TYPE_NAME_MERLIN_MAIL  "Merlin Mail"
#define RRA_TYPE_NAME_MS_TABLE     "MS Table"
#define RRA_TYPE_NAME_TASK         "Task"

struct _RRA;
typedef struct _RRA RRA;

#ifndef SWIG
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

typedef bool (*NotificationFunc)(
    uint32_t type_id,
    ObjectIdArray* object_id_array,
    void* cookie);

typedef struct 
{
  int32_t Bias;                       /* 00 */
  WCHAR Name[32];                     /* 04 */
  uint16_t unknown0;                  /* 44 */
  uint16_t StandardMonthOfYear;       /* 46 */
  uint16_t unknown1;                  /* 48 */
  uint16_t StandardInstance;          /* 4a */
  uint16_t StandardStartHour;         /* 4c */
  uint8_t unknown2[6];                /* 4e */
  int32_t StandardBias;               /* 54 */
  WCHAR Description[32];              /* 58 */
  uint16_t unknown3;                  /* 98 */
  uint16_t DaylightMonthOfYear;       /* 9a */
  uint16_t unknown4;                  /* 9c */
  uint16_t DaylightInstance;          /* 9e */
  uint16_t DaylightStartHour;         /* a0 */
  uint8_t unknown5[6];                /* a2 */
  int32_t DaylightBias;               /* b0 */
} TimeZoneInformation;

TimeZoneInformation* rra_get_time_zone_information(RRA* rra);


/* connection to RRA */

RRA* rra_new();

void rra_free(RRA* rra);

bool rra_connect(RRA* rra);

void rra_disconnect(RRA* rra);


/* get array of object types */

bool rra_get_object_types(RRA* rra, 
                          ObjectType** pp_object_types,
                          size_t* object_type_count);


/* convert a name to a type id */

uint32_t rra_type_id_from_name(
    RRA* rra,
    const char* name);


/* get changes */

bool rra_get_changes(
    RRA* rra, 
    uint32_t* object_type_ids, 
    size_t object_type_count, 
    NotificationFunc func,
    void* cookie);

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
#endif /* SWIG */

#define rra_free_deleted_object_ids(deleted_ids)  if(deleted_ids) free(deleted_ids);


/* get, update, add object data 
   the data buffer must be in little endian format */

#ifndef SWIG
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

bool rra_partner_replace(RRA* rra, uint32_t index);
bool rra_partner_create(RRA* rra, uint32_t* index);

#endif /* SWIG */

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

#ifndef SWIG
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
#endif /* SWIG */

#define rra_contact_free_vcard(p) if (p) free(p)
#define rra_contact_free_data(p)  if (p) free(p)

#define RRA_APPOINTMENT_ID_UNKNOWN  0

#define RRA_APPOINTMENT_NEW     				0x1
#define RRA_APPOINTMENT_UPDATE  				0x2
#define RRA_APPOINTMENT_COMMAND_MASK		0xf

#define RRA_APPOINTMENT_ISO8859_1				0x10
#define RRA_APPOINTMENT_UTF8						0x20
#define RRA_APPOINTMENT_CHARSET_MASK		0xf0

#ifndef SWIG
bool rra_appointment_to_vevent(
    uint32_t id,
    const uint8_t* data,
    size_t data_size,
    char** vevent,
    uint32_t flags,
    TimeZoneInformation* tzi);

bool rra_appointment_from_vevent(
    const char* vevent,
    uint32_t* id,
    uint8_t** data,
    size_t* data_size,
    uint32_t flags);
#endif /* SWIG */

#define RRA_TASK_ID_UNKNOWN  0

#define RRA_TASK_NEW     				0x1
#define RRA_TASK_UPDATE  				0x2
#define RRA_TASK_COMMAND_MASK		0xf

#define RRA_TASK_ISO8859_1				0x10
#define RRA_TASK_UTF8						0x20
#define RRA_TASK_CHARSET_MASK		0xf0

#ifndef SWIG
bool rra_task_to_vtodo(
    uint32_t id,
    const uint8_t* data,
    size_t data_size,
    char** vtodo,
    uint32_t flags);

bool rra_task_from_vtodo(
    const char* vtodo,
    uint32_t* id,
    uint8_t** data,
    size_t* data_size,
    uint32_t flags);
#endif /* SWIG */


#ifdef __cplusplus
}
#endif

#endif

