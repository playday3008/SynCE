/* $Id$ */
#include <stdio.h>
#include <rapitypes.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "dbstream.h"
#include "../lib/recurrence.h"

#include "../rra_config.h"

#if HAVE_UIID_H
#include <uuid.h>
#elif HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#endif

static void
dump(void* data, size_t len)
{
	uint8_t* buf = (uint8_t*)data;
	unsigned i, j;
	char hex[8 * 3 + 1];
	char chr[8 + 1];

	for (i = 0; i < len + 7; i += 8) {
		for (j = 0; j < 8; j++) 
			if (j + i >= len) {
				hex[3*j+0] = ' ';
				hex[3*j+1] = ' ';
				hex[3*j+2] = ' ';
				chr[j] = ' ';
			} else {
				uint8_t c = buf[j + i];
				const char *hexchr = "0123456789abcdef";
				hex[3*j+0] = hexchr[(c >> 4) & 0xf];
				hex[3*j+1] = hexchr[c & 0xf];
				hex[3*j+2] = ' ';
				if (c > ' ' && c <= '~')
					chr[j] = c;
				else
					chr[j] = '.';
			}
		hex[8*3] = '\0';
		chr[8] = '\0';
		if (hex[0] != ' ')
			printf("%04x: %s %s\n", (unsigned)i, hex, chr);
	}
}

static void
db_dump(void* data, size_t len)
{
	uint8_t* buf = (uint8_t*)data;
	unsigned i, j;
	char hex[8 * 3 + 1];
	char chr[8 + 1];

	for (i = 0; i < len + 7; i += 8) {
		for (j = 0; j < 8; j++) 
			if (j + i >= len) {
				hex[3*j+0] = ' ';
				hex[3*j+1] = ' ';
				hex[3*j+2] = ' ';
				chr[j] = ' ';
			} else {
				uint8_t c = buf[j + i];
				const char *hexchr = "0123456789abcdef";
				hex[3*j+0] = hexchr[(c >> 4) & 0xf];
				hex[3*j+1] = hexchr[c & 0xf];
				hex[3*j+2] = ' ';
				if (c > ' ' && c <= '~')
					chr[j] = c;
				else
					chr[j] = '.';
			}
		hex[8*3] = '\0';
		chr[8] = '\0';
		if (hex[0] != ' ')
			printf("\n                     %04x: %s %s", i, hex, chr);
	}
}

static const char* data_type_as_string(uint16_t dataType)
{
	const char* type = NULL;
	
	switch (dataType)
	{
		case CEVT_I2: type = "I2"; break;
		case CEVT_I4: type = "I4"; break;
			
		case CEVT_R8: type = "R8"; break;
		case CEVT_BOOL: type = "BOOL"; break;
			
		case CEVT_UI2: type = "UI2"; break;
		case CEVT_UI4: type = "UI4"; break;

		case CEVT_LPWSTR: type = "LPWSTR"; break;
		case CEVT_FILETIME: type = "FILETIME"; break;
		case CEVT_BLOB: type = "BLOB"; break;

		default: type = "Unknown"; break;
	}

	return type;
}

void decode_directory(uint8_t* buffer)
{
	char* buffer_c = wstr_to_current((WCHAR*)(buffer + 4));
        if (!buffer_c) {
                fprintf(stderr, "Failed to convert directory to current encoding\n");
                return;
        }
	printf("Directory: \"%s\"\n", buffer_c);
	wstr_free_string(buffer_c);
}

void decode_file(uint8_t* buffer, size_t size, const char* filename)
{
	WCHAR* wide   = (WCHAR*)(buffer + 4);
	size_t extra  = 6 + 2 * wstrlen(wide);
	char* buffer_c   = wstr_to_current(wide);
        if (!buffer_c) {
                fprintf(stderr, "Failed to convert file contents to current encoding\n");
                return;
        }

	printf("File: \"%s\"\n", buffer_c);

	if (filename)
	{
		FILE* file = NULL;
		 
		if (0 == strcmp(filename, "-"))
			filename = buffer_c;
		
		file = fopen(filename, "w");
		if (file)
		{
			if (fwrite(buffer + extra, size-extra, 1, file) != 1)
				fprintf(stderr, "Failed to write data to file '%s'\n", filename);
			fclose(file);
		}
		else
		{
			fprintf(stderr, "Unable to open file '%s'\n", filename);
		}

	}
	else	
		dump(buffer + extra, size - extra); 

	wstr_free_string(buffer_c);
}

bool decode_database_stream(uint8_t* buffer, unsigned id)
{
	bool success = false;
	CEPROPVAL* propvals = NULL;
	uint32_t field_count = 0;
	unsigned i;

	field_count = letoh32(*(uint32_t*)(buffer + 0));
	/*printf("Field count: %i\n", field_count);*/

	propvals = (CEPROPVAL*)malloc(sizeof(CEPROPVAL) * field_count);

	if (!dbstream_to_propvals(buffer + 8, field_count, propvals))
	{
		fprintf(stderr, "Failed to convert database stream\n");
		goto exit;
	}

  if (!id)
    printf("ID    TYPE           VALUE\n");

	for (i = 0; i < field_count; i++)
	{
    if (id && (propvals[i].propid >> 16) != id)
      continue;
    
		printf("%04x  %04x %-8s  ",
				propvals[i].propid >> 16,
				propvals[i].propid & 0xff,
				data_type_as_string(propvals[i].propid & 0xff));

		switch (propvals[i].propid & 0xff)
		{
			case CEVT_I2: printf("0x%08x  %i", propvals[i].val.iVal, propvals[i].val.iVal); break;
			case CEVT_I4: printf("0x%08lx  %li", (long unsigned)propvals[i].val.lVal, (long)propvals[i].val.lVal); break;

			case CEVT_R8:  break;
			case CEVT_BOOL:  break;

			case CEVT_UI2: printf("0x%08x  %u", propvals[i].val.uiVal, propvals[i].val.uiVal); break;
			case CEVT_UI4: printf("0x%08x  %u", propvals[i].val.ulVal, propvals[i].val.ulVal); break;

			case CEVT_LPWSTR:  
				{
					char* buffer_c = wstr_to_current(propvals[i].val.lpwstr);
                                        if (!buffer_c) {
                                                fprintf(stderr, "Failed to convert data to current encoding\n");
                                        } else {
                                                printf("\"%s\"", buffer_c);
                                                wstr_free_string(buffer_c);
                                        }
				}
				break;

			case CEVT_FILETIME:
				{
#if 0
					time_t unix_time = filetime_to_unix_time(&propvals[i].val.filetime);
					char* time_str = asctime(gmtime(&unix_time));
					time_str[strlen(time_str)-1] = '\0'; /* remove trailing newline */
					printf("%s  (%lu)", time_str, unix_time);
#else
          TIME_FIELDS time_fields;
          time_fields_from_filetime(&propvals[i].val.filetime, &time_fields);
          printf("%04i-%02i-%02i %02i:%02i:%02i",
              time_fields.Year,
              time_fields.Month,
              time_fields.Day,
              time_fields.Hour,
              time_fields.Minute,
              time_fields.Second);
#endif
          db_dump(&propvals[i].val.filetime, sizeof(FILETIME));
        }
				break;
				
			case CEVT_BLOB:  
        if (id == 0x67)
        {
         
          if (propvals[i].val.blob.dwCount == 0x34)
          {
            static uint8_t header[20] = { 
              0x04, 0x00, 0x00, 0x00, 
              0x82, 0x00, 0xe0, 0x00,
              0x74, 0xc5, 0xb7, 0x10,
              0x1a, 0x82, 0xe0, 0x08,
              0x00, 0x00, 0x00, 0x00 
            };

            if (0 == memcmp(propvals[i].val.blob.lpb, header, sizeof(header)))
              printf("Header OK!\n");
            else
              printf("Unexpected header!\n");
            
#if 0 && HAVE_LIBUUID && (HAVE_UIID_H || HAVE_UUID_UUID_H)
            {
              char buffer[37];
              int variant;
              uuid_t uu;
              
              memcpy(&uu, propvals[i].val.blob.lpb + 20, sizeof(uuid_t));
              uuid_unparse(uu, buffer);
              variant = uuid_variant(uu);
              printf("%s [%i]  ", buffer, variant);
              
              memcpy(&uu, propvals[i].val.blob.lpb + 36, sizeof(uuid_t));
              uuid_unparse(uu, buffer);
              variant = uuid_variant(uu);
              printf("%s [%i]  ", buffer, variant);
            }
#else
            {
              unsigned j, k, l = 0;

               for (j = 0; j < 3; j++)
              {
                unsigned max = (j == 0) ? 20 : 16;
                for (k = 0; k < max; k++)
                  printf("%02x ", propvals[i].val.blob.lpb[l++]);

                printf("   ");
              }
            }
#endif
            printf("\n");
            continue;
          }
          
          printf("\n                     Unexpected BLOB 0067 size!");
        }

				printf("0x%x (%i) bytes:", propvals[i].val.blob.dwCount, propvals[i].val.blob.dwCount);

  			db_dump(propvals[i].val.blob.lpb, propvals[i].val.blob.dwCount);
        break;
		}

		printf("\n");
	}

	success = true;

exit:
	if (propvals)
		free(propvals);

	return success;
}

void decode_favorite(uint8_t* buffer, size_t size)
{
  const char* p = (const char*)(buffer + 4);

  printf("Name:     %s\n", p);
  p += strlen(p) + 1;
  
  printf("Address:  %s\n", p);
  p += strlen(p) + 1;
 }

int main(int argc, char** argv)
{
	int result = 1;
	FILE* file = NULL;
	uint8_t* buffer = NULL;
	long file_size = 0;
	unsigned id = 0;
	if (argc < 2)
	{
		fprintf(stderr, "Filename missing on command line\n");
		goto exit;
	}

	file = fopen(argv[1], "r");
	if (!file)
	{
		fprintf(stderr, "Unable to open file '%s'\n", argv[1]);
		goto exit;
	}

  if (argc > 2)
  {
    /* specific property id */
    id = strtol(argv[2], NULL, 16);
  }

	/* find out file size */
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	buffer = (uint8_t*)malloc(file_size);

	if (fread(buffer, file_size, 1, file) != 1)
	{
		fprintf(stderr, "Failed to read data from file '%s'\n", argv[1]);
		goto exit;
	}

	if (*(uint32_t*)(buffer + 4) != 0)
	{
		switch (*(uint32_t*)(buffer + 0))
    {
      case 2:
        fprintf(stderr, "Maybe some Merlin Mail data?\n");
        break;

      case 0x10:	/* directory */
        decode_directory(buffer);
        result = 0;
        break;

      case 0x20:	/* file*/
        decode_file(buffer, file_size, argv[2]);
        result = 0;
        break;

      case 0x4004:
        decode_favorite(buffer, file_size);
        result = 0;
        break;

      default:
        fprintf(stderr, "Unexpected file header: %08x\n",
            *(uint32_t*)(buffer + 0));
        break;
    }
	}
	else
	{
		if (decode_database_stream(buffer, id))
			result = 0;
	}

exit:
	if (file)
		fclose(file);
	
	if (buffer)
		free(buffer);

	return result;
}

