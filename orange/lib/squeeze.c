/* $Id$ */
#define _BSD_SOURCE 1
#include "liborange_internal.h"
#include <synce_log.h>
#include <dirent.h>
#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DELETE_FILES 1

static char* orange_get_temporary_directory()
{
  char buffer[256];
  const char* tmpdir = getenv("TMPDIR");
 
  if (!tmpdir)
    tmpdir = _PATH_TMP;

  snprintf(buffer, sizeof(buffer), "%s/orange-XXXXXX", tmpdir);

  if (mkdtemp(buffer))
    return strdup(buffer);
  else
    return NULL;
}

static bool orange_is_dot_directory(const char* dirname)/*{{{*/
{
  const char* basename = NULL;
  basename = strrchr(dirname, '/');
  if (basename)
    basename++;
  else
    basename = dirname;

  return STR_EQUAL(basename, ".") || STR_EQUAL(basename, "..");
}/*}}}*/

static bool orange_rmdir(const char* dirname)/*{{{*/
{
  bool success = false;
  DIR* dir = opendir(dirname);
  struct dirent* entry = NULL;

  if (!dir)
    goto exit;

  while (NULL != (entry = readdir(dir)))
  {
    struct stat file_stat;
    char filename[256];
    
    snprintf(filename, sizeof(filename), "%s/%s", dirname, entry->d_name);

    if (0 == stat(filename, &file_stat))
    {
      if (S_ISDIR(file_stat.st_mode))
      {
        if (!orange_is_dot_directory(entry->d_name))
          orange_rmdir(filename);
      }
      else  
        unlink(filename);
    }
  }

exit:
  CLOSEDIR(dir);
  success = (0 == rmdir(dirname));
  return success;
}/*}}}*/

bool orange_squeeze_file(/*{{{*/
    const char* filename,
    orange_filename_callback callback,
    void* cookie)
{
  bool success = false;
  const char* suffix = NULL;
  const char* basename = NULL;
  char* output_directory = NULL;

  if (!filename)
  {
    synce_error("Filename is NULL");
    goto exit;
  }

  output_directory = orange_get_temporary_directory();

  basename = strrchr(filename, '/');
  if (basename)
    basename++;
  else
    basename = filename;
  
  suffix = strrchr(filename, '.');
  if (suffix)
    suffix++;
  
  synce_trace("%s", filename);
  
  if (suffix)
  {
    if (STR_EQUAL(suffix, "apk"))
    {
      success = orange_extract_apk(filename, output_directory);
    }
    else if (STR_EQUAL(suffix, "arh"))
    {
      success = orange_extract_arh(filename, output_directory);
    }
    else if (STR_EQUAL(suffix, "cab"))
    {
      /* Hopefully a Microsoft Cabinet File or an InstallShield Cabinet File */
      CabInfo cab_info;

      if (STR_EQUAL(basename, "data1.cab"))
      {
        success = orange_extract_is_cab(filename, output_directory);
      }
      else if (STR_EQUAL(basename, "_sys1.cab") || STR_EQUAL(basename, "_user1.cab"))
      {
        /* ignore these InstallShield cabs */
      }
      else if (orange_get_installable_cab_info(filename, &cab_info))
      {
        callback(filename, &cab_info, cookie);
      }
      else
        success = orange_extract_ms_cab(filename, output_directory);
    }
    else if (STR_EQUAL(suffix, "exe"))
    {
      /* Maybe a self-extracting executable, try some different options */

      if (!success)
      {
        if (orange_make_sure_directory_exists(output_directory))
        {
          char output_filename[256];
          snprintf(output_filename, sizeof(output_filename), "%s/installer.exe", output_directory);
          success = orange_dllinflate(filename, output_filename);
        }
      }

      if (!success)
        success = orange_extract_setup_factory(filename, output_directory);

      if (!success)
        success = orange_extract_inno(filename, output_directory);

      if (!success)
        success = orange_extract_vise(filename, output_directory);

      if (!success)
        success = orange_extract_zip(filename, output_directory);

      if (!success)
        success = orange_extract_rar(filename, output_directory);
    
      if (!success)
        success = orange_separate(filename, output_directory);
    
      /* must be after call to orange_separate */
      if (!success)
        success = orange_extract_ms_cab(filename, output_directory);
    }
    else if (STR_EQUAL(suffix, "zip"))
    {
      success = orange_extract_zip(filename, output_directory);
    }
    else if (STR_EQUAL(suffix, "rar"))
    {
      success = orange_extract_rar(filename, output_directory);
    }
#if 0
    else
      synce_trace("Ignoring file with unknown suffix: '%s'", basename);
#endif
  }
#if 0
  else
    synce_trace("Ignoring file without suffix: '%s'", basename);
#endif

  if (success)
    success = orange_squeeze_directory(output_directory, callback, cookie);
    
exit:
#if DELETE_FILES
  orange_rmdir(output_directory);
#endif
  FREE(output_directory);
  return success;
}/*}}}*/

bool orange_squeeze_directory(/*{{{*/
    const char* dirname,
    orange_filename_callback callback,
    void* cookie)
{
  bool success = false;
  DIR* dir = opendir(dirname);
  struct dirent* entry = NULL;

  if (!dir)
  {
    synce_error("Failed to open directory '%s'", dirname);
    goto exit;
  }

  while (NULL != (entry = readdir(dir)))
  {
    char filename[256];
    struct stat file_stat;

    snprintf(filename, sizeof(filename), "%s/%s", dirname, entry->d_name);

    if (stat(filename, &file_stat) < 0)
    {
      synce_error("Failed to stat file '%s'", filename);
      goto exit;
    }

    if (S_ISREG(file_stat.st_mode))
    {
      orange_squeeze_file(filename, callback, cookie);
    }
    else if (S_ISDIR(file_stat.st_mode))
    {
      if (!orange_is_dot_directory(entry->d_name))
      {
        orange_squeeze_directory(filename, callback, cookie);
      }
    }
    else
      synce_trace("Bad file mode: 0x%x", file_stat.st_mode);
  }

  success = true;

exit:
  CLOSEDIR(dir);
  return success;
}/*}}}*/


