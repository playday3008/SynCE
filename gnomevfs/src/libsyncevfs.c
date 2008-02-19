#include "config.h"

#include <string.h>
#include <rapi.h>
#include <synce_log.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-module.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#define SHOW_APPLICATIONS   0

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

/* structures for info held by gnomevfs between calls */

typedef struct _VFS_DIR_HANDLE
{
  GnomeVFSURI *uri;
  RapiConnection *rapi_conn;
  int index;
  char *location;
  CE_FIND_DATA *data;
  int itemcount;
  int count;
} VFS_DIR_HANDLE;

typedef struct _VFS_FILE_HANDLE
{
  GnomeVFSURI *uri;
  RapiConnection *rapi_conn;
  HANDLE handle;
} VFS_FILE_HANDLE;

/* static data, mutex */

static GMutex * mutex = NULL;


typedef struct _ErrorCodeTriple
{
  GnomeVFSResult gnome_vfs_result;
  DWORD error;
  HRESULT hresult;
} ErrorCodeTriple;

/* hack, these should be in synce.h */
#ifndef ERROR_NOT_SAME_DEVICE
#define ERROR_NOT_SAME_DEVICE         17
#endif

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


static gchar *get_host_from_uri(const GnomeVFSURI *uri)/*{{{*/
{
  gchar *host = NULL;

  host = gnome_vfs_unescape_string(gnome_vfs_uri_get_host_name(uri),"\\");

  synce_debug("%s: host = '%s'", G_STRFUNC, host);

  return host;
}/*}}}*/


static GnomeVFSResult gnome_vfs_result_from_rapi(gboolean *connection_error)/*{{{*/
{
  GnomeVFSResult result = GNOME_VFS_ERROR_GENERIC;
  HRESULT hr;
  unsigned error;
  int i;

  if (connection_error)
    *connection_error = FALSE;

  hr    = CeRapiGetError();
  error = CeGetLastError();

  if (FAILED(hr))
    {
      /* This is a connection error, so we signal to close the connection */
      if (connection_error)
	*connection_error = TRUE;

      synce_error("HRESULT = %08x: %s", hr, synce_strerror(hr));

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
      synce_info("error = %i: %s", error, synce_strerror(error));

      for (i = 0; i < sizeof(error_codes)/sizeof(ErrorCodeTriple); i++)
        {
          if (error_codes[i].error == error)
            {
              result = error_codes[i].gnome_vfs_result;
              break;
            }
        }
    }

  synce_debug("GnomeVFSResult = %s", gnome_vfs_result_to_string(result));
  return result;
}/*}}}*/


static GnomeVFSResult initialize_rapi(const GnomeVFSURI *uri, RapiConnection **rapi_conn)/*{{{*/
{
  gchar *host;
  RapiConnection *connection = NULL;
  GnomeVFSResult result = GNOME_VFS_OK;
  HRESULT hr;

  host = get_host_from_uri(uri);
  synce_debug("%s: initialize for host %s", G_STRFUNC, host);

  connection = rapi_connection_from_name(host);
  if (!connection) {
    synce_warning("Unable to initialize RAPI for host '%s': connection failed", host);
    result = GNOME_VFS_ERROR_LOGIN_FAILED;
    goto exit;
  }

  rapi_connection_select(connection);
  hr = CeRapiInit();

  if (FAILED(hr))
    {
      synce_warning("Unable to initialize RAPI for host '%s': %s", host, synce_strerror(hr));
      rapi_connection_destroy(connection);
      result = GNOME_VFS_ERROR_LOGIN_FAILED;
    } else {
      synce_debug("%s: new connection for host '%s' successful", G_STRFUNC, host);
      *rapi_conn = connection;
    }

exit:
  g_free(host);
  return result;
}/*}}}*/


static gint get_location(const GnomeVFSURI *uri, gchar **location)/*{{{*/
{
  gint result = INDEX_INVALID;
  gchar **path = NULL;

  path = g_strsplit(gnome_vfs_unescape_string(gnome_vfs_uri_get_path(uri),"\\"), "/", 0);

  {
    int i;
    for (i = 0; path[i]; i++)
      synce_debug("%s: path[%i] = '%s'", G_STRFUNC, i, path[i]);
  }

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

  synce_debug("%s: index = %i, location = '%s'", G_STRFUNC, result, *location);

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
  gchar *location = NULL;
  WCHAR *wide_path = NULL;
  gint synce_open_mode, synce_create_mode;
  HANDLE handle;
  VFS_FILE_HANDLE *fh = NULL;
  RapiConnection *rapi_conn = NULL;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

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

  vfs_to_synce_mode(mode, &synce_open_mode, &synce_create_mode);

  MUTEX_LOCK (mutex);
  if ((result = initialize_rapi(uri, &rapi_conn)) != GNOME_VFS_OK) {
    MUTEX_UNLOCK (mutex);
    goto exit;
  }

  wide_path = wstr_from_utf8(location);

  synce_debug("%s: CeCreateFile()", G_STRFUNC);
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

  wstr_free_string(wide_path);
  if(handle == INVALID_HANDLE_VALUE) {
    result = gnome_vfs_result_from_rapi(NULL);
    CeRapiUninit();
    rapi_connection_destroy(rapi_conn);
  } else {
    fh = (VFS_FILE_HANDLE*) g_malloc0(sizeof(VFS_FILE_HANDLE));
    fh->uri = gnome_vfs_uri_dup(uri);
    fh->handle = handle;
    fh->rapi_conn = rapi_conn;
    *((VFS_FILE_HANDLE**)method_handle_return) = fh;

    result = GNOME_VFS_OK;
  }

  MUTEX_UNLOCK (mutex);

exit:
  g_free(location);
  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  gchar *location = NULL;
  WCHAR *wide_path = NULL;
  gint synce_open_mode, synce_create_mode;
  HANDLE handle;
  VFS_FILE_HANDLE *fh = NULL;
  RapiConnection *rapi_conn = NULL;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

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

  vfs_to_synce_mode(mode, &synce_open_mode, &synce_create_mode);

  MUTEX_LOCK (mutex);
  if ((result = initialize_rapi(uri, &rapi_conn)) != GNOME_VFS_OK) {
    MUTEX_UNLOCK (mutex);
    goto exit;
  }

  wide_path = wstr_from_utf8(location);

  synce_debug("%s: CeCreateFile()", G_STRFUNC);
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

  wstr_free_string(wide_path);
  if(handle == INVALID_HANDLE_VALUE) {
    result = gnome_vfs_result_from_rapi(NULL);
    CeRapiUninit();
    rapi_connection_destroy(rapi_conn);
  } else {
    fh = (VFS_FILE_HANDLE*) g_malloc0(sizeof(VFS_FILE_HANDLE));
    fh->uri = gnome_vfs_uri_dup(uri);
    fh->handle = handle;
    fh->rapi_conn = rapi_conn;
    *((VFS_FILE_HANDLE**)method_handle_return) = fh;

    result = GNOME_VFS_OK;
  }

  MUTEX_UNLOCK (mutex);

exit:
  g_free(location);
  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  VFS_FILE_HANDLE *fh;
  gint success;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

  fh = (VFS_FILE_HANDLE *)method_handle;

  synce_debug("%s: CeCloseHandle()", G_STRFUNC);
  MUTEX_LOCK (mutex);
  rapi_connection_select(fh->rapi_conn);
  success = CeCloseHandle(fh->handle);

  if (success)
    result = GNOME_VFS_OK;
  else
    result = gnome_vfs_result_from_rapi(NULL);

  CeRapiUninit();
  rapi_connection_destroy(fh->rapi_conn);

  MUTEX_UNLOCK (mutex);

  gnome_vfs_uri_unref(fh->uri);
  g_free(fh);

  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  gint success;
  DWORD read_return;
  VFS_FILE_HANDLE *fh;
  gboolean conn_err;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

  fh = (VFS_FILE_HANDLE *)method_handle;

  MUTEX_LOCK (mutex);
  rapi_connection_select(fh->rapi_conn);

  synce_debug("%s: CeReadFile()", G_STRFUNC);
  success = CeReadFile
    (
     fh->handle,
     buffer,
     num_bytes,
     &read_return,
     NULL
    );

  if (!success)
    {
      result = gnome_vfs_result_from_rapi(&conn_err);
      if (conn_err) {
	CeRapiUninit();
	rapi_connection_destroy(fh->rapi_conn);
      }
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

  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  gint success;
  VFS_FILE_HANDLE *fh;
  DWORD bytes_written;
  gboolean conn_err;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

  fh = (VFS_FILE_HANDLE *)method_handle;

  MUTEX_LOCK (mutex);
  rapi_connection_select(fh->rapi_conn);

  synce_debug("%s: CeWriteFile()", G_STRFUNC);
  success = CeWriteFile
    (
     fh->handle,
     buffer,
     num_bytes,
     &bytes_written,
     NULL
    );

  if (!success)
    {
      result = gnome_vfs_result_from_rapi(&conn_err);
      if (conn_err) {
	CeRapiUninit();
	rapi_connection_destroy(fh->rapi_conn);
      }
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

  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  VFS_FILE_HANDLE *fh;
  gboolean conn_err;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

  fh = (VFS_FILE_HANDLE *)method_handle;

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

  MUTEX_LOCK (mutex);
  rapi_connection_select(fh->rapi_conn);

  synce_debug("%s: CeSetFilePointer()", G_STRFUNC);
  retval = CeSetFilePointer (fh->handle,
                             offset,
                             NULL,
                             move_method);

  if (retval == 0xFFFFFFFF)
    {
      result = gnome_vfs_result_from_rapi (&conn_err);
      if (conn_err) {
	CeRapiUninit();
	rapi_connection_destroy(fh->rapi_conn);
      }
    }
  else
    {
      result = GNOME_VFS_OK;
    }

  MUTEX_UNLOCK (mutex);

  synce_debug("%s: ------ leaving -------", G_STRFUNC);
  return result;
}

static GnomeVFSResult
synce_tell (GnomeVFSMethod *method,
            GnomeVFSMethodHandle *method_handle,
            GnomeVFSFileSize *offset_return)
{
  GnomeVFSResult result;
  DWORD retval;
  VFS_FILE_HANDLE *fh;
  gboolean conn_err;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

  fh = (VFS_FILE_HANDLE *)method_handle;

  MUTEX_LOCK (mutex);
  rapi_connection_select(fh->rapi_conn);

  synce_debug("%s: CeSetFilePointer()", G_STRFUNC);
  retval = CeSetFilePointer (fh->handle,
                             0,
                             NULL,
                             FILE_CURRENT);

  if (retval == 0xFFFFFFFF)
    {
      result = gnome_vfs_result_from_rapi (&conn_err);
      if (conn_err) {
	CeRapiUninit();
	rapi_connection_destroy(fh->rapi_conn);
      }
    }
  else
    {
      result = GNOME_VFS_OK;
      *offset_return = retval;
    }

  MUTEX_UNLOCK (mutex);

  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  gchar *location = NULL;
  gchar *new_path = NULL;
  VFS_DIR_HANDLE *dh = NULL;
  CE_FIND_DATA *data = NULL;
  gint optionflags;
  guint itemcount;
  WCHAR *tempwstr = NULL;
  gint index;
  RapiConnection *rapi_conn = NULL;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

  MUTEX_LOCK (mutex); 
  if ((result = initialize_rapi(uri, &rapi_conn)) != GNOME_VFS_OK) {
    MUTEX_UNLOCK (mutex);
    goto exit;
  }

  switch ((index = get_location(uri, &location)))
    {
    case INDEX_DEVICE:
      MUTEX_UNLOCK (mutex); 
      dh = (VFS_DIR_HANDLE*) g_malloc0(sizeof(VFS_DIR_HANDLE));

      dh->index = INDEX_DEVICE;
      dh->location = NULL;
#if SHOW_APPLICATIONS
      dh->itemcount = 3;  /* Applications, Documents, Filesystem */
#else
      dh->itemcount = 2;  /* Documents, Filesystem */
#endif
      dh->count = 0;
      dh->data = NULL;
      dh->uri = gnome_vfs_uri_dup(uri);
      dh->rapi_conn = rapi_conn;

      *((VFS_DIR_HANDLE**)method_handle) = dh;

      result = GNOME_VFS_OK;
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      MUTEX_UNLOCK (mutex); 
      if (location && location[0] != '\0')
        {
          result = GNOME_VFS_ERROR_NOT_FOUND;
          goto exit;
        }

      dh = (VFS_DIR_HANDLE*) g_malloc0(sizeof(DIR_HANDLE));

      dh->index = INDEX_APPLICATIONS;
      dh->location = NULL;
      dh->itemcount = 0;
      dh->count = 0;
      dh->data = NULL;
      dh->uri = gnome_vfs_uri_dup(uri);
      dh->rapi_conn = rapi_conn;

      *(method_handle) = dh;
      result = GNOME_VFS_OK;
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      MUTEX_UNLOCK (mutex); 
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

  synce_debug("%s: CeFindAllFiles()", G_STRFUNC);
  if (!CeFindAllFiles(tempwstr, optionflags, &itemcount, &data))
    {
      wstr_free_string(tempwstr);
      result = gnome_vfs_result_from_rapi(NULL);
      CeRapiUninit();
      rapi_connection_destroy(rapi_conn);
      MUTEX_UNLOCK (mutex); 
      goto exit;
    }
  MUTEX_UNLOCK (mutex); 

  wstr_free_string(tempwstr);

  dh = (VFS_DIR_HANDLE*) g_malloc0(sizeof(VFS_DIR_HANDLE));

  dh->index = index;
  dh->location = g_strdup(location);
  dh->itemcount = itemcount;
  dh->count = 0;
  dh->data = data;
  dh->uri = gnome_vfs_uri_dup(uri);
  dh->rapi_conn = rapi_conn;

  *((VFS_DIR_HANDLE**)method_handle) = dh;

  result = GNOME_VFS_OK;

exit:
  g_free(location);
  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  VFS_DIR_HANDLE *dh;
  HRESULT hr; 

  synce_debug("%s: ------ entering ------", G_STRFUNC);

  dh = (VFS_DIR_HANDLE *)method_handle;

  MUTEX_LOCK (mutex);
  if (dh->rapi_conn) {
    rapi_connection_select(dh->rapi_conn);
    CeRapiUninit();
    rapi_connection_destroy(dh->rapi_conn);
  }

  hr = CeRapiFreeBuffer(dh->data);

  MUTEX_UNLOCK (mutex); 

  if (FAILED(hr)) {
    synce_warning("CeRapiFreeBuffer(): failed");
    result = GNOME_VFS_ERROR_GENERIC;
  } else {
    result = GNOME_VFS_OK;
  }

  g_free(dh->location);
  gnome_vfs_uri_unref(dh->uri);
  g_free(dh);

  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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

static void get_root_attributes(GnomeVFSFileInfo *file_info, gchar *hostname)/*{{{*/
{
  gchar *display_name;
  if (hostname)
    display_name = g_strjoin(NULL, "Mobile Device (", hostname, ")", NULL);
  else
    display_name = g_strdup("Mobile Device");

  get_special_directory_attributes(file_info, display_name);

  g_free(display_name);
}/*}}}*/

static GnomeVFSResult synce_read_dir/*{{{*/
(
 GnomeVFSMethod *method,
 GnomeVFSMethodHandle *method_handle,
 GnomeVFSFileInfo *file_info,
 GnomeVFSContext *context
 )
{
  GnomeVFSResult result = GNOME_VFS_OK;
  VFS_DIR_HANDLE *dh;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

  dh = (VFS_DIR_HANDLE *)method_handle;

  if(dh->itemcount == dh->count)
    {
      synce_debug("%s: Ok: End of file", G_STRFUNC);
      result = GNOME_VFS_ERROR_EOF;
      goto exit;
    }

  synce_debug("%s: index = %i, location = '%s'", G_STRFUNC, dh->index, dh->location);

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
      synce_debug("%s: Failed", G_STRFUNC);
      goto exit;
    }

  synce_debug("%s: Name: %s", G_STRFUNC, file_info->name);
  synce_debug("%s: Mime-type: %s", G_STRFUNC, file_info->mime_type);
  synce_debug("%s: Ok", G_STRFUNC);

exit:
  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  gchar *location = NULL, *host = NULL;
  CE_FIND_DATA entry;
  WCHAR *tempwstr = NULL;
  RapiConnection *rapi_conn = NULL;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

  switch (get_location(uri, &location))
    {
    case INDEX_DEVICE:
      if (location == NULL)
        {
	  host = get_host_from_uri(uri);
	  get_root_attributes(file_info, host);
	  g_free(host);
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
      synce_debug("%s: Root folder", G_STRFUNC);
      host = get_host_from_uri(uri);
      get_root_attributes(file_info, host);
      g_free(host);
      result = GNOME_VFS_OK;
      goto exit;
    }

  if(location[strlen(location)-1] == '\\')
    {
      synce_debug("%s: Folder with \\", G_STRFUNC);
      /* This is a directory, chop of the \ to make it readable to FindFirstFile */
      location[strlen(location)-1] = '\0';
    }
  else
    {
      synce_debug("%s: Folder/File", G_STRFUNC);
    }

  MUTEX_LOCK (mutex);
  if ((result = initialize_rapi(uri, &rapi_conn)) != GNOME_VFS_OK) {
    MUTEX_UNLOCK (mutex);
    goto exit;
  }

  synce_debug("%s: CeFindFirstFile()", G_STRFUNC);
  tempwstr = wstr_from_utf8(location);

  if(CeFindFirstFile(tempwstr, &entry) == INVALID_HANDLE_VALUE)
    {
      result = gnome_vfs_result_from_rapi(NULL);
      synce_debug("%s: Failed", G_STRFUNC);
    }
  else
    {
      get_file_attributes(file_info, &entry);

      synce_debug("%s: Name: %s", G_STRFUNC, file_info->name);
      synce_debug("%s: Mime-type: %s", G_STRFUNC, file_info->mime_type);
      synce_debug("%s: Ok", G_STRFUNC);

      result = GNOME_VFS_OK;
    }
  CeRapiUninit();
  rapi_connection_destroy(rapi_conn);

  MUTEX_UNLOCK (mutex); 

  wstr_free_string(tempwstr);

exit:
  g_free(location);
  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  synce_debug("%s: ------ entering ------", G_STRFUNC);
  synce_debug("%s: ------ leaving -------", G_STRFUNC);

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
  gchar *location = NULL;
  WCHAR *tempwstr = NULL;
  RapiConnection *rapi_conn = NULL;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

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

  MUTEX_LOCK (mutex);
  if ((result = initialize_rapi(uri, &rapi_conn)) != GNOME_VFS_OK) {
    MUTEX_UNLOCK (mutex);
    goto exit;
  }

  result = GNOME_VFS_OK;
  synce_debug("%s: CeCreateDirectory()", G_STRFUNC);
  if(!CeCreateDirectory(tempwstr, NULL))
    {
      result = gnome_vfs_result_from_rapi(NULL);
    }

  CeRapiUninit();
  rapi_connection_destroy(rapi_conn);
  MUTEX_UNLOCK (mutex);

  wstr_free_string(tempwstr);

exit:
  g_free(location);
  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  gchar *location = NULL;
  WCHAR *tempwstr = NULL;
  RapiConnection *rapi_conn = NULL;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

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

  MUTEX_LOCK (mutex);
  if ((result = initialize_rapi(uri, &rapi_conn)) != GNOME_VFS_OK) {
    MUTEX_UNLOCK (mutex);
    goto exit;
  }

  tempwstr = wstr_from_utf8(location);

  synce_debug("%s: CeRemoveDirectory()", G_STRFUNC);
  if (CeRemoveDirectory(tempwstr))
    result = GNOME_VFS_OK;
  else
    result = gnome_vfs_result_from_rapi(NULL);

  CeRapiUninit();
  rapi_connection_destroy(rapi_conn);
  MUTEX_UNLOCK (mutex);

  wstr_free_string(tempwstr);

exit:
  g_free(location);
  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  gchar *new_location = NULL;
  WCHAR *new_wstr = NULL;
  gchar *old_location = NULL;
  WCHAR *old_wstr = NULL;
  int success;
  int err;
  RapiConnection *rapi_conn = NULL;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

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
    synce_error("%s: NULL old_location, should not happen", G_STRFUNC);
    result = GNOME_VFS_ERROR_INVALID_URI;
    goto exit;
  }

  MUTEX_LOCK (mutex);
  if ((result = initialize_rapi(old_uri, &rapi_conn)) != GNOME_VFS_OK) {
    MUTEX_UNLOCK (mutex);
    goto exit;
  }

  old_wstr = wstr_from_utf8(old_location);
  new_wstr = wstr_from_utf8(new_location);

  synce_debug("%s: CeMoveFile()", G_STRFUNC);
  success = CeMoveFile(old_wstr, new_wstr);
  err = CeGetLastError();

  if (err == ERROR_ALREADY_EXISTS) /* If the destination file exists we end up here */
    {
      /* if the user wants we delete the dest file and move the source there */
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
    result = gnome_vfs_result_from_rapi(NULL);

  CeRapiUninit();
  rapi_connection_destroy(rapi_conn);
  MUTEX_UNLOCK (mutex);

  wstr_free_string(old_wstr);
  wstr_free_string(new_wstr);

exit:
  g_free(old_location);
  g_free(new_location);
  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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

  synce_debug("%s: ------ entering ------", G_STRFUNC);

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
  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  gchar *location = NULL;
  WCHAR *tempwstr = NULL;
  RapiConnection *rapi_conn = NULL;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

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

  if (!location) {
    synce_error("%s: NULL location, should not happen", G_STRFUNC);
    result = GNOME_VFS_ERROR_INVALID_URI;
    goto exit;
  }

  MUTEX_LOCK (mutex);
  if ((result = initialize_rapi(uri, &rapi_conn)) != GNOME_VFS_OK) {
    MUTEX_UNLOCK (mutex);
    goto exit;
  }
  tempwstr = wstr_from_utf8(location);

  synce_debug("%s: CeDeleteFile()", G_STRFUNC);
  if (CeDeleteFile(tempwstr))
    result = GNOME_VFS_OK;
  else
    result = gnome_vfs_result_from_rapi(NULL);

  CeRapiUninit();
  rapi_connection_destroy(rapi_conn);
  MUTEX_UNLOCK (mutex);

  wstr_free_string(tempwstr);

exit:
  g_free(location);
  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  gchar *location_a = NULL, *location_b = NULL;
  gint fs_a, fs_b, index_a, index_b;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

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
  g_free(location_a);
  g_free(location_b);

  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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
  gchar *location = NULL;
  gint index;
  RapiConnection *rapi_conn = NULL;

  synce_debug("%s: ------ entering ------", G_STRFUNC);

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

  MUTEX_LOCK (mutex);
  if ((result = initialize_rapi(uri, &rapi_conn)) != GNOME_VFS_OK) {
    MUTEX_UNLOCK (mutex);
    goto exit;
  }

  if (CeGetStoreInformation(&store)) {
    *free_space = store.dwFreeSize;
    result = GNOME_VFS_OK;
  } else {
    synce_error("%s: Failed to get store information", G_STRFUNC);
    result = gnome_vfs_result_from_rapi(NULL);
  }

  CeRapiUninit();
  rapi_connection_destroy(rapi_conn);
  MUTEX_UNLOCK (mutex);
exit:
  g_free(location);

  synce_debug("%s: ------ leaving -------", G_STRFUNC);
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

#ifdef WITH_VERBOSE_DEBUG
  synce_log_set_level(SYNCE_LOG_LEVEL_HIGHEST);
#endif

  synce_log_use_syslog();
  synce_debug("%s: method_name = '%s', args = '%s'", G_STRFUNC,
      method_name, args);

  return &method;
}

void vfs_module_shutdown(GnomeVFSMethod *method)
{
  MUTEX_FREE(mutex);
}
