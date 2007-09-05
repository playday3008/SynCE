#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <rapi.h>
#include <synce_log.h>

#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-module.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#define SHOW_APPLICATIONS   0

#ifdef WITH_VERBOSE_DEBUG
#define D(x...) synce_debug(x)
#else
#define D(x...)
#endif

#ifdef G_THREADS_ENABLED
#define MUTEX_NEW()     g_mutex_new ()
#define MUTEX_FREE(a)   g_mutex_free (a)
#define MUTEX_LOCK(a)   if ((a) != NULL) g_mutex_lock (a)
#define MUTEX_UNLOCK(a) if ((a) != NULL) g_mutex_unlock (a)
#else
#define MUTEX_NEW()     NULL
#define MUTEX_FREE(a)
#define MUTEX_LOCK(a)
#define MUTEX_UNLOCK(a)
#endif

#define NAME_MY_DOCUMENTS  "My Documents"

#define NAME_APPLICATIONS   "Applications"
#define NAME_DOCUMENTS      "Documents"
#define NAME_FILESYSTEM     "Filesystem"

#define INDEX_INVALID       -1
#define INDEX_DEVICE        1
#define INDEX_APPLICATIONS  2
#define INDEX_FILESYSTEM    3
#define INDEX_DOCUMENTS     4

#define NAME_SD_CARD        "SD-MMC Card"
#define NAME_ROM_STORAGE    "ROM Storage"

enum {
  FS_DEVICE = 0,
  FS_SD_CARD,
  FS_ROM_STORAGE
};

typedef struct _DIR_HANDLE
{
  int index;
  char *location;
  CE_FIND_DATA *data;
  int itemcount;
  int count;
} DIR_HANDLE;

static GMutex * mutex = NULL;

static gboolean initialized_rapi = FALSE;

typedef struct _ErrorCodeTriple
{
  GnomeVFSResult gnome_vfs_result;
  DWORD error;
  HRESULT hresult;
} ErrorCodeTriple;

/* hack, these should be in synce.h */
#ifndef ERROR_TOO_MANY_OPEN_FILES
#define ERROR_TOO_MANY_OPEN_FILES      4
#define ERROR_INVALID_HANDLE           6
#define ERROR_NO_MORE_FILES           18
#define ERROR_DISK_FULL              112
#endif

#define ERROR_NOT_SAME_DEVICE         17

static ErrorCodeTriple error_codes[] =
{
    {GNOME_VFS_OK,                        ERROR_SUCCESS,        S_OK        },
    {GNOME_VFS_ERROR_GENERIC,             0,                    E_FAIL      },
    {GNOME_VFS_ERROR_INTERNAL,            0,                    E_UNEXPECTED},
    {GNOME_VFS_ERROR_NOT_FOUND,           ERROR_FILE_NOT_FOUND, 0},
    {GNOME_VFS_ERROR_NOT_FOUND,           ERROR_PATH_NOT_FOUND, 0},
    {GNOME_VFS_ERROR_INVALID_URI,         ERROR_INVALID_NAME,   0},
    {GNOME_VFS_ERROR_FILE_EXISTS,         ERROR_FILE_EXISTS,    0},
    {GNOME_VFS_ERROR_FILE_EXISTS,         ERROR_ALREADY_EXISTS, 0},
    {GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY, ERROR_DIR_NOT_EMPTY,  0},
    {GNOME_VFS_ERROR_NOT_PERMITTED,       ERROR_ACCESS_DENIED,  0},
    {GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES, ERROR_TOO_MANY_OPEN_FILES,  0},
    {GNOME_VFS_ERROR_NOT_FOUND,           ERROR_NO_MORE_FILES,  0},
    {GNOME_VFS_ERROR_NO_SPACE,            ERROR_DISK_FULL,  0},
    {GNOME_VFS_ERROR_NOT_SAME_FILE_SYSTEM, ERROR_NOT_SAME_DEVICE,  0}
};

static GnomeVFSResult gnome_vfs_result_from_rapi()/*{{{*/
{
  GnomeVFSResult result = GNOME_VFS_ERROR_GENERIC;
  HRESULT hr;
  unsigned error;
  int i;

  hr    = CeRapiGetError();
  error = CeGetLastError();

  if (FAILED(hr))
    {
      /* This is a connection error, so we close the connection */
      CeRapiUninit();
      initialized_rapi = FALSE;

      synce_trace("HRESULT = %08x: %s", hr, synce_strerror(hr));

      for (i = 0; i < sizeof(error_codes)/sizeof(ErrorCodeTriple); i++)
        {
          if (error_codes[i].hresult == hr)
            {
              result = error_codes[i].gnome_vfs_result;
              break;
            }
        }
    }
  else
    {
      synce_trace("error = %i: %s", error, synce_strerror(error));

      for (i = 0; i < sizeof(error_codes)/sizeof(ErrorCodeTriple); i++)
        {
          if (error_codes[i].error == error)
            {
              result = error_codes[i].gnome_vfs_result;
              break;
            }
        }
    }

  synce_trace("GnomeVFSResult = %s", gnome_vfs_result_to_string(result));
  return result;
}/*}}}*/

static GnomeVFSResult initialize_rapi()/*{{{*/
{
  if (!initialized_rapi)
    {
      MUTEX_LOCK (mutex); 
      HRESULT hr = CeRapiInit();
      MUTEX_UNLOCK (mutex); 

      if (FAILED(hr))
        {
          synce_error("Unable to initialize RAPI: %08x: %s",
              hr, synce_strerror(hr));
          return GNOME_VFS_ERROR_LOGIN_FAILED;
        }

      initialized_rapi = TRUE;
    }

  return GNOME_VFS_OK;
}/*}}}*/

static gint get_location(const GnomeVFSURI *uri, gchar **location)/*{{{*/
{
  gint result = INDEX_INVALID;
  gchar **path = NULL;

  path = g_strsplit(gnome_vfs_unescape_string(gnome_vfs_uri_get_path(uri),"\\"), "/", 0);

#ifdef WITH_VERBOSE_DEBUG
    {
      int i;
      for (i = 0; path[i]; i++)
        synce_trace("path[%i] = '%s'", i, path[i]);
    }
#endif

  if (!path || !path[0] || !path[1])
    {
      result = INDEX_DEVICE;
      *location = NULL;
    }
  else if (0 == strcmp(path[1], ""))
    {
      result = INDEX_DEVICE;
      *location = NULL;
    }
#if SHOW_APPLICATIONS
  else if (0 == strcmp(path[1], NAME_APPLICATIONS))
    {
      result = INDEX_APPLICATIONS;
      *location = g_strdup(path[2]);
    }
#endif
  else if (0 == strcmp(path[1], NAME_DOCUMENTS))
    {
      /* XXX: what are the name of this on non-english systems? */
      gchar *tmp = g_strjoinv("\\", &path[2]);
      *location = g_strdup_printf("\\%s\\%s", NAME_MY_DOCUMENTS, tmp);
      g_free(tmp);
      result = INDEX_DOCUMENTS;
    }
  else if (0 == strcmp(path[1], NAME_FILESYSTEM))
    {
      gchar *tmp = g_strjoinv("\\", &path[2]);
      *location = g_strdup_printf("\\%s", tmp);
      g_free(tmp);
      result = INDEX_FILESYSTEM;
    }
  else
    {
      result = INDEX_INVALID;
      *location = NULL;
    }

#ifdef WITH_VERBOSE_DEBUG
  synce_trace("index = %i, location = '%s'", result, *location);
#endif

  g_strfreev(path);
  return result;
}/*}}}*/

static gint vfs_to_synce_mode(GnomeVFSOpenMode mode, gint *open_mode, gint *create_mode)/*{{{*/
{
  if(mode & GNOME_VFS_OPEN_READ)
    {
      if(mode & GNOME_VFS_OPEN_WRITE)
        {
          *create_mode = CREATE_NEW;
          *open_mode = GENERIC_READ & GENERIC_WRITE;
        }
      else
        {
          *create_mode = OPEN_EXISTING;
          *open_mode = GENERIC_READ;
        }
    } 
  else
    {
      *create_mode = CREATE_NEW;
      *open_mode = GENERIC_WRITE;
    }
  return 0;
}/*}}}*/

static GnomeVFSResult synce_open/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSMethodHandle **method_handle_return,
 GnomeVFSURI *uri,
 GnomeVFSOpenMode mode,
 GnomeVFSContext *context)
{
  GnomeVFSResult result;
  char *location = NULL;
  WCHAR *wide_path = NULL;
  int synce_open_mode;
  int synce_create_mode;
  HANDLE handle;

  D("------------------ synce_open() ------------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;;

  switch (get_location(uri, &location))
    {
    case INDEX_DEVICE:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;

    case INDEX_APPLICATIONS:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  D("location: %s\n", location);

  wide_path = wstr_from_utf8(location);

  vfs_to_synce_mode(mode, &synce_open_mode, &synce_create_mode);

  D("CeCreateFile()\n");
  MUTEX_LOCK (mutex);
  handle = CeCreateFile
    (
     wide_path,
     synce_open_mode,
     0,
     NULL,
     synce_create_mode,
     FILE_ATTRIBUTE_NORMAL,
     0
    );

  if(handle == INVALID_HANDLE_VALUE)
    result = gnome_vfs_result_from_rapi();
  else
    result = GNOME_VFS_OK;

  MUTEX_UNLOCK (mutex);

  g_free(location);
  wstr_free_string(wide_path);

  *(method_handle_return) = GUINT_TO_POINTER(handle);

exit:
  D("------------------ synce_open() end --------------\n");
  return result;
}/*}}}*/

static GnomeVFSResult synce_create/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSMethodHandle **method_handle_return,
 GnomeVFSURI *uri,
 GnomeVFSOpenMode mode,
 gboolean exclusive,
 guint perm,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result;
  char *location = NULL;
  WCHAR *wide_path = NULL;
  int synce_open_mode;
  int synce_create_mode;
  HANDLE handle;

  D("------------------ synce_create() ----------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  switch (get_location(uri, &location))
    {
    case INDEX_DEVICE:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;

    case INDEX_APPLICATIONS:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  D("location: %s\n", location);

  wide_path = wstr_from_utf8(location);

  vfs_to_synce_mode(mode, &synce_open_mode, &synce_create_mode);

  D("CeCreateFile()");
  MUTEX_LOCK (mutex);
  handle = CeCreateFile
    (
     wide_path,
     synce_open_mode,
     0,
     NULL,
     synce_create_mode,
     FILE_ATTRIBUTE_NORMAL,
     0
    );

  if(handle == INVALID_HANDLE_VALUE)
    result = gnome_vfs_result_from_rapi();
  else
    result = GNOME_VFS_OK;

  MUTEX_UNLOCK (mutex);

  g_free(location);
  wstr_free_string(wide_path);

  *(method_handle_return) = GUINT_TO_POINTER(handle);

exit:
  D("------------------ synce_create() end ------------\n");
  return result;
}/*}}}*/

static GnomeVFSResult synce_close/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSMethodHandle *method_handle,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result;
  HANDLE handle;
  int success;

  D("------------------- synce_close() ----------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  handle = GPOINTER_TO_UINT(method_handle);

  D("synce_close: CeCloseHandle()\n");
  MUTEX_LOCK (mutex);
  success = CeCloseHandle(handle);

  if (success)
    result = GNOME_VFS_OK;
  else
    result = gnome_vfs_result_from_rapi();

  MUTEX_UNLOCK (mutex);

exit:
  D("------------------- synce_close() end ------------\n");
  return result;
}/*}}}*/

static GnomeVFSResult synce_read/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSMethodHandle *method_handle,
 gpointer buffer,
 GnomeVFSFileSize num_bytes,
 GnomeVFSFileSize *bytes_read_return,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result;
  int success;
  DWORD read_return;
  HANDLE handle;

  D("------------------ synce_read() ------------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  handle = GPOINTER_TO_UINT(method_handle);

  D("CeReadFile\n");
  MUTEX_LOCK (mutex);
  success = CeReadFile
    (
     handle,
     buffer,
     num_bytes,
     &read_return,
     NULL
    );

  if (!success)
    {
      result = gnome_vfs_result_from_rapi();
    }
  else if (read_return == 0)
    {
      *bytes_read_return = 0;
      result = GNOME_VFS_ERROR_EOF;
    }
  else
    {
      *bytes_read_return = read_return;
      result = GNOME_VFS_OK;
    }

  MUTEX_UNLOCK (mutex);
exit:
  D("------------------ synce_read() end --------------\n");
  return result;
}/*}}}*/

static GnomeVFSResult synce_write/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSMethodHandle *method_handle,
 gconstpointer buffer,
 GnomeVFSFileSize num_bytes,
 GnomeVFSFileSize *bytes_written_return,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result;
  int success;
  HANDLE handle;
  DWORD bytes_written;

  D("----------------- synce_write() ------------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  handle = GPOINTER_TO_UINT(method_handle);

  D("CeWriteFile()\n");
  MUTEX_LOCK (mutex);
  success = CeWriteFile
    (
     handle,
     buffer,
     num_bytes,
     &bytes_written,
     NULL
    );

  if (!success)
    {
      result = gnome_vfs_result_from_rapi();
    }
  else if (bytes_written == 0)
    {
      *bytes_written_return = 0;
      result = GNOME_VFS_ERROR_EOF;
    }
  else
    {
      *bytes_written_return = bytes_written;
      result = GNOME_VFS_OK;
    }
  MUTEX_UNLOCK (mutex);

exit:
  D("----------------- synce_write() end --------------\n");
  return result;
}/*}}}*/

static GnomeVFSResult
synce_seek (GnomeVFSMethod *method,
            GnomeVFSMethodHandle *method_handle,
            GnomeVFSSeekPosition whence,
            GnomeVFSFileOffset offset,
            GnomeVFSContext *context)
{
  GnomeVFSResult result;
  DWORD retval, move_method;
  HANDLE handle;

  D("------------------ synce_seek() -------------------\n");

  if ((result = initialize_rapi ()) != GNOME_VFS_OK)
    goto exit;

  handle = GPOINTER_TO_UINT(method_handle);

  switch (whence) {
    case GNOME_VFS_SEEK_START:
      move_method = FILE_BEGIN;
      break;
    case GNOME_VFS_SEEK_CURRENT:
      move_method = FILE_CURRENT;
      break;
    case GNOME_VFS_SEEK_END:
      move_method = FILE_END;
      break;
    default:
      g_assert_not_reached ();
  }

  D("CeSetFilePointer()\n");
  MUTEX_LOCK (mutex);
  retval = CeSetFilePointer (handle,
                             offset,
                             NULL,
                             move_method);

  if (retval == 0xFFFFFFFF)
    {
      result = gnome_vfs_result_from_rapi ();
    }
  else
    {
      result = GNOME_VFS_OK;
    }

  MUTEX_UNLOCK (mutex);

exit:
  D("------------------ synce_seek() end --------------\n");
  return result;
}

static GnomeVFSResult
synce_tell (GnomeVFSMethod *method,
            GnomeVFSMethodHandle *method_handle,
            GnomeVFSFileSize *offset_return)
{
  GnomeVFSResult result;
  DWORD retval;
  HANDLE handle;

  D("------------------ synce_tell() -------------------\n");

  if ((result = initialize_rapi ()) != GNOME_VFS_OK)
    goto exit;

  handle = GPOINTER_TO_UINT(method_handle);

  D("CeSetFilePointer()\n");
  MUTEX_LOCK (mutex);
  retval = CeSetFilePointer (handle,
                             0,
                             NULL,
                             FILE_CURRENT);

  if (retval == 0xFFFFFFFF)
    {
      result = gnome_vfs_result_from_rapi ();
    }
  else
    {
      result = GNOME_VFS_OK;
      *offset_return = retval;
    }

  MUTEX_UNLOCK (mutex);

exit:
  D("------------------ synce_tell() end --------------\n");
  return result;
}

static GnomeVFSResult synce_open_dir/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSMethodHandle **method_handle,
 GnomeVFSURI *uri,
 GnomeVFSFileInfoOptions options,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result;
  char *location;
  char *new_path;
  DIR_HANDLE *dh;
  CE_FIND_DATA *data = NULL;
  int optionflags;
  unsigned int itemcount;
  WCHAR *tempwstr;
  int index;

  D("------------------ synce_open_dir() --------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  switch ((index = get_location(uri, &location)))
    {
    case INDEX_DEVICE:
      dh = (DIR_HANDLE*) g_malloc0(sizeof(DIR_HANDLE));

      dh->index = INDEX_DEVICE;
      dh->location = NULL;
#if SHOW_APPLICATIONS
      dh->itemcount = 3;  /* Applications, Documents, Filesystem */
#else
      dh->itemcount = 2;  /* Documents, Filesystem */
#endif
      dh->count = 0;
      dh->data = NULL;

      *((DIR_HANDLE **) method_handle) = dh;

      result = GNOME_VFS_OK;
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      if (location && location[0] != '\0')
        {
          result = GNOME_VFS_ERROR_NOT_FOUND;
          goto exit;
        }

      dh = (DIR_HANDLE*) g_malloc0(sizeof(DIR_HANDLE));

      dh->index = INDEX_APPLICATIONS;
      dh->location = NULL;
      dh->itemcount = 0;
      dh->count = 0;
      dh->data = NULL;

      *((DIR_HANDLE **) method_handle) = dh;
      result = GNOME_VFS_OK;
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  if(!location)
    {
      location = g_strdup("*");
    }
  else if(location[1] == '\0')
    {
      g_free(location);
      location = g_strdup("*");
    }
  else if(location[strlen(location)-1] == '\\')
    {
      new_path = g_strjoin(NULL, location, "*", NULL);
      g_free(location);
      location = new_path;
    }
  else
    {
      new_path = g_strjoin(NULL, location, "\\*", NULL);
      g_free(location);
      location = new_path;
    }

  optionflags =	
    FAF_ATTRIBUTES
    | FAF_CREATION_TIME
    | FAF_LASTACCESS_TIME
    | FAF_LASTWRITE_TIME
    | FAF_NAME
    | FAF_SIZE_LOW
    | FAF_OID;

  tempwstr = wstr_from_utf8(location);

  MUTEX_LOCK (mutex); 
  if (!CeFindAllFiles(tempwstr, optionflags, &itemcount, &data))
    {
      g_free(location);
      wstr_free_string(tempwstr);
      result = gnome_vfs_result_from_rapi();
      MUTEX_UNLOCK (mutex); 
      goto exit;
    }
  MUTEX_UNLOCK (mutex); 

  wstr_free_string(tempwstr);

  dh = (DIR_HANDLE*) g_malloc0(sizeof(DIR_HANDLE));

  dh->index = index;
  dh->location = location;
  dh->itemcount = itemcount;
  dh->count = 0;
  dh->data = data;

  *((DIR_HANDLE **) method_handle) = dh;

  result = GNOME_VFS_OK;

exit:
  D("------------------ synce_open_dir() end ----------\n");
  return result;
}/*}}}*/

static GnomeVFSResult synce_close_dir/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSMethodHandle *method_handle,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result;
  DIR_HANDLE *dh;
  HRESULT hr; 

  D("----------------- synce_close_dir() --------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  dh = (DIR_HANDLE *)method_handle;

  g_free(dh->location);

  MUTEX_LOCK (mutex); 
  hr = CeRapiFreeBuffer(dh->data);
  MUTEX_UNLOCK (mutex); 

  if (FAILED(hr)) {
    synce_trace("CeRapiFreeBuffer(): failed");
    result = GNOME_VFS_ERROR_GENERIC;
  } else {
    result = GNOME_VFS_OK;
  }
  g_free(dh);

exit:
  D("----------------- synce_close_dir() end ----------\n");
  return result;
}/*}}}*/


static time_t
convert_time(const FILETIME* filetime)
{
  struct tm tm_time;
  TIME_FIELDS time_fields;

  time_fields_from_filetime(filetime, &time_fields);

  tm_time.tm_sec = time_fields.Second;
  tm_time.tm_min = time_fields.Minute;
  tm_time.tm_hour = time_fields.Hour;
  tm_time.tm_mday = time_fields.Day;
  tm_time.tm_mon = time_fields.Month - 1;

  /* time_fields.Year count starts at 1601, what happens in mktime with -ve year ? */
  if (time_fields.Year < 1901) {
    tm_time.tm_year = 1;
  } else {
    tm_time.tm_year = time_fields.Year - 1900;
  }

  tm_time.tm_wday = 0;
  tm_time.tm_yday = 0;
  tm_time.tm_isdst = -1;

  return mktime(&tm_time);
}

/* Have to fix the mime-type conversion */
static void get_file_attributes/*{{{*/
(
 GnomeVFSFileInfo *file_info,
 CE_FIND_DATA *entry
 )
{
  file_info->valid_fields =
    GNOME_VFS_FILE_INFO_FIELDS_TYPE
    | GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS
    | GNOME_VFS_FILE_INFO_FIELDS_FLAGS
    | GNOME_VFS_FILE_INFO_FIELDS_SIZE
    | GNOME_VFS_FILE_INFO_FIELDS_ATIME
    | GNOME_VFS_FILE_INFO_FIELDS_MTIME
    | GNOME_VFS_FILE_INFO_FIELDS_CTIME
    | GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE
    | GNOME_VFS_FILE_INFO_FIELDS_IDS
    | GNOME_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE;

  file_info->name = wstr_to_utf8(entry->cFileName);

  file_info->flags = GNOME_VFS_FILE_FLAGS_NONE;

  if(entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    file_info->size = 0;
  else
    file_info->size = entry->nFileSizeLow;

  file_info->atime = convert_time(&entry->ftLastAccessTime);
  file_info->mtime = convert_time(&entry->ftLastWriteTime);
  file_info->ctime = convert_time(&entry->ftCreationTime);;

  if(entry->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
    file_info->permissions = 0444;
  else
    file_info->permissions = 0664;

  if(entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      file_info->permissions |= 0111;
      file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
      file_info->mime_type = g_strdup("x-directory/normal");
    }
  else
    {
      file_info->type = GNOME_VFS_FILE_TYPE_REGULAR;
      file_info->mime_type = g_strdup( gnome_vfs_get_mime_type_for_name(file_info->name) );
    }

  file_info->uid = getuid();
  file_info->gid = getgid();

  /* Should make read/write faster but with less feedback */
  file_info->io_block_size = 64*1024;

  return;
}/*}}}*/

static void get_special_directory_attributes(GnomeVFSFileInfo *file_info, const gchar *name)/*{{{*/
{
  file_info->valid_fields =
    GNOME_VFS_FILE_INFO_FIELDS_TYPE
    | GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS
    | GNOME_VFS_FILE_INFO_FIELDS_FLAGS
    | GNOME_VFS_FILE_INFO_FIELDS_SIZE
    | GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE
    | GNOME_VFS_FILE_INFO_FIELDS_IDS;

  file_info->name = g_strdup(name);

  file_info->flags = GNOME_VFS_FILE_FLAGS_NONE;

  file_info->size = 0;

  file_info->permissions = 0775;

  file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
  file_info->mime_type = g_strdup("x-directory/normal");

  file_info->uid = getuid();
  file_info->gid = getgid();
}/*}}}*/

static void get_root_attributes(GnomeVFSFileInfo *file_info)/*{{{*/
{
  get_special_directory_attributes(file_info, "Mobile Device");
}/*}}}*/

static GnomeVFSResult synce_read_dir/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSMethodHandle *method_handle,
 GnomeVFSFileInfo *file_info,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result;
  DIR_HANDLE *dh;

  D("------------------ synce_read_dir() --------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  dh = (DIR_HANDLE *)method_handle;

  if(dh->itemcount == dh->count)
    {
      D("synce_read_dir: Ok: End of file");
      result = GNOME_VFS_ERROR_EOF;
      goto exit;
    }

  synce_trace("index = %i, location = '%s'", dh->index, dh->location);

  switch (dh->index)
    {
    case INDEX_DEVICE:

      switch (dh->count)
        {
        case 0:
          get_special_directory_attributes(file_info, NAME_DOCUMENTS);
          break;

        case 1:
          get_special_directory_attributes(file_info, NAME_FILESYSTEM);
          break;

#if SHOW_APPLICATIONS
        case 2:
          get_special_directory_attributes(file_info, NAME_APPLICATIONS);
          break;
#endif

        default:
	  result = GNOME_VFS_ERROR_CORRUPTED_DATA;
          break;
        }
      break;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      return GNOME_VFS_ERROR_CORRUPTED_DATA;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      if (dh->location == NULL)    /* should not happen ? */
        {
          if (dh->index == INDEX_DOCUMENTS)
            get_special_directory_attributes(file_info, NAME_DOCUMENTS);
          else if (dh->index == INDEX_FILESYSTEM)
            get_special_directory_attributes(file_info, NAME_FILESYSTEM);
          else
            result = GNOME_VFS_ERROR_CORRUPTED_DATA;
        }
      else
	{
	  get_file_attributes(file_info, dh->data+dh->count);
	}
      break;

    default:
      result = GNOME_VFS_ERROR_CORRUPTED_DATA;
      break;
    }

  dh->count++;

  if(result != GNOME_VFS_OK)
    {
      D("synce_read_dir: Failed\n");
      goto exit;
    }

  D("synce_read_dir: Name: %s\n", file_info->name);
  D("synce_read_dir: Mime-type: %s\n", file_info->mime_type);
  D("synce_read_dir: Ok\n");

exit:
  D("------------------ synce_read_dir() end ----------\n");
  return result;
}/*}}}*/

static GnomeVFSResult synce_get_file_info/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSURI *uri,
 GnomeVFSFileInfo *file_info,
 GnomeVFSFileInfoOptions options,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result;
  gchar *location;
  CE_FIND_DATA entry;
  WCHAR *tempwstr;

  D("------------- synce_get_file_info() --------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  switch (get_location(uri, &location))
    {
    case INDEX_DEVICE:
      if (location == NULL)
        {
          get_root_attributes(file_info);
          result = GNOME_VFS_OK;
        }
      else
        result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      get_special_directory_attributes(file_info, NAME_APPLICATIONS);
      result = GNOME_VFS_OK;
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  if(!location)
    {
      synce_error("%s: NULL location, should not happen", G_STRFUNC);
      result = GNOME_VFS_ERROR_INVALID_URI;
      goto exit;
    }
  if(strcmp(location, "\\") == 0)
    {
      D("synce_get_file_info: Root folder\n");
      get_root_attributes(file_info);
      result = GNOME_VFS_OK;
      goto exit;
    }

  if(location[strlen(location)-1] == '\\')
    {
      D("synce_get_file_info: Folder with \\\n");
      /* This is a directory, chop of the \ to make it readable to FindFirstFile */
      location[strlen(location)-1] = '\0';
    }
  else
    {
      D("synce_get_file_info: Folder/File\n");
    }

  D("synce_get_file_info: CeFindFirstFile()\n");
  tempwstr = wstr_from_utf8(location);

  MUTEX_LOCK (mutex); 
  if(CeFindFirstFile(tempwstr, &entry) == INVALID_HANDLE_VALUE)
    {
      result = gnome_vfs_result_from_rapi();
      D("synce_get_file_info: Failed\n");
    }
  else
    {
      get_file_attributes(file_info, &entry);

      D("synce_get_file_info: Name: %s\n", file_info->name);
      D("synce_get_file_info: Mime-type: %s\n", file_info->mime_type);
      D("synce_get_file_info: Ok\n");

      result = GNOME_VFS_OK;
    }
  MUTEX_UNLOCK (mutex); 

  wstr_free_string(tempwstr);
  g_free(location);

exit:
  D("------------- synce_get_file_info() end ----------\n");
  return result;
}/*}}}*/

static GnomeVFSResult synce_get_file_info_from_handle/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSMethodHandle *method_handle,
 GnomeVFSFileInfo *file_info,
 GnomeVFSFileInfoOptions options,
 GnomeVFSContext *context
 )
{
  D("--------- synce_get_file_info_from_handle --------\n");
  D("------- synce_get_file_info_from_handle end ------\n");

  return GNOME_VFS_ERROR_NOT_SUPPORTED;
}/*}}}*/

static gboolean synce_is_local/*{{{*/
(
 GnomeVFSMethod *method,
 const GnomeVFSURI *uri
 )
{
  return FALSE;
}/*}}}*/

static GnomeVFSResult synce_mkdir/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSURI *uri,
 guint perm,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result;
  gchar *location;
  WCHAR *tempwstr;

  D("------------------ synce_mkdir() -----------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  switch (get_location(uri, &location))
    {
    case INDEX_DEVICE:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  if(!location) {
    synce_error("%s: NULL location, should not happen", G_STRFUNC);
    result = GNOME_VFS_ERROR_INVALID_URI;
    goto exit;
  }

  tempwstr = wstr_from_utf8(location);

  result = GNOME_VFS_OK;
  D("CeCreateDirectory()\n");
  MUTEX_LOCK (mutex);
  if(!CeCreateDirectory(tempwstr, NULL))
    {
      result = gnome_vfs_result_from_rapi();
    }

  MUTEX_UNLOCK (mutex);

  g_free(location);
  wstr_free_string(tempwstr);

exit:
  D("---------------- synce_mkdir() end ---------------\n");
  return result;
}/*}}}*/

static GnomeVFSResult synce_rmdir/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSURI *uri,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result;
  gchar *location;
  WCHAR *tempwstr;

  D("----------------- synce_rmdir() ------------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  switch (get_location(uri, &location))
    {
    case INDEX_DEVICE:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  if(!location) {
    synce_error("%s: NULL location, should not happen", G_STRFUNC);
    result = GNOME_VFS_ERROR_INVALID_URI;
    goto exit;
  }

  tempwstr = wstr_from_utf8(location);
  D("CeRemoveDirectory()\n");
  MUTEX_LOCK (mutex);
  if (CeRemoveDirectory(tempwstr))
    result = GNOME_VFS_OK;
  else
    result = gnome_vfs_result_from_rapi();

  MUTEX_UNLOCK (mutex);

  wstr_free_string(tempwstr);
  g_free(location);

exit:
  D("----------------- synce_rmdir() end --------------\n");
  return result;
}/*}}}*/

static GnomeVFSResult synce_move/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSURI *old_uri,
 GnomeVFSURI *new_uri,
 gboolean force_replace,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result;
  gchar *new_location;
  WCHAR *new_wstr;
  gchar *old_location;
  WCHAR *old_wstr;
  int success;
  int err;

  D("------------------ synce_move() ------------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  switch (get_location(new_uri, &new_location))
    {
    case INDEX_DEVICE:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  if (!new_location) {
    synce_error("%s: NULL new_location, should not happen", G_STRFUNC);
    result = GNOME_VFS_ERROR_INVALID_URI;
    goto exit;
  }

  switch (get_location(old_uri, &old_location))
    {
    case INDEX_DEVICE:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  if (!old_location) {
    g_free(new_location);
    synce_error("%s: NULL old_location, should not happen", G_STRFUNC);
    result = GNOME_VFS_ERROR_INVALID_URI;
    goto exit;
  }

  old_wstr = wstr_from_utf8(old_location);
  new_wstr = wstr_from_utf8(new_location);

  D("CeMoveFile()\n");
  MUTEX_LOCK (mutex);
  success = CeMoveFile(old_wstr, new_wstr);
  err = CeGetLastError();

  if (err == ERROR_ALREADY_EXISTS) /* If the destination file exists we end up here */
    {
      /* if the user wants we delete the dest file and moves the source there */
      if (force_replace)
        {
          success = CeDeleteFile(new_wstr);

          if (success)
            success = CeMoveFile(old_wstr, new_wstr);
        }
    }

  if (success)
    result = GNOME_VFS_OK;
  else
    result = gnome_vfs_result_from_rapi();

  MUTEX_UNLOCK (mutex);

  g_free(old_location);
  g_free(new_location);
  wstr_free_string(old_wstr);
  wstr_free_string(new_wstr);

exit:
  D("---------------- synce_move() end ------------------\n");
  return result;
}/*}}}*/


static GnomeVFSResult/*{{{*/
synce_set_file_info (GnomeVFSMethod *method,
    GnomeVFSURI *uri,
    const GnomeVFSFileInfo *info,
    GnomeVFSSetFileInfoMask mask,
    GnomeVFSContext *context)
{
  GnomeVFSURI *parent_uri, *new_uri;
  GnomeVFSResult result;

  D("-------------- synce_set_file_info() ---------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  /* For now, we only support changing the name. */
  if ((mask & ~(GNOME_VFS_SET_FILE_INFO_NAME)) != 0) {
    result = GNOME_VFS_ERROR_NOT_SUPPORTED;
    goto exit;
  }

  /*
   * Returns an error for incoming names with "/" characters in them,
   * instead of moving the file.
   */
  if (g_utf8_strchr(info->name,-1,'/') != NULL) {
    result = GNOME_VFS_ERROR_BAD_PARAMETERS;
    goto exit;
  }

  /* Share code with do_move. */
  parent_uri = gnome_vfs_uri_get_parent (uri);
  if (parent_uri == NULL) {
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
  }
  new_uri = gnome_vfs_uri_append_file_name (parent_uri, info->name);
  gnome_vfs_uri_unref (parent_uri);
  result = synce_move (method, uri, new_uri, FALSE, context);
  gnome_vfs_uri_unref (new_uri);

exit:
  D("------------ synce_set_file_info() end ---------\n");
  return result;
}/*}}}*/

static GnomeVFSResult synce_unlink/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSURI *uri,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result;
  gchar *location;
  WCHAR *tempwstr;

  D("----------------- synce_unlink() ---------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  switch (get_location(uri, &location))
    {
    case INDEX_DEVICE:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  tempwstr = wstr_from_utf8(location);

  if (!location) {
    synce_error("%s: NULL location, should not happen", G_STRFUNC);
    result = GNOME_VFS_ERROR_INVALID_URI;
    goto exit;
  }

  D("CeDeleteFile()\n");
  MUTEX_LOCK (mutex);

  if (CeDeleteFile(tempwstr))
    result = GNOME_VFS_OK;
  else
    result = gnome_vfs_result_from_rapi();

  MUTEX_UNLOCK (mutex);

  wstr_free_string(tempwstr);
  g_free(location);

exit:
  D("--------------- synce_unlink() end -------------\n");
  return result;
}/*}}}*/

static GnomeVFSResult synce_same_fs/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSURI *a,
 GnomeVFSURI *b,
 gboolean *same_fs_return,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result;
  gchar *location_a, *location_b;
  gint fs_a, fs_b, index_a, index_b;

  D("----------------- synce_same_fs() --------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  index_a = get_location(a, &location_a);
  if (index_a == INDEX_INVALID) {
    result = GNOME_VFS_ERROR_INVALID_URI;
    goto exit;
  }
#if SHOW_APPLICATIONS
  if (index_a == INDEX_APPLICATIONS) {
    result = GNOME_VFS_ERROR_NOT_PERMITTED;
    goto exit;
  }
#endif

  index_b = get_location(b, &location_b);
  if (index_b == INDEX_INVALID) {
    result = GNOME_VFS_ERROR_INVALID_URI;
    goto exit;
  }
#if SHOW_APPLICATIONS
  if (index_b == INDEX_APPLICATIONS) {
    result = GNOME_VFS_ERROR_NOT_PERMITTED;
    goto exit;
  }
#endif

  fs_a = FS_DEVICE;
  if (index_a == INDEX_FILESYSTEM) {
    gchar **split = g_strsplit(location_a, "\\", 0);

    if (split && split[0] && split[1]) {
      if (strcmp(split[1], NAME_SD_CARD) == 0)
	fs_a = FS_SD_CARD;
    }
    if (split && split[0] && split[1]) {
      if (strcmp(split[1], NAME_ROM_STORAGE) == 0)
	fs_a = FS_ROM_STORAGE;
    }
    g_strfreev(split);
  }

  fs_b = FS_DEVICE;
  if (index_b == INDEX_FILESYSTEM) {
    gchar **split = g_strsplit(location_b, "\\", 0);

    if (split && split[0] && split[1]) {
      if (strcmp(split[1], NAME_SD_CARD) == 0)
	fs_b = FS_SD_CARD;
    }

    if (split && split[0] && split[1]) {
      if (strcmp(split[1], NAME_ROM_STORAGE) == 0)
	fs_b = FS_ROM_STORAGE;
    }

    g_strfreev(split);
  }

  if (fs_a == fs_b)
    *same_fs_return = TRUE;
  else
    *same_fs_return = FALSE;

  result = GNOME_VFS_OK;

exit:
  if (location_a) g_free(location_a);
  if (location_b) g_free(location_b);

  D("--------------- synce_same_fs() end ------------\n");
  return result;
}/*}}}*/

static GnomeVFSResult
synce_get_volume_free_space
(
 GnomeVFSMethod *method,
 const GnomeVFSURI *uri,
 GnomeVFSFileSize *free_space
 )
{
  GnomeVFSResult result;
  STORE_INFORMATION store;
  gchar *location;
  gint index;

  D("-------------- synce_get_volume_free_space() ---------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    goto exit;

  index = get_location(uri, &location);
  if (index == INDEX_INVALID) {
    result = GNOME_VFS_ERROR_INVALID_URI;
    goto exit;
  }
#if SHOW_APPLICATIONS
  if (index == INDEX_APPLICATIONS) {
    result = GNOME_VFS_ERROR_NOT_PERMITTED;
    goto exit;
  }
#endif

  if (index == INDEX_FILESYSTEM) {
    gchar **split = g_strsplit(location, "\\", 0);

    if (split && split[0] && split[1]) {
      if (strcmp(split[1], NAME_SD_CARD) == 0) {
	result = GNOME_VFS_ERROR_NOT_SUPPORTED;
	goto exit;
      }

      if (strcmp(split[1], NAME_ROM_STORAGE) == 0) {
	result = GNOME_VFS_ERROR_NOT_SUPPORTED;
	goto exit;
      }

    }
    g_strfreev(split);
  }

  if (CeGetStoreInformation(&store)) {
    *free_space = store.dwFreeSize;
    result = GNOME_VFS_OK;
  } else {
    synce_error("%s: Failed to get store information", G_STRFUNC);
    result = gnome_vfs_result_from_rapi();
  }

exit:
  if (location) g_free(location);

  D("------------ synce_get_volume_free_space() end ---------\n");
  return result;
}

static GnomeVFSMethod method = 
{
  sizeof(GnomeVFSMethod),
  synce_open,
  synce_create,
  synce_close,
  synce_read,
  synce_write,
  synce_seek,
  synce_tell,
  NULL,	/* truncate_handle */
  synce_open_dir,
  synce_close_dir,
  synce_read_dir,
  synce_get_file_info,
  NULL, /* synce_get_file_info_from_handle */
  synce_is_local,
  synce_mkdir,
  synce_rmdir,
  synce_move,
  synce_unlink,
  synce_same_fs,
  synce_set_file_info, 
  NULL,	/* truncate */
  NULL, /* find_directory */
  NULL, /* create_symbolic_link */
  NULL, /* monitor_add */
  NULL, /* monitor_cancel */
  NULL, /* file_control */
  NULL, /* forget_cache */
  synce_get_volume_free_space
};

GnomeVFSMethod *vfs_module_init(const char *method_name, const char *args)
{
  if (!mutex)
    mutex = MUTEX_NEW ();

  synce_log_use_syslog();
  synce_debug("vfs_module_init(method_name = '%s', args = '%s')",
      method_name, args);

  return &method;
}

void vfs_module_shutdown(GnomeVFSMethod *method)
{
  MUTEX_LOCK (mutex);
  CeRapiUninit();
  MUTEX_UNLOCK (mutex);

  MUTEX_FREE(mutex);
}
