/* $Id$ */
#include "librra.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	int result = 1;
	FILE* file = NULL;
	uint8_t* buffer = NULL;
	long file_size = 0;
	char* vevent = NULL;

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

	if (!rra_appointment_to_vevent(
			0,
			buffer,
			file_size,
			&vevent,
			0))
	{
		fprintf(stderr, "Failed to create vEvent\n");
		goto exit;
	}
	
	printf("%s", vevent);
	result = 0;

exit:
	if (file)
		fclose(file);
	
	if (buffer)
		free(buffer);	

	if (vevent)
		free(vevent);
	
	return result;
}
