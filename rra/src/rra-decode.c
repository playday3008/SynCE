/* $Id$ */
#include <stdio.h>
#include <rapi.h>
#include <stdlib.h>
#include <string.h>
#include "dbstream.h"

static void
dump(void* data, size_t len)
{
	uint8_t* buf = (uint8_t*)data;
	size_t i, j;
	char hex[8 * 3 + 1];
	char chr[8 + 1];

	for (i = 0; i < len + 7; i += 8) {
		for (j = 0; j < 8; j++) 
			if (j + i >= len) {
				hex[3*j+0] = ' ';
				hex[3*j+1] = ' ';
				hex[3*j+2] = ' ';
				chr[j] = ' ';
			} else {
				uint8_t c = buf[j + i];
				const char *hexchr = "0123456789abcdef";
				hex[3*j+0] = hexchr[(c >> 4) & 0xf];
				hex[3*j+1] = hexchr[c & 0xf];
				hex[3*j+2] = ' ';
				if (c > ' ' && c <= '~')
					chr[j] = c;
				else
					chr[j] = '.';
			}
		hex[8*3] = '\0';
		chr[8] = '\0';
		if (hex[0] != ' ')
			printf("%04x: %s %s\n", i, hex, chr);
	}
}

static void
db_dump(void* data, size_t len)
{
	uint8_t* buf = (uint8_t*)data;
	size_t i, j;
	char hex[8 * 3 + 1];
	char chr[8 + 1];

	for (i = 0; i < len + 7; i += 8) {
		for (j = 0; j < 8; j++) 
			if (j + i >= len) {
				hex[3*j+0] = ' ';
				hex[3*j+1] = ' ';
				hex[3*j+2] = ' ';
				chr[j] = ' ';
			} else {
				uint8_t c = buf[j + i];
				const char *hexchr = "0123456789abcdef";
				hex[3*j+0] = hexchr[(c >> 4) & 0xf];
				hex[3*j+1] = hexchr[c & 0xf];
				hex[3*j+2] = ' ';
				if (c > ' ' && c <= '~')
					chr[j] = c;
				else
					chr[j] = '.';
			}
		hex[8*3] = '\0';
		chr[8] = '\0';
		if (hex[0] != ' ')
			printf("\n                     %04x: %s %s", i, hex, chr);
	}
}

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

void decode_directory(uint8_t* buffer)
{
	char* ascii = wstr_to_ascii((WCHAR*)(buffer + 4));
	printf("Directory: \"%s\"\n", ascii);
	wstr_free_string(ascii);
}

void decode_file(uint8_t* buffer, size_t size, const char* filename)
{
	WCHAR* wide   = (WCHAR*)(buffer + 4);
	size_t extra  = 6 + 2 * wstrlen(wide);
	char* ascii   = wstr_to_ascii(wide);

	printf("File: \"%s\"\n", ascii);

	if (filename)
	{
		FILE* file = NULL;
		 
		if (0 == strcmp(filename, "-"))
			filename = ascii;
		
		file = fopen(filename, "w");
		if (file)
		{
			fwrite(buffer + extra, size-extra, 1, file);
			fclose(file);
		}
		else
		{
			fprintf(stderr, "Unable to open file '%s'\n", filename);
		}

	}
	else	
		dump(buffer + extra, size - extra); 

	wstr_free_string(ascii);
}

bool decode_database_stream(uint8_t* buffer)
{
	bool success = false;
	CEPROPVAL* propvals = NULL;
	uint32_t field_count = 0;
	int i;

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
			case CEVT_I4: printf("0x%08lx  %li", propvals[i].val.lVal, propvals[i].val.lVal); break;

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
				
			case CEVT_BLOB:  
				printf("0x%x (%i) bytes:", propvals[i].val.blob.dwCount, propvals[i].val.blob.dwCount);
				db_dump(propvals[i].val.blob.lpb, propvals[i].val.blob.dwCount);
				
				break;

		}

		printf("\n");
	}

	success = true;

exit:
	if (propvals)
		free(propvals);

	return success;
}

int main(int argc, char** argv)
{
	int result = 1;
	FILE* file = NULL;
	uint8_t* buffer = NULL;
	long file_size = 0;

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

	if (*(uint32_t*)(buffer + 4) != 0)
	{
		switch (*(uint32_t*)(buffer + 0))
		{
			case 0x10:	/* directory */
				decode_directory(buffer);
				result = 0;
				break;
				
			case 0x20:	/* file*/
				decode_file(buffer, file_size, argv[2]);
				result = 0;
				break;

			default:
				fprintf(stderr, "Unexpected file header: %08x",
						*(uint32_t*)(buffer + 0));
				break;
		}
	}
	else
	{
		if (decode_database_stream(buffer))
			result = 0;
	}

exit:
	if (file)
		fclose(file);
	
	if (buffer)
		free(buffer);

	return result;
}

