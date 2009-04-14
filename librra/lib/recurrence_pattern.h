/* $Id$ */
#ifndef __rra_recurrence_pattern_h__
#define __rra_recurrence_pattern_h__

#include <synce.h>
#include <timezone.h>

/* compatible with OlRecurrenceType in pimstore.h */
typedef enum
{  
  olRecursDaily     = 0,
  olRecursWeekly    = 1,
  olRecursMonthly   = 2,
  olRecursMonthNth  = 3,
  olRecursYearly    = 5,
  olRecursYearNth   = 6,
  RRA_RECURRENCE_TYPE_COUNT = 7
} RRA_RecurrenceType;

typedef enum
{  
  RecurrenceDaily   = 0x200a,
  RecurrenceWeekly  = 0x200b,
  RecurrenceMonthly = 0x200c,
  RecurrenceYearly  = 0x200d,
} RRA_RecurrencePatternType;

#define RRA_RECURRENCE_PATTERN_TYPE_COUNT 4

/* compatible with OlDaysOfWeek in pimstore.h */
typedef enum
{  
  olSunday    = 1,
  olMonday    = 2,
  olTuesday   = 4,
  olWednesday = 8,
  olThursday  = 16,
  olFriday    = 32,
  olSaturday  = 64
} RRA_DaysOfWeek;

#define RRA_Weekdays (olMonday|olTuesday|olWednesday|olThursday|olFriday)

typedef enum
{
  RecurrenceEndsOnDate = 1,
  RecurrenceEndsAfterXOccurrences = 2,
  RecurrenceDoesNotEnd = 3,
  RecurrenceEndMask = 3
} RRA_RecurrenceFlags;

#define RRA_DoesNotEndDate    0x5ae980df

typedef struct _RRA_Exception
{
  /** Just date, not time, of the original appointment */
  uint32_t date;
  /** If it is a deleted appointment (otherwise it is modified) */
  bool deleted;
  /** Original start date and time (in minutes from January 1, 1601) */
  uint32_t original_time;
  /** Use this start date and time instead. (Only used if deleted is false.) */
  uint32_t start_time;
  /** Use this end date and time instead. (Only used if deleted is false.) */
  uint32_t end_time;
  /** Bitmask. (Only used if deleted is false.) */
  uint16_t bitmask;
  /** Subject if bitmask & RRA_EXCEPTION_SUBJECT */
  WCHAR* subject;
  /** Remain this many minutes before start if bitmask & 
   * RRA_EXCEPTION_REMINDER_MINUTES_BEFORE_START */
  uint32_t reminder_minutes_before_start;
  /** Unknown string if bitmask & RRA_EXCEPTION_UNKNOWN_8 */
  WCHAR* unknown_8;
  /** Location if bitmask & RRA_EXCEPTION_LOCATION */
  WCHAR* location;
  /** Status if bitmask & RRA_EXCEPTION_STATUS */
  uint32_t status;
  /** Type if bitmask & RRA_EXCEPTION_TYPE */
  uint32_t type;
  /** Notes if bitmask & RRA_EXCEPTION_NOTES */
  uint32_t notes_size;
  uint8_t* notes_data;
} RRA_Exception;

/* string = 16-bit unknown, 16-bit length, wide string */
#define RRA_EXCEPTION_SUBJECT   0x0001  /* string */
#define RRA_EXCEPTION_REMINDER_MINUTES_BEFORE_START 0x0004  /* 32-bit integer */
#define RRA_EXCEPTION_UNKNOWN_8 0x0008  /* string */
#define RRA_EXCEPTION_LOCATION  0x0010  /* string */
#define RRA_EXCEPTION_STATUS    0x0020  /* 32-bit integer */
#define RRA_EXCEPTION_TYPE      0x0080  /* 32-bit integer */
#define RRA_EXCEPTION_NOTES     0x0100  /* 32-bit size, data */

#define RRA_EXCEPTION_KNOWN_BITS  \
  (RRA_EXCEPTION_SUBJECT|RRA_EXCEPTION_REMINDER_MINUTES_BEFORE_START|RRA_EXCEPTION_UNKNOWN_8|RRA_EXCEPTION_LOCATION|RRA_EXCEPTION_STATUS|RRA_EXCEPTION_TYPE|RRA_EXCEPTION_NOTES)

typedef struct _RRA_Exceptions RRA_Exceptions;

int rra_exceptions_count(RRA_Exceptions *self);
RRA_Exception* rra_exceptions_item(RRA_Exceptions *self, int index);
void rra_exceptions_make_reservation(RRA_Exceptions* self, size_t count);

typedef struct _RRA_RecurrencePattern
{
  int32_t recurrence_type;
  int32_t recurrence_pattern_type;
  uint32_t pattern_start_date;  /* in minutes from January 1, 1601 */
  uint32_t pattern_end_date;    /* in minutes from January 1, 1601 */
  int32_t flags;
  int32_t occurrences;        /* 0 if (flags & RecurrenceEndMask) == RecurrenceDoesNotEnd */
  int32_t duration;
  int32_t interval;           /* olRecursDaily,    olRecursMonthly, olRecursMonthNth, olRecursWeekly */
  int32_t days_of_week_mask;  /* olRecursMonthNth, olRecursWeekly,  olRecursYearNth                  */
  int32_t minutes_to_month;   /* olRecursMonthly,  olRecursMonthNth, olRecursYearly                  */
  int32_t day_of_month;       /* olRecursMonthly,  olRecursYearly                                    */
  int32_t instance;           /* olRecursMonthNth, olRecursYearNth                                   */
  int32_t month_of_year;      /* olRecursYearly,   olRecursYearNth                                   */
  RRA_Exceptions* exceptions;
  int32_t start_minute;
  int32_t end_minute;
} RRA_RecurrencePattern;

RRA_RecurrencePattern* rra_recurrence_pattern_new();
void rra_recurrence_pattern_destroy(RRA_RecurrencePattern* self);

RRA_RecurrencePattern* rra_recurrence_pattern_from_buffer(uint8_t* buffer, size_t size, RRA_Timezone *tzi);
bool rra_recurrence_pattern_to_buffer(RRA_RecurrencePattern* self, uint8_t** buffer, size_t* size, RRA_Timezone *tzi);
#define rra_recurrence_pattern_free_buffer(buffer) if (buffer) free(buffer)

/*
   Date and time conversions
*/

/** Use this constant for your own calculations */
#define RRA_MINUTES_FROM_1601_TO_1970      194074560

/** Convert minutes to a time_t value, assuming date is in range */
time_t rra_minutes_to_unix_time(uint32_t minutes);

/** Convert minutes to a struct tm, assuming date is in range */
struct tm rra_minutes_to_struct(uint32_t minutes);

/** Convert minutes from a time_t value */
uint32_t rra_minutes_from_unix_time(time_t t);

/** Convert minutes from a struct tm */
uint32_t rra_minutes_from_struct(struct tm* t);
  
#endif
