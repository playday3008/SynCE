/* $Id$ */
#ifndef __common_handlers_h__
#define __common_handlers_h__

#include <inttypes.h>
#include <stdbool.h>
#include <libmimedir.h>
#include <rapitypes.h>
#include "timezone.h"
#include "generator.h"
#include "parser.h"
#include "mdir_line_vector.h"

struct _Generator;
struct _Parser;

typedef struct _GeneratorData
{
  CEPROPVAL* start;
  CEPROPVAL* duration;
  CEPROPVAL* type;
  CEPROPVAL* reminder_minutes;
  CEPROPVAL* reminder_enabled;
  CEPROPVAL* reminder_options;
  bool completed;
  FILETIME completed_time;
#if ENABLE_RECURRENCE
  CEPROPVAL* occurrence;
  CEPROPVAL* recurrence_pattern;
  CEPROPVAL* recurrence_timezone;
  CEPROPVAL* unique;
#endif
  const char *codepage;
  uint32_t flags;
} GeneratorData;

typedef struct _EventParserData
{
  mdir_line* dtstart;
  mdir_line* dtend;
  mdir_line* aalarm;
  mdir_line* dalarm;
  mdir_line* palarm;
  mdir_line* malarm;
  mdir_line* trigger;
  RRA_MdirLineVector* attendees;
#if ENABLE_RECURRENCE
  RRA_MdirLineVector* exdates;
  mdir_line* rrule;
  mdir_line* uid;
#endif
  const char *codepage;
  uint32_t flags;
} EventParserData;

bool on_propval_categories (struct _Generator* g, CEPROPVAL* propval, void* cookie);
bool on_propval_location   (struct _Generator* g, CEPROPVAL* propval, void* cookie);
bool on_propval_sensitivity(struct _Generator* g, CEPROPVAL* propval, void* cookie);
bool on_propval_subject    (struct _Generator* g, CEPROPVAL* propval, void* cookie);

bool on_propval_reminder_enabled(Generator* g, CEPROPVAL* propval, void* cookie);
bool on_propval_reminder_minutes(Generator* g, CEPROPVAL* propval, void* cookie);
bool on_propval_reminder_options(Generator* g, CEPROPVAL* propval, void* cookie);
bool on_propval_reminder_sound_file(Generator* g, CEPROPVAL* propval, void* cookie);

bool on_mdir_line_categories (struct _Parser* p, mdir_line* line, void* cookie);
bool on_mdir_line_class      (struct _Parser* p, mdir_line* line, void* cookie);
bool on_mdir_line_location   (struct _Parser* p, mdir_line* line, void* cookie);
bool on_mdir_line_summary    (struct _Parser* p, mdir_line* line, void* cookie);

bool on_alarm_trigger(Parser* p, mdir_line* line, void* cookie);
bool on_mdir_line_aalarm(Parser* p, mdir_line* line, void* cookie);
bool on_mdir_line_dalarm(Parser* p, mdir_line* line, void* cookie);
bool on_mdir_line_palarm(Parser* p, mdir_line* line, void* cookie);
bool on_mdir_line_malarm(Parser* p, mdir_line* line, void* cookie);

bool process_propval_notes   (struct _Generator* g, CEPROPVAL* propval, void* cookie, const char *codepage);
bool process_mdir_line_description(struct _Parser* p, mdir_line* line, void* cookie, const char *codepage);

#define REMINDER_RELATED_START 0
#define REMINDER_RELATED_END   1

void to_propval_trigger(struct _Parser* parser, mdir_line* line, uint8_t related_support);
void to_propval_vcal_alarms(Parser* parser, char *start_time_str, mdir_line* aalarm, mdir_line* dalarm, mdir_line* palarm, mdir_line* malarm);

void to_icalendar_alarm(struct _Generator* generator, CEPROPVAL* reminder_enabled, CEPROPVAL* reminder_minutes, CEPROPVAL* reminder_options, uint8_t related);

bool to_vcalendar_alarm(struct _Generator* generator, CEPROPVAL* start_time, CEPROPVAL* reminder_enabled, CEPROPVAL* reminder_minutes, CEPROPVAL* reminder_options, RRA_Timezone *tzi);

char* convert_to_utf8(const char* inbuf, const char* codepage);
char* convert_from_utf8(const char* source, const char* codepage);

#endif

