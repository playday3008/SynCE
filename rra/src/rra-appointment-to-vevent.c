/* $Id$ */
#include "../lib/appointment.h"
#include "../lib/timezone.h"
#include <rapi.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	int result = 1;
	FILE* file = NULL;
	uint8_t* buffer = NULL;
	long file_size = 0;
	char* vevent = NULL;
  RRA_Timezone tzi;
  RRA_Timezone* p_tzi = NULL;

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

  if (argc >= 3)
  {
    FILE* file = fopen(argv[2], "r");
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
            argv[0], (int) bytes_read, argv[2], strerror(errno));
      }

      fclose(file);
    }
    else
    {
      fprintf(stderr, "%s: Unable to open time zone information file '%s': %s\n", 
          argv[0], argv[2], strerror(errno));
    }
  }

	if (!rra_appointment_to_vevent(
			0,
			buffer,
			file_size,
			&vevent,
			0,
      p_tzi))
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
