/* $Id$ */
#ifndef __appointment_ids_h__
#define __appointment_ids_h__

#define ID_UNKNOWN_0001    0x0001 /* something about repeated appointemnts */
#define ID_UNKNOWN_0002    0x0002
#define ID_SENSITIVITY     0x0004

#define SENSITIVITY_PUBLIC  0
#define SENSITIVITY_PRIVATE 1

#define ID_BUSY_STATUS     0x000f

#define BUSY_STATUS_FREE           0
#define BUSY_STATUS_TENTATIVE      1
#define BUSY_STATUS_BUSY           2
#define BUSY_STATUS_OUT_OF_OFFICE  3

#define ID_CATEGORIES      0x0016
#define ID_NOTES           0x0017
#define ID_SUBJECT         0x0037

#define ID_UNKNOWN_0042    0x0042
#define ID_UNKNOWN_0067    0x0067

#define ID_CATEGORY        0x4005
#define ID_UNKNOWN_4015    0x4015 /* something about repeated appointemnts */
#define ID_UNKNOWN_4171    0x4171

#define ID_LOCATION        0x4208
#define ID_APPOINTMENT_START 0x420d
#define ID_DURATION        0x4213
#define ID_DURATION_UNIT   0x4215

#define DURATION_UNIT_DAYS     1
#define DURATION_UNIT_MINUTES  2

#define ID_OCCURANCE       0x4223 

#define OCCURANCE_ONCE      0
#define OCCURANCE_REPEATED  1

#define ID_REMINDER_MINUTES_BEFORE_START  0x4501
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

