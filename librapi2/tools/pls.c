/* $Id$ */
#include "pcommon.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* devpath = NULL;

static bool numeric_file_attributes = false;
static bool show_hidden_files = false;

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-a] [-d LEVEL] [-p DEVPATH] [-h] [-n] [DIRECTORY]\n"
			"\n"
			"\t-a        Show all files including those marked as hidden\n"
			"\t-d LEVEL  Set debug log level\n"
			"\t              0 - No logging (default)\n"
			"\t              1 - Errors only\n"
			"\t              2 - Errors and warnings\n"
			"\t              3 - Everything\n"
			"\t-h         Show this help message\n"
                        "\t-p DEVPATH Device path\n"
			"\t-n         Show numeric value for file attributes\n"
			"\tDIRECTORY  The remote directory where you want to list files\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** path)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;

	while ((c = getopt(argc, argv, "ad:p:hn")) != -1)
	{
		switch (c)
		{
			case 'a':
				show_hidden_files = true;
				break;
					
                        case 'p':
                                devpath = optarg;
                                break;
			
              		case 'd':
				log_level = atoi(optarg);
				break;

			case 'n':
				numeric_file_attributes = true;
				break;
			
			case 'h':
			default:
				show_usage(argv[0]);
				return false;
		}
	}

	synce_log_set_level(log_level);

	/* TODO: handle more than one path */
	if (optind < argc)
		*path = strdup(argv[optind++]);

	return true;
}

static void print_attribute(CE_FIND_DATA* entry, DWORD attribute, int c)
{
	if (entry->dwFileAttributes & attribute)
		putchar(c);
	else
		putchar('-');
}

static bool print_entry(CE_FIND_DATA* entry)
{
	time_t seconds;
	char time_string[50] = {0};
	struct tm* time_struct = NULL;
	char* filename = NULL;
	
	/*
	 * Print file attributes
	 */
	if (numeric_file_attributes)
		printf("%08x  ", entry->dwFileAttributes);
	else
		switch (entry->dwFileAttributes)
		{
			case FILE_ATTRIBUTE_ARCHIVE:
				printf("Archive   ");
				break;

			case FILE_ATTRIBUTE_NORMAL:
				printf("Normal    ");
				break;

			case FILE_ATTRIBUTE_DIRECTORY:
				printf("Directory ");
				break;

			default:
				print_attribute(entry, FILE_ATTRIBUTE_ARCHIVE,       'A');
				print_attribute(entry, FILE_ATTRIBUTE_COMPRESSED,    'C');
				print_attribute(entry, FILE_ATTRIBUTE_DIRECTORY,     'D');
				print_attribute(entry, FILE_ATTRIBUTE_HIDDEN,        'H');
				print_attribute(entry, FILE_ATTRIBUTE_INROM,         'I');
				print_attribute(entry, FILE_ATTRIBUTE_ROMMODULE,     'M');
				print_attribute(entry, FILE_ATTRIBUTE_NORMAL,        'N');
				print_attribute(entry, FILE_ATTRIBUTE_READONLY,      'R');
				print_attribute(entry, FILE_ATTRIBUTE_SYSTEM,        'S');
				print_attribute(entry, FILE_ATTRIBUTE_TEMPORARY,     'T');
				break;
		}

	printf("  ");

	/*
	 * Size 
	 *
	 * XXX: cheating by ignoring nFileSizeHigh
	 */

	if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		printf("          ");
	else
		printf("%10u", entry->nFileSizeLow);

	printf("  ");

	/*
	 * Modification time
	 */

	seconds = filetime_to_unix_time(&entry->ftLastWriteTime);
	time_struct = localtime(&seconds);
	strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_struct);
	printf("%s", time_string);
	
	printf("  ");

	/*
	 * OID
	 */

//	printf("%08x", entry->dwOID);
	
//	printf("  ");

	/*
	 * Filename
	 */

	filename = wstr_to_current(entry->cFileName);
        printf(filename);
	wstr_free_string(filename);
	if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		printf("/");

	printf("\n");
	return true;
}

static bool list_matching_files(WCHAR* wide_path)
{
	bool success = false;
	BOOL result;
	CE_FIND_DATA* find_data = NULL;
	DWORD file_count = 0;
	unsigned i;

	synce_trace_wstr(wide_path);
	wide_path = adjust_remote_path(wide_path, true);
	synce_trace_wstr(wide_path);

	result = CeFindAllFiles(
			wide_path,
			(show_hidden_files ? 0 : FAF_ATTRIB_NO_HIDDEN) |
		 	FAF_ATTRIBUTES|FAF_LASTWRITE_TIME|FAF_NAME|FAF_SIZE_LOW|FAF_OID,
      &file_count, &find_data);

	if (!result)
		goto exit;
	
	for (i = 0; i < file_count; i++)
		print_entry(find_data + i);
	
	success = true;

exit:
	CeRapiFreeBuffer(find_data);

	return success;
}

static const char *wildcards = "*.*";
static       WCHAR empty[]   = {'\0'};

bool list_directory(WCHAR* directory)
{
	WCHAR path[MAX_PATH];
  WCHAR* tmp = NULL;

	if (!directory)
	{
		synce_error("Directory is NULL. How did that happen?");
		return false;
	}

	synce_trace_wstr(directory);
	wstrcpy(path, directory);
	synce_trace_wstr(path);
	wstr_append(path, (tmp = wstr_from_current(wildcards)), sizeof(path));
  wstr_free_string(tmp);
	return list_matching_files(path);
}

int main(int argc, char** argv)
{
	int result = 1;
        RapiConnection* connection = NULL;
	char* path = NULL;
	WCHAR* wide_path = NULL;
	HRESULT hr;

	if (!handle_parameters(argc, argv, &path))
		goto exit;

        if ((connection = rapi_connection_from_path(devpath)) == NULL)
        {
          fprintf(stderr, "%s: Could not find configuration at path '%s'\n", 
                  argv[0],
                  devpath?devpath:"(Default)");
          goto exit;
        }
        rapi_connection_select(connection);

	hr = CeRapiInit();

	if (FAILED(hr))
	{
		fprintf(stderr, "%s: Unable to initialize RAPI: %s\n", 
				argv[0],
				synce_strerror(hr));
		goto exit;
	}

	if (path)
		convert_to_backward_slashes(path);

	if (!path)
	{
		wide_path = adjust_remote_path(empty, false);
		list_directory(wide_path);
	}
	else if (path[strlen(path)-1] == '\\')
	{
		/* This is a directory, append "*" to show its contents */
		char new_path[MAX_PATH];
		snprintf(new_path, sizeof(new_path), "%s*", path);
		wide_path = wstr_from_current(new_path);

		if (!list_matching_files(wide_path))
			goto exit;
	}
	else
	{

		wide_path = wstr_from_current(path);
		wide_path = adjust_remote_path(wide_path, true);

		if (!list_matching_files(wide_path))
			goto exit;


	}

	result = 0;

exit:
	if (path)
		free(path);

	wstr_free_string(wide_path);

	CeRapiUninit();
	return result;
}
