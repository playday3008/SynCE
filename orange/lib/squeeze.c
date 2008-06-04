/* $Id$ */
#define _BSD_SOURCE 1
#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif
#include "liborange_internal.h"
#include "liborange_log.h"
#include <dirent.h>
#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if HAVE_LIBMAGIC && HAVE_MAGIC_H
#include <magic.h>
#define DO_MAGIC 1
#define VERBOSE_MAGIC 0
#endif

#define DELETE_FILES 1

static char* orange_get_temporary_directory()/*{{{*/
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
}/*}}}*/

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

static bool squeeze_exe(
    const char* filename,
    const char* output_directory)
{
  bool success = false;
  
  /* Maybe a self-extracting executable, try some different options */

  if (!success)
  {
    if (orange_make_sure_directory_exists(output_directory))
    {
      char output_filename[256];
      snprintf(output_filename, sizeof(output_filename), "%s/installer.exe", output_directory);
      success = orange_dllinflate(filename, output_filename);
      if (success)
        synce_trace("Found DllInflate EXE format.");
    }
  }

  if (!success)
  {
    success = orange_extract_installshield_sfx(filename, output_directory);
    if (success)
      synce_trace("Found InstallShield self-extracting executable.");
  }

  if (!success)
  {
    success = orange_extract_installshield_sfx2(filename, output_directory);
    if (success)
      synce_trace("Found InstallShield self-extracting executable (type 2).");
  }

  if (!success)
  {
    success = orange_extract_setup_factory(filename, output_directory);
    if (success)
      synce_trace("Found SetupFactory format.");
  }

#if ENABLE_INNO
  if (!success)
  {
    success = orange_extract_inno(filename, output_directory);
    if (success)
      synce_trace("Found InnoSetup format.");
  }
#endif

#if ENABLE_VISE
  if (!success)
  {
    success = orange_extract_vise(filename, output_directory);
    if (success)
      synce_trace("Found VISE Setup format.");
  }
#endif

  if (!success)
  {
    success = orange_extract_zip(filename, output_directory);
    if (success)
      synce_trace("Found ZIP format.");
  }

  if (!success)
  {
    success = orange_extract_rar(filename, output_directory);
    if (success)
      synce_trace("Found RAR format.");
  }

  if (!success)
  {
    success = orange_is_nullsoft_installer(filename);
    if (success)
      synce_error("Found the unsupported Nullsoft Scriptable Installer format.");
  }

  if (!success)
  {
    /* try to extract ms cab files from file */
    success = orange_separate(filename, output_directory);
  }

  /* must be after call to orange_separate */
  if (!success)
  {
    success = orange_extract_ms_cab(filename, output_directory);
    if (success)
      synce_trace("Found Microsoft CAB format.");
  }

  /* Always extract resources */
  if (orange_extract_rsrc(filename, output_directory))
    success = true;

  return success;
}

#if DO_MAGIC
static bool squeeze_by_magic(/*{{{*/
    const char* filename,
    orange_filename_callback callback,
    void* cookie,
    const char* output_directory)
{
  bool success = false;
  magic_t magic = NULL;
  const char* description = NULL;

  magic = magic_open(MAGIC_NONE);
  if (!magic)
    goto exit;

  magic_load(magic, NULL); 

  description = magic_file(magic, filename);
  
  if (!description)
    goto exit;

#if VERBOSE_MAGIC
  synce_trace("%s: %s", filename, description);
#endif
  if (strstr(description, "Microsoft Cabinet"))
  {
    CabInfo cab_info;
    if (orange_get_installable_cab_info(filename, &cab_info))
    {
      callback(filename, &cab_info, cookie);
    }
    else if (orange_get_new_installable_cab_info(filename, &cab_info))
    {
      callback(filename, &cab_info, cookie);
    }
    else
    {
      success = orange_extract_ms_cab(filename, output_directory);
      if (success)
        synce_trace("Found Microsoft CAB format.");
    }
  }
#if ENABLE_MSI
  else if (strstr(description, "Microsoft Office Document"))
  {
    success = orange_extract_msi(filename, output_directory);
    if (success)
      synce_trace("Found MSI format.");
  }
#endif
  else if (strstr(description, "MS-DOS executable (EXE), OS/2 or MS Windows") ||
      strstr(description, "MS-DOS executable PE"))
  {
    success = squeeze_exe(filename, output_directory);
  }

exit:
  if (magic)
    magic_close(magic);
  return success;
}/*}}}*/
#endif

static bool squeeze_by_suffix(/*{{{*/
    const char* filename,
    orange_filename_callback callback,
    void* cookie,
    const char* output_directory)
{
  bool success = false;
  const char* suffix = NULL;
  const char* basename = NULL;

  basename = strrchr(filename, '/');
  if (basename)
    basename++;
  else
    basename = filename;

  suffix = strrchr(filename, '.');
  if (suffix)
    suffix++;

  /*synce_trace("%s", filename);*/

  if (suffix)
  {
    if (STR_EQUAL(suffix, "apk"))
    {
      synce_trace("Trying TomTom APK format.");
      success = orange_extract_apk(filename, output_directory);
    }
    else if (STR_EQUAL(suffix, "arh"))
    {
      synce_trace("Trying TomTom ARH format.");
      success = orange_extract_arh(filename, output_directory);
    }
    else if (STR_EQUAL(suffix, "cab"))
    {
      /* Hopefully a Microsoft Cabinet File or an InstallShield Cabinet File */
      CabInfo cab_info;

      if (STR_EQUAL(basename, "data1.cab"))
      {
        synce_trace("Trying InstallShield CAB format.");
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
      else if (orange_get_new_installable_cab_info(filename, &cab_info))
      {
        callback(filename, &cab_info, cookie);
      }
      else
      {
        success = orange_extract_ms_cab(filename, output_directory);
        if (success)
          synce_trace("Found Microsoft CAB format.");
      }
    }
#if !DO_MAGIC  /* any interesting .exe files should already have been caught by magic */
    else if (STR_EQUAL(suffix, "exe"))
    {
      success = squeeze_exe(filename, output_directory);
    }
#endif
#if ENABLE_MSI
    else if (STR_EQUAL(suffix, "msi"))
    {
      success = orange_extract_msi(filename, output_directory);
      if (success)
        synce_trace("Found MSI format.");
    }
#endif
    else if (STR_EQUAL(suffix, "zip"))
    {
      success = orange_extract_zip(filename, output_directory);
      if (success)
        synce_trace("Found ZIP format.");
    }
    else if (STR_EQUAL(suffix, "rar"))
    {
      success = orange_extract_rar(filename, output_directory);
      if (success)
        synce_trace("Found RAR format.");
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

  return success;
}/*}}}*/

bool orange_squeeze_file(/*{{{*/
    const char* filename,
    orange_filename_callback callback,
    void* cookie)
{
  bool success = false;
  char* output_directory = NULL;

  if (!filename)
  {
    synce_error("Filename is NULL");
    goto exit;
  }

  output_directory = orange_get_temporary_directory();

#if DO_MAGIC
  success = squeeze_by_magic(filename, callback, cookie, output_directory);
#endif

  if (!success)
    success = squeeze_by_suffix(filename, callback, cookie, output_directory);

  if (success)
    success = orange_squeeze_directory(output_directory, callback, cookie);
    
exit:
#if DELETE_FILES
  orange_rmdir(output_directory);
#else
  /* only remove empty directories, let this fail for non-empty directories */
  rmdir(output_directory);
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

  synce_trace("Directory: %s", dirname);

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


