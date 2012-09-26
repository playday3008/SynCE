/* $Id: rra-file-unpack.c 3750 2009-04-14 12:43:36Z mark_ellis $ */
#include "file.h"
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
			"\t%s [-d LEVEL] INPUT_FILE [OUTPUT_FILE]\n"
			"\n"
			"\t-d LEVEL          Set debug log level\n"
			"\t                  0 - No logging\n"
			"\t                  1 - Errors only (default)\n"
			"\t                  2 - Errors and warnings\n"
			"\t                  3 - Errors, warnings and info\n"
			"\t                  4 - Everything\n"
			"\tINPUT_FILE        The source data filename\n"
			"\tOUTPUT_FILE       The destination filename\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** source, char** dest)
{
	int c;
	int path_count;
	int log_level = SYNCE_LOG_LEVEL_ERROR;

	while ((c = getopt(argc, argv, "d:")) != -1)
	{
		switch (c)
		{
			case 'd':
				log_level = atoi(optarg);
				break;
			
			case 'h':
			default:
				show_usage(argv[0]);
				return false;
		}
	}
				
	synce_log_set_level(log_level);

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
	size_t buf_size = 0;
	DWORD ftype;
	char *filepath = NULL;
	uint8_t* file_data = NULL;
	size_t file_size = 0;
	char *source = NULL, *dest = NULL;

	if (!handle_parameters(argc, argv, &source, &dest))
		goto exit;

	file = fopen(source, "r");
	if (!file)
	{
		fprintf(stderr, "Unable to open file '%s'\n", source);
		goto exit;
	}

	/* find out file size */
	fseek(file, 0, SEEK_END);
	buf_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	buffer = (uint8_t*)malloc(buf_size);
	if (fread(buffer, buf_size, 1, file) != 1)
        {
		fprintf(stderr, "Unable to read data from file '%s'\n", source);
		goto exit;
	}

	if (!rra_file_unpack(
			buffer,
			buf_size,
			&ftype,
			&filepath,
			&file_data,
			&file_size))
	{
		fprintf(stderr, "Failed to unpack file\n");
		goto exit;
	}

	printf("File type %d - ", ftype);
	if (ftype & RRA_FILE_TYPE_DIRECTORY)
		printf("directory\n");
	else
		printf("file\n");

	printf("File path %s\n", filepath);

	if (!(ftype & RRA_FILE_TYPE_DIRECTORY)) {
		if (dest) {
			file = fopen(dest, "w");
			if (!file)
			{
				fprintf(stderr, "Unable to open file '%s'\n", dest);
				goto exit;
			}
			if (fwrite(file_data, file_size, 1, file) != 1) {
				fprintf(stderr, "Unable to write data to file '%s'\n", dest);
				goto exit;
			}
		}
	}
	result = 0;

exit:
	if (file)
		fclose(file);
	
	if (buffer)
		free(buffer);	

	if (filepath)
		free(filepath);
	
	if (file_data)
		free(file_data);
	
	return result;
}
