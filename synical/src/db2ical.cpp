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

#define SUCCESS 0
#define FAILURE 1

#define ASSERT(value) if (!(value)) return FAILURE;

#define TIMEZONE_OFFSET (2*60*60)		// seconds 

class RapiString
{
	private:
		mutable char* mAscii;
		mutable WCHAR* mUcs2;
	
	public:
		RapiString(const char* ascii)
			: mAscii(NULL), mUcs2(NULL)
		{
			mAscii = new char[strlen(ascii)+1];
			strcpy(mAscii, ascii);
		}

		RapiString(const WCHAR* ucs2)
			: mAscii(NULL), mUcs2(NULL)
		{
			mUcs2 = new WCHAR[wcslen(ucs2)+1];
			wcscpy(mUcs2, ucs2);
		}

		~RapiString()
		{
			if (mAscii) delete mAscii;
			if (mUcs2) delete mUcs2;
		}

		operator const WCHAR*() const
		{
			if (!mUcs2)
			{
				mUcs2 = new WCHAR[strlen(mAscii)+1];
				// TODO: make nicer conversion
				for (int i = 0; i <= strlen(mAscii); i++)
				{
					mUcs2[i] = mAscii[i];
				}
			}

			return mUcs2;
		}

		operator const char*() const
		{
			if (!mAscii)
			{
				mAscii = new char[wcslen(mUcs2)+1];
				// TODO: make nicer conversion
				for (int i = 0; i <= wcslen(mUcs2); i++)
				{
					mAscii[i] = (char)mUcs2[i];
				}
			}

			return mAscii;
		}

};

struct RawAppointment
{
	RawAppointment()
		: Duration(0), XDurationUnit(0), ReminderMinutesBeforeStart(0), ReminderEnabled(0)
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

		default:
			break;
	}

	return SUCCESS;
}

int handle_record(icalcomponent * calendar, PCEPROPVAL values, DWORD property_count)
{
	icalcomponent* event = icalcomponent_new(ICAL_VEVENT_COMPONENT);

	// UID should really come from the 52 byte BLOB...
	char temp[1024];
	snprintf(temp, sizeof(temp), "synce-db2ical-%p", event);
	icalcomponent_add_property(event, icalproperty_new_uid(temp));
	
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

	// Subject/Summary
	if (appointment.Subject.size())
	{
		icalcomponent_add_property(event, icalproperty_new_summary(appointment.Subject.c_str()));
	}

	// Location
	if (appointment.Location.size())
	{
		icalcomponent_add_property(event, icalproperty_new_location(appointment.Location.c_str()));
	}

	time_t Start = DOSFS_FileTimeToUnixTime(&(appointment.Start), NULL) + TIMEZONE_OFFSET;

	// Reminder/Alarm
	if (appointment.ReminderEnabled)
	{
		icaltriggertype trigger;
		memset(&trigger, 0, sizeof(trigger));
		
		trigger.time = icaltime_from_timet(Start - appointment.ReminderMinutesBeforeStart * 60, 0);
		
	{
		printf("%i\n",appointment.ReminderMinutesBeforeStart );
		char temp[256];
		time_t x = Start /*- appointment.ReminderMinutesBeforeStart * 60*/;
		strftime(temp, sizeof(temp), "%c", localtime(&x));
		printf("%s", temp);
	}

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
		icalproperty_set_value(prop, icalvalue_new_date(icaltime_from_timet(Start, 1)));
		icalcomponent_add_property(event, prop);

		if (appointment.Duration)
		{
			time_t dtend = Start + (appointment.Duration - 1) * 24 * 60 * 60;
			
			prop = icalproperty_new_dtend(icaltime_null_time());
			icalproperty_set_value(prop, icalvalue_new_date(icaltime_from_timet(dtend, 1)));
			icalcomponent_add_property(event, prop);
		}
	}
	else // minutes
	{
		icalcomponent_add_property(event, icalproperty_new_dtstart(icaltime_from_timet(Start, 0)));

		if (appointment.Duration)
		{
			time_t dtend = Start + appointment.Duration*60;
			icalcomponent_add_property(event, icalproperty_new_dtend(icaltime_from_timet(dtend, 0)));
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
	icalcomponent_add_property(calendar, icalproperty_new_version("2.0"));
	icalcomponent_add_property(calendar, icalproperty_new_prodid("-//Synical//NONSGML db2ical//EN"));
				
	for (unsigned i = 0; i < MIN(5,num_records); i++)
	{
		WORD property_count;
		CEOID oid;
		oid = CeReadRecordProps(db, CEDB_ALLOWREALLOC, &property_count, NULL, (BYTE**)&values, &buffer_size);

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
	HRESULT hr = CeRapiInit();
	if (hr)
	{
		printf("CeRapiInit failed.\n");
		return FAILURE;
	}

	CEDB_FIND_DATA* find_data = NULL;
	WORD db_count = 0;
	
	// Find database
	BOOL success = CeFindAllDatabases(DBTYPE_APPOINTMENTS, 0xffff, &db_count, &find_data);
	if (!success)
	{
		printf("CeFindAllDatabases failed. Error code: %i=0x%x", CeGetLastError(), CeGetLastError());
		return FAILURE;
	}
	
	if (db_count != 1)
	{
		printf("Expected one database, found %u.\n", db_count);
		return FAILURE;
	}

	for (unsigned i = 0; i < db_count; i++)
	{
		HANDLE db = CeOpenDatabase(&find_data[i].OidDb, NULL, 0, CEDB_AUTOINCREMENT, NULL);
		
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

