#include "synce.h"
#include <stdio.h>

void convert_from_unix_time(time_t unix_time)
{
	FILETIME filetime;
	unsigned char* bytes = (unsigned char*)&filetime;
	filetime_from_unix_time(unix_time, &filetime);
	printf(
			"DWORDs: %08x %08x\n"
			"Bytes:  %02x %02x %02x %02x %02x %02x %02x %02x\n", 
			filetime.dwLowDateTime, 
			filetime.dwHighDateTime,
			bytes[0], bytes[1], bytes[2], bytes[3],
			bytes[4], bytes[5], bytes[6], bytes[7]	
			);
}

void convert_to_unix_time(DWORD low, DWORD high)
{
	FILETIME filetime = {low, high};
	time_t unix_time = filetime_to_unix_time(&filetime);
	printf("Dec: %i\nHex: %08x\n", 
			(unsigned)unix_time, 
			(unsigned)unix_time);
}

int main(int argc, char** argv)
{
	switch (argc)
	{
		case 2:
			convert_from_unix_time(
					(time_t)strtol(argv[1], NULL, 0)
					);
			break;

		case 3:
			convert_to_unix_time(
					(DWORD)strtol(argv[1], NULL, 16),
					(DWORD)strtol(argv[2], NULL, 16)
					);
			break;


		default:
			return 1;
	}

	return 0;
}
