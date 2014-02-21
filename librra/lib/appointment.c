/* $Id$ */
#define _GNU_SOURCE 1
#include "appointment.h"
#include "appointment_ids.h"
#include "generator.h"
#include "parser.h"
#include "strbuf.h"
#include "strv.h"
#include <rapitypes.h>
#include <synce_log.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "common_handlers.h"
#include "recurrence.h"
#include "timezone.h"
#include "../rra_config.h"

/** 
 * @defgroup RRA_converters RRA format conversion public API
 * @ingroup RRA
 * @brief The public RRA API for converting data objects to and from vcard, vcal etc.
 *
 * @{ 
 */ 

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

#define MINUTES_PER_DAY (24*60)
#define SECONDS_PER_DAY (MINUTES_PER_DAY*60)

#define BLOB0067_STR  "BLOB0067="

#if 0
static const uint8_t BLOB0067_HEADER[20] = { 
  0x04, 0x00, 0x00, 0x00, 
  0x82, 0x00, 0xe0, 0x00,
  0x74, 0xc5, 0xb7, 0x10,
  0x1a, 0x82, 0xe0, 0x08,
  0x00, 0x00, 0x00, 0x00 
};
#endif

#define VCAL_1_0_PROP_TRANSP_OPAQUE      "0"
#define VCAL_1_0_PROP_TRANSP_TRANSPARENT "1"

/*
   Any on_propval_* functions not here are found in common_handlers.c
*/

static bool on_propval_busy_status_vcal(Generator* g, CEPROPVAL* propval, void* cookie)/*{{{*/
{
  switch (propval->val.iVal)
  {
    case BUSY_STATUS_FREE:
      generator_add_simple(g, "TRANSP", VCAL_1_0_PROP_TRANSP_TRANSPARENT);
      break;
      
    case BUSY_STATUS_TENTATIVE:
      synce_warning("Busy status 'tentative' not specified for Vcal, setting to OPAQUE");
      generator_add_simple(g, "TRANSP", VCAL_1_0_PROP_TRANSP_OPAQUE);
      break;
      
    case BUSY_STATUS_BUSY:
      generator_add_simple(g, "TRANSP", VCAL_1_0_PROP_TRANSP_OPAQUE);
      break;
      
    case BUSY_STATUS_OUT_OF_OFFICE:
      synce_warning("Busy status 'out of office' not specified for Vcal, setting to OPAQUE");
      generator_add_simple(g, "TRANSP", VCAL_1_0_PROP_TRANSP_OPAQUE);
      break;
      
    default:
      synce_warning("Unknown busy status: %04x", propval->val.iVal);
      break;
  }
  return true;
}/*}}}*/

static bool on_propval_busy_status_ical(Generator* g, CEPROPVAL* propval, void* cookie)/*{{{*/
{
  switch (propval->val.iVal)
  {
    case BUSY_STATUS_FREE:
      generator_add_simple(g, "TRANSP", "TRANSPARENT");
      break;
      
    case BUSY_STATUS_TENTATIVE:
      synce_warning("Busy status 'tentative' not specified for iCal, setting to OPAQUE");
      generator_add_simple(g, "TRANSP", "OPAQUE");
      break;
      
    case BUSY_STATUS_BUSY:
      generator_add_simple(g, "TRANSP", "OPAQUE");
      break;
      
    case BUSY_STATUS_OUT_OF_OFFICE:
      synce_warning("Busy status 'out of office' not specified for iCal, setting to OPAQUE");
      generator_add_simple(g, "TRANSP", "OPAQUE");
      break;
      
    default:
      synce_warning("Unknown busy status: %04x", propval->val.iVal);
      break;
  }
  return true;
}/*}}}*/

static bool on_propval_duration(Generator* g, CEPROPVAL* propval, void* cookie)
{
  GeneratorData* data = (GeneratorData*)cookie;
  data->duration = propval;
  return true;
}

static bool on_propval_type(Generator* g, CEPROPVAL* propval, void* cookie)
{
  GeneratorData* data = (GeneratorData*)cookie;
  data->type = propval;
  return true;
}

static bool on_propval_recurrence_pattern(Generator* g, CEPROPVAL* propval, void* cookie)
{
#if ENABLE_RECURRENCE
  GeneratorData* data = (GeneratorData*)cookie;
  data->recurrence_pattern = propval;
#else
  synce_warning("Recurrence support not enabled");
#endif
  return true;
}

static bool on_propval_recurrence_timezone(Generator* g, CEPROPVAL* propval, void* cookie)
{
#if ENABLE_RECURRENCE
  GeneratorData* data = (GeneratorData*)cookie;
  data->recurrence_timezone = propval;
#else
  synce_warning("Recurrence support not enabled");
#endif
  return true;
}

static bool on_propval_start(Generator* g, CEPROPVAL* propval, void* cookie)
{
  GeneratorData* data = (GeneratorData*)cookie;
  data->start = propval;
  return true;
}

#if ENABLE_RECURRENCE
static bool on_propval_unique(Generator* g, CEPROPVAL* propval, void* cookie)
{
  GeneratorData* data = (GeneratorData*)cookie;
  data->unique = propval;
  return true;
}
#endif

static bool on_propval_notes(Generator* g, CEPROPVAL* propval, void* cookie)/*{{{*/
{
  return process_propval_notes(g, propval, cookie, ((GeneratorData*)cookie)->codepage);
}

static bool on_propval_occurrence(Generator* g, CEPROPVAL* propval, void* cookie)/*{{{*/
{
  GeneratorData* data = (GeneratorData*)cookie;
  data->occurrence = propval;
  return true;
}

static bool on_propval_attendees(Generator* g, CEPROPVAL* propval, void* cookie)/*{{{*/
{
  char *name = NULL;
  char *email = NULL;
  char *tmp = NULL;
  int i;
  size_t offset;

  if ((propval->propid & 0xffff) != CEVT_BLOB)
  {
    synce_error("For ID_ATTENDEES, expecting a BLOB");
    return true;
  }

  LPBYTE buffer_start = propval->val.blob.lpb;

  /* first field is number of entries; assuming a DWORD */
  int num_attendees = letoh32(*((DWORD*)buffer_start));

  /* following the 4 byte number of attendees are 12 empty bytes */
  offset = 16;

  for (i = 0; i < num_attendees; i++) {
    /* Each attendee entry starts with 16 bytes, purpose unknown,
     * followed by the name as WSTR, and email address as WSTR.
     * The entry finishes on an 8 byte boundary
     */
    offset = offset + 16;

    name = wstr_to_utf8((LPWSTR)(buffer_start + offset));
    offset = offset + ((wstrlen((LPWSTR)(buffer_start + offset)) + 1) * sizeof(WCHAR));

    email = wstr_to_utf8((LPWSTR)(buffer_start + offset));
    offset = offset + ((wstrlen((LPWSTR)(buffer_start + offset)) + 1) * sizeof(WCHAR));

    tmp = malloc(strlen(name) + strlen(email) + 4);
    snprintf(tmp, strlen(name) + strlen(email) + 4, "%s <%s>", name, email);

    generator_add_simple(g, "ATTENDEE", tmp);
    free(tmp);

    offset = offset + (8 - (offset % 8));

    free(name);
    free(email);
  }

  return true;
}

static bool on_propval_attendee_notified_time(Generator* g, CEPROPVAL* propval, void* cookie)/*{{{*/
{
  time_t start_time;
  char buffer[32];
  parser_filetime_to_unix_time(&propval->val.filetime, &start_time);
  strftime(buffer, sizeof(buffer), "%Y%m%dT%H%M%SZ", gmtime(&start_time));
  synce_debug("No vcal equivalent for ID_ATTENDEE_NOTIFIED_TIME: filetime: %08x %08x=%s",
              propval->val.filetime.dwHighDateTime,
              propval->val.filetime.dwLowDateTime,
              buffer);
  return true;
}


/** @brief Convert an RRA appointment to vcal
 * 
 * This function converts an appointment in RRA format from a WM
 * device to vcal format.
 * 
 * @param[in] id unique id of the record, or RRA_APPOINTMENT_ID_UNKNOWN
 * @param[in] data record data to be converted
 * @param[in] data_size size of the data
 * @param[out] vevent address of a pointer to recieve the location of the resulting vevent
 * @param[in] flags bitwise or the flags to control the conversion
 * @param[in] tzi timezone of the device
 * @param[in] codepage encoding used by the device
 * @return TRUE on success, FALSE on failure
 */ 
bool rra_appointment_to_vevent(/*{{{*/
    uint32_t id,
    const uint8_t* data,
    size_t data_size,
    char** vevent,
    uint32_t flags,
    RRA_Timezone* tzi,
    const char *codepage)
{
  bool success = false;
  Generator* generator = NULL;
  unsigned generator_flags = 0;
  GeneratorData event_generator_data;
  memset(&event_generator_data, 0, sizeof(GeneratorData));
  event_generator_data.codepage = codepage;

  if (!(flags & RRA_APPOINTMENT_VERSION_MASK))
    flags = flags | RRA_APPOINTMENT_VERSION_DEFAULT;

  event_generator_data.flags = flags;

  switch (flags & RRA_APPOINTMENT_CHARSET_MASK)
  {
    case RRA_APPOINTMENT_UTF8:
      generator_flags = GENERATOR_UTF8;
      break;

    case RRA_APPOINTMENT_ISO8859_1:
    default:
      /* do nothing */
      break;
  }

  generator = generator_new(generator_flags, &event_generator_data);
  if (!generator)
    goto exit;

  switch (flags & RRA_APPOINTMENT_VERSION_MASK)
  {
  case RRA_APPOINTMENT_VCAL_1_0:
    generator_add_property(generator, ID_BUSY_STATUS, on_propval_busy_status_vcal);
    break;
  case RRA_APPOINTMENT_VCAL_2_0:
    generator_add_property(generator, ID_BUSY_STATUS, on_propval_busy_status_ical);
    break;
  default:
    break;
  }

  generator_add_property(generator, ID_CATEGORIES,  on_propval_categories);
  generator_add_property(generator, ID_DURATION,    on_propval_duration);
  generator_add_property(generator, ID_APPOINTMENT_TYPE, on_propval_type);
  generator_add_property(generator, ID_LOCATION,    on_propval_location);
  generator_add_property(generator, ID_NOTES,       on_propval_notes);
  generator_add_property(generator, ID_REMINDER_MINUTES_BEFORE, on_propval_reminder_minutes);
  generator_add_property(generator, ID_REMINDER_ENABLED, on_propval_reminder_enabled);
  generator_add_property(generator, ID_REMINDER_OPTIONS, on_propval_reminder_options);
  generator_add_property(generator, ID_REMINDER_SOUND_FILE,    on_propval_reminder_sound_file);
  generator_add_property(generator, ID_SENSITIVITY, on_propval_sensitivity);
  generator_add_property(generator, ID_APPOINTMENT_START,       on_propval_start);
  generator_add_property(generator, ID_OCCURRENCE,  on_propval_occurrence);
  generator_add_property(generator, ID_RECURRENCE_PATTERN, on_propval_recurrence_pattern);
  generator_add_property(generator, ID_RECURRENCE_TIMEZONE, on_propval_recurrence_timezone);
  generator_add_property(generator, ID_SUBJECT,     on_propval_subject);
#if ENABLE_RECURRENCE
  generator_add_property(generator, ID_UNIQUE,      on_propval_unique);
#endif
  generator_add_property(generator, ID_ATTENDEE_NOTIFIED_TIME, on_propval_attendee_notified_time);
  generator_add_property(generator, ID_ATTENDEES, on_propval_attendees);

  if (!generator_set_data(generator, data, data_size))
    goto exit;

  /*
   * VCALENDAR container prelim
   */
  generator_add_simple(generator, "BEGIN", "VCALENDAR");

  switch (flags & RRA_APPOINTMENT_VERSION_MASK)
  {
    case RRA_APPOINTMENT_VCAL_1_0:
      generator_add_simple(generator, "VERSION", "1.0");
      break;
    case RRA_APPOINTMENT_VCAL_2_0:
      generator_add_simple(generator, "VERSION", "2.0");
      /*
      generator_add_simple(generator, "METHOD", "PUBLISH");
      */
      break;
  }

  generator_add_simple(generator, "PRODID", "-//SynCE//NONSGML SynCE RRA//EN");

  /*
   * VEVENT data
   */ 
  generator_add_simple(generator, "BEGIN", "VEVENT");

  if (id != RRA_APPOINTMENT_ID_UNKNOWN)
  {
    char id_str[32];
    snprintf(id_str, sizeof(id_str), "RRA-ID-%08x", id);
    generator_add_simple(generator, "UID", id_str);
  }
  if (!generator_run(generator))
    goto exit;

  if (event_generator_data.start && 
      event_generator_data.duration &&
      event_generator_data.type)
  {
    char buffer[32];
    time_t start_time = 0;
    time_t end_time = 0;
    const char* type = NULL;
    const char* format = NULL;
    struct tm* (*xtime)(const time_t *timep) = NULL;

    if (!parser_filetime_to_unix_time(&event_generator_data.start->val.filetime, &start_time))
      goto exit;

    switch (event_generator_data.type->val.lVal)
    {
      case APPOINTMENT_TYPE_ALL_DAY:
        xtime  = localtime;
        type   = "DATE";
        format = "%Y%m%d";

        /* days to seconds */
        end_time = start_time + 
          ((event_generator_data.duration->val.lVal / MINUTES_PER_DAY) + 1) *
          SECONDS_PER_DAY;
        break;


      case APPOINTMENT_TYPE_NORMAL:
        xtime  = gmtime;
        type   = "DATE-TIME";
        if (!tzi)
          format = "%Y%m%dT%H%M%SZ";
        else
          format = "%Y%m%dT%H%M%S";

        /* minutes to seconds */
        end_time = start_time + 
          event_generator_data.duration->val.lVal * 60;

        if (tzi)
        {
          start_time = rra_timezone_convert_from_utc(tzi, start_time);
          end_time   = rra_timezone_convert_from_utc(tzi, end_time);
        }
      
        break;

      default:
        synce_warning("Unknown appintment type: %i", 
            event_generator_data.type->val.lVal);
        break;
    }

    if (type && format)
    {
      strftime(buffer, sizeof(buffer), format, xtime(&start_time));
      generator_add_with_type(generator, "DTSTART", type, buffer);
      
      if (end_time)
      {
        strftime(buffer, sizeof(buffer), format, xtime(&end_time));
        generator_add_with_type(generator, "DTEND",   type, buffer);
      }
    }
  }
  else
  {
    synce_warning("Missing start, duration or duration unit");
  }

  switch (flags & RRA_APPOINTMENT_VERSION_MASK)
  {
  case RRA_APPOINTMENT_VCAL_1_0:
    to_vcalendar_alarm(generator,
		       event_generator_data.start,
		       event_generator_data.reminder_enabled,
		       event_generator_data.reminder_minutes,
		       event_generator_data.reminder_options,
		       tzi);
    break;

  case RRA_APPOINTMENT_VCAL_2_0:
    to_icalendar_alarm(generator,
                       event_generator_data.reminder_enabled,
                       event_generator_data.reminder_minutes,
                       event_generator_data.reminder_options,
                       REMINDER_RELATED_START);
    break;

  default:
    break;
  }

#if ENABLE_RECURRENCE
  if (event_generator_data.occurrence && (event_generator_data.occurrence->val.iVal == OCCURRENCE_REPEATED))
  {
    if (!event_generator_data.recurrence_pattern) {
      synce_error("Found recurring appointment with no recurrence pattern");
    }
    else
    {

      /*
       * If we've been passed a timezone, use that, otherwise try to use the timezone
       * info in the appointment itself
       */
      RRA_Timezone *recurr_tzi = NULL;
      if (tzi == NULL && event_generator_data.recurrence_timezone)
      {
	if ( (CEVT_BLOB == (event_generator_data.recurrence_timezone->propid & 0xffff))
	    && event_generator_data.recurrence_timezone->val.blob.dwCount == 104)
	{
	  LPBYTE p = event_generator_data.recurrence_timezone->val.blob.lpb;

	  recurr_tzi = calloc(1,sizeof(RRA_Timezone));

	  recurr_tzi->Bias = letoh32(*((PLONG)p));
	  p+=4;
	  recurr_tzi->StandardBias = letoh32(*((PLONG)p));
	  p+=4;
	  recurr_tzi->DaylightBias = letoh32(*((PLONG)p));
	  p+=4;
	  /* unknown 4 bytes */
	  p+=4;

	  recurr_tzi->StandardMonthOfYear = letoh16(*((LPWORD)p));
	  p+=2;
	  /* unknown 2 bytes, same as unknown_1 ? */
	  p+=2;
	  recurr_tzi->StandardInstance = letoh16(*((LPWORD)p));
	  p+=2;
	  recurr_tzi->StandardStartHour = letoh16(*((LPWORD)p));
	  p+=2;
	  /* unknown 8 bytes */
	  p+=8;
	  /* unknown 2 bytes, same as unknown_3 ? */
	  p+=2;
	  recurr_tzi->DaylightMonthOfYear = letoh16(*((LPWORD)p));
	  p+=2;
	  /* unknown 2 bytes, same as unknown_4 ? */
	  p+=2;
	  recurr_tzi->DaylightInstance = letoh16(*((LPWORD)p));
	  p+=2;
	  recurr_tzi->DaylightStartHour = letoh16(*((LPWORD)p));
	  p+=2;

	  tzi = recurr_tzi;
	}
	else
	  synce_error("Recurrence Timezone not a BLOB 104 bytes in length");
      }

      switch (flags & RRA_APPOINTMENT_VERSION_MASK)
      {
      case RRA_APPOINTMENT_VCAL_1_0:
	if (!recurrence_generate_rrule_vcal(generator, event_generator_data.recurrence_pattern, tzi))
	  synce_warning("Failed to generate RRULE from recurrence pattern.");
	break;
      case RRA_APPOINTMENT_VCAL_2_0:
	if (!recurrence_generate_rrule_ical(generator, event_generator_data.recurrence_pattern, tzi))
	  synce_warning("Failed to generate RRULE from recurrence pattern.");
	break;
      default:
	break;
      }
      if (recurr_tzi)
      {
	tzi = NULL;
	free(recurr_tzi);
      }
    }

    if (event_generator_data.unique && id == RRA_APPOINTMENT_ID_UNKNOWN)
    {
      char* buffer = NULL;
      unsigned i;
      bool is_text = true;

      assert(CEVT_BLOB == (event_generator_data.unique->propid & 0xffff));

      for (i = 0; i < event_generator_data.unique->val.blob.dwCount; i++)
        if (!isprint(event_generator_data.unique->val.blob.lpb[i]))
        {
          is_text = false;
          break;
        }

      if (is_text)
      {
        /* This is if SynCE has saved a text UID to this BLOB */
        buffer = malloc(event_generator_data.unique->val.blob.dwCount + 1);
        memcpy(
            buffer, 
            event_generator_data.unique->val.blob.lpb, 
            event_generator_data.unique->val.blob.dwCount);
        buffer[event_generator_data.unique->val.blob.dwCount] = '\0';
      }
      else
      {
        /* This is for the usual binary UID */
        char* p = NULL;
        buffer = (char*)malloc(
            event_generator_data.unique->val.blob.dwCount * 2 + sizeof(BLOB0067_STR));

        strcpy(buffer, BLOB0067_STR);
        p = buffer + strlen(buffer);

        for (i = 0; i < event_generator_data.unique->val.blob.dwCount; i++)
        {
          sprintf(p, "%02x", event_generator_data.unique->val.blob.lpb[i]);
          p += 2;
        }
      }

      generator_add_simple(generator, "UID", buffer);
      free(buffer);
    }
  }
#endif

  generator_add_simple(generator, "END", "VEVENT");

  generator_add_simple(generator, "END", "VCALENDAR");
 
  if (!generator_get_result(generator, vevent))
    goto exit;

  success = true;

exit:
  generator_destroy(generator);
  return success;
}/*}}}*/



/***********************************************************************

  Conversion from vEvent to Appointment

 ***********************************************************************/

static bool on_timezone_tzid(Parser* p, mdir_line* line, void* cookie)
{
  if (line)
    synce_trace("TZID = '%s'", line->values[0]);
  return true;
}

static bool on_mdir_line_dtend(Parser* p, mdir_line* line, void* cookie)/*{{{*/
{
  EventParserData* event_parser_data = (EventParserData*)cookie;
  event_parser_data->dtend = line;
  return true;
}/*}}}*/

static bool on_mdir_line_dtstart(Parser* p, mdir_line* line, void* cookie)/*{{{*/
{
  EventParserData* event_parser_data = (EventParserData*)cookie;
  event_parser_data->dtstart = line;
  return true;
}/*}}}*/

#if ENABLE_RECURRENCE
static bool on_mdir_line_exdate(Parser* p, mdir_line* line, void* cookie)/*{{{*/
{
  EventParserData* event_parser_data = (EventParserData*)cookie;
  if (line)
    rra_mdir_line_vector_add(event_parser_data->exdates, line);
  return true;
}/*}}}*/
#endif

static bool on_mdir_line_rrule(Parser* p, mdir_line* line, void* cookie)/*{{{*/
{
#if ENABLE_RECURRENCE
  EventParserData* event_parser_data = (EventParserData*)cookie;
  event_parser_data->rrule = line;
#else
  synce_warning("Recurrence support not enabled");
#endif
  return true;
}/*}}}*/

static bool on_mdir_line_transp(Parser* p, mdir_line* line, void* cookie)/*{{{*/
{

  /* be forgiving of ical/vcal mix-ups and check all possibilities */
  if (!line || STR_EQUAL(line->values[0], VCAL_1_0_PROP_TRANSP_OPAQUE))
    parser_add_int16(p, ID_BUSY_STATUS, BUSY_STATUS_BUSY);
  else if (STR_EQUAL(line->values[0], VCAL_1_0_PROP_TRANSP_TRANSPARENT))
    parser_add_int16(p, ID_BUSY_STATUS, BUSY_STATUS_FREE);
  else if (!line || STR_EQUAL(line->values[0], "OPAQUE"))
    parser_add_int16(p, ID_BUSY_STATUS, BUSY_STATUS_BUSY);
  else if (STR_EQUAL(line->values[0], "TRANSPARENT"))
    parser_add_int16(p, ID_BUSY_STATUS, BUSY_STATUS_FREE);
  else
    synce_warning("Unknown value for TRANSP: '%s'", line->values[0]);
  return true;
}/*}}}*/

#if ENABLE_RECURRENCE
static bool on_mdir_line_uid(Parser* parser, mdir_line* line, void* cookie)/*{{{*/
{
  EventParserData* event_parser_data = (EventParserData*)cookie;
  event_parser_data->uid = line;
  return true;
}/*}}}*/
#endif

static bool on_mdir_line_description(Parser* p, mdir_line* line, void* cookie)
{
  return process_mdir_line_description(p, line, cookie, ((EventParserData*)cookie)->codepage);
}

static bool on_mdir_line_attendee(Parser* p, mdir_line* line, void* cookie)/*{{{*/
{
  EventParserData* event_parser_data = (EventParserData*)cookie;
  if (line)
    rra_mdir_line_vector_add(event_parser_data->attendees, line);
  return true;
}/*}}}*/

static bool process_attendees(Parser *p, RRA_MdirLineVector *attendees)/*{{{*/
{
  uint8_t *data = NULL;
  size_t data_size = 0;
  int num_attendees = 0;

  if (attendees->used == 0)
    return true;

  size_t i;
  for (i = 0; i < attendees->used; i++)
  {
    int matches;
    char *name, *email;

    synce_debug("ATTENDEE %d - %s", i, attendees->items[i]->values[0]);

    name = malloc(strlen(attendees->items[i]->values[0]) + 1);
    email = malloc(strlen(attendees->items[i]->values[0]) + 1);
    matches = sscanf(attendees->items[i]->values[0], " %s < %s > ", name, email);

    if (matches != 2) {
      synce_info("Didn't find name and email address in format Name <email> in ATTENDEE line, skipping");
      free(name);
      free(email);
      continue;
    }

    size_t attendee_size = 16 + ((strlen(name) + 1) * sizeof(WCHAR)) + ((strlen(email) + 1) * sizeof(WCHAR));
    attendee_size = attendee_size + (8 - (attendee_size % 8));

    uint8_t *tmp_data = realloc(data, data_size + attendee_size);
    if (!tmp_data)
    {
      synce_error("Failed to allocate buffer for ATTENDEE");
      free(data);
      free(name);
      free(email);
      return false;
    }

    data = tmp_data;
    uint8_t *p = data + data_size;
    data_size = data_size + attendee_size;

    LPWSTR widestr = NULL;
    memset(p, 0, attendee_size);
    p += 16;

    widestr = wstr_from_utf8(name);
    memcpy(p, widestr, (wstrlen(widestr) + 1) * sizeof(WCHAR));
    p += ((wstrlen(widestr) + 1) * sizeof(WCHAR));
    wstr_free_string(widestr);

    widestr = wstr_from_utf8(email);
    memcpy(p, widestr, (wstrlen(widestr) + 1) * sizeof(WCHAR));
    p += ((wstrlen(widestr) + 1) * sizeof(WCHAR));
    wstr_free_string(widestr);

    free(name);
    free(email);

    num_attendees++;
  }

  LPBYTE blob = malloc(16 + data_size);

  *(DWORD*)blob = htole32(num_attendees);

  memcpy(blob+16, data, data_size);
  free(data);

  if (!parser_add_blob(p, ID_ATTENDEES, blob, 16+data_size))
  {
    synce_error("Failed to add ATTENDEES");
  }

  free(blob);

  return true;
}

/** @brief Convert to an RRA appointment from vcal
 * 
 * This function converts an event in vcal format to RRA format
 * for a WM device.
 * 
 * @param[in] vevent the event to convert
 * @param[out] id location to store the unique id of the record
 * @param[out] data location to store the resulting record
 * @param[out] data_size location to store the size of the data
 * @param[in] flags bitwise or the flags to control the conversion
 * @param[in] tzi timezone used by the device
 * @param[in] codepage encoding used by the device
 * @return TRUE on success, FALSE on failure
 */ 
bool rra_appointment_from_vevent(/*{{{*/
    const char* vevent,
    uint32_t* id,
    uint8_t** data,
    size_t* data_size,
    uint32_t flags,
    RRA_Timezone* tzi,
    const char *codepage)
{
  bool success = false;
  Parser* parser = NULL;
  ParserComponent* base;
  ParserComponent* calendar;
  ParserComponent* event;
  ParserComponent* alarm;
  ParserComponent* timezone;
  int parser_flags = 0;
  EventParserData event_parser_data;
  memset(&event_parser_data, 0, sizeof(EventParserData));
  event_parser_data.codepage = codepage;  
  event_parser_data.attendees = rra_mdir_line_vector_new();
#if ENABLE_RECURRENCE
  event_parser_data.exdates = rra_mdir_line_vector_new();
#endif

  if (!(flags & RRA_APPOINTMENT_VERSION_MASK))
    flags = flags | RRA_APPOINTMENT_VERSION_DEFAULT;

  event_parser_data.flags = flags;

  switch (flags & RRA_APPOINTMENT_CHARSET_MASK)
  {
    case RRA_APPOINTMENT_UTF8:
      parser_flags = PARSER_UTF8;
      break;

    case RRA_APPOINTMENT_ISO8859_1:
    default:
      /* do nothing */
      break;
  }

  /* check for ical VTimezone component */
  timezone = parser_component_new("vTimeZone");
  parser_component_add_parser_property(timezone, 
      parser_property_new("tzid", on_timezone_tzid));

  /* check for ical VAlarm component */
  alarm = parser_component_new("vAlarm");
  parser_component_add_parser_property(alarm, 
      parser_property_new("trigger", on_alarm_trigger));

  event = parser_component_new("vEvent");
  parser_component_add_parser_component(event, alarm);

  /* check for vcal alarm properties */
  parser_component_add_parser_property(event, 
      parser_property_new("aAlarm", on_mdir_line_aalarm));
  parser_component_add_parser_property(event, 
      parser_property_new("dAlarm", on_mdir_line_dalarm));
  parser_component_add_parser_property(event, 
      parser_property_new("pAlarm", on_mdir_line_palarm));
  parser_component_add_parser_property(event, 
      parser_property_new("mAlarm", on_mdir_line_malarm));

  parser_component_add_parser_property(event, 
      parser_property_new("Categories", on_mdir_line_categories));
  parser_component_add_parser_property(event, 
      parser_property_new("Class", on_mdir_line_class));
  parser_component_add_parser_property(event, 
      parser_property_new("Description", on_mdir_line_description));
  parser_component_add_parser_property(event, 
      parser_property_new("dtEnd", on_mdir_line_dtend));
  parser_component_add_parser_property(event, 
      parser_property_new("dtStart", on_mdir_line_dtstart));
#if ENABLE_RECURRENCE
  parser_component_add_parser_property(event, 
      parser_property_new("exDate", on_mdir_line_exdate));
#endif
  parser_component_add_parser_property(event, 
      parser_property_new("Attendee", on_mdir_line_attendee));
  parser_component_add_parser_property(event, 
      parser_property_new("Location", on_mdir_line_location));
  parser_component_add_parser_property(event, 
      parser_property_new("RRule", on_mdir_line_rrule));
  parser_component_add_parser_property(event, 
      parser_property_new("Summary", on_mdir_line_summary));
  parser_component_add_parser_property(event, 
      parser_property_new("Transp", on_mdir_line_transp));
#if ENABLE_RECURRENCE
  parser_component_add_parser_property(event, 
      parser_property_new("UId", on_mdir_line_uid));
#endif

  calendar = parser_component_new("vCalendar");
  parser_component_add_parser_component(calendar, event);
  parser_component_add_parser_component(calendar, timezone);

  /* allow parsing to start with either vCalendar or vEvent */
  base = parser_component_new(NULL);
  parser_component_add_parser_component(base, calendar);
  parser_component_add_parser_component(base, event);

  parser = parser_new(base, parser_flags, tzi, &event_parser_data);
  if (!parser)
  {
    synce_error("Failed to create parser");
    goto exit;
  }

  if (!parser_set_mimedir(parser, vevent))
  {
    synce_error("Failed to parse input data");
    goto exit;
  }

  if (!parser_run(parser))
  {
    synce_error("Failed to convert input data");
    goto exit;
  }

  parser_call_unused_properties(parser);

  process_attendees(parser, event_parser_data.attendees);

  if (event_parser_data.dtstart)
  {
    if (!parser_add_time_from_line(parser, ID_APPOINTMENT_START, event_parser_data.dtstart))
    {
      synce_error("Failed add time from line");
      goto exit;
    }

    if (event_parser_data.dtend)
    {
      time_t start = 0;
      time_t end = 0;
      int32_t minutes = 0;
      ParserTimeFormat format = parser_get_time_format(event_parser_data.dtstart);

      if (!parser_datetime_to_unix_time(event_parser_data.dtstart->values[0], &start, NULL))
        goto exit;
      if (!parser_datetime_to_unix_time(event_parser_data.dtend->values[0],   &end, NULL))
        goto exit;

      switch (format)
      {
        case PARSER_TIME_FORMAT_UNKNOWN:
          goto exit;

        case PARSER_TIME_FORMAT_DATE_AND_TIME:
          minutes = (end - start) / 60;
          parser_add_int32(parser, ID_APPOINTMENT_TYPE, APPOINTMENT_TYPE_NORMAL);
          break;

        case PARSER_TIME_FORMAT_ONLY_DATE:
          minutes = (end - start - SECONDS_PER_DAY) / 60 + 1;
          parser_add_int32(parser, ID_APPOINTMENT_TYPE, APPOINTMENT_TYPE_ALL_DAY);
          break;
      }

      parser_add_int32(parser, ID_DURATION, minutes);
    }

#if ENABLE_RECURRENCE
    if (event_parser_data.rrule)
    { 
      if (!recurrence_parse_rrule(
            parser, 
            event_parser_data.dtstart,
            event_parser_data.dtend,
            event_parser_data.rrule, 
            event_parser_data.exdates,
            tzi))
        synce_warning("Failed to parse recurrence rule");

      if (event_parser_data.uid)
      {
        if (0 == strncmp(event_parser_data.uid->values[0], BLOB0067_STR, strlen(BLOB0067_STR)))
        {
          /* A binary UID from SynCE */
          size_t size = (strlen(event_parser_data.uid->values[0]) - strlen(BLOB0067_STR)) / 2;
          uint8_t* buffer = malloc(size);
          unsigned i;
          char* p;

          p = event_parser_data.uid->values[0] + strlen(BLOB0067_STR);

          for (i = 0; i < size; i++, p+=2)
          {
            char tmp[3] = {p[0], p[1], '\0'};
            buffer[i] = (uint8_t)strtol(tmp, NULL, 16);
          }

          parser_add_blob(parser, ID_UNIQUE, buffer, size);
          free(buffer);
        }
        else
        {
          /* A text UID */
          parser_add_blob(
              parser, 
              ID_UNIQUE, 
              (uint8_t*)event_parser_data.uid->values[0], 
              strlen(event_parser_data.uid->values[0]));
        }
      }
    }
    else
#endif
      parser_add_int16(parser, ID_OCCURRENCE, OCCURRENCE_ONCE);
  }
  else
  {
    synce_error("No DTSTART found");
    goto exit;
  }

  if (event_parser_data.trigger)
  {
    /* process ical vAlarm */
    to_propval_trigger(parser, event_parser_data.trigger, REMINDER_RELATED_START);
  }
  else if (event_parser_data.aalarm || event_parser_data.dalarm || event_parser_data.palarm || event_parser_data.malarm)
  {
    /* process vcal alarms */
    to_propval_vcal_alarms(parser, event_parser_data.dtstart->values[0], event_parser_data.aalarm, event_parser_data.dalarm, event_parser_data.palarm, event_parser_data.malarm);
  }

  /* The calendar application on my HP 620LX just hangs without this! */
  parser_add_int32(parser, ID_UNKNOWN_0002, 0);

  if (!parser_get_result(parser, data, data_size))
  {
    synce_error("Failed to retrieve result");
    goto exit;
  }
  
 	success = true;

exit:
#if ENABLE_RECURRENCE
  rra_mdir_line_vector_destroy(event_parser_data.exdates, true);
#endif
  rra_mdir_line_vector_destroy(event_parser_data.attendees, true);
  /* destroy components (the order is important!) */
  parser_component_destroy(base);
  parser_component_destroy(calendar);
  parser_component_destroy(event);
  parser_component_destroy(alarm);
  parser_component_destroy(timezone);
  parser_destroy(parser);

  if (!success)
  {
    synce_trace("Failure on this vEvent: '%s'", vevent);
  }
  
	return success;
}/*}}}*/

