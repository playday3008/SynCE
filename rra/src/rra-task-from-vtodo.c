/* $Id$ */
#include "librra.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	int result = 1;
	FILE* file = NULL;
	char* vtodo = NULL;
	long file_size = 0;
	uint8_t* buffer = NULL;
	size_t buffer_size = 0;

	if (argc < 3)
	{
		fprintf(stderr, "Filenames missing on command line\n");
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

	vtodo = (char*)malloc(file_size + 1);
	fread(vtodo, file_size, 1, file);
	vtodo[file_size] = '\0';
	fclose(file);
	file = NULL;

	if (!rra_task_from_vtodo(
			vtodo,
			NULL,
			&buffer,
			&buffer_size,
			0))
	{
		fprintf(stderr, "Failed to create data\n");
		goto exit;
	}
	
	file = fopen(argv[2], "w");
	if (!file)
	{
		fprintf(stderr, "Unable to open file '%s'\n", argv[1]);
		goto exit;
	}

	fwrite(buffer, buffer_size, 1, file);

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
