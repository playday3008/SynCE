/* $Id$ */
#ifndef __recurrence_internal_h__
#define __recurrence_internal_h__

#include <stdint.h>

#define P __attribute__((packed))

/* 
   POCKET PC CONSTANTS AND STRUCTURES 
 */


/* compatible with pimstore.h */
typedef enum OlRecurrenceType
{  
  olRecursDaily     = 0,
  olRecursWeekly    = 1,
  olRecursMonthly   = 2,
  olRecursMonthNth  = 3,
  olRecursYearly    = 5,
  olRecursYearNth   = 6
} OlRecurrenceType;

/* compatible with pimstore.h */
typedef enum OlDaysOfWeek
{  
  olSunday    = 1,
  olMonday    = 2,
  olTuesday   = 4,
  olWednesday = 8,
  olThursday  = 16,
  olFriday    = 32,
  olSaturday  = 64
} OlDaysOfWeek;

/* 
   Format for BLOB 0x4015 
 */

/* size = 0x20 */
typedef struct
{
  uint32_t  interval;           /* 0x0e */
  uint32_t  unknown1;           /* 0x12 */
  uint32_t  days_of_week_mask;  /* 0x16 */
  uint32_t  flags;              /* 0x1a */
  uint32_t  occurrences;        /* 0x1e */
  uint8_t   padding[0xc];       /* 0x22 */
} RecurringWeekly P;

/* size = 0x20 */
typedef struct
{
  uint32_t  interval;           /* 0x0e */
  uint32_t  unknown1;           /* 0x12 */
  uint32_t  day_of_month;       /* 0x16 */
  uint32_t  flags;              /* 0x1a */
  uint32_t  occurrences;        /* 0x1e */
  uint8_t   padding[0xc];       /* 0x22 */
} RecurringMonthly P;

/* size = 0x20 */
typedef struct
{
  uint32_t  interval;           /* 0x0e */
  uint32_t  unknown1;           /* 0x12 */
  uint32_t  days_of_week_mask;  /* 0x16 */
  uint32_t  instance;           /* 0x1a */
  uint32_t  flags;              /* 0x1e */
  uint32_t  occurrences;        /* 0x22 */
  uint8_t   padding[8];         /* 0x26 */
} RecurringMonthNth P;

/* size = 0x68 */
typedef struct
{
  uint8_t   unknown1[6] P;        /* 0x00 */
  uint32_t  recurrence_type P;    /* 0x06 */
  uint32_t  unknown2 P;           /* 0x0a */
  union                           /* 0x0e */
  {
    RecurringWeekly   weekly P;
    RecurringMonthly  monthly P;
    RecurringMonthNth month_nth P;
  } details P;
  uint8_t   unknown3[0x10] P;     /* 0x2e */
  uint32_t  start_minute P;       /* 0x3e */
  uint32_t  end_minute P;         /* 0x42 */
  uint8_t   unknown4[0x22] P;     /* 0x46 */
} RecurrencePattern P;

enum
{
  FLAG_ENDS_ON_DATE             = 1,
  FLAG_ENDS_AFTER_X_OCCURENCES  = 2,
  FLAG_DOES_NOT_END             = 3
};

#define DEFAULT_FLAGS    0x2020


/* 
   ICALENDAR CONSTANTS 
 */

typedef enum 
{
  FREQ_UNKNOWN,
  FREQ_SECONDLY,
  FREQ_MINUTELY,
  FREQ_HOURLY,
  FREQ_DAILY,
  FREQ_WEEKLY, 
  FREQ_MONTHLY, 
  FREQ_YEARLY
} RRULE_FREQ;


/* bitmasks for all possible recurrence properties */
enum
{
  PROPERTY_FREQ     = 1<<0,
  PROPERTY_UNTIL    = 1<<1,
  PROPERTY_COUNT    = 1<<2,
  PROPERTY_INTERVAL = 1<<3,
  PROPERTY_BYDAY    = 1<<4,
  PROPERTY_BYSETPOS = 1<<5
};

typedef struct
{
  unsigned    properties;
  RRULE_FREQ  freq;
  uint32_t    interval;
  uint32_t    count;
  char*       byday;
  char*       bysetpos;
} RecurrenceRule;

#undef P

#endif

