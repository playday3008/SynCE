/* $Id$ */
#define _GNU_SOURCE 1
#include "librra.h"
#include "appointment_ids.h"
#include "dbstream.h"
#include <synce_log.h>
#include <libmimedir.h>
#include <string.h>
#include <assert.h>

#define MAX_FIELD_COUNT 50 /* just a guess */

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

#if 0

struct _AppointmentToVevent/*{{{*/
{
  uint32_t id;
	CEPROPVAL* fields;
	uint32_t field_count;
  char** vevent;
  uint32_t flags;
};/*}}}*/

typedef struct _AppointmentToVevent AppointmentToVevent;

bool rra_appointment_to_vevent(/*{{{*/
    uint32_t id,
    const uint8_t* data,
    size_t data_size,
    char** vevent,
    uint32_t flags)
{
	bool success = false;
	uint32_t field_count = 0;
	CEPROPVAL* fields = NULL;

	if (!data)
	{
		synce_error("Data is NULL");
		goto exit;
	}

	if (data_size < 8)
	{
		synce_error("Invalid data size");
		goto exit;
	}

	field_count = letoh32(*(uint32_t*)(data + 0));
	synce_trace("Field count: %i", field_count);

	if (0 == field_count)
	{
		synce_error("No fields!");
		goto exit;
	} 
	
	if (field_count > MAX_FIELD_COUNT)
	{
		synce_error("A contact does not have this many fields");
		goto exit;
	}

	fields = (CEPROPVAL*)malloc(sizeof(CEPROPVAL) * field_count);

	if (!dbstream_to_propvals(data + 8, field_count, fields))
	{
		fprintf(stderr, "Failed to convert database stream\n");
		goto exit;
	}

	if (!rra_contact_to_vcard2(
				id, 
				fields, 
				field_count, 
				vcard,
				flags))
	{
		fprintf(stderr, "Failed to create vCard\n");
		goto exit;
	}

	success = true;

exit:
	dbstream_free_propvals(fields);
	return success;
}/*}}}*/

#endif

#if 0
typedef bool (*mdir_func)(mdir_line* line, void* cookie);

/**
  Call func for each line until func returns false or we are out of lines
*/
static mdir_foreach(mdir_line** lines, mdir_func func, void* cookie)/*{{{*/
{
  mdir_line **i;
  
  for(i = lines; *i; i++)
  {
    if (!func(*i, cookie))
      break;
  }
}/*}}}*/
#endif

struct _AppointmentFromVevent
{
  const char* vevent;
  uint32_t id;
  CEPROPVAL fields[MAX_FIELD_COUNT];
	size_t field_index;
	size_t field_count;
  uint32_t flags;
  mdir_line** lines;
  mdir_line** iter;
};

typedef struct _AppointmentFromVevent AppointmentFromVevent;

#if 0
static bool rra_on_vevent_line(mdir_line* line, void* cookie)
{
  AppointmentFromVevent* afv = (AppointmentFromVevent*)cookie;
  
}
#endif

static time_t datetime_to_unix_time(const char* datetime)/*{{{*/
{
  int count;
  char suffix = 0;
  struct tm time_struct;
  memset(&time_struct, 0, sizeof(time_struct));
  
  count = sscanf(datetime, "%4d%2d%2dT%2d%2d%2d%1c", 
      &time_struct.tm_year,
      &time_struct.tm_mon,
      &time_struct.tm_mday,
      &time_struct.tm_hour,
      &time_struct.tm_min,
      &time_struct.tm_sec,
      &suffix);

  if (count < 6)
  {
    synce_error("Bad date-time: '%s'", datetime);
    return 0;
  }
  
  if (count >= 7)
  {
    if ('Z' != suffix)
      synce_warning("Unknown date-time suffix: '%c'", suffix);
  }

  time_struct.tm_year -= 1900;
  time_struct.tm_mon--;
  time_struct.tm_isdst = -1;
  
  return mktime(&time_struct);
}/*}}}*/

static bool add_filetime(AppointmentFromVevent* afv, uint32_t id, mdir_line *line)/*{{{*/
{
	CEPROPVAL* field = &afv->fields[afv->field_index++];
  char **data_type = mdir_get_param_values(line, "VALUE");

	assert(line);
	
	field->propid = (id << 16) | CEVT_FILETIME;

  if (data_type) 
  {
    if (0 != strcasecmp(data_type[0], "DATE-TIME"))
    {
      synce_error("Unexpected data type: '%s'", data_type[0]);
      return false;
    }
  }

  filetime_from_unix_time(datetime_to_unix_time(line->values[0]), &field->val.filetime);

  return true;
}/*}}}*/

static bool rra_begin_any(/*{{{*/
    AppointmentFromVevent* afv,
    const char *type)
{
  bool success = false;
  mdir_line* line = NULL;

  for (afv->iter++; (line = *afv->iter); afv->iter++)
  {
    /*synce_trace("%s", line->name);*/

    if (STR_EQUAL(line->name, "BEGIN"))/*{{{*/
    {
      synce_warning("Unexpected BEGIN: '%s'", line->values[0]);
      if (!rra_begin_any(afv, line->values[0]))
        break;
    }/*}}}*/
    else if (STR_EQUAL(line->name, "END"))/*{{{*/
    {
      if (STR_EQUAL(line->values[0], type))
      {
        success = true;
        break;
      }
      else
      {
        synce_error("Unexpected END: '%s'", line->values[0]);
        break;
      }
    }/*}}}*/
  }

  return success;
}/*}}}*/

static bool rra_begin_valarm(/*{{{*/
    AppointmentFromVevent* afv)
{
  bool success = false;
  mdir_line* line = NULL;

  for (afv->iter++; (line = *afv->iter); afv->iter++)
  {
    synce_trace("%s", line->name);
    
    if (STR_EQUAL(line->name, "END"))/*{{{*/
    {
      if (STR_EQUAL(line->values[0], "VALARM"))
      {
        success = true;
        break;
      }
      else
      {
        synce_error("Unexpected END: '%s'", line->values[0]);
        break;
      }
    }/*}}}*/
  }

  return success;
}/*}}}*/

static bool rra_begin_vevent(/*{{{*/
    AppointmentFromVevent* afv)
{
  bool success = false;
  mdir_line* line = NULL;

  for (afv->iter++; (line = *afv->iter); afv->iter++)
  {
    synce_trace("%s", line->name);
    if (STR_EQUAL(line->name, "BEGIN"))/*{{{*/
    {
#if 0
      if (STR_EQUAL(line->values[0], "VTIMEZONE"))
      {
        if (!rra_begin_vtimezone(afv))
          break;
      }
      else
#endif
      if (STR_EQUAL(line->values[0], "VALARM"))
      {
        if (!rra_begin_valarm(afv))
          break;
      }
      else
      {
        synce_warning("Unexpected BEGIN: '%s'", line->values[0]);
        if (!rra_begin_any(afv, line->values[0]))
          break;
      }
    }/*}}}*/
    else if (STR_EQUAL(line->name, "END"))/*{{{*/
    {
      if (STR_EQUAL(line->values[0], "VEVENT"))
      {
        success = true;
        break;
      }
      else
      {
        synce_error("Unexpected END: '%s'", line->values[0]);
        break;
      }
    }/*}}}*/
    else if (STR_EQUAL(line->name, "DTSTART"))/*{{{*/
    {
      add_filetime(afv, ID_START, line);
    }/*}}}*/
  }

  return success;
}/*}}}*/

static bool rra_begin_vcalendar(/*{{{*/
    AppointmentFromVevent* afv)
{
  bool success = false;
  mdir_line* line = NULL;

  for (afv->iter++; (line = *afv->iter); afv->iter++)
  {
    synce_trace("%s", line->name);
    if (STR_EQUAL(line->name, "BEGIN"))/*{{{*/
    {
      if (STR_EQUAL(line->values[0], "VEVENT"))
      {
        if (!rra_begin_vevent(afv))
          break;
      }
      else
      {
        synce_warning("Unexpected BEGIN: '%s'", line->values[0]);
        if (!rra_begin_any(afv, line->values[0]))
          break;
      }
    }/*}}}*/
    else if (STR_EQUAL(line->name, "END"))/*{{{*/
    {
      if (STR_EQUAL(line->values[0], "VCALENDAR"))
      {
        success = true;
        break;
      }
      else
      {
        synce_error("Unexpected END: '%s'", line->values[0]);
        break;
      }
    }/*}}}*/
  }

  return success;
}/*}}}*/

static bool rra_appointment_from_vevent2(
    AppointmentFromVevent* afv)
{
  bool success = false;
  afv->iter = afv->lines = mdir_parse((char*)afv->vevent);
  mdir_line* line = *afv->iter;
  
  if (!afv->lines || !line)
  {
    fprintf(stderr, "Failed to parse vEvent\n");
    goto exit;
  }

  synce_trace("%s", line->name);
  if (STR_EQUAL(line->name, "BEGIN"))/*{{{*/
  {
    if (STR_EQUAL(line->values[0], "VCALENDAR"))
    {
      if (!rra_begin_vcalendar(afv))
        goto exit;
    }
    else if (STR_EQUAL(line->values[0], "VEVENT"))
    {
      if (!rra_begin_vevent(afv))
        goto exit;
    }
    else
    {
      synce_error("Unexpected BEGIN");
      goto exit;
    }
  }/*}}}*/

  if (*++afv->iter)
  {
    synce_warning("Unexpected data in buffer: '%s'", (*afv->iter)->name);
  }

  success = true;

exit:
  mdir_free(afv->lines);
  afv->lines = NULL;
  return success;
}

bool rra_appointment_from_vevent(/*{{{*/
    const char* vevent,
    uint32_t* id,
    uint8_t** data,
    size_t* data_size,
    uint32_t flags)
{
	bool success = false;
  AppointmentFromVevent afv;

  memset(&afv, 0, sizeof(AppointmentFromVevent));
  afv.vevent = vevent;
  afv.flags  = flags;
	
	if (!rra_appointment_from_vevent2(&afv))
	{
		synce_error("Failed to convert vEvent to database entries");
		goto exit;
	}

	if (!dbstream_from_propvals(
				afv.fields,
				afv.field_index,
				data,
				data_size))
	{
		synce_error("Failed to convert database entries to stream");
		goto exit;
	}

  if (id)
    *id = afv.id;

	success = true;

exit:
	return success;
}/*}}}*/

