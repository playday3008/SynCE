/*
 * $Id$
 *
 * This file contains
 *
 * - Constants for database types
 * - Constants for database property ID:s
 *  
 */

/*
 * Database types
 *
 * Values retrieved from the CeFindAllDatabases test
 */
#define DBTYPE_CONTACTS 0x18;
#define DBTYPE_CATEGORIES	0x1b
#define DBTYPE_TASKS 0x1a
#define DBTYPE_APPOINTMENTS 0x19


/*
 * Database property ID:s
 *
 * Notes:
 * - This is the high word of the property id
 * - Unofficial property names are prefixed with 'X'
 * - ID:s are sorted first by category, then by value
 */

/*
 * Categories
 */

/*
 * Contacts
 */

/*
 * Tasks
 */

/*
 * Appointments
 *
 * Offical property names from:
 *   http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcesdkr/htm/_wcesdk_iappointment_propertylist.asp
 */
#define PROPID_Sensitivity                 0x0004
#define PROPID_BusyStatus                  0x000f	// 0=Free,1=Tentative,2=Busy,3=OutOfOffice
	#define olFree 0
	#define olTentative 1
	#define olBusy 2
	#define olOutOfOffice 3
#define PROPID_Categories	                 0x0016
#define PROPID_XNotes	                     0x0017
#define PROPID_Subject	                   0x0037
#define PROPID_X52bytes	                   0x0067
#define PROPID_Location	                   0x4208
#define PROPID_Start	                     0x420d
#define PROPID_Duration	                   0x4213 // see DurationUnit
#define PROPID_XDurationUnit               0x4215 // 1=Days,2=Minutes
	#define DURATION_DAYS 1
  #define DURATION_MINUTES 2
#define PROPID_ReminderMinutesBeforeStart  0x4501 // minutes
#define PROPID_ReminderEnabled             0x4503
#define PROPID_ReminderSoundFile           0x4509


