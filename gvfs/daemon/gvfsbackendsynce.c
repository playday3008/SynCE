/* Gvfs backend for Synce
 * 
 * Copyright (C) 2008 Mark Ellis <mark@mpellis.org.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Mark Ellis <mark@mpellis.org.uk>
 */

#include <config.h>

#include <glib/gi18n.h>
#include <synce_log.h>
#include <rapi.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>

#include "gvfsbackendsynce.h"
#include "gvfsjobopenforread.h"
#include "gvfsjobread.h"
#include "gvfsjobseekread.h"
#include "gvfsjobopenforwrite.h"
#include "gvfsjobwrite.h"
#include "gvfsjobseekwrite.h"
#include "gvfsjobsetdisplayname.h"
#include "gvfsjobqueryinfo.h"
#include "gvfsjobqueryfsinfo.h"
#include "gvfsjobqueryattributes.h"
#include "gvfsjobenumerate.h"
#include "gvfsdaemonprotocol.h"
#include "gvfsdaemonutils.h"
#include "gvfskeyring.h"

#define MOUNT_ICON_NAME "synce-color"

struct _GVfsBackendSynce
{
  GVfsBackend		backend;

  gchar *device_name;
  RapiConnection *rapi_conn;
  GMutex * mutex;
};

G_DEFINE_TYPE (GVfsBackendSynce, g_vfs_backend_synce, G_VFS_TYPE_BACKEND)


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


typedef struct _ErrorCodeTriple
{
  GIOErrorEnum gio_error_result;
  DWORD error;
  HRESULT hresult;
} ErrorCodeTriple;


/* these should be in synce.h
   put here for error codes not in latest libsynce release */
#ifndef ERROR_NOT_SAME_DEVICE
#define ERROR_NOT_SAME_DEVICE         17
#endif


static ErrorCodeTriple error_codes[] =
  {
    {G_IO_ERROR_FAILED,                   ERROR_SUCCESS,             S_OK           },            /* is this required ? */
    {G_IO_ERROR_FAILED,                   0,                         E_FAIL         },
    {G_IO_ERROR_FAILED,                   0,                         E_UNEXPECTED   },
    {G_IO_ERROR_NOT_FOUND,                ERROR_FILE_NOT_FOUND,      0              },
    {G_IO_ERROR_NOT_FOUND,                ERROR_PATH_NOT_FOUND,      0              },
    {G_IO_ERROR_INVALID_ARGUMENT,         ERROR_INVALID_NAME,        0              },
    {G_IO_ERROR_EXISTS,                   ERROR_FILE_EXISTS,         0              },
    {G_IO_ERROR_EXISTS,                   ERROR_ALREADY_EXISTS,      0              },
    {G_IO_ERROR_NOT_EMPTY,                ERROR_DIR_NOT_EMPTY,       0              },
    {G_IO_ERROR_PERMISSION_DENIED,        ERROR_ACCESS_DENIED,       0              },
    {G_IO_ERROR_FAILED,                   ERROR_TOO_MANY_OPEN_FILES, 0              },
    {G_IO_ERROR_NOT_FOUND,                ERROR_NO_MORE_FILES,       0              },
    {G_IO_ERROR_NO_SPACE,                 ERROR_DISK_FULL,           0              },
    {G_IO_ERROR_FAILED,                   ERROR_NOT_SAME_DEVICE,     0              }
  };


void
log_to_syslog(const gchar *log_domain,
	      GLogLevelFlags log_level,
	      const gchar *message,
	      gpointer user_data)
{
  gint max_log_level;
  if (user_data)
    max_log_level = *(gint*)user_data;
  else
    max_log_level = 3;

  gboolean is_fatal = (log_level & G_LOG_FLAG_FATAL) != 0;
  gsize msg_len = 0;
  gchar msg_prefix[25];
  gchar *msg;
  gint priority;

  switch (log_level & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_ERROR:
      if (max_log_level < 1) return;
      strcpy (msg_prefix, "ERROR");
      priority = LOG_ERR;
      break;
    case G_LOG_LEVEL_CRITICAL:
      if (max_log_level < 2) return;
      strcpy (msg_prefix, "CRITICAL");
      priority = LOG_WARNING;
      break;
    case G_LOG_LEVEL_WARNING:
      if (max_log_level < 3) return;
      strcpy (msg_prefix, "WARNING");
      priority = LOG_WARNING;
      break;
    case G_LOG_LEVEL_MESSAGE:
      if (max_log_level < 4) return;
      strcpy (msg_prefix, "Message");
      priority = LOG_INFO;
      break;
    case G_LOG_LEVEL_INFO:
      if (max_log_level < 5) return;
      strcpy (msg_prefix, "INFO");
      priority = LOG_INFO;
      break;
    case G_LOG_LEVEL_DEBUG:
      if (max_log_level < 6) return;
      strcpy (msg_prefix, "DEBUG");
      priority = LOG_DEBUG;
      break;
    default:
      if (max_log_level < 6) return;
      strcpy (msg_prefix, "LOG");
      priority = LOG_DEBUG;
      break;
    }
  if (log_level & G_LOG_FLAG_RECURSION)
    strcat (msg_prefix, " (recursed)");
  strcat(msg_prefix, ": ");

  if (log_domain) {
    msg_len = strlen(msg_prefix) + strlen(log_domain) + 1;
    msg = malloc(msg_len + 1);
    strcpy(msg, log_domain);
    strcat(msg, "-");
  } else {
    msg_len = strlen(msg_prefix);
    msg = malloc(msg_len + 1);
    strcpy(msg, "");
  }
  strcat(msg, msg_prefix);

  if (!message) {
    msg_len = msg_len + 14;
    msg = realloc(msg, msg_len + 1);
    strcat(msg, "(NULL) message");
  } else {
    msg_len = msg_len + strlen(message);
    msg = realloc(msg, msg_len + 1);
    strcat(msg, message);
  }

  if (is_fatal) {
    msg_len = msg_len + 13;
    msg = realloc(msg, msg_len + 1);
    strcat(msg, ", aborting...");
  }

  syslog(priority, "%s", msg);
  free(msg);
}


static GError*
g_error_from_rapi(gboolean *connection_error)
{
  GError *result = NULL;
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

      g_warning("HRESULT = %08x: %s", hr, synce_strerror(hr));

      for (i = 0; i < sizeof(error_codes)/sizeof(ErrorCodeTriple); i++)
        {
          if (error_codes[i].hresult == hr)
            {
	      result = g_error_new(G_IO_ERROR,
				   error_codes[i].gio_error_result,
				   synce_strerror(hr));
              break;
            }
        }
    }
  else
    {
      g_message("error = %i: %s", error, synce_strerror(error));

      for (i = 0; i < sizeof(error_codes)/sizeof(ErrorCodeTriple); i++)
        {
          if (error_codes[i].error == error)
            {
	      result = g_error_new(G_IO_ERROR,
				   error_codes[i].gio_error_result,
				   synce_strerror(error));
              break;
            }
        }
    }

  return result;
}


static gint
get_location(const gchar *pathname,
	     gchar **location)
{
  gint result = INDEX_INVALID;
  gchar **path = NULL;

  path = g_strsplit(pathname, "/", 0);

  /* path[0] is an empty string before the root / */

  gint i;
  for (i = 0; path[i]; i++)
    g_debug("%s: path[%i] = '%s'", G_STRFUNC, i, path[i]);

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
      /* XXX: what is the name of this on non-english systems? */
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

  g_debug("%s: index = %i, location = '%s'", G_STRFUNC, result, *location);

  g_strfreev(path);
  return result;
}


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


static void
get_file_attributes(GVfsBackendSynce *backend,
                    GFileInfo *info,
                    CE_FIND_DATA *entry,
                    GFileAttributeMatcher *matcher)
{
  GTimeVal t;
  GIcon *icon;
  gchar *content_type;
  gchar *display_name;
  gchar *basename;

  g_debug("%s: get filename from wide str", G_STRFUNC);
  gchar *filename = wstr_to_utf8(entry->cFileName);

  g_debug("%s: get basename", G_STRFUNC);
  basename = g_path_get_basename(filename);
  g_file_info_set_name (info, basename);

  g_debug("%s: set display name", G_STRFUNC);
  if (g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME))
    {
      if (strcmp (basename, "/") == 0)
        display_name = g_strdup_printf (_("Mobile Device %s"), backend->device_name);
      else
	{
	  display_name = g_filename_display_name (basename);

	  if (strstr (display_name, "\357\277\275") != NULL)
	    {
	      gchar *p = display_name;
	      display_name = g_strconcat (display_name, _(" (invalid encoding)"), NULL);
	      g_free (p);
	    }
	}

      g_file_info_set_display_name (info, display_name);
      g_free (display_name);
    }

  g_debug("%s: set edit name", G_STRFUNC);
  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME))
    {
      char *edit_name = g_filename_display_name (basename);
      g_file_info_set_edit_name (info, edit_name);
      g_free (edit_name);
    }

  g_debug("%s: set hidden", G_STRFUNC);
  if (entry->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
    g_file_info_set_is_hidden(info, TRUE);

  g_debug("%s: set type", G_STRFUNC);
  if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
    g_file_info_set_size (info, 0);
  } else {
    g_file_info_set_file_type (info, G_FILE_TYPE_REGULAR);
    g_file_info_set_size (info, entry->nFileSizeLow);
  }

  g_debug("%s: set mod time", G_STRFUNC);
  t.tv_usec = 0;
  t.tv_sec = convert_time(&entry->ftLastWriteTime);
  g_file_info_set_modification_time (info, &t);

  g_debug("%s: set access time", G_STRFUNC);
  g_file_info_set_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_ACCESS, convert_time(&entry->ftLastAccessTime));

  g_debug("%s: set creation time", G_STRFUNC);
  g_file_info_set_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_CREATED, convert_time(&entry->ftCreationTime));

  g_debug("%s: set content type and icon", G_STRFUNC);
  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE) ||
      g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_ICON))
    {
      icon = NULL;
      if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          content_type = g_strdup ("inode/directory");
          if (strcmp (basename, "/") == 0)
            icon = g_themed_icon_new ("folder-remote");
          else
            icon = g_themed_icon_new ("folder");
        }
      else
        {
          content_type = g_content_type_guess (basename, NULL, 0, NULL);

          if (content_type)
            {
              icon = g_content_type_get_icon (content_type);
              if (G_IS_THEMED_ICON (icon))
                g_themed_icon_append_name (G_THEMED_ICON (icon), "text-x-generic");
            }
        }

      if (content_type)
        {
          g_file_info_set_content_type (info, content_type);
          g_free (content_type);
        }

      if (icon == NULL)
        icon = g_themed_icon_new ("text-x-generic");

      g_file_info_set_icon (info, icon);
      g_object_unref (icon);
    }

  g_debug("%s: set readonly", G_STRFUNC);
  if(entry->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
    g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE, FALSE);

  g_debug("%s: set archive", G_STRFUNC);
  if(entry->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
    g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_DOS_IS_ARCHIVE, TRUE);

  g_debug("%s: set system", G_STRFUNC);
  if(entry->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
    g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_DOS_IS_SYSTEM, TRUE);

  /*
  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_ETAG_VALUE))
    {
      char *etag = create_etag (entry);
      g_file_info_set_attribute_string (info, G_FILE_ATTRIBUTE_ETAG_VALUE, etag);
      g_free (etag);
    }
  */

  /*
  file_info->uid = getuid();
  file_info->gid = getgid();
  */

  g_debug("%s: free basename", G_STRFUNC);
  g_free(basename);

}

static void
get_special_directory_attributes(GVfsBackendSynce *backend,
				 GFileInfo *file_info,
				 const gchar *filename,
				 GFileAttributeMatcher *matcher)
{
  GIcon *icon;
  gchar *content_type;
  gchar *display_name;
  gchar *basename;

  basename = g_path_get_basename(filename);
  g_file_info_set_name (file_info, basename);

  if (g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME))
    {
      if (strcmp (basename, "/") == 0)
        display_name = g_strdup_printf (_("Mobile Device %s"), backend->device_name);
      else
	{
	  display_name = g_filename_display_name (basename);

	  if (strstr (display_name, "\357\277\275") != NULL)
	    {
	      gchar *p = display_name;
	      display_name = g_strconcat (display_name, _(" (invalid encoding)"), NULL);
	      g_free (p);
	    }
	}

      g_file_info_set_display_name (file_info, display_name);
      g_free (display_name);
    }

  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME))
    {
      char *edit_name = g_filename_display_name (basename);
      g_file_info_set_edit_name (file_info, edit_name);
      g_free (edit_name);
    }

  g_file_info_set_file_type (file_info, G_FILE_TYPE_DIRECTORY);
  g_file_info_set_size (file_info, 0);

  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE) ||
      g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_ICON))
    {
      icon = NULL;

      content_type = g_strdup ("inode/directory");
      if (strcmp (basename, "/") == 0)
	icon = g_themed_icon_new ("folder-remote");
      else
	icon = g_themed_icon_new ("folder");

      g_file_info_set_content_type (file_info, content_type);
      g_free (content_type);


      g_file_info_set_icon (file_info, icon);
      g_object_unref (icon);
    }


  /*
  file_info->uid = getuid();
  file_info->gid = getgid();
  */

  g_free(basename);
}


static void
get_root_attributes(GVfsBackendSynce *backend,
		    GFileInfo *file_info,
		    const gchar *hostname,
		    GFileAttributeMatcher *matcher)
{
  gchar *display_name;
  if (hostname)
    display_name = g_strjoin(NULL, "Mobile Device (", hostname, ")", NULL);
  else
    display_name = g_strdup("Mobile Device");

  get_special_directory_attributes(backend, file_info, display_name, matcher);

  g_free(display_name);
}


static void
synce_gvfs_query_info (GVfsBackend *backend,
		       GVfsJobQueryInfo *job,
		       const char *filename,
		       GFileQueryInfoFlags flags,
		       GFileInfo *info,
		       GFileAttributeMatcher *matcher)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE(backend);
  GError *error = NULL;
  gchar *location = NULL;
  CE_FIND_DATA entry;
  WCHAR *tempwstr = NULL;

  g_debug("%s: getting info for filename %s", G_STRFUNC, filename);

  switch (get_location(filename, &location))
    {
    case INDEX_DEVICE:
      if (location == NULL)
        {
          get_root_attributes(synce_backend, info, synce_backend->device_name, matcher);
	  g_vfs_job_succeeded (G_VFS_JOB (job));
        }
      else
	g_vfs_job_failed (G_VFS_JOB (job),
			  G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
			  _("File not found"));
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      get_special_directory_attributes(info, NAME_APPLICATIONS, matcher);
      g_vfs_job_succeeded (G_VFS_JOB (job));
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
			_("File not found"));
      goto exit;
    }

  if(!location)
    {
      g_critical("%s: NULL location, should not happen", G_STRFUNC);
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
			_("Invalid argument"));
      goto exit;
    }
  if(strcmp(location, "\\") == 0)
    {
      g_debug("%s: Root folder", G_STRFUNC);
      get_root_attributes(synce_backend, info, synce_backend->device_name, matcher);
      g_vfs_job_succeeded (G_VFS_JOB (job));
      goto exit;
    }

  if(location[strlen(location)-1] == '\\')
    {
      /* This is a directory, chop of the \ to make it readable to FindFirstFile */
      g_debug("%s: Folder with \\", G_STRFUNC);
      location[strlen(location)-1] = '\0';
    }
  else
    {
      g_debug("%s: Folder/File", G_STRFUNC);
    }

  MUTEX_LOCK (synce_backend->mutex);

  g_debug("%s: wstr_from_utf8(): %s", G_STRFUNC, location);
  tempwstr = wstr_from_utf8(location);

  rapi_connection_select(synce_backend->rapi_conn);

  g_debug("%s: CeFindFirstFile()", G_STRFUNC);
  if(CeFindFirstFile(tempwstr, &entry) == INVALID_HANDLE_VALUE)
    {
      g_debug("%s: CeFindFirstFile failed", G_STRFUNC);

      error = g_error_from_rapi(NULL);
      g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
    }
  else
    {
      g_debug("%s: CeFindFirstFile succeeded", G_STRFUNC);

      get_file_attributes(synce_backend, info, &entry, matcher);

      g_debug("%s: Name: %s", G_STRFUNC, g_file_info_get_display_name(info));
      g_debug("%s: Mime-type: %s", G_STRFUNC, g_file_info_get_content_type(info));
      g_debug("%s: Ok", G_STRFUNC);

      g_vfs_job_succeeded (G_VFS_JOB (job));
    }

  MUTEX_UNLOCK (synce_backend->mutex);

  wstr_free_string(tempwstr);

exit:
  g_free(location);
  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}


/* ******************************************************************************** */

static void
synce_gvfs_open_for_read (GVfsBackend *backend,
			  GVfsJobOpenForRead *job,
			  const char *filename)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE(backend);

  g_debug("%s: open_for_read file %s", G_STRFUNC, filename);

  gchar *location = NULL;
  WCHAR *wide_path = NULL;
  gint synce_open_mode, synce_create_mode;
  HANDLE handle;
  GError *error = NULL;

  switch (get_location(filename, &location))
    {
    case INDEX_DEVICE:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
			_("Not permitted"));
      goto exit;

    case INDEX_APPLICATIONS:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
			_("Not permitted"));
      goto exit;

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
			_("Not found"));
      goto exit;
    }

  /*
  vfs_to_synce_mode(mode, &synce_open_mode, &synce_create_mode);
  */

  synce_open_mode = GENERIC_READ;
  synce_create_mode = OPEN_EXISTING;

  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  wide_path = wstr_from_utf8(location);

  g_debug("%s: CeCreateFile()", G_STRFUNC);
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

  if(handle == INVALID_HANDLE_VALUE) {
    error = g_error_from_rapi(NULL);
    g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
    g_error_free (error);
  } else {
    g_vfs_job_open_for_read_set_can_seek (job, TRUE);

    /* use GINT_TO_POINTER ? */
    g_vfs_job_open_for_read_set_handle (job, GUINT_TO_POINTER(handle));


    g_vfs_job_succeeded (G_VFS_JOB (job));
  }

  MUTEX_UNLOCK (synce_backend->mutex);
  wstr_free_string(wide_path);

 exit:
  g_free(location);
  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}


/* ******************************************************************************** */





static void
synce_gvfs_read (GVfsBackend *backend,
		 GVfsJobRead *job,
		 GVfsBackendHandle handle,
		 char *buffer,
		 gsize bytes_requested)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE(backend);
  gint success;
  DWORD read_return;
  gboolean conn_err;
  HANDLE synce_handle;
  GError *error = NULL;

  g_debug("%s: read file", G_STRFUNC);

  /* use GPOINTER_TO_UINT ? */
  synce_handle = GPOINTER_TO_UINT(handle);

  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  g_debug("%s: CeReadFile()", G_STRFUNC);
  success = CeReadFile
    (
     synce_handle,
     buffer,
     bytes_requested,
     &read_return,
     NULL
     );

  if (!success)
    {
      error = g_error_from_rapi(NULL);
      g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
      g_error_free (error);
    }
  else if (read_return == 0)
    {
      g_vfs_job_read_set_size (job, 0);
      g_vfs_job_succeeded (G_VFS_JOB (job));

      /*
       *bytes_read_return = 0;
       g_vfs_job_failed (G_VFS_JOB (job),
                        G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
			_("Not found"));
       result = GNOME_VFS_ERROR_EOF;
      */
    }
  else
    {
      g_vfs_job_read_set_size (job, read_return);
      g_vfs_job_succeeded (G_VFS_JOB (job));
    }
  MUTEX_UNLOCK (synce_backend->mutex);

  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}


/* ******************************************************************************** */

static void
synce_gvfs_close_read (GVfsBackend *backend,
		       GVfsJobCloseRead *job,
		       GVfsBackendHandle handle)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE(backend);

  gint success;
  HANDLE synce_handle;
  GError *error = NULL;

  g_debug("%s: close file", G_STRFUNC);

  /* use GPOINTER_TO_UINT ? */
  synce_handle = GPOINTER_TO_UINT(handle);

  g_debug("%s: CeCloseHandle()", G_STRFUNC);
  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);
  success = CeCloseHandle(synce_handle);

  if (success)
    g_vfs_job_succeeded (G_VFS_JOB (job));
  else {
    error = g_error_from_rapi(NULL);
    g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
    g_error_free (error);
  }

  MUTEX_UNLOCK (synce_backend->mutex);

  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}


/* ******************************************************************************** */


static void
synce_gvfs_seek_on_read (GVfsBackend *backend,
			 GVfsJobSeekRead *job,
			 GVfsBackendHandle handle,
			 goffset    offset,
			 GSeekType  type)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE(backend);

  DWORD retval, move_method;
  gboolean conn_err;
  HANDLE synce_handle;
  GError *error = NULL;

  g_debug("%s: seek file", G_STRFUNC);

  /* use GPOINTER_TO_UINT ? */
  synce_handle = GPOINTER_TO_UINT(handle);

  switch (type) {
  case G_SEEK_SET:
    move_method = FILE_BEGIN;
    break;
  case G_SEEK_CUR:
    move_method = FILE_CURRENT;
    break;
  case G_SEEK_END:
    move_method = FILE_END;
    break;
  default:
    g_vfs_job_failed (G_VFS_JOB (job),
		      G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
		      _("Unsupported seek type"));
    return;
  }

  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  g_debug("%s: CeSetFilePointer()", G_STRFUNC);
  retval = CeSetFilePointer (synce_handle,
                             offset,
                             NULL,
                             move_method);

  if (retval == 0xFFFFFFFF)
    {
      error = g_error_from_rapi(NULL);
      g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
      g_error_free (error);
    }
  else
    {
      g_vfs_job_seek_read_set_offset (job, retval);
      g_vfs_job_succeeded (G_VFS_JOB (job));
    }

  MUTEX_UNLOCK (synce_backend->mutex);

  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}


/* ******************************************************************************** */

static void
synce_gvfs_enumerate(GVfsBackend *backend,
		     GVfsJobEnumerate *job,
		     const char *filename,
		     GFileAttributeMatcher *matcher,
		     GFileQueryInfoFlags flags)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE(backend);

  g_debug("%s: enumerating dir %s", G_STRFUNC, filename);

  GError *error = NULL;
  GList *files = NULL;
  GFileInfo *info = NULL;
  GString *fullpath = NULL;
  gint fullpath_start_len;

  gchar *location = NULL;
  gchar *new_path = NULL;
  CE_FIND_DATA *data = NULL;
  gint optionflags;
  guint itemcount;
  WCHAR *tempwstr = NULL;
  gint index;
  gint count;
  HRESULT hr;

  fullpath = g_string_new(filename);
  /* put a terminating slash if not present */
  if (fullpath->str[fullpath->len - 1] != '/')
    g_string_append_c (fullpath, '/');
  fullpath_start_len = fullpath->len;

  g_debug("%s: entering ...", G_STRFUNC);

  switch ((index = get_location(filename, &location)))
    {
    case INDEX_DEVICE:
      info = g_file_info_new ();
      get_special_directory_attributes(synce_backend, info, NAME_DOCUMENTS, matcher);
      files = g_list_prepend (files, info);

      info = g_file_info_new ();
      get_special_directory_attributes(synce_backend, info, NAME_FILESYSTEM, matcher);
      files = g_list_prepend (files, info);

#if SHOW_APPLICATIONS
      info = g_file_info_new ();
      get_special_directory_attributes(synce_backend, info, NAME_APPLICATIONS, matcher);
      files = g_list_prepend (files, info);
#endif

      g_vfs_job_succeeded (G_VFS_JOB (job));
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
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
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
			_("File not found"));
      goto exit;
    }

  g_debug("%s: location %s", G_STRFUNC, location);

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

  g_debug("%s: modified location %s", G_STRFUNC, location);

  optionflags =
    FAF_ATTRIBUTES
    | FAF_CREATION_TIME
    | FAF_LASTACCESS_TIME
    | FAF_LASTWRITE_TIME
    | FAF_NAME
    | FAF_SIZE_LOW
    | FAF_OID;

  g_debug("%s: getting wide string", G_STRFUNC);
  tempwstr = wstr_from_utf8(location);

  g_debug("%s: locking mutex", G_STRFUNC);
  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  g_debug("%s: CeFindAllFiles()", G_STRFUNC);
  if (!CeFindAllFiles(tempwstr, optionflags, &itemcount, &data))
    {
      g_warning("%s: CeFindAllFiles() failed", G_STRFUNC);
      wstr_free_string(tempwstr);
      error = g_error_from_rapi(NULL);
      MUTEX_UNLOCK (synce_backend->mutex);
      g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
      g_error_free (error);
      goto exit;
    }

  g_debug("%s: CeFindAllFiles() succeeded", G_STRFUNC);

  g_debug("%s: unlocking mutex", G_STRFUNC);
  MUTEX_UNLOCK (synce_backend->mutex);

  g_debug("%s: freeing wide string", G_STRFUNC);
  wstr_free_string(tempwstr);

  count = 0;

  g_debug("%s: found %d items", G_STRFUNC);

  while (count < itemcount) {
    info = g_file_info_new ();
    g_debug("%s: running get_file_attributes for file %d", G_STRFUNC, count);
    get_file_attributes(synce_backend, info, data+count, matcher);

    g_debug("%s: appending info to list", G_STRFUNC);
    files = g_list_prepend (files, info);
    count++;
  }

  MUTEX_LOCK (synce_backend->mutex);

  g_debug("%s: CeRapiFreeBuffer()", G_STRFUNC);
  hr = CeRapiFreeBuffer(data);
  if (FAILED(hr))
    g_warning("CeRapiFreeBuffer(): failed");
  MUTEX_UNLOCK (synce_backend->mutex);

  g_vfs_job_succeeded (G_VFS_JOB (job));

 exit:
  g_free(location);
  g_string_free (fullpath, TRUE);

  if (files)
    {
      files = g_list_reverse (files);
      g_vfs_job_enumerate_add_infos (job, files);
      g_list_foreach (files, (GFunc)g_object_unref, NULL);
      g_list_free (files);
    }

  g_vfs_job_enumerate_done (job);

  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}


static void
synce_gvfs_mount (GVfsBackend *backend,
		  GVfsJobMount *job,
		  GMountSpec *mount_spec,
		  GMountSource *mount_source,
		  gboolean is_automount)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE (backend);
  GMountSpec *synce_mount_spec = NULL;
  gchar *display_name = NULL;
  HRESULT hr;

  g_debug("%s: entering ...", G_STRFUNC);

  synce_backend->device_name = g_strdup(g_mount_spec_get (mount_spec, "host"));
  g_debug("%s: hostname: %s", G_STRFUNC, synce_backend->device_name);

  MUTEX_LOCK (synce_backend->mutex);
  synce_backend->rapi_conn = rapi_connection_from_name(synce_backend->device_name);
  if (!synce_backend->rapi_conn) {
    g_warning("%s: failed to obtain rapi connection", G_STRFUNC);
    g_free(synce_backend->device_name);
    MUTEX_UNLOCK (synce_backend->mutex);

    g_vfs_job_failed (G_VFS_JOB (job),
		      G_IO_ERROR, G_IO_ERROR_HOST_NOT_FOUND,
		      _("Unable to connect to device"));
    return;
  }

  rapi_connection_select(synce_backend->rapi_conn);
  hr = CeRapiInit();

  if (FAILED(hr))
    {
      g_warning("%s: failed to initialize RAPI connection: %s", G_STRFUNC, synce_strerror(hr));
      rapi_connection_destroy(synce_backend->rapi_conn);
      g_free(synce_backend->device_name);
      MUTEX_UNLOCK (synce_backend->mutex);

      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_FAILED,
			_("Failed to initialize connection to device"));
      return;
    } else {
      g_debug("%s: initialized connection for host '%s'", G_STRFUNC, synce_backend->device_name);
    }
  MUTEX_UNLOCK (synce_backend->mutex);

  /*
  file = g_mount_spec_get (mount_spec, "file");
  g_debug("%s: filename: %s", G_STRFUNC, file);
  */

  if (synce_backend->device_name == NULL) {
    display_name = g_strdup_printf(_("Mobile Device"));
  } else {
    display_name = g_strdup_printf(_("Mobile Device %s"), synce_backend->device_name);
  }
  g_vfs_backend_set_display_name (backend, display_name);
  g_free(display_name);

  /* doesn't seem to get this information
     g_vfs_backend_set_icon_name (backend, MOUNT_ICON_NAME);
  */

  synce_mount_spec = g_mount_spec_new("synce");
  g_mount_spec_set(synce_mount_spec, "host", synce_backend->device_name);
  g_vfs_backend_set_mount_spec(backend, synce_mount_spec);
  g_mount_spec_unref(synce_mount_spec);

  g_vfs_job_succeeded (G_VFS_JOB (job));

  g_debug("%s: leaving ...", G_STRFUNC);
}

static void
synce_gvfs_unmount (GVfsBackend *backend,
		    GVfsJobUnmount *job)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE(backend);

  g_debug("%s: unmounting %s", G_STRFUNC, synce_backend->device_name);

  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);
  CeRapiUninit();
  rapi_connection_destroy(synce_backend->rapi_conn);
  MUTEX_UNLOCK (synce_backend->mutex);

  g_free(synce_backend->device_name);

  g_vfs_job_succeeded (G_VFS_JOB (job));

  g_debug("%s: leaving ...", G_STRFUNC);
}


/* 
 *
 * class functions
 *
 *
 */

static void
g_vfs_backend_synce_finalize (GObject *object)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE(object);

  MUTEX_FREE(synce_backend->mutex);

  if (G_OBJECT_CLASS (g_vfs_backend_synce_parent_class)->finalize)
    (*G_OBJECT_CLASS (g_vfs_backend_synce_parent_class)->finalize) (object);
}

static void
g_vfs_backend_synce_init (GVfsBackendSynce *synce_backend)
{
  GVfsBackend *backend = G_VFS_BACKEND (synce_backend);

  /* we could set this to the device name after do_mount() ?? */
  g_vfs_backend_set_display_name (backend, "Mobile Device");

  synce_backend->mutex = MUTEX_NEW ();
}



static void
g_vfs_backend_synce_class_init (GVfsBackendSynceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GVfsBackendClass *backend_class = G_VFS_BACKEND_CLASS (klass);
  
  gobject_class->finalize = g_vfs_backend_synce_finalize;

  backend_class->mount = synce_gvfs_mount;
  backend_class->unmount = synce_gvfs_unmount;
  backend_class->query_info = synce_gvfs_query_info;
  backend_class->enumerate = synce_gvfs_enumerate;

  backend_class->open_for_read = synce_gvfs_open_for_read;
  backend_class->close_read = synce_gvfs_close_read;
  backend_class->read = synce_gvfs_read;
  backend_class->seek_on_read = synce_gvfs_seek_on_read;


  /*
  backend_class->mount = 
  backend_class->unmount = do_unmount;
  backend_class->mount_mountable = NULL;
  backend_class->unmount_mountable = NULL;
  backend_class->eject_mountable = NULL;
  backend_class->open_for_read = do_open_for_read;
  backend_class->close_read = do_close_read;
  backend_class->read = do_read;
  backend_class->seek_on_read = do_seek_on_read;
  backend_class->create = do_create;
  backend_class->append_to = do_append_to;
  backend_class->replace = do_replace;
  backend_class->close_write = do_close_write;
  backend_class->write = do_write;
  backend_class->seek_on_write = do_seek_on_write;
  backend_class->query_info = do_query_info;
  backend_class->query_fs_info = do_query_fs_info;
  backend_class->enumerate = do_enumerate;
  backend_class->set_display_name = do_set_display_name;
  backend_class->delete = do_delete;
  backend_class->trash = do_trash;
  backend_class->make_directory = do_make_directory;
  backend_class->make_symlink = do_make_symlink;
  backend_class->copy = do_copy; 
  backend_class->upload = do_move;
  backend_class->move = do_move;
  backend_class->set_attribute = do_set_attribute;
  backend_class->create_dir_monitor = do_create_dir_monitor;
  backend_class->create_file_monitor = do_create_file_monitor;
  backend_class->query_settable_attributes = do_query_settable_attributes;
  backend_class->query_writable_namespaces = do_query_writable_namespaces;

  */

}


void
g_vfs_synce_daemon_init (void)
{
  gint log_level = 6;

  g_set_application_name (_("Pocket PC Filesystem Service"));

  openlog(g_get_prgname(), LOG_PID, LOG_USER);
  g_log_set_default_handler(log_to_syslog, &log_level);

  synce_log_use_syslog();
}
