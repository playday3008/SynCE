/*
    Synical - Convert Pocket PC databases to iCalendar format
    Copyright (C) 2001  David Eriksson

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

//
// Retrieve Calendar database and store in iCalendar format
//
// Requires libical from http://softwarestudio.org/libical/index.html
//
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <iconv.h>
#include <unistd.h>
#include <time.h>

#include <string>
using namespace std;

extern "C"
{
#include <ical.h>
#include <icalss.h>
}

extern "C"
{
#include "rapi.h"
}
#include "cedatabase.h"
#include "rapistring.h"

#define SUCCESS 0
#define FAILURE 1

#define ASSERT(value) if (!(value)) return FAILURE;

struct icaltimetype icaltime_timet_as_local(time_t time, int is_date)
{
	icaltimetype tt = icaltime_as_zone(icaltime_from_timet(time, 0), tzname[0]);
	tt.is_date = is_date;
	return tt;
}


struct RawAppointment
{
	RawAppointment()
		: Duration(0), XDurationUnit(0), ReminderMinutesBeforeStart(0), 
			ReminderEnabled(0)
	{
		memset(&Start, 0, sizeof(Start));
	}
	
	string Subject;
	string Location;
	LONG Duration;
	LONG XDurationUnit;
	FILETIME Start;
	LONG ReminderMinutesBeforeStart;
	short ReminderEnabled;
	unsigned char Secret[XSECRET_SIZE];
};



int handle_property(PCEPROPVAL value, RawAppointment& appointment)
{
	unsigned id = value->propid >> 16;
	unsigned type = value->propid & 0xffff;

	switch (id)
	{
		case PROPID_Subject:
			ASSERT(CEVT_LPWSTR == type);
			appointment.Subject = RapiString(value->val.lpwstr);
			break;
		
		case PROPID_Location:
			ASSERT(CEVT_LPWSTR == type);
			appointment.Location = RapiString(value->val.lpwstr);
			break;
			
		case PROPID_Start:
			ASSERT(CEVT_FILETIME == type);
			appointment.Start = value->val.filetime;
			break;

		case PROPID_Duration:
			ASSERT(CEVT_I4 == type);
			appointment.Duration = value->val.lVal;
			break;

		case PROPID_XDurationUnit:
			ASSERT(CEVT_I4 == type);
			appointment.XDurationUnit = value->val.lVal;
			break;

		case PROPID_ReminderEnabled:
			ASSERT(CEVT_I2 == type);
			appointment.ReminderEnabled = value->val.iVal;
			break;

		case PROPID_ReminderMinutesBeforeStart:
			ASSERT(CEVT_I4 == type);
			appointment.ReminderMinutesBeforeStart = value->val.lVal;
			break;

		case PROPID_XSecret:
			ASSERT(CEVT_BLOB == type);
			ASSERT(XSECRET_SIZE == value->val.blob.dwCount);
			memcpy(appointment.Secret, value->val.blob.lpb, 
					value->val.blob.dwCount);
			break;

		default:
			break;
	}

	return SUCCESS;
}

int handle_record(icalcomponent * calendar, PCEPROPVAL values, 
		DWORD property_count)
{
	icalcomponent* event = icalcomponent_new(ICAL_VEVENT_COMPONENT);

#if 0
	icalproperty* x = icalproperty_new_x("David is testing");
	icalproperty_set_x_name(x, "X-SYNICAL");
	icalcomponent_add_property(event, x);
#endif
	
	icalcomponent_add_property(event, icalproperty_new_sequence(0));
	icalcomponent_add_property(event, icalproperty_new_class("PUBLIC"));

//	time_t now = time(NULL);
//	icalcomponent_add_property(event, icalproperty_new_created(icaltime_from_timet(now, 0)));
//	icalcomponent_add_property(event, icalproperty_new_lastmodified(icaltime_from_timet(now, 0)));
//	icalcomponent_add_property(event, icalproperty_new_dtstamp(icaltime_from_timet(now, 0)));

	RawAppointment appointment;
	
	for (unsigned j = 0; j < property_count; j++)
	{
		if (SUCCESS != handle_property(values+j, appointment))
				return FAILURE;
	} // for every property

	// use the 52 byte BLOB as UID...
	char temp[1024];
	snprintf(temp, sizeof(temp), "SYNICAL:", event);
	char* offset = temp + strlen(temp);
	for (int i = 0; i < XSECRET_SIZE; i++, offset+=2)
	{
		snprintf(offset, 3, "%02x", appointment.Secret[i]);
	}
//	printf("\nuid=%s\n", temp);
	icalcomponent_add_property(event, icalproperty_new_uid(temp));
	
	// Subject/Summary
	if (appointment.Subject.size())
	{
		icalcomponent_add_property(event, 
				icalproperty_new_summary(appointment.Subject.c_str()));
	}

	// Location
	if (appointment.Location.size())
	{
		icalcomponent_add_property(event,
			 icalproperty_new_location(appointment.Location.c_str()));
	}

	// This is UTC time
	time_t Start = DOSFS_FileTimeToUnixTime(&(appointment.Start), NULL);

	// Reminder/Alarm
	if (appointment.ReminderEnabled)
	{
		icaltriggertype trigger;
		memset(&trigger, 0, sizeof(trigger));
		
		trigger.time = icaltime_timet_as_local(
				Start - appointment.ReminderMinutesBeforeStart * 60, 0);
//		printf("trigger.time.is_utc=%i\n", trigger.time.is_utc);
		
#if 0
	{
		printf("%i\n",appointment.ReminderMinutesBeforeStart );
		char temp[256];
		time_t x = Start /*- appointment.ReminderMinutesBeforeStart * 60*/;
		strftime(temp, sizeof(temp), "%c", localtime(&x));
		printf("%s", temp);
	}
#endif

		// Create alarm and add to event
		icalcomponent* alarm = icalcomponent_new(ICAL_VALARM_COMPONENT);
		icalcomponent_add_property(alarm, icalproperty_new_trigger(trigger));
		icalcomponent_add_component(event, alarm);
	}

	if (DURATION_DAYS == appointment.XDurationUnit)
	{
		//
		// This uses the DATE value type in KMail, so do the same
		//
		icalproperty* prop = NULL;
		
		prop = icalproperty_new_dtstart(icaltime_null_time());
		icalproperty_set_value(prop, 
				icalvalue_new_date(icaltime_timet_as_local(Start, 1)));
		icalcomponent_add_property(event, prop);

		if (appointment.Duration)
		{
			time_t dtend = Start + (appointment.Duration - 1) * 24 * 60 * 60;
			
			prop = icalproperty_new_dtend(icaltime_null_time());
			icalproperty_set_value(prop, icalvalue_new_date(
						icaltime_timet_as_local(dtend, 1)));
			icalcomponent_add_property(event, prop);
		}
	}
	else // minutes
	{
		icalproperty* prop = NULL;
		prop = icalproperty_new_dtstart(icaltime_timet_as_local(Start, 0));
		icalcomponent_add_property(event, prop);

		if (appointment.Duration)
		{
			time_t dtend = Start + appointment.Duration*60;
			icalcomponent_add_property(event, 
					icalproperty_new_dtend(icaltime_timet_as_local(dtend, 0)));
		}
	}
	
	icalcomponent_add_component(calendar, event);

	return SUCCESS;
}

int handle_database(HANDLE db, DWORD num_records)
{
#define MYBUFSIZE  (16*1024)
	PCEPROPVAL values = (PCEPROPVAL)malloc(MYBUFSIZE);
	DWORD buffer_size = MYBUFSIZE;
	
	icalcomponent* calendar = icalcomponent_new(ICAL_VCALENDAR_COMPONENT);
	icalcomponent_add_property(calendar, 
			icalproperty_new_version("2.0"));
	icalcomponent_add_property(calendar, 
			icalproperty_new_prodid("-//Synical//NONSGML db2ical//EN"));
				
	for (unsigned i = 0; i < MIN(1000,num_records); i++)
	{
		WORD property_count;
		CEOID oid;
		oid = CeReadRecordProps(db, CEDB_ALLOWREALLOC, &property_count, NULL, 
				(BYTE**)&values, &buffer_size);

		if (!oid)
		{
			printf("CeReadRecordProps failed.\n");
			return FAILURE;
		}
	
		printf("Row %u (oid=0x%lx): ", i, oid);
	
		if (SUCCESS != handle_record(calendar, values, property_count))
			return FAILURE;
	
		printf("\n");
		
	} // for every row
	
	icalrestriction_check(calendar);

	const char * filename = "db2ical.ics";
	
	unlink(filename);
	icalset* fileset = icalset_new_file(filename);
	//icalset* fileset = icalfileset_new_open("db2ical.ics", O_CREAT|O_TRUNC|O_RDWR, S_IRWXU);
	icalset_add_component(fileset, calendar);
	icalset_commit(fileset);
	icalset_free(fileset);

	
	return SUCCESS;
}

int main()
{
	tzset();

	HRESULT hr = CeRapiInit();
	if (hr)
	{
		printf("CeRapiInit failed.\n");
		return FAILURE;
	}

	CEDB_FIND_DATA* find_data = NULL;
	WORD db_count = 0;
	
	// Find database
	BOOL success = CeFindAllDatabases(DBTYPE_APPOINTMENTS, 0xffff, &db_count, 
			&find_data);
	if (!success)
	{
		printf("CeFindAllDatabases failed. Error code: %i=0x%x", 
				CeGetLastError(), CeGetLastError());
		return FAILURE;
	}
	
	if (db_count != 1)
	{
		printf("Expected one database, found %u.\n", db_count);
		return FAILURE;
	}

	for (unsigned i = 0; i < db_count; i++)
	{
		HANDLE db = CeOpenDatabase(&find_data[i].OidDb, NULL, 0, 
				CEDB_AUTOINCREMENT, NULL);
		
		if (INVALID_HANDLE_VALUE == db)
		{
			printf ("CeOpenDatabase failed.");
			return FAILURE;
		}

		handle_database(db, find_data[i].DbInfo.wNumRecords);
	
		success = CeCloseHandle(db);
		if (!success)
		{
			printf("CeCloseHandle failed.\n");
			return FAILURE;
		}
	}

	hr = CeRapiFreeBuffer(find_data);

	hr = CeRapiUninit();
	return SUCCESS;
}

