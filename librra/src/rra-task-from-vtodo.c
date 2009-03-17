/* $Id$ */
#include "../lib/task.h"
#include "../lib/timezone.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *codepage = NULL;

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-c CODEPAGE] [-t TZFILE] VTODO_FILE TASK_FILE\n"
			"\n"
                        "\t-c CODEPAGE       Codepage to be used for TASK_FILE (default CP1252)\n"
                        "\t-t TZFILE         Timezone filename\n"
			"\tVTODO_FILE        The source vtodo filename\n"
			"\tTASK_FILE         The destination task filename\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** source, char** dest, char** tzfile)
{
	int c;
	int path_count;

	while ((c = getopt(argc, argv, "c:t:")) != -1)
	{
		switch (c)
		{
			case 'c':
				codepage = optarg;
				break;

			case 't':
				*tzfile = optarg;
				break;

			case 'h':
			default:
				show_usage(argv[0]);
				return false;
		}
	}
				
	path_count = argc - optind;
	if (path_count != 2) {
		fprintf(stderr, "%s: You need to specify source and destination file names on command line\n\n", argv[0]);
		show_usage(argv[0]);
		return false;
	}
		
	*source = strdup(argv[optind++]);
       	*dest = strdup(argv[optind++]);

	return true;
}


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
        char *source = NULL, *dest = NULL, *tzfile = NULL;

	if (!handle_parameters(argc, argv, &source, &dest, &tzfile))
		goto exit;

	if (!codepage)
		codepage = "CP1252";

	file = fopen(source, "r");
	if (!file)
	{
		fprintf(stderr, "Unable to open file '%s'\n", source);
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

  if (tzfile)
  {
    FILE* file = fopen(tzfile, "r");
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
            argv[0], (int) bytes_read, tzfile, strerror(errno));
      }

      fclose(file);
    }
    else
    {
      fprintf(stderr, "%s: Unable to open time zone information file '%s': %s\n", 
          argv[0], tzfile, strerror(errno));
    }
  }

	if (!rra_task_from_vtodo(
			vtodo,
			NULL,
			&buffer,
			&buffer_size,
			0,
			p_tzi,
			codepage))
	{
		fprintf(stderr, "Failed to create data\n");
		goto exit;
	}
	
	file = fopen(dest, "w");
	if (!file)
	{
		fprintf(stderr, "Unable to open file '%s'\n", dest);
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
