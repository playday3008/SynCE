/* $Id$ */
#include <rapi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-h] [DIRECTORY]\n"
			"\n"
			"\t-h         Show this help message\n"
			"\tDIRECTORY  The remote directory where you want to list files\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** path)
{
	int c;

	while ((c = getopt(argc, argv, "h")) != -1)
	{
		switch (c)
		{
			case 'h':
			default:
				show_usage(argv[0]);
				return false;
		}
	}

	/* TODO: handle more than one path */
	if (optind < argc)
		*path = strdup(argv[optind]);

	return true;
}

static void convert_to_backward_slashes(char* path)
{
	while (*path)
	{
		if ('/' == *path)
			*path = '\\';

		path++;
	}
}

static bool print_entry(CE_FIND_DATA* entry)
{
	time_t seconds;
	char time_string[20];
	struct tm* time_struct;
	char* filename = NULL;
	
	/*
	 * Print file attributes
	 */
	switch (entry->dwFileAttributes)
	{
		case FILE_ATTRIBUTE_ARCHIVE:
			printf("Archive  ");
			break;

		case FILE_ATTRIBUTE_NORMAL:
			printf("Normal   ");
			break;

		case FILE_ATTRIBUTE_DIRECTORY:
			printf("Directory");
			break;

		default:
			printf("%08x ", entry->dwFileAttributes);
			break;
	}

	printf("  ");

	/*
	 * Size 
	 *
	 * XXX: cheating by ignoring nFileSizeHigh
	 */

	if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		printf("      -");
	else
		printf("%7u", entry->nFileSizeLow);

	printf("  ");

	/*
	 * Modification time
	 */

	seconds = filetime_to_unix_time(&entry->ftLastWriteTime);
	time_struct = localtime(&seconds);
	strftime(time_string, sizeof(time_string), "%c", time_struct);
	printf("%s", time_string);
	
	printf("  ");

	/*
	 * Filename
	 */

	filename = wstr_to_ascii(entry->cFileName);
	printf("%s", filename);
	wstr_free_string(filename);

	printf("\n");

	return true;
}

static bool list_directory(const char* name)
{
	bool success = false;
	BOOL result;
	CE_FIND_DATA* find_data = NULL;
	DWORD file_count = 0;
	char ascii_path[256];
	WCHAR* wide_path = NULL;
	int i;

	snprintf(ascii_path, sizeof(ascii_path), "%s\\*.*", name);
	convert_to_backward_slashes(ascii_path);
	
	wide_path = wstr_from_ascii(ascii_path);

	result = CeFindAllFiles(
			wide_path,
			FAF_ATTRIBUTES|FAF_LASTWRITE_TIME|FAF_NAME|FAF_SIZE_LOW,
      &file_count, &find_data);

	if (!result)
		goto exit;
	
	for (i = 0; i < file_count; i++)
		print_entry(find_data + i);
	
	success = true;

exit:
	CeRapiFreeBuffer(find_data);

	if (wide_path)
		wstr_free_string(wide_path);
	return success;
}

int main(int argc, char** argv)
{
	int result = 1;
	char* path = NULL;
	HRESULT hr;
	
	if (!handle_parameters(argc, argv, &path))
		goto exit;

	hr = CeRapiInit();

	if (FAILED(hr))
	{
		fprintf(stderr, "%s: Unable to initialize RAPI: %s\n", 
				argv[0],
				synce_strerror(hr));
		goto exit;
	}


	/* TODO: handle files too */
	if (!list_directory(path))
		goto exit;

	result = 0;

exit:
	if (path)
		free(path);

	CeRapiUninit();
	return result;
}
