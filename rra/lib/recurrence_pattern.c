/* $Id$ */
#include "recurrence_pattern.h"
#include <stdlib.h>
#include <synce_log.h>
#include <ctype.h>
#include <string.h>

#define MINUTES_FROM_1601_TO_1970      194074560

#define READ_INT16(p)   (*(int16_t*)(p))
#define READ_INT32(p)   (*(int32_t*)(p))

#define READ_UINT16(p)   (*(uint16_t*)(p))
#define READ_UINT32(p)   (*(uint32_t*)(p))

#define TRACE_DATE(format, date) \
do { \
  char* _time_str = NULL; \
  time_t _unix_time = date; \
  if ((time_t)-1 == _unix_time) \
  { \
    _time_str = "(some time in year 2038 or later!)"; \
  } \
  else \
  { \
    _time_str = asctime(gmtime(&_unix_time)); \
    _time_str[strlen(_time_str)-1] = '\0'; \
  } \
  synce_trace(format, _time_str); \
} \
while (0)

#define TRACE_DATE_INT(format, date) \
do { \
  char* _time_str = NULL; \
  if ((date - MINUTES_FROM_1601_TO_1970) < 0x4444444) \
  { \
    time_t _unix_time = (date - MINUTES_FROM_1601_TO_1970) * 60; \
    _time_str = asctime(gmtime(&_unix_time)); \
    _time_str[strlen(_time_str)-1] = '\0'; \
  } \
  else \
  { \
    _time_str = "(some time in year 2038 or later!)"; \
  } \
  synce_trace(format, _time_str); \
} \
while (0)


static const char* RECURRENCE_TYPE_NAME[] = 
{
  "Daily", "Weekly", "Monthly", "MonthNth", "Yearly", "YearNth"
};

struct _RRA_Exceptions
{
  int32_t total_count;
  int32_t modified_count;
};

int rra_exceptions_count(RRA_Exceptions *self)
{
  return self->total_count;
}

RRA_Exception* rra_exceptions_item(RRA_Exceptions *self, int index)
{
  /* TODO: implement */
  return NULL;
}


static time_t read_date(uint8_t* buffer)
{
  int32_t minutes = READ_INT32(buffer);

  /* Assuming time_t is 32-bit, we will run into problems with dates after 2038... */
  
  if ((minutes - MINUTES_FROM_1601_TO_1970) < 0x4444444) \
    return (minutes - MINUTES_FROM_1601_TO_1970) * 60;
  else
    return (time_t)-1;
}

RRA_Exceptions* rra_exceptions_new()
{
  RRA_Exceptions* self = calloc(1, sizeof(RRA_Exceptions));
  return self;
}

void rra_exceptions_destroy(RRA_Exceptions* self)
{
  if (self)
  {
    /* probably some contents to destroy */
    free(self);
  }
}

static bool rra_exceptions_read_summary(RRA_Exceptions* self, uint8_t** buffer)
{
  uint8_t* p = *buffer;
  int i;

  self->total_count = READ_INT32(p);  p += 4;

  for (i = 0; i < self->total_count; i++)
  {
    time_t date = read_date(p); p += 4;
    TRACE_DATE("Exception date  = %s", date);
  }

  self->modified_count = READ_INT32(p);  p += 4;

  for (i = 0; i < self->modified_count; i++)
  {
    time_t date = read_date(p); p += 4;
    TRACE_DATE("Modified date   = %s", date);
  }
  
  *buffer = p;
  return true;
}

bool rra_exceptions_read_details(RRA_Exceptions* self, uint8_t** buffer)
{
  uint8_t* p = *buffer;
  int i;
  int16_t count;

  count = READ_INT16(p); p += 2;

  if (count != self->modified_count)
    synce_warning("Unexpected exception detail count!");

  for (i = 0; i < self->modified_count; i++)
  {
    time_t date;
    int16_t unknown;
    date = read_date(p); p += 4;
    TRACE_DATE("Modified appointment start time  = %s", date);
    date = read_date(p); p += 4;
    TRACE_DATE("Modified appointment end time    = %s", date);
    date = read_date(p); p += 4;
    TRACE_DATE("Original appointment start time  = %s", date);
    unknown = READ_INT16(p); p += 2;
    synce_trace("Modified appointment unknown     = %04x", unknown);
  }

  *buffer = p;
  return true;
}

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
  uint32_t unknown_b;

  unknown_b = READ_UINT32(p);  p += 4;
  synce_trace("Days to month   = %08x = %u minutes = %f days", 
      unknown_b, unknown_b, unknown_b / (60.0 * 24.0));

  self->interval          = READ_INT32(p);  p += 4;   /* 0x0e */
  
  p += 4;
  
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

bool rra_recurrence_pattern_read_header(/*{{{*/
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
    synce_trace("Unknown         = %04x = %i", unknown_a[i], unknown_a[i]);
  }

  if (unknown_a[0] != 0x3004)
    synce_warning("Expected 0x3004, got %04x", unknown_a[1]);

  if (unknown_a[0] != unknown_a[1])
    synce_warning("%04x != %04x", unknown_a[0], unknown_a[1]);

  self->recurrence_type = READ_INT32(p);  p += 4;

  /*
     I wish this was a flag to specify what fields are present, but I'm afraid not.

     a = 1010 = interval
     b = 1011 = interval + days_of_week_mask
     c = 1100 = interval + days_of_week_mask + instance    (for olRecursMonthNth)
     d = 1101 = interval + day_of_month                    (for olRecursMonthly)
     d = 1101 = interval + day_of_month                    (for olRecursMonthly)
     d = 1101 = interval + days_of_week_mask + instance    (for olRecursMonthNth)

     Maybe it is the number of 4-byte
  */
  switch (self->recurrence_type)
  {
    case olRecursDaily:
      if (unknown_a[2] != 0x200a)
        synce_warning("Expected 0x200a, got %04x", unknown_a[2]);
      break;
    case olRecursWeekly:
      if (unknown_a[2] != 0x200b)
        synce_warning("Expected 0x200b, got %04x", unknown_a[2]);
      break;
    case olRecursMonthly:
    case olRecursMonthNth:
      if (unknown_a[2] != 0x200c && unknown_a[2] != 0x200d)
        synce_warning("Expected 0x200c or 0x200d, got %04x", unknown_a[2]);
      break;
    default:
      break;
  }

  if (self->recurrence_type < RRA_RECURRENCE_TYPE_COUNT)
    recurrence_type_name = RECURRENCE_TYPE_NAME[self->recurrence_type];
  synce_trace("Recurrence type = %08x (%s)", self->recurrence_type, recurrence_type_name);

 
  *buffer = p;
  return true;
}/*}}}*/

RRA_RecurrencePattern* rra_recurrence_pattern_from_buffer(uint8_t* buffer, size_t size)/*{{{*/
{
  bool success;
  uint8_t* p = buffer;
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

  if ((self->flags & ~3) != 0x2020) 
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
  
  self->pattern_start_date = read_date(p);  p += 4;
  TRACE_DATE("Pattern start date   = %s", self->pattern_start_date);
  self->pattern_end_date   = read_date(p);  p += 4;
  TRACE_DATE("Pattern end date     = %s", self->pattern_end_date);

#if 0
  {
    uint32_t end_date = READ_INT32(p); p += 4;

    synce_trace("End date        = %08x", end_date);
    if (end_date == 0x5ae980df)
      synce_trace("                = (never)");
    else
      TRACE_DATE_INT("                = %s", end_date);
  }
#endif

  {
    int i;
    uint32_t unknown[2];

    for (i = 0; i < 2; i++)
    {
      unknown[i] = READ_INT32(p); p += 4;
      synce_trace("Unknown         = %08x", unknown[i]);
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
  return false;
}

