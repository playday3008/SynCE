/* $Id$ */
#include "../lib/task.h"
#include "../lib/timezone.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	int result = 1;
	FILE* file = NULL;
	char* vtodo = NULL;
	long file_size = 0;
	uint8_t* buffer = NULL;
	size_t buffer_size = 0;
  RRA_Timezone tzi;
  RRA_Timezone* p_tzi = NULL;

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

  if (argc >= 4)
  {
    FILE* file = fopen(argv[3], "r");
    if (file)
    {
      size_t bytes_read = fread(&tzi, 1, sizeof(RRA_Timezone), file);
      if (sizeof(RRA_Timezone) == bytes_read)
      {
        p_tzi = &tzi;
      }
      else
      {
        fprintf(stderr, "%s: Only read %i bytes from time zone information file '%s': %s\n", 
            argv[0], (int) bytes_read, argv[3], strerror(errno));
      }

      fclose(file);
    }
    else
    {
      fprintf(stderr, "%s: Unable to open time zone information file '%s': %s\n", 
          argv[0], argv[3], strerror(errno));
    }
  }

	if (!rra_task_from_vtodo(
			vtodo,
			NULL,
			&buffer,
			&buffer_size,
			0,
      p_tzi))
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
