/* $Id$ */
#include "../lib/appointment.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char *codepage = NULL;

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-c CODEPAGE] VEVENT_FILE APPOINTMENT_FILE\n"
			"\n"
                        "\t-c CODEPAGE       Codepage to be used for APPOINTMENT_FILE (default CP1252) \n"
			"\tVEVENT_FILE       The source vevent filename\n"
			"\tAPPOINTMENT_FILE  The destination appointment filename\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** source, char** dest)
{
	int c;
	int path_count;

	while ((c = getopt(argc, argv, "c:")) != -1)
	{
		switch (c)
		{
			case 'c':
				codepage = optarg;
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
	char* vevent = NULL;
	long file_size = 0;
	uint8_t* buffer = NULL;
	size_t buffer_size = 0;
        char *source = NULL, *dest = NULL;

	if (!handle_parameters(argc, argv, &source, &dest))
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

	vevent = (char*)malloc(file_size + 1);
	fread(vevent, file_size, 1, file);
	vevent[file_size] = '\0';
	fclose(file);
	file = NULL;

	if (!rra_appointment_from_vevent(
			vevent,
			NULL,
			&buffer,
			&buffer_size,
			0,
			NULL,
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

	if (vevent)
		free(vevent);
	
	return result;
}
