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

/* Seems like this function is not declared in the gnome-vfs headers? */
const char *
gnome_vfs_mime_type_from_name_or_default (const char *filename, const char *defaultv);

#define SHOW_APPLICATIONS   0

#define D(x...) synce_debug(x)

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
    {GNOME_VFS_ERROR_NOT_PERMITTED,       ERROR_ACCESS_DENIED,  0}
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

      synce_trace("HRESULT = %08x", hr);

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
      synce_trace("error = %i", error);

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
          fprintf(stderr, "Unable to initialize RAPI: %s\n", 
              synce_strerror(hr));
          return GNOME_VFS_ERROR_LOGIN_FAILED;
        }

      initialized_rapi = TRUE;
    }

  return GNOME_VFS_OK;
}/*}}}*/

static int get_location(GnomeVFSURI *uri, gchar **location)/*{{{*/
{
  int result = INDEX_INVALID;
  *location = NULL;
  gchar **path = NULL;

  path = g_strsplit(gnome_vfs_unescape_string(gnome_vfs_uri_get_path(uri),"\\"), "/", 0);

    {
      int i;
      for (i = 0; path[i]; i++)
        synce_trace("path[%i] = '%s'", i, path[i]);
    }

  if (!path || !path[0] || !path[1])
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
      result = INDEX_DEVICE;
      *location = NULL;
    }


  synce_trace("index = %i, location = '%s'", result, *location);

  g_strfreev(path);
  return result;
}/*}}}*/

static int vfs_to_synce_mode(GnomeVFSOpenMode mode, int *open_mode, int *create_mode)/*{{{*/
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

  D("--------------------------------------------\n");
  D("synce_open()\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

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
      synce_trace("location = '%s'", location);
      // Continue after switch() */
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

  g_free(location);
  wstr_free_string(wide_path);

	*(method_handle_return) = GUINT_TO_POINTER(handle);

  if((handle == INVALID_HANDLE_VALUE) || (synce_open_mode & GENERIC_WRITE))
    result = gnome_vfs_result_from_rapi();
  else
    result = GNOME_VFS_OK;

  MUTEX_UNLOCK (mutex);

exit:
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
  char *tempstring;

  D("--------------------------------------------\n");
  D("synce_create()\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

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
      synce_trace("location = '%s'", location);
      // Continue after switch() */
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  D("location: %s\n", location);

  wide_path = wstr_from_utf8(location);
  tempstring = wstr_to_utf8(wide_path);
  D("wide_path: %s\n", tempstring);
  g_free(tempstring);

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

  g_free(location);
  wstr_free_string(wide_path);

	*(method_handle_return) = GUINT_TO_POINTER(handle);

  if((handle == INVALID_HANDLE_VALUE) || (synce_open_mode & GENERIC_WRITE))
    result = gnome_vfs_result_from_rapi();
  else
    result = GNOME_VFS_OK;

  MUTEX_UNLOCK (mutex);

exit:
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

  D("------------------- synce_close() -----------------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

	handle = GPOINTER_TO_UINT(method_handle);

  D("synce_close: CeCloseHandle()\n");
  MUTEX_LOCK (mutex);
  success = CeCloseHandle(handle);

  if (success)
    result = GNOME_VFS_OK;
  else
    result = gnome_vfs_result_from_rapi();

  MUTEX_UNLOCK (mutex);

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

  D("------------------ synce_read() ---------------------\n");
  D("synce_read()\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

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
      result = GNOME_VFS_ERROR_EOF;
    }
  else
    {
      *bytes_read_return = read_return;
      result = GNOME_VFS_OK;
    }

  MUTEX_UNLOCK (mutex);
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

  D("----------------- synce_write() -------------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

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
      D("synce_write: Failed\n");
      result = gnome_vfs_result_from_rapi();
    }
  else if (bytes_written == 0)
    {
      D("synce_write: End of file\n");
      *bytes_written_return = 0;
      result = GNOME_VFS_ERROR_EOF;
    }
  else
    {
      *bytes_written_return = bytes_written;
      result = GNOME_VFS_OK;
    }
  MUTEX_UNLOCK (mutex);

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
    return result;

  handle = (HANDLE) method_handle;

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
      D("synce_seek: Failed\n");
      result = gnome_vfs_result_from_rapi ();
    }
  else
    {
      result = GNOME_VFS_OK;
    }

  MUTEX_UNLOCK (mutex);

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
    return result;

  handle = (HANDLE) method_handle;

  D("CeSetFilePointer()\n");

  MUTEX_LOCK (mutex);

  retval = CeSetFilePointer (handle,
                             0,
                             NULL,
                             FILE_CURRENT);

  if (retval == 0xFFFFFFFF)
    {
      D("synce_seek: Failed\n");
      result = gnome_vfs_result_from_rapi ();
    }
  else
    {
      result = GNOME_VFS_OK;
      *offset_return = retval;
    }

  MUTEX_UNLOCK (mutex);

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

  D("------------------ synce_open_dir() -------------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

  switch ((index = get_location(uri, &location)))
    {
    case INDEX_DEVICE:
      synce_trace("location = '%s'", location);
      dh = (DIR_HANDLE*) g_malloc0(sizeof(DIR_HANDLE));

      dh->index = INDEX_DEVICE;
      dh->location = NULL;
      dh->itemcount = 3;  /* Applications, Documents, Filesystem */
      dh->count = 0;
      dh->data = NULL;

      *((DIR_HANDLE **) method_handle) = dh;

      result = GNOME_VFS_OK;
      goto exit;

    case INDEX_APPLICATIONS:
      synce_trace("location = '%s'", location);
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

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      synce_trace("location = '%s'", location);
      // Continue after switch() */
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  D("synce_open_dir: location: %s\n", location);

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
      new_path = (char *)g_malloc(sizeof(char)*(strlen(location)+2));
      strcpy(new_path, location);
      new_path[strlen(location)] = '*';
      new_path[strlen(location)+1] = '\0';
      g_free(location);
      location = new_path;
    }
  else
    {
      new_path = (char *)g_malloc(sizeof(char)*(strlen(location)+3));
      strcpy(new_path, location);
      new_path[strlen(location)] = '\\';
      new_path[strlen(location)+1] = '*';
      new_path[strlen(location)+2] = '\0';
      g_free(location);
      location = new_path;
    }

  optionflags =	
    FAF_ATTRIBUTES
    | FAF_CREATION_TIME
    | FAF_LASTACCESS_TIME
    | FAF_LASTWRITE_TIME
    | FAF_NAME
    |	FAF_SIZE_LOW
    |	FAF_OID;

  tempwstr = wstr_from_utf8(location);

  MUTEX_LOCK (mutex); 
  if (!CeFindAllFiles(tempwstr, optionflags, &itemcount, &data))
    {
      g_free(location);
      wstr_free_string(tempwstr);
      result = gnome_vfs_result_from_rapi();
      MUTEX_UNLOCK (mutex); 
      return result;
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

  D("----------------- synce_close_dir() ------------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

  dh = (DIR_HANDLE *)method_handle;

  g_free(dh->location);

  MUTEX_LOCK (mutex); 
  hr = CeRapiFreeBuffer(dh->data);
  MUTEX_UNLOCK (mutex); 

  if (FAILED(hr))
    {
      D("synce_close_dir: Failed\n");
      D("----------------- synce_close_dir() end --------------\n");
      return GNOME_VFS_ERROR_GENERIC;
    }
  else
    {
      D("synce_close_dir: Ok\n");
      D("----------------- synce_close_dir() end --------------\n");
      return GNOME_VFS_OK;
    }
  g_free(dh);
}/*}}}*/

/* Have to fix the mime-type conversion */
static BOOL get_file_attributes/*{{{*/
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
    | GNOME_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE;

  file_info->name = wstr_to_utf8(entry->cFileName);

  if(entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    file_info->size = 0;
  else
    file_info->size = entry->nFileSizeLow;

  file_info->atime = filetime_to_unix_time(&entry->ftLastAccessTime);
  file_info->mtime = filetime_to_unix_time(&entry->ftLastWriteTime);
  file_info->ctime = filetime_to_unix_time(&entry->ftCreationTime);;

  file_info->permissions = 0664;

  if(entry->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
    file_info->permissions |= 0222;

  if(entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      file_info->permissions |= 0111;
      file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
      file_info->mime_type = g_strdup("x-directory/normal");
    }
  else
    {
      file_info->type = GNOME_VFS_FILE_TYPE_REGULAR;
      file_info->mime_type = (char *)
        gnome_vfs_mime_type_from_name_or_default
        ((const char *)file_info->name, 
         (const char *)GNOME_VFS_MIME_TYPE_UNKNOWN);
      file_info->mime_type = g_strdup(file_info->mime_type);
    }

  file_info->uid = getuid();
  file_info->gid = getgid();

  /* Should make read/write faster but with less feedback */
  file_info->io_block_size = 64*1024;

  return TRUE;
}/*}}}*/

static void get_special_directory_attributes(GnomeVFSFileInfo *file_info, const gchar *name)/*{{{*/
{
  file_info->valid_fields =
    GNOME_VFS_FILE_INFO_FIELDS_TYPE
    | GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS
    | GNOME_VFS_FILE_INFO_FIELDS_FLAGS
    | GNOME_VFS_FILE_INFO_FIELDS_SIZE;

  file_info->name = g_strdup(name);

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
  BOOL success = FALSE;

  D("------------------ synce_read_dir() --------------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

  dh = (DIR_HANDLE *)method_handle;

  if(dh->itemcount == dh->count)
    {
      D("synce_read_dir: Synce Error: %d %s\n", 38, "End of file");
      D("synce_read_dir: Ok\n");
      D("------------------ synce_read_dir() end ----------------\n");
      return GNOME_VFS_ERROR_EOF;
    }

  synce_trace("index = %i, location = '%s'", dh->index, dh->location);

  switch (dh->index)
    {
    case INDEX_DEVICE:
      success = TRUE;

      switch (dh->count)
        {
        case 0:
          get_special_directory_attributes(file_info, NAME_DOCUMENTS);
          break;

        case 1:
          get_special_directory_attributes(file_info, NAME_FILESYSTEM);
          break;

        case 2:
#if SHOW_APPLICATIONS
          get_special_directory_attributes(file_info, NAME_APPLICATIONS);
#else
          return GNOME_VFS_ERROR_EOF;
#endif
          break;

        default:
          success = FALSE;
          break;
        }
      break;

    case INDEX_APPLICATIONS:
      return GNOME_VFS_ERROR_CORRUPTED_DATA;

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      if (dh->location == NULL)
        {
          success = TRUE;
          if (dh->index == INDEX_DOCUMENTS)
            get_special_directory_attributes(file_info, NAME_DOCUMENTS);
          else if (dh->index == INDEX_FILESYSTEM)
            get_special_directory_attributes(file_info, NAME_FILESYSTEM);
          else
            success = FALSE;
        }
      else
        success = get_file_attributes(file_info, dh->data+dh->count);
      break;
    }

  dh->count++;

  D("synce_read_dir: Error %d: %s\n", 0, "Success");

  if(!success)
    {
      D("synce_read_dir: Failed\n");
      D("------------------ synce_read_dir() end ----------------\n");
      return GNOME_VFS_ERROR_CORRUPTED_DATA;
    }
  else
    {
      D("synce_read_dir: Name: %s\n", file_info->name);
      D("synce_read_dir: Mime-type: %s\n", file_info->mime_type);
      D("synce_read_dir: Ok\n");
      D("------------------ synce_read_dir() end ----------------\n");
      return GNOME_VFS_OK;
    }
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
  int err;

  D("------------- synce_get_file_info() -----------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

  switch (get_location(uri, &location))
    {
    case INDEX_DEVICE:
      synce_trace("location = '%s'", location);
      if (location == NULL)
        {
          get_root_attributes(file_info);
          result = GNOME_VFS_OK;
        }
      else
        result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;

    case INDEX_APPLICATIONS:
      get_special_directory_attributes(file_info, NAME_APPLICATIONS);
      result = GNOME_VFS_OK;
      goto exit;

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      // Continue after switch() */
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }
  D("%s\n", location);

  if(!location)
    {
      D("synce_get_file_info Synce Error: %d %s\n", 2, "ERROR_FILE_NOT_FOUND");
      D("synce_get_file_info Failed\n");
      D("------------- synce_get_file_info() end --------------\n");
      return GNOME_VFS_ERROR_INVALID_URI;
    }
  else if(strcmp(location, "\\") == 0)
    {
      D("synce_get_file_info: Root folder\n");

      get_root_attributes(file_info);

      D("synce_get_file_info: Ok\n");
      D("------------- synce_get_file_info() end --------------\n");
      return GNOME_VFS_OK;
    }
  else if(location[strlen(location)-1] == '\\')
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
      err = CeGetLastError();
      MUTEX_UNLOCK (mutex); 

      D("synce_get_file_info: Error %d: %s\n", err, synce_strerror(err));

      wstr_free_string(tempwstr);
      g_free(location);

      D("synce_get_file_info: Failed\n");
      D("------------- synce_get_file_info() end --------------\n");

      result = gnome_vfs_result_from_rapi();
    }
  else
    {
      MUTEX_UNLOCK (mutex); 
      get_file_attributes(file_info, &entry);

      wstr_free_string(tempwstr);
      g_free(location);

      D("synce_get_file_info: Name: %s\n", file_info->name);
      D("synce_get_file_info: Mime-type: %s\n", file_info->mime_type);
      D("synce_get_file_info: Ok\n");
      D("------------- synce_get_file_info() end --------------\n");

      result = GNOME_VFS_OK;
    }

exit:
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
  D("synce_get_files_info_from_handle\n");

  return GNOME_VFS_ERROR_ACCESS_DENIED;
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

  D("--------------------------------------------\n");
  D("synce_mkdir()\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

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
      // Continue after switch() */
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  if(!location)
    {
      D("Synce Error: %d %s\n", 13, "ERROR_INVALID_DATA");
      D("Failed\n");
      D("--------------------------------------------\n");
      return GNOME_VFS_ERROR_INVALID_URI;
    }

  D("CeCreateDirectory()\n");
  tempwstr = wstr_from_utf8(location);
  MUTEX_LOCK (mutex);
  if(!CeCreateDirectory(tempwstr, NULL))
    {
      g_free(location);
      wstr_free_string(tempwstr);
      result = gnome_vfs_result_from_rapi();
      MUTEX_UNLOCK (mutex);
      return result;
    }

  g_free(location);
  wstr_free_string(tempwstr);

  D("Error %d: %s\n", CeGetLastError(), synce_strerror(CeGetLastError()));
  D("Ok\n");
  D("--------------------------------------------\n");

  MUTEX_UNLOCK (mutex);
  result = GNOME_VFS_OK;

exit:
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
  int err;
  int success;

  D("----------------- synce_rmdir() -----------------------\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

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
      // Continue after switch() */
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  if(!location)
    {
      D("synce_rmdir: Synce Error: %d %s\n", 13, "ERROR_INVALID_DATA");
      D("synce_rmdir: Failed\n");
      D("----------------- synce_rmdir() end -------------------\n");
      return GNOME_VFS_ERROR_INVALID_URI;
    }

  D("CeRemoveDirectory()\n");
  tempwstr = wstr_from_utf8(location);
  MUTEX_LOCK (mutex);
  success = CeRemoveDirectory(tempwstr);
  err = CeGetLastError();
  wstr_free_string(tempwstr);

  g_free(location);
  D("synce_rmdir: success %d\n", success);

  /* In librapi tool prmdir result is used to see if things went OK */
  if (success) 
    result = GNOME_VFS_OK;
  else
    result = gnome_vfs_result_from_rapi();

  MUTEX_UNLOCK (mutex);

exit:
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

  D("--------------------------------------------\n");
  D("synce_move()\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

  switch (get_location(new_uri, &new_location))
    {
    case INDEX_DEVICE:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;

    case INDEX_APPLICATIONS:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      // Continue after switch() */
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }


  if(!new_location)
    {
      D("Failed\n");
      return GNOME_VFS_ERROR_INVALID_URI;
    }

  switch (get_location(old_uri, &old_location))
    {
    case INDEX_DEVICE:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;

    case INDEX_APPLICATIONS:
      result = GNOME_VFS_ERROR_NOT_PERMITTED;
      goto exit;

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      // Continue after switch() */
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }


  if(!old_location)
    {
      g_free(new_location);
      D("Synce Error: %d %s\n", 13, "ERROR_INVALID_DATA");
      D("Failed\n");
      D("--------------------------------------------\n");
      return GNOME_VFS_ERROR_INVALID_URI;
    }

  D("CeMoveFile()\n");
  old_wstr = wstr_from_utf8(old_location);
  new_wstr = wstr_from_utf8(new_location);
  MUTEX_LOCK (mutex);
  success = CeMoveFile(old_wstr, new_wstr);
  err = CeGetLastError();

  if (err == ERROR_ALREADY_EXISTS) /* If the destination file exists we end up here */
    {
      /* if the user wants we delete the dest file and moves the source there */
      if (force_replace)
        {
          MUTEX_LOCK (mutex);
          success = CeDeleteFile(new_wstr);
          MUTEX_UNLOCK (mutex);

          if (success)
            success = CeMoveFile(old_wstr, new_wstr);
        }
    }

  g_free(old_location);
  g_free(new_location);
  wstr_free_string(old_wstr);
  wstr_free_string(new_wstr);

  if (success)
    result = GNOME_VFS_OK;
  else
    result = gnome_vfs_result_from_rapi();

  MUTEX_UNLOCK (mutex);

exit:
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

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

  /* For now, we only support changing the name. */
  if ((mask & ~(GNOME_VFS_SET_FILE_INFO_NAME)) != 0) {
      return GNOME_VFS_ERROR_NOT_SUPPORTED;
  }

  /*
   * Returns an error for incoming names with "/" characters in them,
   * instead of moving the file.
   */
  if (g_utf8_strchr(info->name,-1,'/') != NULL) {
      return GNOME_VFS_ERROR_BAD_PARAMETERS;
  }

  /* Share code with do_move. */
  parent_uri = gnome_vfs_uri_get_parent (uri);
  if (parent_uri == NULL) {
      return GNOME_VFS_ERROR_NOT_FOUND;
  }
  new_uri = gnome_vfs_uri_append_file_name (parent_uri, info->name);
  gnome_vfs_uri_unref (parent_uri);
  result = synce_move (method, uri, new_uri, FALSE, context);
  gnome_vfs_uri_unref (new_uri);
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
  int success;
  WCHAR *tempwstr;

  D("--------------------------------------------\n");
  D("synce_unlink()\n");

  if ((result = initialize_rapi()) != GNOME_VFS_OK)
    return result;

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
      // Continue after switch() */
      break;

    default:
      result = GNOME_VFS_ERROR_NOT_FOUND;
      goto exit;
    }

  tempwstr = wstr_from_utf8(location);

  if (!location)
    {
      g_free(location);
      wstr_free_string(tempwstr);
      D("Synce Error: %d %s\n", 13, "ERROR_INVALID_DATA");
      D("Failed\n");
      D("--------------------------------------------\n");
      return GNOME_VFS_ERROR_INVALID_URI;
    }

  D("CeDeleteFile()\n");
  MUTEX_LOCK (mutex);
  success = CeDeleteFile(tempwstr);
  MUTEX_UNLOCK (mutex);

  wstr_free_string(tempwstr);
  g_free(location);

  if (success)
    {
      D("Ok\n");
      D("--------------------------------------------\n");
      result = GNOME_VFS_OK;
    }
  else
    {
      D("Failed\n");
      D("--------------------------------------------\n");
      result = gnome_vfs_result_from_rapi();
    }

exit:
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
  *same_fs_return = 1;

  return GNOME_VFS_OK;
}/*}}}*/

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
  NULL,	/* truncate */
  synce_open_dir,
  synce_close_dir,
  synce_read_dir,
  synce_get_file_info,
  synce_get_file_info_from_handle,
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
