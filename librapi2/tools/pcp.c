/* $Id$ */
#include "pcommon.h"
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
			"\t%s [-d LEVEL] [-p DEVNAME] [-h] SOURCE DESTINATION\n"
			"\n"
			"\t-d LEVEL     Set debug log level\n"
			"\t                 0 - No logging (default)\n"
			"\t                 1 - Errors only\n"
			"\t                 2 - Errors and warnings\n"
			"\t                 3 - Everything\n"
			"\t-h           Show this help message\n"
                        "\t-p DEVNAME   Mobile device name\n"
			"\tSOURCE       The source filename\n"
			"\tDESTINATION  The destination filename\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** source, char** dest)
{
	int c;
	int path_count;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;

	while ((c = getopt(argc, argv, "d:hp:")) != -1)
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

	path_count = argc - optind;
	if (path_count < 1 || path_count > 2)
	{
		fprintf(stderr, "%s: You need to specify source and destination file names on command line\n\n", argv[0]);
		show_usage(argv[0]);
		return false;
	}
		
	*source = strdup(argv[optind++]);
	if (path_count > 1)
		*dest = strdup(argv[optind++]);

	return true;
}

static bool remote_copy(const char* ascii_source, const char* ascii_dest)
{
	return CeCopyFileA(ascii_source, ascii_dest, false);
}

#define ANYFILE_BUFFER_SIZE (64*1024)

static bool anyfile_copy(char* source_ascii, char* dest_ascii, const char* name, size_t* bytes_copied)
{
	bool success = false;
	size_t bytes_read;
	size_t bytes_written;
	unsigned char* buffer = NULL;
	AnyFile* source = NULL;
	AnyFile* dest   = NULL;

	if (!(buffer = malloc(ANYFILE_BUFFER_SIZE)))
	{
		fprintf(stderr, "%s: Failed to allocate buffer of size %i\n", name, ANYFILE_BUFFER_SIZE);
		goto exit;
	}

	if (!(source = anyfile_open(source_ascii, READ)))
	{
		fprintf(stderr, "%s: Failed to open source file '%s'\n", name, source_ascii);
		goto exit;
	}

	if (!(dest = anyfile_open(dest_ascii, WRITE)))
	{
		fprintf(stderr, "%s: Failed to open destination file '%s'\n", name, dest_ascii);
		goto exit;
	}

	for(;;)
	{
		if (!anyfile_read(source, buffer, ANYFILE_BUFFER_SIZE, &bytes_read))
		{
			fprintf(stderr, "%s: Failed to read from source file '%s'\n", name, source_ascii);
			goto exit;
		}

		if (0 == bytes_read)
		{
			/* End of file */
			break;
		}

		if (!anyfile_write(dest, buffer, bytes_read, &bytes_written))
		{
			fprintf(stderr, "%s: Failed to write to destination file '%s'\n", name, dest_ascii);
			goto exit;
		}

		if ((int)bytes_written != (int)bytes_read)
		{
			fprintf(stderr, "%s: Only wrote %zi bytes of %zi to destination file '%s'\n", name, 
					bytes_written, bytes_read, dest_ascii);
			goto exit;
		}

		*bytes_copied += bytes_written;
	}

	success = true;

exit:
	if (buffer)
		free(buffer);
	
	if (source)
	{
		anyfile_close(source);
		free(source);
	}

	if (dest)
	{
		anyfile_close(dest);
		free(dest);
	}

	return success;
}

int main(int argc, char** argv)
{
	int result = 1;
        RapiConnection* connection = NULL;
	char* source = NULL;
	char* dest = NULL;
	HRESULT hr;
	time_t start;
	time_t duration;
	size_t bytes_copied = 0;
	
	if (!handle_parameters(argc, argv, &source, &dest))
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

	if (!dest)
	{
		char* p;

		if (is_remote_file(source))
		{

			for (p = source + strlen(source); p != source; p--)
			{
				if (*p == '/' || *p == '\\')
				{
					dest = strdup(p+1);
					break;
				}
			}

			if (!dest || '\0' == dest[0])
			{
				fprintf(stderr, "%s: Unable to extract destination filename from source path '%s'\n",
						argv[0], source);
				goto exit;
			}
		}
		else
		{
			WCHAR mydocuments[MAX_PATH];
			char* mydocuments_ascii = NULL;
			p = strrchr(source, '/');

			if (p)
				p++;
			else
				p = source;

			if ('\0' == *p)
			{
				fprintf(stderr, "%s: Unable to extract destination filename from source path '%s'\n",
						argv[0], source);
				goto exit;
			}

			if (!CeGetSpecialFolderPath(CSIDL_PERSONAL, sizeof(mydocuments), mydocuments))
			{
				fprintf(stderr, "%s: Unable to get the \"My Documents\" path.\n",
						argv[0]);
				goto exit;
			}

			dest = calloc(1, 1 + wstr_strlen(mydocuments) + 1 + strlen(p) + 1);
			
			mydocuments_ascii = wstr_to_current(mydocuments);
			
			strcat(dest, ":");
			strcat(dest, mydocuments_ascii);
			strcat(dest, "\\");
			strcat(dest, p);
			
			wstr_free_string(mydocuments_ascii);
		}
	}

	if (0 == strcmp(source, dest))
	{
		fprintf(stderr, "You don't want to copy a file to itself.\n");
		goto exit;
	}

	if (is_remote_file(source) && is_remote_file(dest))
	{
		/*
		 * Both are remote; use CeCopyFile()
		 */
		if (!remote_copy(source, dest))
			goto exit;
	}
	else
	{
		start = time(NULL);

			/*
		 * At least one is local, Use the AnyFile functions
		 */
		if (!anyfile_copy(source, dest, argv[0], &bytes_copied))
			goto exit;

		duration = time(NULL) - start;

		if (0 == duration)
			printf("File copy took less than one second!\n");
		else
			printf("File copy of %zi bytes took %li minutes and %li seconds, that's %li bytes/s.\n",
					bytes_copied, duration / 60, duration % 60, bytes_copied / duration);

	}

	result = 0;

exit:
	if (source)
		free(source);

	if (dest)
		free(dest);

	CeRapiUninit();
	return result;
}
