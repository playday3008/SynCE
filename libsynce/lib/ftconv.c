#include "synce.h"
#include <stdio.h>
#include <string.h>

void convert_from_unix_time(time_t unix_time)
{
	FILETIME filetime;
	uint8_t* bytes = (uint8_t*)&filetime;
	struct tm* tm = localtime(&unix_time);
	DATE date;
	uint8_t* date_bytes = (uint8_t*)&date;

	memset(&date, 0, sizeof(DATE));
	
	if (!date_from_tm(tm, &date))
		fprintf(stderr, "Failed to convert to DATE.\n");

	filetime_from_unix_time(unix_time, &filetime);
	
	printf(
			"Time:   %s\n"
			"FILETIME DWORDs: %08x %08x\n"
			"FILETIME Bytes:  %02x %02x %02x %02x %02x %02x %02x %02x\n"
			"DATE     Bytes:  %02x %02x %02x %02x %02x %02x %02x %02x\n", 
			ctime(&unix_time),
			filetime.dwLowDateTime, 
			filetime.dwHighDateTime,
			bytes[0], bytes[1], bytes[2], bytes[3],
			bytes[4], bytes[5], bytes[6], bytes[7],
			date_bytes[0], date_bytes[1], date_bytes[2], date_bytes[3],
			date_bytes[4], date_bytes[5], date_bytes[6], date_bytes[7]	
			);
}

void convert_from_date(uint8_t bytes[8])
{
	DATE* date = (DATE*)bytes;
	struct tm tm;
	
	if (date_to_tm(*date, 0, &tm))
	{
		char str[50];
		strftime(str, sizeof(str), "%c", &tm);
		printf("Date+time: %s\n", str);
	}
	else
	{
		printf("Does not look like a DATE\n");
	}
	
	if (date_to_tm(*date, DATE_DATEVALUEONLY, &tm))
	{
		char str[50];
		strftime(str, sizeof(str), "%c", &tm);
		printf("Only date: %s\n", str);
	}

	if (date_to_tm(*date, DATE_TIMEVALUEONLY, &tm))
	{
		char str[50];
		strftime(str, sizeof(str), "%c", &tm);
		printf("Only time: %s\n", str);
	}
}

void convert_to_unix_time(uint8_t bytes[8])
{
	FILETIME filetime = *(FILETIME*)bytes;
	time_t unix_time = filetime_to_unix_time(&filetime);
	printf("Dec:  %u\nHex:  %08x\nTime: %s\n", 
			(unsigned)unix_time, 
			(unsigned)unix_time,
			ctime(&unix_time));
}

int main(int argc, char** argv)
{
	uint8_t bytes[8];
	
	switch (argc)
	{
		case 2:
			convert_from_unix_time(
					(time_t)strtol(argv[1], NULL, 0)
					);
			break;

		case 9:
			{
				int i;
				for (i = 0; i < 8; i++)
					bytes[i] = strtol(argv[i+1], NULL, 16);

				convert_from_date(bytes);
				convert_to_unix_time(bytes);
			}
			break;

		default:
			fprintf(stderr, "Why this number of parameters?\n");
			return 1;
	}

	return 0;
}
