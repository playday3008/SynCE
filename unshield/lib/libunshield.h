/* $Id$ */
#ifndef __unshield_h__
#define __unshield_h__

#include <stdbool.h>
#include <stddef.h>

#define UNSHIELD_LOG_LEVEL_LOWEST    0

#define UNSHIELD_LOG_LEVEL_ERROR     1
#define UNSHIELD_LOG_LEVEL_WARNING   2
#define UNSHIELD_LOG_LEVEL_TRACE     3

#define UNSHIELD_LOG_LEVEL_HIGHEST   4


#ifdef __cplusplus
extern "C" {
#endif
 
typedef struct _Unshield Unshield;


/*
   Logging
 */

void unshield_set_log_level(int level);


/*
   Open/close functions
 */

Unshield* unshield_open(const char* filename);
void unshield_close(Unshield* unshield);

/*
   Component functions
 */

typedef struct
{
  const char* name;
  unsigned file_group_count;
  const char** file_group_names;
} UnshieldComponent;

int         unshield_component_count    (Unshield* unshield);
const char* unshield_component_name     (Unshield* unshield, int index);

/*
   File group functions
 */

typedef struct
{
  const char* name;
  unsigned first_file;
  unsigned last_file;
} UnshieldFileGroup;

int                 unshield_file_group_count (Unshield* unshield);
UnshieldFileGroup*  unshield_file_group_get   (Unshield* unshield, int index);
UnshieldFileGroup*  unshield_file_group_find  (Unshield* unshield, const char* name);
const char*         unshield_file_group_name  (Unshield* unshield, int index);

/*
   Directory functions
 */

int         unshield_directory_count    (Unshield* unshield);
const char* unshield_directory_name     (Unshield* unshield, int index);

/*
   File functions
 */

int         unshield_file_count         (Unshield* unshield);
const char* unshield_file_name          (Unshield* unshield, int index);
bool        unshield_file_is_valid      (Unshield* unshield, int index);
bool        unshield_file_save          (Unshield* unshield, int index, const char* filename);
int         unshield_file_directory     (Unshield* unshield, int index);
size_t      unshield_file_size          (Unshield* unshield, int index);

#ifdef __cplusplus
}
#endif

  
#endif

