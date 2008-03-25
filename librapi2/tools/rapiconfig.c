/* $Id$ */
#include <rapi.h>
#include <synce.h>
#include <synce_log.h>
#include <errno.h>
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
			"\t%s [-d LEVEL] [-P DEVNAME ] [-h] [-p] [-m] FILENAME"
			"\n"
			"\t-d LEVEL     Set debug log level\n"
			"\t                 0 - No logging (default)\n"
			"\t                 1 - Errors only\n"
			"\t                 2 - Errors and warnings\n"
			"\t                 3 - Everything\n"
			"\t-P DEVNAME   Mobile device name\n"
			"\t-h           Show this help message\n"
			"\t-p           Process the document\n"
			"\t-m           Return metadata\n"
			"\tFILENAME     Name of the XML configuration document\n",
			name);
}

static bool handle_parameters(int argc, char** argv, DWORD* flags, const char** filename)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;

  *flags = 0;

	while ((c = getopt(argc, argv, "d:hmpP:")) != -1)
	{
		switch (c)
		{
			case 'd':
				log_level = atoi(optarg);
				break;
			
                        case 'P':
                                dev_name = optarg;
                                break;
			
			case 'h':
			default:
				show_usage(argv[0]);
				return false;

      case 'm':
        *flags |= CONFIG_RETURN_METADATA;
        break;

      case 'p':
        *flags |= CONFIG_PROCESS_DOCUMENT;
        break;
		}
	}
				
	synce_log_set_level(log_level);

  if ((argc - optind) < 1)
  {
 		fprintf(stderr, "%s: Filename is missing on command line\n", 
				argv[0]);
    return false;
  }

  *filename = argv[optind++];

	return true;
}

#define BUFFER_SIZE   1024

static long fsize(FILE *file)
{
  long size = -1;
  if (fseek(file, 0, SEEK_END) == 0)
  {
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
  }
  return size;
}


int main(int argc, char** argv)
{
  int result = 1;
  HRESULT hr = E_UNEXPECTED;
  DWORD flags = 0;
  FILE* file = NULL;
  const char* filename = NULL;
  char* buffer = NULL;
  char* p = NULL;
  long bytes_left;
  LPWSTR config = NULL;
  LPWSTR reply = NULL;
  RapiConnection* connection = NULL;

  /*
     Initialization
   */

	if (!handle_parameters(argc, argv, &flags, &filename))
		goto exit;

  /*
     Read input file and convert to UNICODE
   */

  file = fopen(filename, "r");
  if (!file)
  {
		fprintf(stderr, "%s: Failed to open file '%s': %s\n", 
        argv[0],
				filename,
				strerror(errno));
		goto exit;
  }

  bytes_left = fsize(file);
  buffer = malloc(bytes_left+1);
  memset(buffer, 0, bytes_left+1);

  if (!buffer)
  {
 		fprintf(stderr, "%s: Failed to allocate %li bytes\n", 
        argv[0],
				bytes_left);
		goto exit;
  }

  p = buffer;

  while (bytes_left)
  {
    size_t bytes_read = fread(p, 1, bytes_left, file);

    if (0 == bytes_read)
    {
      fprintf(stderr, "%s: Reading stopped at offset %i: %s\n",
          argv[0],
          p - buffer,
          strerror(errno));
      goto exit;
    }
    
    p += bytes_read;
    bytes_left -= bytes_read;
  }

  config = wstr_from_current(buffer);
  if (!config)
  {
    fprintf(stderr, "%s: Failed to convert file contents to UNICODE\n",
          argv[0]);
    goto exit;
  }

  free(buffer);
  buffer = NULL;

  /* 
     Do the bossanova
   */

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

  hr = CeProcessConfig(config, flags, &reply);

  if (SUCCEEDED(hr))
  {
    result = 0;
    buffer = wstr_to_current(reply);
    printf("%s", buffer);
    CeRapiFreeBuffer(buffer);
    buffer = NULL;
  }

exit:
  CeRapiUninit();

  if (buffer)
    free(buffer);

  wstr_free_string(config);

  return result;
}

