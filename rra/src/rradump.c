/* $Id$ */
#include <stdio.h>
#include <rapi.h>
#include "dbstream.h"

static const char* data_type_as_string(uint16_t dataType)
{
	const char* type = NULL;
	
	switch (dataType)
	{
		case CEVT_I2: type = "I2"; break;
		case CEVT_I4: type = "I4"; break;
			
		case CEVT_R8: type = "R8"; break;
		case CEVT_BOOL: type = "BOOL"; break;
			
		case CEVT_UI2: type = "UI2"; break;
		case CEVT_UI4: type = "UI4"; break;

		case CEVT_LPWSTR: type = "LPWSTR"; break;
		case CEVT_FILETIME: type = "FILETIME"; break;
		case CEVT_BLOB: type = "BLOB"; break;

		default: type = "Unknown"; break;
	}

	return type;
}

int main(int argc, char** argv)
{
	int result = 1;
	FILE* file = NULL;
	uint32_t field_count = 0;
	uint8_t* buffer = NULL;
	long file_size = 0;
	CEPROPVAL* propvals = NULL;
	int i;

	if (argc < 2)
	{
		fprintf(stderr, "Filename missing on command line\n");
		goto exit;
	}

	file = fopen(argv[1], "r");
	if (!file)
	{
		fprintf(stderr, "Unable to open file '%s'\n", argv[1]);
		goto exit;
	}

	/* find out file size */
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	buffer = (uint8_t*)malloc(file_size);
	fread(buffer, file_size, 1, file);

	field_count = letoh32(*(uint32_t*)(buffer + 0));
	/*printf("Field count: %i\n", field_count);*/

	propvals = (CEPROPVAL*)malloc(sizeof(CEPROPVAL) * field_count);

	if (!dbstream_to_propvals(buffer + 8, field_count, propvals))
	{
		fprintf(stderr, "Failed to convert database stream\n");
		goto exit;
	}

	printf("ID    TYPE           VALUE\n");
	
	for (i = 0; i < field_count; i++)
	{
		printf("%04x  %04x %-8s  ",
				propvals[i].propid >> 16,
				propvals[i].propid & 0xff,
				data_type_as_string(propvals[i].propid & 0xff));

		switch (propvals[i].propid & 0xff)
		{
			case CEVT_I2: printf("0x%08x  %i", propvals[i].val.iVal, propvals[i].val.iVal); break;
			case CEVT_I4: printf("0x%08x  %i", propvals[i].val.lVal, propvals[i].val.lVal); break;

			case CEVT_R8:  break;
			case CEVT_BOOL:  break;

			case CEVT_UI2: printf("0x%08x  %u", propvals[i].val.uiVal, propvals[i].val.uiVal); break;
			case CEVT_UI4: printf("0x%08x  %u", propvals[i].val.ulVal, propvals[i].val.ulVal); break;

			case CEVT_LPWSTR:  
				{
					char* ascii = wstr_to_ascii(propvals[i].val.lpwstr);
					printf("\"%s\"", ascii);
					wstr_free_string(ascii);
				}
				break;

			case CEVT_FILETIME:
				{
					time_t unix_time = filetime_to_unix_time(&propvals[i].val.filetime);
					char* time_str = ctime(&unix_time);
					time_str[strlen(time_str)-1] = '\0'; /* remove trailing newline */
					printf("%s", time_str);
				}
				break;
				
			case CEVT_BLOB:  break;
		}

		printf("\n");
	}


exit:
	if (file)
		fclose(file);
	
	if (buffer)
		free(buffer);

	if (propvals)
		free(propvals);

	return result;
}

