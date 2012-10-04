/* $Id$ */
#ifndef __appointment_ids_h__
#define __appointment_ids_h__

#define ID_RECURRENCE_TIMEZONE 0x0001 /* something about repeated appointemnts */
#define ID_UNKNOWN_0002    0x0002
#define ID_UNKNOWN_0003    0x0003
#define ID_SENSITIVITY     0x0004
#define ID_UNKNOWN_0005    0x0005

#define SENSITIVITY_PUBLIC  0
#define SENSITIVITY_PRIVATE 1

#define ID_BUSY_STATUS     0x000f

#define BUSY_STATUS_FREE           0
#define BUSY_STATUS_TENTATIVE      1
#define BUSY_STATUS_BUSY           2
#define BUSY_STATUS_OUT_OF_OFFICE  3

#define ID_CATEGORIES_X    0x0016
#define ID_NOTES           0x0017

#define ID_IMPORTANCE       0x0026

#define IMPORTANCE_HIGH     1
#define IMPORTANCE_NORMAL   2
#define IMPORTANCE_LOW      3

#define ID_SUBJECT         0x0037

#define ID_UNKNOWN_0042    0x0042
#define ID_UNIQUE          0x0067

#define ID_CATEGORIES      0x4005 /* task only ? */
#define ID_RECURRENCE_PATTERN 0x4015
#define ID_TASK_START       0x4104
#define ID_TASK_DUE         0x4105

/* This ID exists both as a FILETIME and an 16-bit integer! */
#define ID_TASK_COMPLETED        0x410f
#define ID_UNKNOWN_4126          0x4126

#define ID_UNKNOWN_4171    0x4171

#define ID_LOCATION             0x4208
#define ID_APPOINTMENT_START    0x420d
#define ID_DURATION             0x4213
#define ID_APPOINTMENT_TYPE     0x4215

#define APPOINTMENT_TYPE_ALL_DAY     1
#define APPOINTMENT_TYPE_NORMAL      2

#define ID_OCCURENCE       0x4223 

#define OCCURENCE_ONCE      0
#define OCCURENCE_REPEATED  1

#define ID_REMINDER_MINUTES_BEFORE        0x4501
#define ID_REMINDER_ENABLED               0x4503
#define ID_REMINDER_SOUND_FILE            0x4509
#define ID_REMINDER_OPTIONS               0x450a

#define REMINDER_LED 1
#define REMINDER_VIBRATE 2
#define REMINDER_DIALOG 4
#define REMINDER_SOUND 8
#define REMINDER_REPEAT 16


#define ID_UNKNOWN_FFFD  0xfffd
#define ID_UNKNOWN_FFFE  0xfffe


#endif

