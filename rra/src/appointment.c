/* $Id$ */
#define _BSD_SOURCE 1
#include "appointment.h"
#include "appointment_ids.h"
#include "strbuf.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>

char* vcal_time_string(time_t t, long duration_unit)
{
	static char time_string[32]; /* evil static data */
	struct tm* time_struct = localtime(&t);

	if (duration_unit == DURATION_UNIT_DAYS)
		strftime(time_string, sizeof(time_string), "%Y%m%d", time_struct);
	else
		strftime(time_string, sizeof(time_string), "%Y%m%dT%H%M%S", time_struct);

	return time_string;
}

bool appointment_to_vcal(
	uint32_t id,
	CEPROPVAL* pFields,
	uint32_t count,
	char** ppVcal)
{
	int i;
	StrBuf* vcal = strbuf_new(NULL);
	time_t start = 0;
	long duration = 0;
	long duration_unit = 0;
	long reminder_minutes_before_start = 0;
	long reminder_enabled = 0;

	strbuf_append(vcal, "BEGIN:vCalendar\n");
	strbuf_append(vcal, "CALSCALE:GREGORIAN\n");
	strbuf_append(vcal, "VERSION:2.0\n");
	strbuf_append(vcal, "PRODID:-//SYNCE RRA//NONSGML Version 1//EN\n");
	strbuf_append(vcal, "BEGIN:vEvent\n");

	for (i = 0; i < count; i++)
	{
		/* TODO: validate data types */
		switch (pFields[i].propid >> 16)
		{
			case ID_SENSITIVITY:
				strbuf_append(vcal, "CLASS:");
				switch (pFields[i].val.iVal)
				{
					case SENSITIVITY_PUBLIC:
						strbuf_append(vcal, "PUBLIC");
						break;

					default:
						synce_warning("Unknown sensitivity: %i", pFields[i].val.iVal);
						break;
				}
				strbuf_append_c(vcal, '\n');
				break;
			
			case ID_BUSY_STATUS:
				/* XXX: don't know what vCalendar entry this is */
				synce_trace("Don't know how to handle busy status yet");
				break;

			case ID_NOTES:
				synce_trace("Don't know how to handle notes yet");
				break;
			
			case ID_SUBJECT:
				strbuf_append(vcal, "SUMMARY:");
				strbuf_append_wstr(vcal, pFields[i].val.lpwstr);
				strbuf_append_c(vcal, '\n');
				break;

			case ID_LOCATION:
				strbuf_append(vcal, "LOCATION:");
				strbuf_append_wstr(vcal, pFields[i].val.lpwstr);
				strbuf_append_c(vcal, '\n');
				break;

			case ID_START:
				start = filetime_to_unix_time(&pFields[i].val.filetime);
				break;

			case ID_DURATION:
				duration = pFields[i].val.lVal;
				break;

			case ID_OCCURANCE:
				if (OCCURANCE_ONCE != pFields[i].val.iVal)
					synce_warning("Can't handle repeated appointments yet");
				break;
				
			case ID_DURATION_UNIT:
				duration_unit = pFields[i].val.lVal;
				break;

			case ID_REMINDER_MINUTES_BEFORE_START:
				reminder_minutes_before_start = pFields[i].val.lVal;
				break;

			case ID_REMINDER_ENABLED:
				reminder_enabled = pFields[i].val.iVal;
				break;

			case ID_REMINDER_SOUND_FILE:
				/* handled but not used */
				break;
				
			default:
				synce_warning("Did not handle field with ID %04x", pFields[i].propid >> 16);
				break;
		}
	}

	if (start)
	{
		strbuf_append(vcal, "DTSTART");
		if (duration_unit == DURATION_UNIT_DAYS)
			strbuf_append(vcal, ";VALUE=DATE");
		strbuf_append_c(vcal, ':');
		strbuf_append(vcal, vcal_time_string(start, duration_unit));
		strbuf_append_c(vcal, '\n');

		if (duration_unit)
		{
			switch (duration_unit)
			{
				case DURATION_UNIT_DAYS:
					duration *= 24 * 60;
					break;

				case DURATION_UNIT_MINUTES:
					duration *= 60;
					break;

				default:
					synce_warning("Unknown duration unit %i", duration_unit);
					break;
			}

			strbuf_append(vcal, "DTEND");
			if (duration_unit == DURATION_UNIT_DAYS)
				strbuf_append(vcal, ";VALUE=DATE");
			strbuf_append_c(vcal, ':');
			strbuf_append(vcal, vcal_time_string(start + duration, duration_unit));
			strbuf_append_c(vcal, '\n');
		}
	}

	if (reminder_enabled)
	{
		char alarm[64];

		snprintf(alarm, sizeof(alarm),
				"TRIGGER;VALUE=DURATION;RELATED=START:-PT%liM\n",
				reminder_minutes_before_start);

		strbuf_append(vcal, "BEGIN:vAlarm\n");
		strbuf_append(vcal, alarm);
		strbuf_append(vcal, "END:vAlarm\n");
	}
	
	strbuf_append(vcal, "END:vEvent\n");
	strbuf_append(vcal, "END:vCalendar\n");

	*ppVcal = vcal->buffer;
	strbuf_free(vcal, false);
	return true;
}

