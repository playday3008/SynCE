/* $Id$ */
#define _BSD_SOURCE 1
#include "recurrence_pattern.h"
#include "environment.h"
#include <stdio.h>
#include <stdlib.h>
#include <synce_log.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

struct _RRA_Exceptions
{
  int32_t total_count;
  int32_t modified_count;
  RRA_Exception* items;
};

#define MINUTES_PER_DAY     (24*60)

#define UNKNOWN_FLAGS       0x2020
#define KNOWN_FLAGS_MASK    3

#define UNKNOWN_3004        0x3004
#define UNKNOWN_3005        0x3005

#define SHOW_AS_EXTRA       0x200a


#define READ_INT16(p)   (*(int16_t*)(p))
#define READ_INT32(p)   (*(int32_t*)(p))

#define READ_UINT16(p)   (*(uint16_t*)(p))
#define READ_UINT32(p)   (*(uint32_t*)(p))

#define WRITE_INT16(p,v)   (*(int16_t*)(p) = (v))
#define WRITE_INT32(p,v)   (*(int32_t*)(p) = (v))

#define WRITE_UINT16(p,v)   (*(uint16_t*)(p) = (v))
#define WRITE_UINT32(p,v)   (*(uint32_t*)(p) = (v))

size_t my_strftime(char *s, size_t max, const char  *fmt,  const
    struct tm *tm) {
  return strftime(s, max, fmt, tm);
}

#define TRACE_DATE(format, date) \
do { \
  /*char* _time_str = NULL;*/ \
  uint32_t _minutes = date; \
  /*time_t _time = rra_minutes_to_unix_time(_minutes);*/ \
  struct tm _tm = rra_minutes_to_struct(_minutes); \
  if (date == RRA_DoesNotEndDate) \
  { \
    synce_trace(format, "(does not end)"); \
  } \
  else if (_tm.tm_mday == 0 /* (-1) == _time */) \
  { \
    char buffer[256]; \
    snprintf(buffer, sizeof(buffer),  "(date out of range: %08x)", date); \
    synce_trace(format, buffer); \
  } \
  else \
  { \
    char _time_str[256]; \
    /*_time_str = asctime(gmtime(&_time)); \
    _time_str[strlen(_time_str)-1] = '\0';*/ \
    my_strftime(_time_str, sizeof(_time_str), "%c", &_tm); \
    synce_trace(format, _time_str); \
  } \
} \
while (0)

static const char* RECURRENCE_TYPE_NAME[] = 
{
  "Daily", "Weekly", "Monthly", "MonthNth", "Yearly", "YearNth"
};

static const char* RECURRENCE_PATTERN_TYPE_NAME[] = 
{
  "Daily", "Weekly", "Monthly", "Yearly"
};

static const int DAYS_TO_MONTH[13] =
{
  0,                                  /* jan */
  31,                                 /* feb */
  31+28,                              /* mar */
  31+28+31,                           /* apr */
  31+28+31+30,                        /* may */
  31+28+31+30+31,                     /* jun */
  31+28+31+30+31+30,                  /* jul */
  31+28+31+30+31+30+31,               /* aug */
  31+28+31+30+31+30+31+31,            /* sep */
  31+28+31+30+31+30+31+31+30,         /* oct */
  31+28+31+30+31+30+31+31+30+31,      /* nov */
  31+28+31+30+31+30+31+31+30+31+30,   /* dec */
  31+28+31+30+31+30+31+31+30+31+30+31 /* a whole year*/
};

/*
   Conversion to and from minutes from January 1, 1601
 */

time_t rra_minutes_to_unix_time(uint32_t minutes)
{
  /* We will run into problems with dates before 1970 and after 2038... */
  if (minutes >= RRA_MINUTES_FROM_1601_TO_1970)
  {
    minutes -= RRA_MINUTES_FROM_1601_TO_1970;
    if (minutes < 0x4444444)
      return (time_t)(minutes * 60);
  }

  return (time_t)-1;
}

uint32_t rra_minutes_from_unix_time(time_t t)
{
  return (t / 60) + RRA_MINUTES_FROM_1601_TO_1970;
}

struct tm rra_minutes_to_struct(uint32_t minutes)
{
  struct tm result;
#if 1
  time_t unix_time = rra_minutes_to_unix_time(minutes);

  if ((time_t)-1 == unix_time)
    memset(&result, 0, sizeof(struct tm));
  else
    /* XXX: localtime or gmtime? i always forget which one do what i want */
    gmtime_r(&unix_time, &result);
#else
  /* strftime() does not handle this as we like! */
  memset(&result, 0, sizeof(struct tm));
  result.tm_min = minutes;
#endif

  return result;
}

uint32_t rra_minutes_from_struct(struct tm* t)
{
  /* fool around with the TZ environment variable in order to get mktime do
   * what it is supposed to do */
  uint32_t result = 0;
  void* handle = environment_push_timezone("UTC");
  result = rra_minutes_from_unix_time(mktime(t));
  environment_pop_timezone(handle);

  return result;
}


/*
   Exceptions
 */

RRA_Exceptions* rra_exceptions_new()/*{{{*/
{
  RRA_Exceptions* self = calloc(1, sizeof(RRA_Exceptions));
  return self;
}/*}}}*/

void rra_exceptions_destroy(RRA_Exceptions* self)/*{{{*/
{
  if (self)
  {
    /* probably some contents to destroy */
    free(self);
  }
}/*}}}*/

int rra_exceptions_count(RRA_Exceptions *self)/*{{{*/
{
  return self->total_count;
}/*}}}*/

RRA_Exception* rra_exceptions_item(RRA_Exceptions *self, int index)/*{{{*/
{
  if (index >= 0 && index < self->total_count)
    return &self->items[index];
  else
    return NULL;
}/*}}}*/

void rra_exceptions_make_reservation(RRA_Exceptions* self, size_t count)
{
  if (self->items)
    free(self->items);
  self->total_count = count;
  self->items = (RRA_Exception*)calloc(self->total_count, sizeof(RRA_Exception));
}

static bool rra_exceptions_read_summary(RRA_Exceptions* self, uint8_t** buffer)/*{{{*/
{
  uint8_t* p = *buffer;
  int i;

  rra_exceptions_make_reservation(self, READ_INT32(p)); p += 4;

  for (i = 0; i < self->total_count; i++)
  {
    uint32_t date = READ_UINT32(p); p += 4;
    TRACE_DATE("Exception date  = %s", date);

    self->items[i].deleted = true;
    self->items[i].date = date;
  }

  self->modified_count = READ_INT32(p);  p += 4;

  for (i = 0; i < self->modified_count; i++)
  {
    uint32_t date = READ_UINT32(p); p += 4;
    TRACE_DATE("Modified date   = %s", date);
  }
  
  *buffer = p;
  return true;
}/*}}}*/

static bool rra_exception_read_integer(uint8_t** buffer, uint32_t* value)/*{{{*/
{
  uint8_t* p = *buffer;

  if (!value)
    return false;

  *value = READ_UINT32(p); p += 4;

  synce_trace("Value                            = %08x",  *value);

  *buffer = p;
  return true;
}/*}}}*/

static bool rra_exception_read_notes(uint8_t** buffer, uint32_t* size, uint8_t** data)/*{{{*/
{
  uint8_t* p = *buffer;

  if (!size || !data)
    return false;

  *size = READ_UINT32(p); p += 4;

  synce_trace("Size                             = %08x",  *size);

  *data = malloc(*size);
  memcpy(*data, p, *size);
  p += *size;

  *buffer = p;
  return true;
}/*}}}*/

static bool rra_exception_read_string(uint8_t** buffer, WCHAR** wide_str)/*{{{*/
{
  uint8_t* p = *buffer;
  int16_t unknown;
  int16_t length;

  unknown = READ_INT16(p); p += 2;
  length  = READ_INT16(p); p += 2;

  if (unknown != (length + 1) && !(length == 0 && unknown == 0))
    synce_error("Unexpected unknown %04x for length %04x",
        unknown, length);

  *wide_str = (WCHAR*)calloc(length + 1, 2);
  memcpy(*wide_str, p, length * 2); p += length * 2;
  synce_trace_wstr(*wide_str);

  *buffer = p;
  return true;
}/*}}}*/

static bool rra_exceptions_read_details(RRA_Exceptions* self, uint8_t** buffer)/*{{{*/
{
  uint8_t* p = *buffer;
  int i;
  int16_t count;

  count = READ_INT16(p); p += 2;

  if (count != self->modified_count)
    synce_warning("Unexpected exception detail count!");

  for (i = 0; i < self->modified_count; i++)
  {
    RRA_Exception e;
    int j;
    
    e.deleted         = false;
    e.start_time      = READ_UINT32(p);   p += 4;
    e.end_time        = READ_UINT32(p);   p += 4;
    e.original_time   = READ_UINT32(p);   p += 4;
    e.bitmask         = READ_UINT16(p);   p += 2;

    TRACE_DATE ("Modified appointment start time  = %s",    e.start_time);
    TRACE_DATE ("Modified appointment end time    = %s",    e.end_time);
    TRACE_DATE ("Original appointment start time  = %s",    e.original_time);
    synce_trace("Modified appointment bitmask     = %04x",  e.bitmask);

    /* Start with the lowest bit... */

    if (e.bitmask & RRA_EXCEPTION_SUBJECT)
    {
      synce_trace("Subject changed in exception");
      rra_exception_read_string(&p, &e.subject);
    }

    if (e.bitmask & RRA_EXCEPTION_REMINDER_MINUTES_BEFORE_START)
    {
      synce_trace("Unknown 4 changed in exception");
      rra_exception_read_integer(&p, &e.reminder_minutes_before_start);
    }

    if (e.bitmask & RRA_EXCEPTION_UNKNOWN_8)
    {
      synce_trace("Unknown 8 changed in exception");
      rra_exception_read_string(&p, &e.unknown_8);
    }
    
    if (e.bitmask & RRA_EXCEPTION_LOCATION)
    {
      synce_trace("Location changed in exception");
      rra_exception_read_string(&p, &e.location);
    }

    if (e.bitmask & RRA_EXCEPTION_STATUS)
    {
      synce_trace("Status changed in exception");
      rra_exception_read_integer(&p, &e.status);
    }

    if (e.bitmask & RRA_EXCEPTION_TYPE)
    {
      synce_trace("Type changed in exception");
      rra_exception_read_integer(&p, &e.type);
    }

    if (e.bitmask & RRA_EXCEPTION_NOTES)
    {
      synce_trace("Notes changed in exception");
      rra_exception_read_notes(&p, &e.notes_size, &e.notes_data);
    }

    if (e.bitmask & ~RRA_EXCEPTION_KNOWN_BITS)
    {
      synce_warning("Unknown bits in bitmask %04x - expect failure.",
          e.bitmask);
    }

    for (j = 0; j < self->total_count; j++)
    {
      if (e.original_time / MINUTES_PER_DAY == self->items[j].date / MINUTES_PER_DAY)
      {
        /* Copy data to this array item */
        e.date = self->items[j].date;
        self->items[j] = e;
        break;
      }
    }
    if (j == self->total_count)
      synce_warning("Failed to store exception details in the right place");
  }

  *buffer = p;
  return true;
}/*}}}*/

static size_t rra_exceptions_summary_size(RRA_Exceptions* self)/*{{{*/
{
  return 4 * (1 + self->total_count + 1 + self->modified_count);
}/*}}}*/

static size_t rra_exception_size(RRA_Exception* self)/*{{{*/
{
  size_t result = 14;

  if (self->bitmask & RRA_EXCEPTION_SUBJECT)
    result += 4 + wstrlen(self->subject) * 2;

  if (self->bitmask & RRA_EXCEPTION_REMINDER_MINUTES_BEFORE_START)
    result += 4;

  if (self->bitmask & RRA_EXCEPTION_UNKNOWN_8)
    result += 4 + wstrlen(self->unknown_8) * 2;

  if (self->bitmask & RRA_EXCEPTION_LOCATION)
    result += 4 + wstrlen(self->location) * 2;

  if (self->bitmask & RRA_EXCEPTION_STATUS)
    result += 4;

  if (self->bitmask & RRA_EXCEPTION_TYPE)
    result += 4;

  if (self->bitmask & RRA_EXCEPTION_NOTES)
    result += 4 + self->notes_size;

  return result;
}/*}}}*/

static size_t rra_exceptions_details_size(RRA_Exceptions* self)/*{{{*/
{
  int i;
  size_t result = 2;

  for (i = 0; i < self->total_count; i++)
  {
    if (!self->items[i].deleted)
      result += rra_exception_size(&self->items[i]);
  }

  return result;
}/*}}}*/

static bool rra_exceptions_write_summary(RRA_Exceptions* self, uint8_t** buffer)/*{{{*/
{
  uint8_t* p = *buffer;
  int i;

  /* Recalculate modified_count */
  self->modified_count = 0;

  WRITE_INT32(p, self->total_count);  p += 4;

  for (i = 0; i < self->total_count; i++)
  {
    WRITE_UINT32(p, self->items[i].date); p += 4;
    if (!self->items[i].deleted)
      self->modified_count++;
  }

  WRITE_INT32(p, self->modified_count);  p += 4;

  for (i = 0; i < self->total_count; i++)
  {
    if (!self->items[i].deleted)
    {
      WRITE_UINT32(p, self->items[i].date);     p += 4;
    }
  }
  
  *buffer = p;
  return true;
}/*}}}*/

static bool rra_exception_write_integer(uint8_t** buffer, uint32_t value)/*{{{*/
{
  uint8_t* p = *buffer;

  WRITE_UINT32(p, value); p += 4;

  *buffer = p;
  return true;
}/*}}}*/

static bool rra_exception_write_notes(uint8_t** buffer, uint32_t size, uint8_t* data)/*{{{*/
{
  uint8_t* p = *buffer;

  WRITE_UINT32(p, size); p += 4;

  memcpy(p, data, size); p += size;

  *buffer = p;
  return true;
}/*}}}*/

static bool rra_exception_write_string(uint8_t** buffer, WCHAR* wide_str)/*{{{*/
{
  uint8_t* p = *buffer;
  int16_t unknown;
  int16_t length;

  length = wstrlen(wide_str);
  if (length == 0)
    unknown = 0;
  else
    unknown = length + 1;

  WRITE_INT16(p, unknown);  p += 2;
  WRITE_INT16(p, length);   p += 2;

  memcpy(p, wide_str, length * 2); p += length * 2;

  *buffer = p;
  return true;
}/*}}}*/


static bool rra_exception_write(RRA_Exception* self, uint8_t** buffer)
{
  uint8_t* p = *buffer;

  WRITE_UINT32(p, self->start_time);     p += 4;
  WRITE_UINT32(p, self->end_time);       p += 4;
  WRITE_UINT32(p, self->original_time);  p += 4;
  WRITE_UINT16(p, self->bitmask);        p += 2;

  /* Start with the lowest bit... */

  if (self->bitmask & RRA_EXCEPTION_SUBJECT)
    rra_exception_write_string(&p, self->subject);

  if (self->bitmask & RRA_EXCEPTION_REMINDER_MINUTES_BEFORE_START)
    rra_exception_write_integer(&p, self->reminder_minutes_before_start);

  if (self->bitmask & RRA_EXCEPTION_UNKNOWN_8)
    rra_exception_write_string(&p, self->unknown_8);

  if (self->bitmask & RRA_EXCEPTION_LOCATION)
    rra_exception_write_string(&p, self->location);

  if (self->bitmask & RRA_EXCEPTION_STATUS)
    rra_exception_write_integer(&p, self->status);

  if (self->bitmask & RRA_EXCEPTION_TYPE)
    rra_exception_write_integer(&p, self->type);

  if (self->bitmask & RRA_EXCEPTION_NOTES)
    rra_exception_write_notes(&p, self->notes_size, self->notes_data);

  if (self->bitmask & ~RRA_EXCEPTION_KNOWN_BITS)
  {
    synce_warning("Unknown bits in bitmask %04x - expect failure.",
        self->bitmask);
  }

  *buffer = p;
  return true;
}

static bool rra_exceptions_write_details(RRA_Exceptions* self, uint8_t** buffer)/*{{{*/
{
  uint8_t* p = *buffer;
  int i;

  WRITE_UINT16(p, self->modified_count); p += 2;

  for (i = 0; i < self->total_count; i++)
  {
    if (!self->items[i].deleted)
      rra_exception_write(&self->items[i], &p);
  }

  *buffer = p;
  return true;
}/*}}}*/


/*
   Recurrence pattern
 */

RRA_RecurrencePattern* rra_recurrence_pattern_new()/*{{{*/
{
  RRA_RecurrencePattern* self = calloc(1, sizeof(RRA_RecurrencePattern));
  self->exceptions = rra_exceptions_new();
  return self;
}/*}}}*/

void rra_recurrence_pattern_destroy(RRA_RecurrencePattern* self)/*{{{*/
{
  if (self)
  {
    rra_exceptions_destroy(self->exceptions);
    free(self);
  }
}/*}}}*/

static bool rra_recurrence_pattern_read_daily(/*{{{*/
    RRA_RecurrencePattern* self,
    uint8_t** buffer, 
    size_t size)
{
  uint8_t* p = *buffer;
  uint32_t unknown_b;

  /* 0x0a */
  unknown_b = READ_UINT32(p);  p += 4;
  synce_trace("Unknown      = %08x (maybe a duration of %f days?)", 
      unknown_b, unknown_b / (60.0 * 24.0));

  self->interval      = READ_INT32(p);  p += 4;   /* 0x0e */
  synce_trace("Interval     = %08x = %f days", self->interval,
      self->interval / (60.0 * 24.0));
  
  synce_trace("Unknown      = %08x", READ_INT32(p));
  p += 4;
  
  self->flags         = READ_INT32(p);  p += 4;
  self->occurrences   = READ_INT32(p);  p += 4;
  
  synce_trace("Flags        = %08x", self->flags);
  synce_trace("Occurrences  = %08x", self->occurrences);

  *buffer = p;
  return true;
}/*}}}*/

static bool rra_recurrence_pattern_read_weekly(/*{{{*/
    RRA_RecurrencePattern* self,
    uint8_t** buffer, 
    size_t size)
{
  uint8_t* p = *buffer;
  uint32_t unknown_b;

  unknown_b = READ_UINT32(p);  p += 4;
  synce_trace("Unknown         = %08x (maybe something that is %f days or %f weeks?)", 
      unknown_b, 1 + unknown_b / (60.0 * 24.0), (1 + unknown_b / (60.0 * 24.0)) / 7);

  self->interval          = READ_INT32(p);  p += 4;   /* 0x0e */
  synce_trace("Interval        = %08x", self->interval);
  
  synce_trace("Unknown         = %08x", READ_INT32(p));
  p += 4;
  
  self->days_of_week_mask = READ_INT32(p);  p += 4;   /* 0x16 */
  self->flags             = READ_INT32(p);  p += 4;   /* 0x1a */
  self->occurrences       = READ_INT32(p);  p += 4;   /* 0x1e */
  
  synce_trace("DaysOfWeekMask  = %08x", self->days_of_week_mask);
  synce_trace("Flags           = %08x", self->flags);
  synce_trace("Occurrences     = %08x", self->occurrences);

  *buffer = p;
  return true;
}/*}}}*/

static uint32_t rra_recurrence_pattern_get_minutes_to_month(uint32_t minutes, uint32_t interval)
{
  uint32_t result = (uint32_t)-1;
  time_t unix_time = rra_minutes_to_unix_time(minutes);
  struct tm* time_struct = gmtime(&unix_time);

  if (time_struct)
  {
    /*synce_trace("Month = %i", time_struct->tm_mon);*/
    result = DAYS_TO_MONTH[time_struct->tm_mon] * MINUTES_PER_DAY;

    if (interval > 12)
    {
      int extra_years = (interval - 1) / 12;
      result += DAYS_TO_MONTH[12] * extra_years * MINUTES_PER_DAY;
    }
  }
  else
    synce_error("Minutes is probably out of range.");

  return result;
}

static bool rra_recurrence_pattern_read_monthly(/*{{{*/
    RRA_RecurrencePattern* self,
    uint8_t** buffer, 
    size_t size)
{
  uint8_t* p = *buffer;

  self->minutes_to_month = READ_UINT32(p);  p += 4;
  synce_trace("Days to month   = %08x = %u minutes = %f days", 
      self->minutes_to_month, self->minutes_to_month, self->minutes_to_month / (60.0 * 24.0));

  self->interval      = READ_INT32(p);  p += 4;   /* 0x0e */
  synce_trace("Interval        = %08x", self->interval);
  
  synce_trace("Unknown         = %08x", READ_INT32(p));
  p += 4;
  
  self->day_of_month  = READ_INT32(p);  p += 4;   /* 0x16 */
  self->flags         = READ_INT32(p);  p += 4;   /* 0x1a */
  self->occurrences   = READ_INT32(p);  p += 4;   /* 0x1e */

  synce_trace("DayOfMonth      = %08x", self->day_of_month);
  synce_trace("Flags           = %08x", self->flags);
  synce_trace("Occurrences     = %08x", self->occurrences);

  *buffer = p;
  return true;
}/*}}}*/

static bool rra_recurrence_pattern_read_monthnth(/*{{{*/
    RRA_RecurrencePattern* self,
    uint8_t** buffer, 
    size_t size)
{
  uint8_t* p = *buffer;
  uint32_t unknown;

  self->minutes_to_month = READ_UINT32(p);  p += 4;
  synce_trace("Days to month   = %08x = %u minutes = %f days", 
              self->minutes_to_month, self->minutes_to_month, self->minutes_to_month / (60.0 * 24.0));

  self->interval          = READ_INT32(p);  p += 4;   /* 0x0e */
  synce_trace("Interval        = %08x", self->interval);
  
  unknown = READ_UINT32(p);  p += 4;
  synce_trace("Unknown         = %08x", unknown);
  if (unknown != 0)
    synce_warning("Unknown not zero!");
   
  self->days_of_week_mask = READ_INT32(p);  p += 4;   /* 0x16 */
  self->instance          = READ_INT32(p);  p += 4;
  self->flags             = READ_INT32(p);  p += 4;
  self->occurrences       = READ_INT32(p);  p += 4;
 
  synce_trace("DaysOfWeekMask  = %08x", self->days_of_week_mask);
  synce_trace("Instance        = %08x", self->instance);
  synce_trace("Flags           = %08x", self->flags);
  synce_trace("Occurrences     = %08x", self->occurrences);

  *buffer = p;
  return true;
}/*}}}*/

static bool rra_recurrence_pattern_read_header(/*{{{*/
    RRA_RecurrencePattern* self,
    uint8_t** buffer)
{
  uint16_t unknown_a[2];
  uint8_t* p = *buffer;
  int i;
  const char* recurrence_type_name = "Unknown";

  for (i = 0; i < 2; i++)
  {
    unknown_a[i] = READ_UINT16(p);  p += 2;
    synce_trace("Unknown[%d] = %04x = %i", i, unknown_a[i], unknown_a[i]);
  }

  if (unknown_a[0] != 0x3004)
    synce_warning("Expected 0x3004, got %04x", unknown_a[1]);

  if (unknown_a[0] != unknown_a[1])
    synce_warning("%04x != %04x", unknown_a[0], unknown_a[1]);

  /* 0x04 */
  self->recurrence_pattern_type = READ_UINT16(p);  p += 2;

  /* 0x06 */
  self->recurrence_type = READ_INT32(p);  p += 4;

  switch (self->recurrence_type)
  {
    case olRecursDaily:
      if (self->recurrence_pattern_type != RecurrenceDaily)
        synce_warning("Expected 0x200a, got %04x", self->recurrence_pattern_type);
      break;
    case olRecursWeekly:
      /* 0x200a is used for "Daily - Every weekday" */
      /* 0x200b is used for "Weekly - Mo,Tu,On,To,Fr */
      if (self->recurrence_pattern_type != RecurrenceDaily && self->recurrence_pattern_type != RecurrenceWeekly)
        synce_warning("Expected 0x200a or 0x200b, got %04x", self->recurrence_pattern_type);
      break;
    case olRecursMonthly:
    case olRecursMonthNth:
      /* 0x200c probably means "show as Monthly" */
      /* 0x200d probably means "show as Yearly" */
      if (self->recurrence_pattern_type != RecurrenceMonthly && self->recurrence_pattern_type != RecurrenceYearly)
        synce_warning("Expected 0x200c or 0x200d, got %04x", self->recurrence_pattern_type);
      break;
    default:
      break;
  }

  {
    unsigned show_type = self->recurrence_pattern_type - SHOW_AS_EXTRA;
    const char* recurrence_pattern_type_name = "Unknown";

    if (show_type < RRA_RECURRENCE_PATTERN_TYPE_COUNT)
      recurrence_pattern_type_name = RECURRENCE_PATTERN_TYPE_NAME[show_type];
    synce_trace("Show as recurrence type %04x (%s)", self->recurrence_pattern_type, recurrence_pattern_type_name);
  }

  if (self->recurrence_type < RRA_RECURRENCE_TYPE_COUNT)
    recurrence_type_name = RECURRENCE_TYPE_NAME[self->recurrence_type];
  synce_trace("Recurrence type %i (%s)", self->recurrence_type, recurrence_type_name);
 
  *buffer = p;
  return true;
}/*}}}*/

static size_t rra_recurrence_pattern_size(RRA_RecurrencePattern* self)
{
  size_t size = 0;

  switch (self->recurrence_type)
  {
    case olRecursDaily:
      size = 58;
      break;
    case olRecursWeekly:
      size = 62;
      break;
    case olRecursMonthly:
      size = 62;
      break;
    case olRecursMonthNth:
      size = 66;
      break;
    default:
      break;
  }

  size += rra_exceptions_summary_size(self->exceptions);
  size += rra_exceptions_details_size(self->exceptions);
  
  if (size < 0x68)
    size = 0x68;

  return size;
}

void rra_recurrence_pattern_calculate_real_start(RRA_RecurrencePattern *self)
{

  if (self->recurrence_type == olRecursDaily)
  {
    struct tm real_start = rra_minutes_to_struct(self->pattern_start_date);
    uint32_t minutes;
    real_start.tm_sec = 0;
    real_start.tm_min = 0;
    real_start.tm_hour = 0;
    minutes = rra_minutes_from_struct(&real_start);

    self->pattern_start_date = minutes;
    return;
  }

  if (self->recurrence_type == olRecursWeekly)
  {
    /* if pattern start day week day is earlier or on any recurrence day of week,
         start on that or the next recurrence day of that week,
       else start on first recurrence day of that week + interval */

    uint32_t minutes;
    int days_list[7];
    int start_pattern_wday = 0, real_start_day = 7, wday = 0, first = 0;
    struct tm t_tmp;

    /* calculate first day of week */
    t_tmp = rra_minutes_to_struct(self->pattern_start_date);

    start_pattern_wday = t_tmp.tm_wday;

    t_tmp.tm_sec = 0;
    t_tmp.tm_min = 0;
    t_tmp.tm_hour = 0;

    memset(days_list, 0, sizeof(days_list));

    if (self->days_of_week_mask & olSunday)
            days_list[0] = 1;
    if (self->days_of_week_mask & olMonday)
            days_list[1] = 1;
    if (self->days_of_week_mask & olTuesday)
            days_list[2] = 1;
    if (self->days_of_week_mask & olWednesday)
            days_list[3] = 1;
    if (self->days_of_week_mask & olThursday)
            days_list[4] = 1;
    if (self->days_of_week_mask & olFriday)
            days_list[5] = 1;
    if (self->days_of_week_mask & olSaturday)
            days_list[6] = 1;

    while(wday < 7) {
            if (days_list[wday] == 1) {
                    if (wday >= start_pattern_wday) {
                            real_start_day = wday;
                            break;
                    }
                    if (first == 0)
                            first = wday;
            }
            wday++;
    }

    if (real_start_day < 7)
            t_tmp.tm_mday = t_tmp.tm_mday - start_pattern_wday + real_start_day;
    else
            t_tmp.tm_mday = t_tmp.tm_mday - (t_tmp.tm_wday - first) + (7 * self->interval);

    minutes = rra_minutes_from_struct(&t_tmp);

    self->pattern_start_date = minutes;
    return;
  }

  if (self->recurrence_type == olRecursMonthly && self->recurrence_pattern_type == RecurrenceMonthly)
  {
    /* if recurrence day is after or on pattern start day, start on recurrence day of pattern start month,
       else start on recurrence day of pattern start month + interval */

    struct tm real_start = rra_minutes_to_struct(self->pattern_start_date);
    uint32_t minutes;
    real_start.tm_sec = 0;
    real_start.tm_min = 0;
    real_start.tm_hour = 0;

    if (self->day_of_month < real_start.tm_mday)
    {
            real_start.tm_mon = real_start.tm_mon + self->interval;
    }

    real_start.tm_mday = self->day_of_month;

    minutes = rra_minutes_from_struct(&real_start);

    self->pattern_start_date = minutes;
    return;
  }


  if (self->recurrence_type == olRecursMonthNth && self->recurrence_pattern_type == RecurrenceMonthly)
  {
    /* get the first day in the month of pattern_start_date, find the recurrence day in
       that month, and determine if it's before pattern_start_date */

    uint32_t minutes;
    int days_in_month;
    int days_list[7];
    int mday = 0, wday = 0, count = 0, last = 0;
    struct tm t_tmp;

    /* calculate number of days in target month */
    t_tmp = rra_minutes_to_struct(self->pattern_start_date);
    t_tmp.tm_sec = 0;
    t_tmp.tm_min = 0;
    t_tmp.tm_hour = 0;
    t_tmp.tm_mday = 32;
    rra_minutes_from_struct(&t_tmp);
    days_in_month = 32 - t_tmp.tm_mday;

    t_tmp = rra_minutes_to_struct(self->pattern_start_date);
    t_tmp.tm_sec = 0;
    t_tmp.tm_min = 0;
    t_tmp.tm_hour = 0;
    t_tmp.tm_mday = 1;
    rra_minutes_from_struct(&t_tmp);

    mday = 0, wday = 0, count = 0, last = 0;
    memset(days_list, 0, sizeof(days_list));

    if (self->days_of_week_mask & olSunday)
            days_list[0] = 1;
    if (self->days_of_week_mask & olMonday)
            days_list[1] = 1;
    if (self->days_of_week_mask & olTuesday)
            days_list[2] = 1;
    if (self->days_of_week_mask & olWednesday)
            days_list[3] = 1;
    if (self->days_of_week_mask & olThursday)
            days_list[4] = 1;
    if (self->days_of_week_mask & olFriday)
            days_list[5] = 1;
    if (self->days_of_week_mask & olSaturday)
            days_list[6] = 1;

    wday = t_tmp.tm_wday - 1;

    while(mday < days_in_month) {
            mday++;
            wday++;
            if (wday == 7) wday = 0;
            if (days_list[wday] == 1) {
                    count++;
                    last = mday;
                    if ((self->instance < 5) && (count == self->instance))
                            break;
            }
    }

    if (self->instance == 5)
            t_tmp.tm_mday = last;
    else
            t_tmp.tm_mday = mday;

    minutes = rra_minutes_from_struct(&t_tmp);

    /* if this recurrence date is on or after pattern_start_date, start on
       recurrence day of pattern start month, otherwise start on recurrence
       day of pattern start month + interval */

    if (minutes >= self->pattern_start_date)
    {
      self->pattern_start_date = minutes;
      return;
    }

    /* calculate number of days in target month */
    t_tmp = rra_minutes_to_struct(self->pattern_start_date);
    t_tmp.tm_sec = 0;
    t_tmp.tm_min = 0;
    t_tmp.tm_hour = 0;
    t_tmp.tm_mday = 32;
    t_tmp.tm_mon = t_tmp.tm_mon + self->interval;
    rra_minutes_from_struct(&t_tmp);
    days_in_month = 32 - t_tmp.tm_mday;

    t_tmp = rra_minutes_to_struct(self->pattern_start_date);
    t_tmp.tm_sec = 0;
    t_tmp.tm_min = 0;
    t_tmp.tm_hour = 0;
    t_tmp.tm_mday = 1;
    t_tmp.tm_mon = t_tmp.tm_mon + self->interval;
    rra_minutes_from_struct(&t_tmp);

    mday = 0, wday = 0, count = 0, last = 0;

    memset(days_list, 0, sizeof(days_list));

    if (self->days_of_week_mask & olSunday)
            days_list[0] = 1;
    if (self->days_of_week_mask & olMonday)
            days_list[1] = 1;
    if (self->days_of_week_mask & olTuesday)
            days_list[2] = 1;
    if (self->days_of_week_mask & olWednesday)
            days_list[3] = 1;
    if (self->days_of_week_mask & olThursday)
            days_list[4] = 1;
    if (self->days_of_week_mask & olFriday)
            days_list[5] = 1;
    if (self->days_of_week_mask & olSaturday)
            days_list[6] = 1;

    wday = t_tmp.tm_wday - 1;

    while(mday < days_in_month) {
            mday++;
            wday++;
            if (wday == 7) wday = 0;
            if (days_list[wday] == 1) {
                    count++;
                    last = mday;
                    if ((self->instance < 5) && (count == self->instance))
                            break;
            }
    }

    if (self->instance == 5)
            t_tmp.tm_mday = last;
    else
            t_tmp.tm_mday = mday;

    minutes = rra_minutes_from_struct(&t_tmp);

    self->pattern_start_date = minutes;
    return;
  }


  if (self->recurrence_type == olRecursMonthly && self->recurrence_pattern_type == RecurrenceYearly)
  {
    struct tm real_start = rra_minutes_to_struct(self->pattern_start_date);
    uint32_t minutes;
    real_start.tm_sec = 0;
    real_start.tm_min = 0;
    real_start.tm_hour = 0;
    real_start.tm_mday = (self->minutes_to_month / MINUTES_PER_DAY) + self->day_of_month;
    real_start.tm_mon = 0;
    minutes = rra_minutes_from_struct(&real_start);

    self->pattern_start_date = minutes;
    return;
  }

  if (self->recurrence_type == olRecursMonthNth && self->recurrence_pattern_type == RecurrenceYearly)
  {
    struct tm real_start = rra_minutes_to_struct(self->pattern_start_date);
    uint32_t minutes;
    int days_in_month;
    real_start.tm_sec = 0;
    real_start.tm_min = 0;
    real_start.tm_hour = 0;
    real_start.tm_mday = (self->minutes_to_month / MINUTES_PER_DAY);
    real_start.tm_mon = 0;
    minutes = rra_minutes_from_struct(&real_start);

    /* calculate number of days in target month */
    struct tm t_tmp = rra_minutes_to_struct(self->pattern_start_date);
    t_tmp.tm_sec = 0;
    t_tmp.tm_min = 0;
    t_tmp.tm_hour = 0;
    t_tmp.tm_mday = (self->minutes_to_month / MINUTES_PER_DAY) + 32;
    t_tmp.tm_mon = 0;
    rra_minutes_from_struct(&t_tmp);
    days_in_month = 32 - t_tmp.tm_mday;

    int days_list[7];
    int mday = 0, wday = 0, count = 0, last = 0;

    memset(days_list, 0, sizeof(days_list));

    if (self->days_of_week_mask & olSunday)
            days_list[0] = 1;
    if (self->days_of_week_mask & olMonday)
            days_list[1] = 1;
    if (self->days_of_week_mask & olTuesday)
            days_list[2] = 1;
    if (self->days_of_week_mask & olWednesday)
            days_list[3] = 1;
    if (self->days_of_week_mask & olThursday)
            days_list[4] = 1;
    if (self->days_of_week_mask & olFriday)
            days_list[5] = 1;
    if (self->days_of_week_mask & olSaturday)
            days_list[6] = 1;

    wday = real_start.tm_wday;

    while(mday < days_in_month) {
            mday++;
            wday++;
            if (wday == 7) wday = 0;
            if (days_list[wday] == 1) {
                    count++;
                    last = mday;
                    if ((self->instance < 5) && (count == self->instance))
                            break;
            }
    }

    if (self->instance == 5)
            real_start.tm_mday = real_start.tm_mday + last;
    else
            real_start.tm_mday = real_start.tm_mday + mday;

    minutes = rra_minutes_from_struct(&real_start);

    self->pattern_start_date = minutes;
    return;
  }

  return;
}

RRA_RecurrencePattern* rra_recurrence_pattern_from_buffer(uint8_t* buffer, size_t size, RRA_Timezone *tzi)/*{{{*/
{
  bool success;
  uint8_t* p = buffer;

  if (!tzi)
    return NULL;
  
  RRA_RecurrencePattern* self = rra_recurrence_pattern_new();
  if (!self)
    return NULL;

  rra_recurrence_pattern_read_header(self, &p);

  switch (self->recurrence_type)
  {
    case olRecursDaily:
      success = rra_recurrence_pattern_read_daily(self, &p, size);
      break;
    case olRecursWeekly:
      success = rra_recurrence_pattern_read_weekly(self, &p, size);
      break;
    case olRecursMonthly:
      success = rra_recurrence_pattern_read_monthly(self, &p, size);
      break;
    case olRecursMonthNth:
      success = rra_recurrence_pattern_read_monthnth(self, &p, size);
      break;
    default:
      synce_error("Unexpected recurrence type: %08x", self->recurrence_type);
      success = false;
      break;
  }

  if (!success)
  {
    synce_error("Failed to decode buffer");
    goto exit;
  }

  if ((self->flags & (~KNOWN_FLAGS_MASK)) != UNKNOWN_FLAGS) 
  {
    synce_warning("Unexpected flags!");
  }

  {
    uint32_t unknown = READ_INT32(p); p += 4;
    synce_trace("Unknown         = %08x", unknown);
    if (unknown != 0)
      synce_trace("Expected 0 but got %08x", unknown);
  }
  
  rra_exceptions_read_summary(self->exceptions, &p);
  
  self->pattern_start_date = READ_UINT32(p);  p += 4;
  TRACE_DATE("Original pattern start date (localtime) = %s", self->pattern_start_date);

  rra_recurrence_pattern_calculate_real_start(self);
  TRACE_DATE("Adjusted pattern start date (localtime) = %s", self->pattern_start_date);

  self->pattern_end_date   = READ_UINT32(p);  p += 4;
  TRACE_DATE("Pattern end   date (localtime) = %s", self->pattern_end_date);

  if ((self->flags & RecurrenceEndMask) == RecurrenceDoesNotEnd && 
      self->pattern_end_date != RRA_DoesNotEndDate) 
  {
    synce_warning("Recurrence does not end, but the end date is not the expected value");
  }

  {
    int i;
    uint32_t unknown[2];

    for (i = 0; i < 2; i++)
    {
      unknown[i] = READ_INT32(p); p += 4;
      /*synce_trace("Unknown         = %08x", unknown[i]);*/
    }

    if (unknown[0] != unknown[1])
      synce_warning("%08x != %08x", unknown[0], unknown[1]);
    
    if (unknown[1] != 0x3005)
      synce_warning("Expected 0x3005 but got 0x%04x", unknown[1]);
  }
  
  self->start_minute = READ_INT32(p); p += 4;
  self->end_minute   = READ_INT32(p); p += 4;
  
  synce_trace("Start time         (localtime) = %02i days, %02i hours, %02i min", 
      self->start_minute / (60*24), (self->start_minute / 60) % 24, self->start_minute % 60);
  synce_trace("End time           (localtime) = %02i days, %02i hours, %02i min", 
      self->end_minute   / (60*24), (self->end_minute   / 60) % 24, self->end_minute   % 60);

  /* ce stores recurring events as localtime with a timezone as blob.
     we ignore the blob and use the passed timezone */
  {
    uint32_t minutes;
    uint32_t utc_pattern_start_date;
    uint32_t utc_pattern_end_date;
    uint32_t utc_start_minute;
    uint32_t utc_end_minute;

    minutes = rra_minutes_from_unix_time(rra_timezone_convert_to_utc(tzi, rra_minutes_to_unix_time(self->pattern_start_date + self->start_minute)));
    utc_pattern_start_date = (minutes / MINUTES_PER_DAY) * MINUTES_PER_DAY;
    utc_start_minute       = minutes % MINUTES_PER_DAY;

    /* pattern_start_date with end_minutes */
    minutes = rra_minutes_from_unix_time(rra_timezone_convert_to_utc(tzi, rra_minutes_to_unix_time(self->pattern_start_date + self->end_minute)));
    utc_end_minute         = minutes % MINUTES_PER_DAY;

    if (self->pattern_end_date == RRA_DoesNotEndDate)
      utc_pattern_end_date = self->pattern_end_date;
    else
    {
      /* pattern_end_date with end_minutes */
      minutes = rra_minutes_from_unix_time(rra_timezone_convert_to_utc(tzi, rra_minutes_to_unix_time(self->pattern_end_date + self->end_minute)));
      utc_pattern_end_date   = (minutes / MINUTES_PER_DAY) * MINUTES_PER_DAY;
    }

    self->pattern_start_date = utc_pattern_start_date;
    self->pattern_end_date   = utc_pattern_end_date;
    self->start_minute       = utc_start_minute;
    self->end_minute         = utc_end_minute;
  }

   TRACE_DATE("Pattern start date (utc)       = %s", self->pattern_start_date);
   TRACE_DATE("Pattern end   date (utc)       = %s", self->pattern_end_date);
   synce_trace("Start time         (utc)       = %02i days, %02i hours, %02i min", 
      self->start_minute / (60*24), (self->start_minute / 60) % 24, self->start_minute % 60);
   synce_trace("End time           (utc)       = %02i days, %02i hours, %02i min", 
      self->end_minute   / (60*24), (self->end_minute   / 60) % 24, self->end_minute   % 60);

  rra_exceptions_read_details(self->exceptions, &p);

  if (p != (buffer + size))
  {
    synce_trace("Data that should be zero at offset %04x, size %04x of %04x", 
        p - buffer, buffer + size - p, size);

    {
      uint8_t* end = buffer + size;

      for (; p != end; p++)
      {
        if (*p != 0)
          synce_warning("Byte att offset %04x is not zero but %02x (%c)", 
              p - buffer, *p, isprint(*p) ? *p : '.' );
      }
    }
  }

  if (size != rra_recurrence_pattern_size(self))
    synce_warning("Calculated pattern size to %04x but it was %04x",
        rra_recurrence_pattern_size(self), size);

  /** DEBUG --> */
  {
    uint8_t* new_buffer = NULL;
    size_t new_size = 0;
    success = rra_recurrence_pattern_to_buffer(self, &new_buffer, &new_size, tzi);
    if (success)
    {
      if (size == new_size && 0 == memcmp(buffer, new_buffer, size))
        synce_info("rra_recurrence_pattern_to_buffer() works great!");
      else
      {
        synce_warning("rra_recurrence_pattern_to_buffer() returned a different buffer! (expected size %04x, got %04x)", 
            size, new_size);

        /*
        FILE* file;
        file = fopen("pattern-right.bin", "w");
        fwrite(buffer, size, 1, file);
        fclose(file);
        file = fopen("pattern-wrong.bin", "w");
        fwrite(new_buffer, new_size, 1, file);
        fclose(file);      
        */
      }
    }
    else
      synce_warning("rra_recurrence_pattern_to_buffer() failed");
  }
  /** <-- DEBUG */

  success = true;

exit:
  if (success)
    return self;
  else
  {
    rra_recurrence_pattern_destroy(self);
    return NULL;
  }
}/*}}}*/

bool rra_recurrence_pattern_to_buffer(RRA_RecurrencePattern* self, uint8_t** buffer, size_t* size, RRA_Timezone *tzi)
{
  bool success = false;
  uint8_t* p = NULL;

  if (!tzi)
    return NULL;

  uint32_t localtime_pattern_start_date = self->pattern_start_date;
  uint32_t localtime_pattern_end_date   = self->pattern_end_date;
  uint32_t localtime_start_minute       = self->start_minute;
  uint32_t localtime_end_minute         = self->end_minute;

  if (!self || !buffer || !size) 
  {
    synce_error("One or more invalid function parameters");
    return false;
  }
  
  *size = rra_recurrence_pattern_size(self);
  p = *buffer = calloc(1, *size);

  WRITE_UINT16(p, UNKNOWN_3004); p += 2;
  WRITE_UINT16(p, UNKNOWN_3004); p += 2;

   TRACE_DATE("Pattern start date (utc)       = %s", localtime_pattern_start_date);
   TRACE_DATE("Pattern end   date (utc)       = %s", localtime_pattern_end_date);
  synce_trace("Start time         (utc)       = %02i days, %02i hours, %02i min", 
      self->start_minute / (60*24), (self->start_minute / 60) % 24, self->start_minute % 60);
  synce_trace("End time           (utc)       = %02i days, %02i hours, %02i min", 
      self->end_minute   / (60*24), (self->end_minute   / 60) % 24, self->end_minute   % 60);

  /* ce stores recurring events as localtime with a timezone as blob.
     we ignore the blob and use the passed timezone */
  {
    uint32_t minutes;

    minutes = rra_minutes_from_unix_time(rra_timezone_convert_from_utc(tzi, rra_minutes_to_unix_time(self->pattern_start_date + self->start_minute)));
    localtime_pattern_start_date = (minutes / MINUTES_PER_DAY) * MINUTES_PER_DAY;
    localtime_start_minute       = minutes % MINUTES_PER_DAY;

    /* pattern_start_date with end_minutes */
    minutes = rra_minutes_from_unix_time(rra_timezone_convert_from_utc(tzi, rra_minutes_to_unix_time(self->pattern_start_date + self->end_minute)));
    localtime_end_minute         = minutes % MINUTES_PER_DAY;

    if (self->pattern_end_date == RRA_DoesNotEndDate)
      localtime_pattern_end_date = self->pattern_end_date;
    else
    {
      /* pattern_start_date with end_minutes */
      minutes = rra_minutes_from_unix_time(rra_timezone_convert_from_utc(tzi, rra_minutes_to_unix_time(self->pattern_end_date + self->end_minute)));
      localtime_pattern_end_date   = (minutes / MINUTES_PER_DAY) * MINUTES_PER_DAY;
    }
  }

   TRACE_DATE("Pattern start date (localtime) = %s", localtime_pattern_start_date);
   TRACE_DATE("Pattern end   date (localtime) = %s", localtime_pattern_end_date);
  synce_trace("Start time         (localtime) = %02i days, %02i hours, %02i min", 
      localtime_start_minute / (60*24), (localtime_start_minute / 60) % 24, localtime_start_minute % 60);
  synce_trace("End time           (localtime) = %02i days, %02i hours, %02i min", 
      localtime_end_minute   / (60*24), (localtime_end_minute   / 60) % 24, localtime_end_minute   % 60);

  switch (self->recurrence_type)
  {
    case olRecursDaily:
      WRITE_UINT16(p, 0x200a); p += 2;
      break;
      
    case olRecursWeekly:
      if (self->days_of_week_mask == RRA_Weekdays)
      {
        WRITE_UINT16(p, 0x200a); p += 2;
      }
      else
      {
        WRITE_UINT16(p, 0x200b); p += 2;
      }
      break;
      
    case olRecursMonthly:
    case olRecursMonthNth:
      /* Write 0x200c or 0x200d? */
      if (self->interval == 12)
        WRITE_UINT16(p, 0x200d);
      else
        WRITE_UINT16(p, 0x200c);
      p += 2;
      break;
     
    default:
      synce_error("Unhandled recurrence type");
      goto exit;
  }

  WRITE_UINT32(p, self->recurrence_type); p += 4;

  switch (self->recurrence_type)
  {
    case olRecursDaily:
      WRITE_UINT32(p, 0); p += 4;
      WRITE_UINT32(p, self->interval);      p += 4;
      WRITE_UINT32(p, 0);                   p += 4;
      WRITE_UINT32(p, UNKNOWN_FLAGS | (self->flags & KNOWN_FLAGS_MASK)); p += 4;
      WRITE_UINT32(p, self->occurrences);   p += 4;
      break;
      
    case olRecursWeekly:
      WRITE_UINT32(p, (self->interval * 7 - 1) * MINUTES_PER_DAY); p += 4;
      WRITE_UINT32(p, self->interval);      p += 4;
      WRITE_UINT32(p, 0);                   p += 4;
      WRITE_UINT32(p, self->days_of_week_mask);  p += 4;
      WRITE_UINT32(p, UNKNOWN_FLAGS | (self->flags & KNOWN_FLAGS_MASK)); p += 4;
      WRITE_UINT32(p, self->occurrences);   p += 4;
      break;
      
    case olRecursMonthly:
      if (self->interval == 1)
        WRITE_UINT32(p, 0);
      else
        WRITE_UINT32(p, rra_recurrence_pattern_get_minutes_to_month(
              localtime_pattern_start_date, self->interval)); 
      p += 4;
      WRITE_UINT32(p, self->interval);      p += 4;
      WRITE_UINT32(p, 0);                   p += 4;
      WRITE_UINT32(p, self->day_of_month);  p += 4;
      WRITE_UINT32(p, UNKNOWN_FLAGS | (self->flags & KNOWN_FLAGS_MASK)); p += 4;
      WRITE_UINT32(p, self->occurrences);   p += 4;
      break;

    case olRecursMonthNth:
      if (self->interval == 1 || self->instance == 1)
        WRITE_UINT32(p, 0);
      else
        WRITE_UINT32(p, rra_recurrence_pattern_get_minutes_to_month(
              localtime_pattern_start_date, self->interval)); 
      p += 4;
      WRITE_UINT32(p, self->interval);            p += 4;
      WRITE_UINT32(p, 0);                         p += 4;
      WRITE_UINT32(p, self->days_of_week_mask);   p += 4;
      WRITE_UINT32(p, self->instance);            p += 4;
      WRITE_UINT32(p, UNKNOWN_FLAGS | (self->flags & KNOWN_FLAGS_MASK)); p += 4;
      WRITE_UINT32(p, self->occurrences);         p += 4;
      break;
      
    default:
      synce_error("Unhandled recurrence type");
      goto exit;
  }

  WRITE_UINT32(p, 0); p += 4;

  rra_exceptions_write_summary(self->exceptions, &p);

  WRITE_UINT32(p, localtime_pattern_start_date); p += 4;
  WRITE_UINT32(p, localtime_pattern_end_date); p += 4;

  WRITE_UINT32(p, UNKNOWN_3005); p += 4;
  WRITE_UINT32(p, UNKNOWN_3005); p += 4;

  WRITE_UINT32(p, localtime_start_minute);  p += 4;
  WRITE_UINT32(p, localtime_end_minute);    p += 4;

  rra_exceptions_write_details(self->exceptions, &p);

  success = true;

exit:
  if (!success)
  {
    free(*buffer);
    *buffer = NULL;
    *size = 0;
  }
  return success;
}

