/* $Id$ */
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* dev_name = NULL;

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-d LEVEL] [-p DEVNAME] [-h]\n"
			"\n"
			"\t-d LEVEL     Set debug log level\n"
			"\t                 0 - No logging (default)\n"
			"\t                 1 - Errors only\n"
			"\t                 2 - Errors and warnings\n"
			"\t                 3 - Everything\n"
                        "\t-p DEVNAME   Mobile device name\n"
			"\t-h           Show this help message\n",
			name);
}

static bool handle_parameters(int argc, char** argv)
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
                                dev_name = optarg;
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
	int return_value = 1;
        RapiConnection* connection = NULL;
	HRESULT hr;
	LONG result;
	HKEY parent_key;
	WCHAR* parent_key_name = NULL;
	WCHAR* value_name = NULL;
	DWORD i;
  bool smartphone = false;
	
	if (!handle_parameters(argc, argv))
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
	
  
  /* Path on SmartPhone 2002 */
  parent_key_name = wstr_from_current("Security\\AppInstall");

	result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, parent_key_name, 0, 0, &parent_key);
  
	if (ERROR_SUCCESS == result)
  {
    smartphone = true;
  }
  else
  {
    smartphone = false;
    wstr_free_string(parent_key_name);

    /* Path on Pocket PC 2002 */
    parent_key_name = wstr_from_current("Software\\Apps");

    result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, parent_key_name, 0, 0, &parent_key);

    if (ERROR_SUCCESS != result)
    {
      fprintf(stderr, "%s: Unable to open parent registry key: %s\n", 
          argv[0],
          synce_strerror(result));
      goto exit;
    }
  }
  
  value_name = wstr_from_current("Instl");
	
	for (i = 0; ; i++)
	{
		WCHAR wide_name[MAX_PATH];
		DWORD name_size = sizeof(wide_name);
		HKEY program_key;
		DWORD installed = 0;
		DWORD value_size = sizeof(installed);

		result = CeRegEnumKeyEx(parent_key, i, wide_name, &name_size, NULL, NULL,
				NULL, NULL);
		if (ERROR_SUCCESS != result)
			break;

    if (smartphone)
    {
      char* name = wstr_to_current(wide_name);
      puts(name);
      wstr_free_string(name);
    }
    else
    {
      result = CeRegOpenKeyEx(parent_key, wide_name, 0, 0, &program_key);
      if (ERROR_SUCCESS != result)
        continue;

      result = CeRegQueryValueEx(program_key, value_name, NULL, NULL,
          (LPBYTE)&installed, &value_size);

      if (ERROR_SUCCESS == result && installed)
      {
        char* name = wstr_to_current(wide_name);
        puts(name);
        wstr_free_string(name);
      }
      CeRegCloseKey(program_key);
    }

	}

	CeRegCloseKey(parent_key);

	return_value = 0;

exit:
	wstr_free_string(parent_key_name);
	wstr_free_string(value_name);

	CeRapiUninit();
	return return_value;
}
