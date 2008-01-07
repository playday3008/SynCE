/* $Id$ */
#include "../lib/contact.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	int result = 1;
	FILE* file = NULL;
	char* vcard = NULL;
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
			RRA_CONTACT_NEW | RRA_CONTACT_ISO8859_1))
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

	if (vcard)
		free(vcard);
	
	return result;
}
