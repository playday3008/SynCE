/* $Id$ */
#ifndef __task_ids_h__
#define __task_ids_h__

#define ID_UNKNOWN_0002     0x0002
#define ID_UNKNOWN_0003     0x0003
#define ID_SENSITIVITY      0x0004
#define ID_UNKNOWN_0005     0x0005

#define SENSITIVITY_PUBLIC  0
#define SENSITIVITY_PRIVATE 1

#define ID_NOTES            0x0017
#define ID_IMPORTANCE       0x0026

#define IMPORTANCE_HIGH     1
#define IMPORTANCE_NORMAL   2
#define IMPORTANCE_LOW      3

#define ID_SUBJECT          0x0037

#define ID_TASK_CATEGORIES  0x4005

#define ID_TASK_START       0x4104
#define ID_TASK_DUE         0x4105

/* This ID exists both as a FILETIME and an 16-bit integer! */
#define ID_TASK_COMPLETED        0x410f
#define ID_UNKNOWN_4126          0x4126

#define ID_REMINDER_MINUTES_BEFORE_END    0x4501
#define ID_REMINDER_ENABLED               0x4503
#define ID_REMINDER_SOUND_FILE            0x4509
#define ID_REMINDER_OPTIONS               0x450a

#define REMINDER_LED 1
#define REMINDER_VIBRATE 2
#define REMINDER_DIALOG 4
#define REMINDER_SOUND 8
#define REMINDER_REPEAT 16


#endif

