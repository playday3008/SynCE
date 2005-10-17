/* $Id$ */
#ifndef __liborange_internal_h__
#define __liborange_internal_h__

#include "liborange.h"
#include <stdio.h>

/*
   Helper functions
 */

bool orange_make_sure_directory_exists(const char* directory);
long orange_fsize(FILE* file);
bool orange_copy(FILE* input_file, 
    size_t size, 
    const char* output_directory, 
    const char* filename);
bool orange_write(const uint8_t* output_buffer, size_t output_size, const char* output_directory, const char* basename);

uint8_t orange_read_byte(FILE* input_file);
uint32_t orange_read32(FILE* input_file);
bool orange_write_byte(FILE* output_file, uint8_t byte);

/*
   Macros for in-place byte order conversion
 */

#define LETOH16(x)  x = letoh16(x)
#define LETOH32(x)  x = letoh32(x)
#define HTOLE16(x)  x = htole16(x)
#define HTOLE32(x)  x = htole32(x)


/*
   Macros for safer development
 */

#define FREE(ptr)       { if (ptr) { free(ptr); ptr = NULL; } }
#define STRDUP(str)     ((str) ? strdup(str) : NULL)
#define NEW1(type)      ((type*)calloc(1, sizeof(type)))
#define FCLOSE(file)    if (file) { fclose(file); file = NULL; }
#define FSIZE(file)     (file ? orange_fsize(file) : 0)
#define CLOSEDIR(dir)   if (dir) { closedir(dir); dir = NULL; }

/*
   Utility macros
 */

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))


#endif

