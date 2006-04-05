/* $Id$ */
#include "pcommon.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* devpath = NULL;

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-d LEVEL] [-p DEVPATH] [-h] SOURCE DESTINATION\n"
			"\n"
			"\t-d LEVEL     Set debug log level\n"
			"\t                 0 - No logging (default)\n"
			"\t                 1 - Errors only\n"
			"\t                 2 - Errors and warnings\n"
			"\t                 3 - Everything\n"
			"\t-h           Show this help message\n"
                        "\t-p DEVPATH   Device path\n"
			"\tSOURCE       The current filename\n"
			"\tDESTINATION  The new filename\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** source, char** dest)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;

	while ((c = getopt(argc, argv, "d:p:h")) != -1)
	{
		switch (c)
		{
			case 'd':
				log_level = atoi(optarg);
				break;
			
                        case 'p':
                                devpath = optarg;
                                break;
			
			case 'h':
			default:
				show_usage(argv[0]);
				return false;
		}
	}

	synce_log_set_level(log_level);

	if ((argc - optind) != 2)
	{
		fprintf(stderr, "%s: You need to specify source and destination file names on command line\n\n", argv[0]);
		show_usage(argv[0]);
		return false;
	}
		
	*source = strdup(argv[optind++]);
	*dest   = strdup(argv[optind++]);

	return true;
}

int main(int argc, char** argv)
{
	int result = 1;
        RapiConnection* connection = NULL;
	char* source = NULL;
	char* dest = NULL;
	HRESULT hr;
	WCHAR* wide_source = NULL;
	WCHAR* wide_dest = NULL;
	
	if (!handle_parameters(argc, argv, &source, &dest))
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

	convert_to_backward_slashes(source);
	wide_source = wstr_from_current(source);
	wide_source = adjust_remote_path(wide_source, true);

	convert_to_backward_slashes(dest);
	wide_dest   = wstr_from_current(dest);
	wide_dest   = adjust_remote_path(wide_dest, true);

	if (!CeMoveFile(wide_source, wide_dest))
	{
		fprintf(stderr, "%s: Cannot move '%s' to '%s': %s\n", 
				argv[0],
				source,
				dest,
				synce_strerror(CeGetLastError()));
		goto exit;
	}

	result = 0;

exit:
	wstr_free_string(wide_source);
	wstr_free_string(wide_dest);

	if (source)
		free(source);

	if (dest)
		free(dest);

	CeRapiUninit();
	return result;
}
