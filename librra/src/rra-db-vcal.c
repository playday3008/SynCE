/* $Id$ */
#include <stdio.h>
#include <rapi.h>
#include "dbstream.h"
#include "appointment.h"
#include <stdlib.h>

int main(int argc, char** argv)
{
	int result = 1;
	FILE* file = NULL;
	uint32_t field_count = 0;
	uint8_t* buffer = NULL;
	long file_size = 0;
	CEPROPVAL* propvals = NULL;
	char* vcard = NULL;

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

	if (!appointment_to_vcal(APPOINTMENT_OID_UNKNOWN, propvals, field_count, &vcard))
	{
		fprintf(stderr, "Failed to create vCard\n");
		goto exit;
	}

	printf("%s", vcard);
	result = 0;

exit:
	if (file)
		fclose(file);
	
	if (buffer)
		free(buffer);	

	if (propvals)
		free(propvals);
	
	if (vcard)
		free(vcard);
	
	return result;
}
