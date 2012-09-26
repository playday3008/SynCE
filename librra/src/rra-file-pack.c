/* $Id: rra-file-pack.c 3750 2009-04-14 12:43:36Z mark_ellis $ */
#include "../lib/file.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <synce_log.h>

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-d LEVEL] INPUT_FILE OUTPUT_FILE\n"
			"\n"
			"\t-d LEVEL          Set debug log level\n"
			"\t                  0 - No logging\n"
			"\t                  1 - Errors only (default)\n"
			"\t                  2 - Errors and warnings\n"
			"\t                  3 - Errors, warnings and info\n"
			"\t                  4 - Everything\n"
			"\t-p PATH           Set the pathname to use\n"
			"\tINPUT_FILE        The source filename\n"
			"\tOUTPUT_FILE       The destination filename\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** path, char** source, char** dest)
{
	int c;
	int path_count;
	int log_level = SYNCE_LOG_LEVEL_ERROR;

	while ((c = getopt(argc, argv, "d:p:")) != -1)
	{
		switch (c)
		{
			case 'd':
				log_level = atoi(optarg);
				break;
			
			case 'p':
				*path = optarg;
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
	uint8_t* indata = NULL;
	long file_size = 0;
	uint8_t* buffer = NULL;
	size_t buffer_size = 0;
	char *source = NULL, *dest = NULL;
	DWORD ftype;
	char *filepath = NULL;
	struct stat statinfo;

	if (!handle_parameters(argc, argv, &filepath, &source, &dest))
		goto exit;

	if (stat(source, &statinfo) == -1) {
		fprintf(stderr, "Unable to stat file '%s': %sstrerror(errno)\n", source, strerror(errno));
		goto exit;
	}
	if (S_ISDIR(statinfo.st_mode)) {
		ftype = RRA_FILE_TYPE_DIRECTORY;
	} else {
		ftype = RRA_FILE_TYPE_FILE;
		file = fopen(source, "r");
		if (!file) {
			fprintf(stderr, "Unable to open file '%s'\n", source);
			goto exit;
		}

		/* find out file size */
		fseek(file, 0, SEEK_END);
		file_size = ftell(file);
		fseek(file, 0, SEEK_SET);

		indata = (uint8_t*)malloc(file_size);
		if (fread(indata, file_size, 1, file) != 1) {
			fprintf(stderr, "Unable to read data from file '%s'\n", source);
			goto exit;
		}

		fclose(file);
		file = NULL;
	}

	if (!filepath)
		filepath = source;

	if (!rra_file_pack(
			ftype,
			filepath,
			indata,
			file_size,
			&buffer,
			&buffer_size))
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

	if (fwrite(buffer, buffer_size, 1, file) != 1)
	{
		fprintf(stderr, "Unable to write data to file '%s'\n", dest);
		goto exit;
	}

	result = 0;

exit:
	if (file)
		fclose(file);
	
	if (buffer)
		free(buffer);	

	if (indata)
		free(indata);
	
	return result;
}
