/* Perl Wrapper for the librra of the SynCE Project (http://synce.sourceforge.net)
 *
 * AUTHOR: Andreas Pohl <osar@users.sourceforge.net>
 *
 * TODO: Check for memory leaks.
 *
 * $Id$
 */

%module RRA
%{
#include "librra.h"
%}

%include "librra.h"
%include "typemaps.i"

%{
bool rapiinit=false;
%}

%typemap (out) uint32_t { $result=sv_2mortal(newSVuv($1)); argvi++; }
%typemap (in) uint32_t
{
  if(! SvIOK($input))
    croak("expected an uint32_t");
  $1=SvUV($input);
}

typedef struct _ObjectType
{
  uint32_t  id;
  uint32_t  count;        /* number of objects in folder */
  uint32_t  total_size;   /* total size in bytes */
  //time_t    modified;     /* 0 or last time any object was modified */
  unsigned int modified;
  char      name[100];
} ObjectType;

typedef struct _TimeZoneInformation
{
  int32_t Bias;                       /* 00 */
  WCHAR StandardName[32];             /* 04 */
  uint16_t unknown0;                  /* 44 */
  uint16_t StandardMonthOfYear;       /* 46 */
  uint16_t unknown1;                  /* 48 */
  uint16_t StandardInstance;          /* 4a */
  uint16_t StandardStartHour;         /* 4c */
  uint8_t unknown2[6];                /* 4e */
  int32_t StandardBias;               /* 54 */
  WCHAR DaylightName[32];             /* 58 */
  uint16_t unknown3;                  /* 98 */
  uint16_t DaylightMonthOfYear;       /* 9a */
  uint16_t unknown4;                  /* 9c */
  uint16_t DaylightInstance;          /* 9e */
  uint16_t DaylightStartHour;         /* a0 */
  uint8_t unknown5[6];                /* a2 */
  int32_t DaylightBias;               /* b0 */
} TimeZoneInformation;

/*typedef bool (*NotificationFunc)(
    uint32_t type_id,
    ObjectIdArray* object_id_array,
    void* cookie);*/

/* put all ids into an array */
%typemap (out) uint32_t *ids
{
  AV *av=newAV();
  int i;
  for (i=0; i < (arg1->unchanged + arg1->changed); i++)
    av_push(av, newSVuv($1[i]));
  $result=sv_2mortal(newRV_inc((SV*)av));
  argvi++;
}

typedef struct _ObjectIdArray
{
  uint32_t  *ids;         /* changed ids first, then unchanged ids */
  uint32_t   unchanged;   /* number of unchanged ids */
  uint32_t   changed;     /* number of changed ids */
} ObjectIdArray;

/* connection to RRA */

void synce_log_set_level(int);

%typemap (out) RRA *
{
  $result=sv_newmortal();
  SWIG_MakePtr($result, $1, SWIGTYPE_p_RRA, 0);
  argvi++;

  // Initializing rapi
  if (!rapiinit)
  {
    CeRapiInit();
    rapiinit=true;
    //synce_log_set_level(4);
  }
}

RRA* rra_new();

%typemap (in) RRA *
{
  if ((SWIG_ConvertPtr($input, (void **) &$1, SWIGTYPE_p_RRA, 0)) < 0) 
    croak("no RRA (use rra_new())");
}

void rra_free(RRA* rra);
bool rra_connect(RRA* rra);
void rra_disconnect(RRA* rra);

%typemap (out) TimeZoneInformation *
{
  $result=sv_newmortal();
  SWIG_MakePtr($result, $1, SWIGTYPE_p_TimeZoneInformation, 0);
  argvi++;
}

TimeZoneInformation* rra_get_time_zone_information(RRA* rra);

%typemap(out) bool {}

/* get array of object types */

%typemap (in, numinputs=0) (ObjectType** pp_object_types, size_t* object_type_count)
  (ObjectType *tmp0, size_t tmp1)
{
  $1=&tmp0;
  $2=&tmp1;
}

%typemap (argout) (ObjectType** pp_object_types, size_t* object_type_count)
{
  if (result)
  {
    ObjectType *ot=*$1;
    AV *ret=newAV();
    int i;
    if(! $2)
      croak("rra_get_object_types failed");
    for(i=0; i<*$2; i++)
    {
      SV *sv=newSV(0);
      SWIG_MakePtr(sv, &ot[i], SWIGTYPE_p_ObjectType, 0);
      av_push(ret, sv);
    }
    $result=newRV((SV*)ret);
  }
  else
    $result=&PL_sv_undef;
  
  $result=sv_2mortal($result);
  argvi++;
}

bool rra_get_object_types(RRA* rra, 
                          ObjectType** pp_object_types,
                          size_t* object_type_count);

/* convert a name to a type id */

uint32_t rra_type_id_from_name(RRA* rra, const char* name);

/* get object ids */

%typemap (in, numinputs=0) ObjectIdArray** (ObjectIdArray *tmp) { $1=&tmp; }


%typemap (argout) ObjectIdArray**
{
  if (result)
  {
    $result=sv_newmortal();
    SWIG_MakePtr($result, *$1, SWIGTYPE_p_ObjectIdArray, 0);
  }
  else
    $result=sv_2mortal(&PL_sv_undef);
  
  argvi++;  
}

bool rra_get_object_ids(RRA* rra,
                        uint32_t object_type_id,
                        ObjectIdArray** object_id_array);

%typemap (in, numinputs=0) (uint32_t** deleted_id_array, size_t* deleted_count)
  (uint32_t *tmp0, size_t tmp1)
{
  $1=&tmp0;
  $2=&tmp1;
}

%typemap (argout) (uint32_t** deleted_id_array, size_t* deleted_count)
{
  if (result)
  {
    uint32_t *da=*$1;
    AV *ret=newAV();
    int i;
    if(! $2)
      croak("rra_get_deleted_object_ids failed");
    for(i=0; i<*$2; i++)
    {
      SV *sv=newSVuv(da[i]);
      av_push(ret, sv);
    }
    $result=newRV((SV*)ret);
  }
  else
    $result=&PL_sv_undef;
  
  $result=sv_2mortal($result);
  argvi++;  
}

bool rra_get_deleted_object_ids(RRA* rra,
                                uint32_t object_type_id,
				ObjectIdArray* object_id_array,
				uint32_t** deleted_id_array,
				size_t* deleted_count);


/* get, update, add object data 
   the data buffer must be in little endian format */ 

%typemap (in, numinputs=0) (uint8_t** data, size_t* data_size)
  (uint8_t *tmp0, size_t tmp1)
{
  $1=&tmp0;
  $2=&tmp1;
}

%typemap (argout) (uint8_t** data, size_t* data_size)
{
  if (result)
  {
    $result=sv_2mortal(newSVpvn((const char *)(*$1), *$2));
    argvi++;
    $result=sv_2mortal(newSVuv(*$2));
    argvi++;
  }
  else
  {
    $result=sv_2mortal(&PL_sv_undef);
    argvi++;
  }
}

bool rra_object_get(RRA* rra, 
                    uint32_t type_id,
                    uint32_t object_id,
                    uint8_t** data,
                    size_t* data_size);

%typemap (in, numinputs=0) uint32_t* new_object_id (uint32_t tmp) { $1=&tmp; }

%typemap (in) uint8_t* data
{
  if(! SvPOK($input))
    croak("expected a string");
  $1=(uint8_t *)$input;
}

%typemap (in) size_t data_size
{
  if(! SvIOK($input))
    croak("expected an integer");
  $1=SvIV($input);
}

/*
 * The 'check' typemaps will be placed after the conversions. At this point the data_size is
 * usable.
 */
%typemap (check) (uint8_t* data, size_t data_size)
{
  $1=(uint8_t *)SvPV((SV *)$1, $2);
}

%typemap (argout) uint32_t*
{
  if (result)
    $result=sv_2mortal(newSVuv(*$1));
  else
    $result=sv_2mortal(&PL_sv_undef);
  argvi++;
}

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

bool rra_object_delete(RRA* rra,
		       uint32_t type_id,
		       uint32_t object_id);

/* manage partnerships */

%typemap (in, numinputs=0) uint32_t* (uint32_t tmp) { $1=&tmp; }

bool rra_partner_set_current(RRA* rra, uint32_t index);
bool rra_partner_get_current(RRA* rra, uint32_t* index);

bool rra_partner_replace(RRA* rra, uint32_t index);
bool rra_partner_create(RRA* rra, uint32_t* index);

bool rra_partner_set_id(RRA* rra, uint32_t index, uint32_t id);
bool rra_partner_get_id(RRA* rra, uint32_t index, uint32_t* id);

bool rra_partner_set_name(RRA* rra, uint32_t index, const char* name);

%typemap (in, numinputs=0) char ** (char *tmp) { $1=&tmp; }
%typemap (argout) char **
{
  if (result)
  {
    SV *sv=newSV(0);
    sv_setpv(sv, *$1);
    $result=sv_2mortal(sv);
  }
  else
    $result=sv_2mortal(&PL_sv_undef);
  
  argvi++;
}

bool rra_partner_get_name(RRA* rra, uint32_t index, char** name);

/* Convert functions */

%typemap (check) (const uint8_t* data, size_t data_size)
{
  $1=(uint8_t *)SvPV((SV *)$1, $2);
}

bool rra_contact_to_vcard(uint32_t id, 
			  const uint8_t* data, 
			  size_t data_size,
			  char** vcard,
			  uint32_t flags);

bool rra_contact_from_vcard(const char* vcard, 
			    uint32_t* id,
			    uint8_t** data, 
			    size_t* data_size,
			    uint32_t flags);

bool rra_appointment_to_vevent(uint32_t id,
			       const uint8_t* data,
			       size_t data_size,
			       char** vevent,
			       uint32_t flags,
			       TimeZoneInformation* tzi);

bool rra_appointment_from_vevent(const char* vevent,
				 uint32_t* id,
				 uint8_t** data,
				 size_t* data_size,
				 uint32_t flags,
				 TimeZoneInformation* tzi);

bool rra_task_to_vtodo(uint32_t id,
		       const uint8_t* data,
		       size_t data_size,
		       char** vtodo,
		       uint32_t flags,
		       TimeZoneInformation* tzi);

bool rra_task_from_vtodo(const char* vtodo,
			 uint32_t* id,
			 uint8_t** data,
			 size_t* data_size,
			 uint32_t flags,
			 TimeZoneInformation* tzi);
