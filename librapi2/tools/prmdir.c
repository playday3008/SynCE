/* $Id$ */
#include "pcommon.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-d] [-h] FILE\n"
			"\n"
			"\t-d LEVEL  Set debug log level\n"
			"\t              0 - No logging (default)\n"
			"\t              1 - Errors only\n"
			"\t              2 - Errors and warnings\n"
			"\t              3 - Everything\n"
			"\t-h        Show this help message\n"
			"\tFILE      The remote directory you want to remove\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** path)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;

	while ((c = getopt(argc, argv, "d:h")) != -1)
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

	if (optind == argc)
	{
		fprintf(stderr, "%s: No directory name specified on command line\n\n", argv[0]);
		show_usage(argv[0]);
		return false;
	}
		
	/* TODO: handle more than one path */
	*path = strdup(argv[optind]);

	return true;
}

int main(int argc, char** argv)
{
	int result = 1;
	char* path = NULL;
	HRESULT hr;
	WCHAR* wide_path = NULL;
	
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

	convert_to_backward_slashes(path);
	wide_path = wstr_from_ascii(path);

	if (!CeRemoveDirectory(wide_path))
	{
		fprintf(stderr, "%s: Failed to remove directory '%s': %s\n", 
				argv[0],
				path,
				synce_strerror(CeGetLastError()));
		goto exit;
	}

	result = 0;

exit:
	if (wide_path)
		wstr_free_string(wide_path);

	if (path)
		free(path);

	CeRapiUninit();
	return result;
}
