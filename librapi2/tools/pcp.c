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
			"\t%s [-h] SOURCE DESTINATION\n"
			"\n"
			"\t-h           Show this help message\n"
			"\tSOURCE       The source filename\n"
			"\tDESTINATION  The destination filename\n",
			name);
}

static bool handle_parameters(int argc, char** argv, char** source, char** dest)
{
	int c;
	int path_count;

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

	path_count = argc - optind;
	if (path_count < 1 || path_count > 2)
	{
		fprintf(stderr, "%s: You need to specify source and destination file names on command line\n\n", argv[0]);
		show_usage(argv[0]);
		return false;
	}
		
	*source = strdup(argv[optind++]);
	if (path_count > 1)
		*dest   = strdup(argv[optind++]);

	return true;
}

static bool remote_copy(const char* ascii_source, const char* ascii_dest)
{
	return CeCopyFileA(ascii_source, ascii_dest, false);
}

#define ANYFILE_BUFFER_SIZE (16*1024)

static bool anyfile_copy(const char* source_ascii, const char* dest_ascii, const char* name)
{
	bool success = false;
	size_t bytes_read;
	size_t bytes_written;
	char* buffer = NULL;
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

		if (bytes_written != bytes_read)
		{
			fprintf(stderr, "%s: Only wrote %i bytes of %i to destination file '%s'\n", name, 
					bytes_written, bytes_read, dest_ascii);
			goto exit;
		}
	}

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
	char* source = NULL;
	char* dest = NULL;
	HRESULT hr;
	
	if (!handle_parameters(argc, argv, &source, &dest))
		goto exit;

	hr = CeRapiInit();

	if (FAILED(hr))
	{
		fprintf(stderr, "%s: Unable to initialize RAPI: %s\n", 
				argv[0],
				synce_strerror(hr));
/*		goto exit;*/
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

			dest = calloc(1, wstr_strlen(mydocuments) + 1 + strlen(p) + 1);
			
			mydocuments_ascii = wstr_to_ascii(mydocuments);
			
			strcat(dest, mydocuments_ascii);
			strcat(dest, "\\");
			strcat(dest, p);
			
			wstr_free_string(mydocuments_ascii);
		}
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
		/*
		 * At least one is local, Use the AnyFile functions
		 */
		if (!anyfile_copy(source, dest, argv[0]))
			goto exit;
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
