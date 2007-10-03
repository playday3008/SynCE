/* $Id$ */
#define _BSD_SOURCE 1
#define _POSIX_C_SOURCE 2
#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif
#include <liborange.h>
#if WITH_LIBUNSHIELD
#include <libunshield.h>
#endif
#include <liborange_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#if WITH_LIBGSF
#include <gsf/gsf-utils.h>
#endif

#define FREE(ptr)       { if (ptr) { free(ptr); ptr = NULL; } }
#define FCLOSE(file)    if (file) { fclose(file); file = NULL; }

static const char* output_directory = NULL;
static int count = 0;

static void show_usage(const char* name)
{
  fprintf(stderr,
      "Syntax:\n"
      "\n"
      "\t%s [-d DIRECTORY] [-D LEVEL] [-h] FILENAME\n"
      "\n"
      "\t-d DIRECTORY  Extract files to DIRECTORY\n"
      "\t-D LEVEL      Set debug log level\n"
      "\t                0 - No logging (default)\n"
      "\t                1 - Errors only\n"
      "\t                2 - Errors and warnings\n"
      "\t                3 - Everything\n"
      "\t-h            Show this help message\n"
      "\tFILENAME      The file to extract contents of\n"
      ,
      name);

#if 0
      "\t-n            Never overwrite files\n"
      "\t-o            Overwrite files WITHOUT prompting\n"
      "\t-v            Verbose output\n"
#endif
}

static bool handle_parameters(
    int argc, 
    char** argv,
    const char** input_filename)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;

	while ((c = getopt(argc, argv, "d:D:h")) != -1)
	{
		switch (c)
		{
			case 'd':
				output_directory = optarg;
				break;
				
			case 'D':
				log_level = atoi(optarg);
				break;
       
      case 'h':
      default:
				show_usage(argv[0]);
				return false;
		}
	}

	synce_log_set_level(log_level);
#if WITH_LIBUNSHIELD
	unshield_set_log_level(log_level);
#endif

  if (optind < argc)
    *input_filename = argv[optind];

	return true;
}

#define BUFFER_SIZE  (256*1024)

static long fsize(FILE* file)
{
  long result;
  long previous = ftell(file);
  fseek(file, 0L, SEEK_END);
  result = ftell(file);
  fseek(file, previous, SEEK_SET);
  return result;
}


static bool copy_file(const char* input_filename, const char* output_filename)
{
  bool success = false;
  uint8_t* buffer = malloc(BUFFER_SIZE);
  FILE* input_file  = fopen(input_filename,  "r");
  FILE* output_file = fopen(output_filename, "w");
  long bytes_left = 0;
  
  if (!buffer)
    goto exit;
  
  if (!input_file)
    goto exit;

  if (!output_file)
    goto exit;

  bytes_left = fsize(input_file);

  while (bytes_left)
  {
    unsigned bytes_to_copy = MIN(bytes_left, BUFFER_SIZE);

    if (bytes_to_copy != fread(buffer, 1, bytes_to_copy, input_file))
      goto exit;
    
    if (bytes_to_copy != fwrite(buffer, 1, bytes_to_copy, output_file))
      goto exit;

    bytes_left -= bytes_to_copy;
  }

  success = true;

exit:
  FREE(buffer);
  FCLOSE(input_file);
  FCLOSE(output_file);
  return success;
}


static bool callback(
    const char* filename, 
    CabInfo* info,
    void* cookie)
{
  bool success = false;
  const char* p = strrchr(filename, '/');
  char* basename = NULL;
  char output_filename[256];
  
  if (p)
    basename = strdup(p+1);
  else
    basename = strdup(filename);
    
  snprintf(output_filename, sizeof(output_filename), "%s/%s", output_directory, basename);

  printf("squeezing out: %s\n", output_filename);

  if (!copy_file(filename, output_filename))
  {
    fprintf(stderr, "Failed to copy from '%s' to '%s'\n", filename, output_filename);
    goto exit;
  }

  count++;
  success = true;

exit:
  FREE(basename);
  return success;
}


int main(int argc, char** argv)
{
  int result = 1;
  const char* input_filename = NULL;
  char working_directory[256];

#if WITH_LIBGSF
  gsf_init();
#endif

   output_directory = getcwd(working_directory, sizeof(working_directory));

  if (!handle_parameters(argc, argv, &input_filename))
    goto exit;
  
  if (!output_directory)
  {
    fprintf(stderr, "Failed to get current directory, use the -d parameter.\n");
    show_usage(argv[0]);
    goto exit;
  }

  if (!input_filename)
  {
    fprintf(stderr, "Missing input filename on command line\n");
    show_usage(argv[0]);
    goto exit;
  }


  if (!orange_squeeze_file(input_filename, callback, NULL))
    goto exit;

  printf("-------\n%i files\n", count);

  result = 0;
  
exit:
#if WITH_LIBGSF
  gsf_shutdown();
#endif
   return result;
}

