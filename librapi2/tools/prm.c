/* $Id$ */
#include "pcommon.h"
#include <rapi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-h] FILE\n"
			"\n"
			"\t-h    Show this help message\n"
			"\tFILE  The remote file you want to remove\n",
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

	if (optind == argc)
	{
		fprintf(stderr, "%s: No file specified on command line\n\n", argv[0]);
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
	BOOL success;
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

	if (!CeDeleteFile(wide_path))
	{
		fprintf(stderr, "%s: Failed to remove '%s': %s\n", 
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
