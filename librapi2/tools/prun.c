/* $Id$ */
#include "pcommon.h"
#include "rapi.h"
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
			"\t%s [-d LEVEL] [-p DEVPATH] [-h] PROGRAM [--] [PARAMETERS]\n"
			"\n"
			"\t-d LEVEL    Set debug log level\n"
			"\t                0 - No logging (default)\n"
			"\t                1 - Errors only\n"
			"\t                2 - Errors and warnings\n"
			"\t                3 - Everything\n"
			"\t-h          Show this help message\n"
                        "\t-p DEVPATH  Device path\n"
			"\tPROGRAM     The program you want to run\n"
      "\t--          Needed if PARAMETERS begin with a dash ('-')\n"
			"\tPARAMETERS  Parameters to the program\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** program, char** parameters)
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

	if (optind == argc)
	{
		fprintf(stderr, "%s: No program specified on command line\n\n", argv[0]);
		show_usage(argv[0]);
		return false;
	}
		
	*program = strdup(argv[optind++]);

	if (optind < argc)
		*parameters = strdup(argv[optind]);

	return true;
}

int main(int argc, char** argv)
{
	int result = 1;
        RapiConnection* connection = NULL;
	char* program = NULL;
	char* parameters = NULL;
	HRESULT hr;
	WCHAR* wide_program = NULL;
	WCHAR* wide_parameters = NULL;
	PROCESS_INFORMATION info;

	if (!handle_parameters(argc, argv, &program, &parameters))
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

	convert_to_backward_slashes(program);
	wide_program = wstr_from_current(program);
	if (parameters)
		wide_parameters = wstr_from_current(parameters);

	memset(&info, 0, sizeof(info));
	
	if (!CeCreateProcess(
				wide_program,
				wide_parameters,
				NULL,
				NULL,
				false,
				0,
				NULL,
				NULL,
				NULL,
				&info
				))
	{
		fprintf(stderr, "%s: Failed to execute '%s': %s\n", 
				argv[0],
				program,
				synce_strerror(CeGetLastError()));
		goto exit;
	}

	CeCloseHandle(info.hProcess);
	CeCloseHandle(info.hThread);

	result = 0;

exit:
	wstr_free_string(wide_program);
	wstr_free_string(wide_parameters);

	if (program)
		free(program);

	if (parameters)
		free(parameters);

	CeRapiUninit();
	return result;
}
