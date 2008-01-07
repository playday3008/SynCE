/* $Id$ */
#include "../lib/task.h"
#include "../lib/timezone.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	int result = 1;
	FILE* file = NULL;
	uint8_t* buffer = NULL;
	long file_size = 0;
	char* vtodo = NULL;

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

	if (!rra_task_to_vtodo(
			0,
			buffer,
			file_size,
			&vtodo,
			0,
      NULL))
	{
		fprintf(stderr, "Failed to create vEvent\n");
		goto exit;
	}
	
	printf("%s", vtodo);
	result = 0;

exit:
	if (file)
		fclose(file);
	
	if (buffer)
		free(buffer);	

	if (vtodo)
		free(vtodo);
	
	return result;
}
