#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

char* devpath = NULL;

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-d LEVEL] [-p DEVPATH] [-h]\n"
			"\n"
			"\t-d LEVEL     Set debug log level\n"
			"\t                 0 - No logging (default)\n"
			"\t                 1 - Errors only\n"
			"\t                 2 - Errors and warnings\n"
			"\t                 3 - Everything\n"
			"\t-h           Show this help message\n"
                        "\t-p DEVPATH   Device path\n",
			name);
}

static bool handle_parameters(int argc, char** argv)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;

	while ((c = getopt(argc, argv, "d:hp:")) != -1)
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

	return true;
}

int main(int argc, char** argv)
{
  int result = 0;
  RapiConnection* connection = NULL;
  HRESULT hr;

  if (!handle_parameters(argc, argv))
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
    result = 1;
    goto exit;
  }

  if (!CeKillAllApps())
  {
    fprintf(stderr,"%s: CeKillAllApps failed\n",argv[0]);
    result = 2;
    goto exit;
  }

 exit:
  CeRapiUninit();
  rapi_connection_destroy(connection);

  return result;
}
