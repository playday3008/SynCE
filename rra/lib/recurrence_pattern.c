/* $Id$ */
#include "recurrence_pattern.h"
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


#define TRACE_DATE(format, date) \
do { \
  char* _time_str = NULL; \
  uint32_t _minutes = date; \
  time_t _time = rra_minutes_to_unix_time(_minutes); \
  if ((-1) == _time) \
  { \
    _time_str = "(date out of range)"; \
  } \
  else \
  { \
    _time_str = asctime(gmtime(&_time)); \
    _time_str[strlen(_time_str)-1] = '\0'; \
  } \
  synce_trace(format, _time_str); \
} \
while (0)

static const char* RECURRENCE_TYPE_NAME[] = 
{
  "Daily", "Weekly", "Monthly", "MonthNth", "Yearly", "YearNth"
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
  time_t unix_time = rra_minutes_to_unix_time(minutes);

  if ((time_t)-1 == unix_time)
    memset(&result, 0, sizeof(struct tm));
  else
    /* XXX: localtime or gmtime? i always forget which one do what i want */
    memcpy(&result, localtime(&unix_time), sizeof(struct tm));

  return result;
}

uint32_t rra_minutes_from_struct(struct tm* t)
{
  return rra_minutes_from_unix_time(mktime(t));
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
  /* TODO: implement */
  return NULL;
}/*}}}*/

static bool rra_exceptions_read_summary(RRA_Exceptions* self, uint8_t** buffer)/*{{{*/
{
  uint8_t* p = *buffer;
  int i;

  self->total_count = READ_INT32(p);  p += 4;

  self->items = (RRA_Exception*)calloc(self->total_count, sizeof(RRA_Exception));

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
    e.unknown         = READ_INT16(p);  p += 2;

    TRACE_DATE ("Modified appointment start time  = %s",    e.start_time);
    TRACE_DATE ("Modified appointment end time    = %s",    e.end_time);
    TRACE_DATE ("Original appointment start time  = %s",    e.original_time);
    synce_trace("Modified appointment unknown     = %04x",  e.unknown);

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

static size_t rra_exceptions_details_size(RRA_Exceptions* self)/*{{{*/
{
  return 2 + (self->modified_count * 14);
}/*}}}*/

static bool rra_exceptions_write_summary(RRA_Exceptions* self, uint8_t** buffer)/*{{{*/
{
  uint8_t* p = *buffer;
  int i;

  WRITE_INT32(p, self->total_count);  p += 4;

  for (i = 0; i < self->total_count; i++)
  {
    WRITE_UINT32(p, self->items[i].date); p += 4;
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

static bool rra_exceptions_write_details(RRA_Exceptions* self, uint8_t** buffer)/*{{{*/
{
  uint8_t* p = *buffer;
  int i;

  WRITE_UINT16(p, self->modified_count); p += 2;

  for (i = 0; i < self->total_count; i++)
  {
    if (!self->items[i].deleted)
    {
      WRITE_UINT32(p, self->items[i].start_time);     p += 4;
      WRITE_UINT32(p, self->items[i].end_time);       p += 4;
      WRITE_UINT32(p, self->items[i].original_time);  p += 4;
      WRITE_UINT16(p, self->items[i].unknown);        p += 2;
    }
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
  synce_trace("Unknown         = %08x (maybe something that is %f days?)", 
      unknown_b, unknown_b / (60.0 * 24.0));

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
  uint32_t unknown_b;

  unknown_b = READ_UINT32(p);  p += 4;
  synce_trace("Days to month   = %08x = %u minutes = %f days", 
      unknown_b, unknown_b, unknown_b / (60.0 * 24.0));

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
  uint32_t unknown_b, unknown_c;

  unknown_b = READ_UINT32(p);  p += 4;
  synce_trace("Days to month   = %08x = %u minutes = %f days", 
      unknown_b, unknown_b, unknown_b / (60.0 * 24.0));

  self->interval          = READ_INT32(p);  p += 4;   /* 0x0e */
  synce_trace("Interval        = %08x", self->interval);
  
  unknown_c = READ_UINT32(p);  p += 4;
  synce_trace("Unknown         = %08x", unknown_c);
  if (unknown_c != 0)
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
  uint16_t unknown_a[3];
  uint8_t* p = *buffer;
  int i;
  const char* recurrence_type_name = "Unknown";

  for (i = 0; i < 3; i++)
  {
    unknown_a[i] = READ_UINT16(p);  p += 2;
    /*synce_trace("Unknown         = %04x = %i", unknown_a[i], unknown_a[i]);*/
  }

  if (unknown_a[0] != 0x3004)
    synce_warning("Expected 0x3004, got %04x", unknown_a[1]);

  if (unknown_a[0] != unknown_a[1])
    synce_warning("%04x != %04x", unknown_a[0], unknown_a[1]);

  self->recurrence_type = READ_INT32(p);  p += 4;

  synce_trace("unknown_a[2]    = %04x", unknown_a[2]);
  switch (self->recurrence_type)
  {
    case olRecursDaily:
      if (unknown_a[2] != 0x200a)
        synce_warning("Expected 0x200a, got %04x", unknown_a[2]);
      break;
    case olRecursWeekly:
      /* 0x200a is used for "Daily - Every weekday" */
      /* 0x200b is used for "Weekly - Mo,Tu,On,To,Fr */
      if (unknown_a[2] != 0x200a && unknown_a[2] != 0x200b)
        synce_warning("Expected 0x200a or 0x200b, got %04x", unknown_a[2]);
      break;
    case olRecursMonthly:
    case olRecursMonthNth:
      /* 0x200c probably means "show as Monthly" */
      /* 0x200d probably means "show as Yearly" */
      if (unknown_a[2] != 0x200c && unknown_a[2] != 0x200d)
        synce_warning("Expected 0x200c or 0x200d, got %04x", unknown_a[2]);
      break;
    default:
      break;
  }

  {
    unsigned show_type = unknown_a[2] - SHOW_AS_EXTRA;
    const char* show_type_name = "Unknown";

    if (show_type < RRA_RECURRENCE_TYPE_COUNT)
      show_type_name = RECURRENCE_TYPE_NAME[show_type];
    synce_trace("Show as recurrence type %i (%s)", show_type, show_type_name);
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

RRA_RecurrencePattern* rra_recurrence_pattern_from_buffer(uint8_t* buffer, size_t size)/*{{{*/
{
  bool success;
  uint8_t* p = buffer;
  uint8_t* p_saved = NULL;
  RRA_RecurrencePattern* self = rra_recurrence_pattern_new();
  if (!self)
    return NULL;

  rra_recurrence_pattern_read_header(self, &p);

  p_saved = p;

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
  TRACE_DATE("Pattern start date   = %s", self->pattern_start_date);
  self->pattern_end_date   = READ_UINT32(p);  p += 4;
  /*synce_trace("Pattern end date     = %08x", self->pattern_end_date);*/
  TRACE_DATE("Pattern end date     = %s", self->pattern_end_date);

  if ((self->flags & RecurrenceEndMask) == RecurrenceDoesNotEnd && 
      self->pattern_end_date != RRA_DoesNotEndDate) 
  {
    synce_warning("Recurrence does not end, but the end date is not the expected value");
  }

  /* Testing */
  if (self->recurrence_type == olRecursMonthly)
  {
    uint32_t minutes_in_year = READ_UINT32(p_saved);
    struct tm t = rra_minutes_to_struct(self->pattern_start_date);
    uint32_t minutes;
    t.tm_sec = 0;
    t.tm_min = 0;
    t.tm_hour = 0;
    t.tm_mday = minutes_in_year / MINUTES_PER_DAY;
    t.tm_mon = 0;
    minutes = rra_minutes_from_struct(&t);
    TRACE_DATE("Real start date?     = %s", minutes);
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
  synce_trace("Start time      = %02i days, %02i hours, %02i min", 
      self->start_minute / (60*24), (self->start_minute / 60) % 24, self->start_minute % 60);
  synce_trace("End time        = %02i days, %02i hours, %02i min", 
      self->end_minute   / (60*24), (self->end_minute   / 60) % 24, self->end_minute   % 60);

  rra_exceptions_read_details(self->exceptions, &p);

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

  if (size != rra_recurrence_pattern_size(self))
    synce_warning("Calculated pattern size to %04x but it was %04x",
        rra_recurrence_pattern_size(self), size);

  /** DEBUG --> */
  {
    uint8_t* new_buffer = NULL;
    size_t new_size = 0;
    success = rra_recurrence_pattern_to_buffer(self, &new_buffer, &new_size);
    if (success)
    {
      if (size == new_size && 0 == memcmp(buffer, new_buffer, size))
        synce_info("rra_recurrence_pattern_to_buffer() works great!");
      else
      {
        FILE* file;

        synce_warning("rra_recurrence_pattern_to_buffer() returned a different buffer! (expected size %04x, got %04x)", 
            size, new_size);

        file = fopen("pattern-right.bin", "w");
        fwrite(buffer, size, 1, file);
        fclose(file);
        file = fopen("pattern-wrong.bin", "w");
        fwrite(new_buffer, new_size, 1, file);
        fclose(file);      
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

bool rra_recurrence_pattern_to_buffer(RRA_RecurrencePattern* self, uint8_t** buffer, size_t* size)
{
  bool success = false;
  uint8_t* p = NULL;

  if (!self || !buffer || !size) 
  {
    synce_error("One or more invalid function parameters");
    return false;
  }
  
  *size = rra_recurrence_pattern_size(self);
  p = *buffer = calloc(1, *size);

  WRITE_UINT16(p, UNKNOWN_3004); p += 2;
  WRITE_UINT16(p, UNKNOWN_3004); p += 2;

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
              self->pattern_start_date, self->interval)); 
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
              self->pattern_start_date, self->interval)); 
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

  WRITE_UINT32(p, self->pattern_start_date); p += 4;
  WRITE_UINT32(p, self->pattern_end_date); p += 4;

  WRITE_UINT32(p, UNKNOWN_3005); p += 4;
  WRITE_UINT32(p, UNKNOWN_3005); p += 4;

  WRITE_UINT32(p, self->start_minute);  p += 4;
  WRITE_UINT32(p, self->end_minute);    p += 4;

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

