/* $Id$ */
#ifndef __rra_recurrence_pattern_h__
#define __rra_recurrence_pattern_h__

#include <synce.h>

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

typedef enum
{
  RecurrenceEndsOnDate = 1,
  RecurrenceEndsAfterXOccurrences = 2,
  RecurrenceDoesNotEnd = 3,
  RecurrenceEndMask = 3
} RRA_RecurrenceFlags;

typedef struct _RRA_Exception
{
  /** If it is a deleted appointment (otherwise it is modified) */
  bool deleted;
  /** Original start date (in minutes from January 1, 1601) */
  uint32_t original_date;
  /** Use this start date instead. (Only used if deleted is false.) */
  uint32_t start_date;
  /** Use this end date instead. (Only used if deleted is false.) */
  uint32_t end_date;
  /** Unknown value. (Only used if deleted is false.) */
  uint16_t unknown;
} RRA_Exception;

typedef struct _RRA_Exceptions RRA_Exceptions;

int rra_exceptions_count(RRA_Exceptions *self);
RRA_Exception* rra_exceptions_item(RRA_Exceptions *self, int index);

typedef struct _RRA_RecurrencePattern
{
  int32_t recurrence_type;
  uint32_t pattern_start_date;  /* in minutes from January 1, 1601 */
  uint32_t pattern_end_date;    /* in minutes from January 1, 1601 */
  int32_t flags;
  int32_t occurrences;        /* 0 if (flags & RecurrenceEndMask) == RecurrenceDoesNotEnd */
  int32_t duration;
  int32_t interval;           /* olRecursDaily,    olRecursMonthly, olRecursMonthNth, olRecursWeekly */
  int32_t days_of_week_mask;  /* olRecursMonthNth, olRecursWeekly,  olRecursYearNth                  */
  int32_t day_of_month;       /* olRecursMonthly,  olRecursYearly                                    */
  int32_t instance;           /* olRecursMonthNth, olRecursYearNth                                   */
  int32_t month_of_year;      /* olRecursYearly,   olRecursYearNth                                   */
  RRA_Exceptions* exceptions;
  int32_t start_minute;
  int32_t end_minute;
} RRA_RecurrencePattern;

RRA_RecurrencePattern* rra_recurrence_pattern_new();
void rra_recurrence_pattern_destroy(RRA_RecurrencePattern* self);

RRA_RecurrencePattern* rra_recurrence_pattern_from_buffer(uint8_t* buffer, size_t size);
bool rra_recurrence_pattern_to_buffer(RRA_RecurrencePattern* self, uint8_t** buffer, size_t* size);
#define rra_recurrence_pattern_free_buffer(buffer) if (buffer) free(buffer)

/*
   Date and time conversions
*/

/** Use this constant for your own calculations */
#define RRA_MINUTES_FROM_1601_TO_1970      194074560

/** Convert minutes to a time_t value, assuming it is in range */
time_t rra_minutes_to_unix_time(uint32_t minutes);

/** Convert minutes from a time_t value */
uint32_t rra_minutes_from_unix_time(time_t t);


#endif
