/* $Id$ */
#include "pcommon.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* dev_name = NULL;

static bool numeric_file_attributes = false;
static bool show_hidden_files = false;
static bool recursive = false;
static const char *wildcards = "*.*";

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-a] [-R] [-d LEVEL] [-p DEVNAME] [-h] [-n] [DIRECTORY]\n"
			"\n"
			"\t-a        Show all files including those marked as hidden\n"
			"\t-R        Recursively display directories\n"
			"\t-d LEVEL  Set debug log level\n"
			"\t              0 - No logging (default)\n"
			"\t              1 - Errors only\n"
			"\t              2 - Errors and warnings\n"
			"\t              3 - Everything\n"
			"\t-h         Show this help message\n"
                        "\t-p DEVNAME Mobile device name\n"
			"\t-n         Show numeric value for file attributes\n"
			"\tPATH       The remote path you want to list\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** path)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;

	while ((c = getopt(argc, argv, "aRd:p:hn")) != -1)
	{
		switch (c)
		{
			case 'a':
				show_hidden_files = true;
				break;
					
			case 'R':
				recursive = true;
				break;
					
                        case 'p':
                                dev_name = optarg;
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
        printf("%s", filename);
	wstr_free_string(filename);
	if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		printf("/");

	printf("\n");
	return true;
}

char *
absolutize_path(const char *path)
{
	WCHAR path_w[MAX_PATH];
	char *tmp_path1 = NULL;
	char *tmp_path2 = NULL;

	if ('\\' == path[0])
		return strdup(path);

	/* if not an absolute path, append to "My Documents" */

	if (!CeGetSpecialFolderPath(CSIDL_PERSONAL, (MAX_PATH * sizeof(WCHAR)), path_w))
	{
		fprintf(stderr, "Unable to get the \"My Documents\" path.\n");
		return NULL;
	}

	tmp_path1 = wstr_to_current(path_w);
	tmp_path2 = malloc(strlen(tmp_path1) + strlen(path) + 2);
	snprintf(tmp_path2, strlen(tmp_path1) + strlen(path) + 2, "%s\\%s", tmp_path1, path);
	free(tmp_path1);
	return tmp_path2;
}

char *
dirname(const char *path)
{
	int dir_end = 0, i;
	bool wildcard = FALSE;
	char *tmp_path = NULL;

	/* if no wildcard, return whole path
	   if wildcard, return path up to last backslash, or empty string if none
	*/

	i = strlen(path) - 1;

	while (i >= 0)
	{
		if ( (path[i] == '*') || (path[i] == '?') ) {
			wildcard = true;
			dir_end = 0;
		}

		if ( (path[i] == '\\') && (dir_end == 0) ) {
			dir_end = i;
		}

		i--;
	}

	if (!wildcard)
		return strdup(path);

	if (dir_end == 0)
		return strdup("");

	tmp_path = malloc(dir_end + 1);
	snprintf(tmp_path, dir_end +1, "%s", path);
	return tmp_path;
}

static bool list_matching_files(const char* path, bool first_pass)
{
	bool success = false;
	BOOL result;
	CE_FIND_DATA* find_data = NULL;
	DWORD file_count = 0;
	unsigned i;
	HRESULT hr;
	DWORD last_error;
	char *full_path = NULL;
	char *base_path = NULL;
	char *new_path = NULL;
	WCHAR *wide_path = NULL;
	char *entry_name = NULL;

	if (!(full_path = absolutize_path(path)))
	  return FALSE;

	wide_path = wstr_from_current(full_path);
	free(full_path);
	synce_trace_wstr(wide_path);

	result = CeFindAllFiles(
			wide_path,
			(show_hidden_files ? 0 : FAF_ATTRIB_NO_HIDDEN) |
		 	FAF_ATTRIBUTES|FAF_LASTWRITE_TIME|FAF_NAME|FAF_SIZE_LOW|FAF_OID,
			&file_count, &find_data);
	wstr_free_string(wide_path);

	if (!result) {
		if (FAILED(hr = CeRapiGetError())) {
		  fprintf(stderr, "Error finding files: %08x: %s.\n",
			  hr, synce_strerror(hr));
		  return false;
		}

		last_error = CeGetLastError();
		fprintf(stderr, "Error finding files: %d: %s.\n",
			last_error, synce_strerror(last_error));
		return false;
	}

	if ((file_count == 1) && (find_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && first_pass) {

		base_path = dirname(path);

		entry_name = wstr_to_current(find_data->cFileName);

		new_path = malloc(strlen(base_path) + strlen(entry_name) + 6);
		snprintf(new_path, strlen(base_path) + strlen(entry_name) + 6, "%s\\%s\\%s", base_path, entry_name, wildcards);
		free(base_path);
		free(entry_name);

		success = list_matching_files(new_path, FALSE);

		free(new_path);
		return success;
	}
	
	if ((file_count == 0) && first_pass) {
		fprintf(stderr, "No such file or directory\n");
		goto exit;
	}

	for (i = 0; i < file_count; i++)
		print_entry(find_data + i);

	if (recursive)
		printf("Total %d\n", file_count);

	if (recursive) {
		for (i = 0; i < file_count; i++) {

			if ((find_data + i)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				entry_name = wstr_to_current((find_data + i)->cFileName);
				printf("\n%s:\n", entry_name);

				base_path = dirname(path);

				new_path = malloc(strlen(base_path) + strlen(entry_name) + 6);
				snprintf(new_path, strlen(base_path) + strlen(entry_name) + 6, "%s\\%s\\%s", base_path, entry_name, wildcards);
				free(base_path);
				free(entry_name);

				list_matching_files(new_path, FALSE);
				free(new_path);
			}
		}
	}
	
	success = true;

exit:
	CeRapiFreeBuffer(find_data);

	return success;
}


int main(int argc, char** argv)
{
	int result = 1;
        RapiConnection* connection = NULL;
	char* path = NULL;
	char* tmp_path = NULL;
	HRESULT hr;

	if (!handle_parameters(argc, argv, &path))
		goto exit;

        if ((connection = rapi_connection_from_name(dev_name)) == NULL)
        {
          fprintf(stderr, "%s: Could not find configuration at path '%s'\n", 
                  argv[0],
                  dev_name?dev_name:"(Default)");
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

	if (!path)
		path = strdup("");

	convert_to_backward_slashes(path);

	if ((strlen(path) > 1) && (path[strlen(path) - 1] == '\\')) {
		tmp_path = malloc(strlen(path));
		snprintf(tmp_path, strlen(path), "%s", path);
		free(path);
		path = tmp_path;
	}

	if (!list_matching_files(path, TRUE))
		goto exit;

	result = 0;

exit:
	if (path)
		free(path);

	CeRapiUninit();
	return result;
}

