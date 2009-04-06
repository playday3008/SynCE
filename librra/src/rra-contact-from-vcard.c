/* $Id$ */
#include "../lib/contact.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <synce_log.h>

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-d LEVEL] [-c CODEPAGE] VCARD_FILE CONTACT_FILE\n"
			"\n"
			"\t-d LEVEL          Set debug log level\n"
			"\t                  0 - No logging\n"
			"\t                  1 - Errors only (default)\n"
			"\t                  2 - Errors and warnings\n"
			"\t                  3 - Errors, warnings and info\n"
			"\t                  4 - Everything\n"
                        "\t-c CODEPAGE       Codepage to be used for CONTACT_FILE (default CP1252) \n"
                        "\t-u                Input vCard is UTF8\n"
			"\tVCARD_FILE        The source vcard filename\n"
			"\tCONTACT_FILE      The destination contact filename\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** source, char** dest, char** codepage, uint32_t *flags)
{
	int c;
	int path_count;
	int log_level = SYNCE_LOG_LEVEL_ERROR;

	while ((c = getopt(argc, argv, "d:c:u")) != -1)
	{
		switch (c)
		{
			case 'd':
				log_level = atoi(optarg);
				break;
			
			case 'c':
				*codepage = optarg;
				break;
			
			case 'u':
				*flags = *flags | RRA_CONTACT_UTF8;
				break;
			
			case 'h':
			default:
				show_usage(argv[0]);
				return false;
		}
	}
				
	synce_log_set_level(log_level);

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
	char* vcard = NULL;
	long file_size = 0;
	uint8_t* buffer = NULL;
	size_t buffer_size = 0;
	char *source = NULL, *dest = NULL;
	char *codepage = NULL;
	uint32_t flags = 0;

	if (!handle_parameters(argc, argv, &source, &dest, &codepage, &flags))
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

	vcard = (char*)malloc(file_size + 1);
	fread(vcard, file_size, 1, file);
	vcard[file_size] = '\0';
	fclose(file);
	file = NULL;

	if (!rra_contact_from_vcard(
			vcard,
			NULL,
			&buffer,
			&buffer_size,
			RRA_CONTACT_NEW | flags,
			codepage))
	{
		fprintf(stderr, "Failed to create data\n");
		goto exit;
	}
	
	file = fopen(dest, "w");
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

	if (vcard)
		free(vcard);
	
	return result;
}
