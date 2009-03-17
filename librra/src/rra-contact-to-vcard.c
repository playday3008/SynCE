/* $Id$ */
#include "contact.h"
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
			"\t%s [-c CODEPAGE] CONTACT_FILE [VCARD_FILE]\n"
			"\n"
                        "\t-c CODEPAGE       Codepage to be used for CONTACT_FILE (default CP1252)\n"
			"\tCONTACT_FILE      The source contact filename\n"
			"\tVCARD_FILE        The destination vcard filename\n",
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
	if (path_count < 1 || path_count > 2) {
		fprintf(stderr, "%s: You need to specify source and optional destination file names on command line\n\n", argv[0]);
		show_usage(argv[0]);
		return false;
	}
		
	*source = strdup(argv[optind++]);
	if (path_count > 1)
	       	*dest = strdup(argv[optind++]);

	return true;
}

int main(int argc, char** argv)
{
	int result = 1;
	FILE* file = NULL;
	uint8_t* buffer = NULL;
	long file_size = 0;
	char* vcard = NULL;
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

	buffer = (uint8_t*)malloc(file_size);
	fread(buffer, file_size, 1, file);

	if (!rra_contact_to_vcard(
			RRA_CONTACT_ID_UNKNOWN,
			buffer,
			file_size,
			&vcard,
			RRA_CONTACT_VERSION_3_0,
			codepage))
	{
		fprintf(stderr, "Failed to create vCard\n");
		goto exit;
	}
	
	if (dest) {
		file = fopen(dest, "w");
		if (!file)
		{
			fprintf(stderr, "Unable to open file '%s'\n", dest);
			goto exit;
		}
		fprintf(file, "%s", vcard);
	} else {
		printf("%s", vcard);
	}

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
