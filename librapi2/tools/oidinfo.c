/* $Id$ */
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* devname = NULL;

static void show_usage(const char* name)
{
	fprintf(stderr,
			"Syntax:\n"
			"\n"
			"\t%s [-d LEVEL] [-p DEVNAME] [-h] OID\n"
			"\n"
			"\t-d LEVEL     Set debug log level\n"
			"\t                 0 - No logging (default)\n"
			"\t                 1 - Errors only\n"
			"\t                 2 - Errors and warnings\n"
			"\t                 3 - Everything\n"
                        "\t-p DEVNAME   Mobile device name\n"
			"\t-h           Show this help message\n"
			"\tOID          The object identifier we want to know more about\n",
			name);
}

static bool handle_parameters(int argc, char** argv, CEOID* oid)
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
                                devname = optarg;
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
		printf("Missing OID parameter\n\n");
		show_usage(argv[0]);
		return false;
	}

	*oid = strtol(argv[optind], NULL, 0);

	return true;
}

int main(int argc, char** argv)
{
	int result = 1;
	HRESULT hr;
	CEOID oid = 0;
	CEOIDINFO info;
        RapiConnection* connection = NULL;
	
	if (!handle_parameters(argc, argv, &oid))
		goto exit;

        if ((connection = rapi_connection_from_name(devname)) == NULL)
        {
          fprintf(stderr, "%s: Could not find configuration at path '%s'\n", 
                  argv[0],
                  devname?devname:"(Default)");
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

	memset(&info, 0, sizeof(info));

	if (CeOidGetInfo(oid, &info))
	{
		switch (info.wObjType)
		{
			case OBJTYPE_INVALID:
				printf("Invalid object\n");
				break;

			case OBJTYPE_FILE:
				printf("File object:\n"
						   "============\n");
				{
					char *name = wstr_to_current(info.u.infFile.szFileName);
					printf("Attributes:  %08x\n"
							   "Name:        \"%s\"\n"
								 , 
								 info.u.infFile.dwAttributes,
								 name);
					wstr_free_string(name);
				}
				break;
				
			case OBJTYPE_DIRECTORY:
				printf("Directory object:\n"
						   "=================\n");
				{
					char *name = wstr_to_current(info.u.infDirectory.szDirName);
					printf("Attributes:  %08x\n"
							   "Parent OID:  %08x\n"
							   "Name:        \"%s\"\n"
								 , 
								 info.u.infDirectory.dwAttributes,
								 info.u.infDirectory.oidParent,
								 name);
					
					wstr_free_string(name);
				}
				break;

			case OBJTYPE_DATABASE:
				printf(
						"Database:\n"
						"=========\n");
				{
					char *name = wstr_to_current(info.u.infDatabase.szDbaseName);
					printf(
							"Flags: %08x\n"
							"Name: \"%s\"\n"
							"Type: %08x\n"
							"Record count: %i\n"
							"Sort order count: %i\n"
							"Size: %i\n"
							,
							info.u.infDatabase.dwFlags,
							name,
							info.u.infDatabase.dwDbaseType,
							info.u.infDatabase.wNumRecords,
							info.u.infDatabase.wNumSortOrder,
							info.u.infDatabase.dwSize
							);
				}
				break;

			case OBJTYPE_RECORD:
				printf(
						"Database record:\n"
						"================\n"
						"Parent OID: %08x\n", 
						info.u.infRecord.oidParent);
				break;

			default:
				printf("Unknown object with type %i\n", info.wObjType);
				break;
		}
	}
	else
	{
		fprintf(stderr, "%s: Failed to get object information: %s\n", 
				argv[0],
				synce_strerror(CeGetLastError()));
		goto exit;
	}

	result = 0;

exit:
	CeRapiUninit();
	return result;
}
