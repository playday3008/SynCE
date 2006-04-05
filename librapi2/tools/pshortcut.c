#include "pcommon.h"
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
			"\t%s [-d LEVEL] [ -p DEVPATH ] [-h] SHORTCUT TARGET\n"
			"\n"
			"\t-d LEVEL     Set debug log level\n"
			"\t                 0 - No logging (default)\n"
			"\t                 1 - Errors only\n"
			"\t                 2 - Errors and warnings\n"
			"\t                 3 - Everything\n"
			"\t-h           Show this help message\n"
                        "\t-p DEVPATH   Device path\n"
                        "\tSHORTCUT     Shortcut to create\n"
                        "\tTARGET       Shortcut target\n"
			,name);
}

static bool handle_parameters(int argc, char** argv, char **shortcut, char **target)
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

	if ((argc - optind) != 2)
	{
		fprintf(stderr, "%s: You need to specify shortcut and target file names on command line\n\n", argv[0]);
		show_usage(argv[0]);
		return false;
	}
		
	*shortcut = strdup(argv[optind++]);
	*target   = strdup(argv[optind++]);

	return true;
}

int main(int argc, char** argv)
{
  int result = 0;
  RapiConnection* connection = NULL;
  HRESULT hr;
  char *shortcut, *target;
  WCHAR* wide_shortcut = NULL;
  WCHAR* wide_target = NULL;
  WCHAR *tmp, *tmp_quote;
  int tmpsize;

  if (!handle_parameters(argc, argv, &shortcut, &target))
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


  convert_to_backward_slashes(shortcut);
  wide_shortcut = wstr_from_current(shortcut);
  wide_shortcut = adjust_remote_path(wide_shortcut, true);

  convert_to_backward_slashes(target);
  wide_target = wstr_from_current(target);
  wide_target = adjust_remote_path(wide_target, true);
  /* Wrap target in quotes.  This is required for paths with spaces (for some reason) */
  tmp_quote = wstr_from_current("\"");
  tmpsize = (wstrlen(wide_target) + 3) * sizeof(WCHAR);
  tmp = malloc(tmpsize);
  if (!tmp)
    goto exit;
  wstrcpy(tmp,tmp_quote);
  if (!wstr_append(tmp,wide_target,tmpsize))
    goto exit;
  if (!wstr_append(tmp,tmp_quote,tmpsize))
    goto exit;
  wstr_free_string(wide_target);
  wstr_free_string(tmp_quote);
  wide_target = tmp;
  
  BOOL res = CeSHCreateShortcut(wide_shortcut, wide_target);
  if (!res)
  {
    fprintf(stderr, "%s: Unable to create shortcut to '%s' at '%s': %s\n",
            argv[0],target,shortcut,
        synce_strerror(hr));
    result = 1;
    goto exit;
  }

 exit:
  wstr_free_string(wide_shortcut);
  wstr_free_string(wide_target);
  
  if (shortcut)
    free(shortcut);
  
  if (target)
    free(target);

  CeRapiUninit();
  rapi_connection_destroy(connection);
  
  return result;
}
