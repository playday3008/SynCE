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
  bool deleted;
  time_t original_date;
  time_t start_date;
  time_t end_date;
} RRA_Exception;

typedef struct _RRA_Exceptions RRA_Exceptions;

int rra_exceptions_count(RRA_Exceptions *self);
RRA_Exception* rra_exceptions_item(RRA_Exceptions *self, int index);

typedef struct _RRA_RecurrencePattern
{
  int32_t recurrence_type;
  time_t pattern_start_date;
  time_t pattern_end_date;
  int32_t flags;
  int32_t occurrences;
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


#endif
