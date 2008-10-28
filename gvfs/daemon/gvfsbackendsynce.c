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

#define MOUNT_ICON_NAME "synce-gvfs"

struct _GVfsBackendSynce
{
  GVfsBackend		backend;

  gchar *device_name;
  RapiConnection *rapi_conn;
  GMutex * mutex;
};

G_DEFINE_TYPE (GVfsBackendSynce, g_vfs_backend_synce, G_VFS_TYPE_BACKEND)


#define SHOW_APPLICATIONS   0

/* CeFindClose not in rapi2 until librapi2 > 0.11.1 */
#define USE_FIND_CLOSE      0

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


gint max_log_level = 6;

void
log_to_syslog(const gchar *log_domain,
	      GLogLevelFlags log_level,
	      const gchar *message,
	      gpointer user_data)
{
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
      /* This is a connection error, so should we try to unmount ? */

      if (connection_error)
        *connection_error = TRUE;

      g_warning("HRESULT = %08x: %s", hr, synce_strerror(hr));

      for (i = 0; i < sizeof(error_codes)/sizeof(ErrorCodeTriple); i++)
        {
          if (error_codes[i].hresult == hr)
            {
	      result = g_error_new(G_IO_ERROR,
				   error_codes[i].gio_error_result,
                                   "%s",
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
                                   "%s",
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


static gchar *
create_etag (CE_FIND_DATA *entry)
{
  return g_strdup_printf ("%lu", (long unsigned int)convert_time(&entry->ftLastWriteTime));
}


static void
get_file_attributes(GVfsBackendSynce *backend,
                    GFileInfo *info,
		    gint index,
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
  g_debug("%s: filename is %s", G_STRFUNC, filename);

  g_debug("%s: get basename", G_STRFUNC);
  basename = g_path_get_basename(filename);
  g_file_info_set_name (info, basename);

  g_debug("%s: set display name", G_STRFUNC);
  if (g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME))
    {
      if (strcmp (basename, "/") == 0)
        display_name = g_strdup(NAME_FILESYSTEM);
      else if ((strcmp (basename, NAME_MY_DOCUMENTS) == 0) && (index == INDEX_DOCUMENTS))
	{
	  display_name = g_strdup(NAME_DOCUMENTS);
	}
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

  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_ETAG_VALUE))
    {
      gchar *etag = create_etag (entry);
      g_file_info_set_attribute_string (info, G_FILE_ATTRIBUTE_ETAG_VALUE, etag);
      g_free (etag);
    }

  /* just set owner to the user */
  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_OWNER_USER))
    {
      g_file_info_set_attribute_string (info, G_FILE_ATTRIBUTE_OWNER_USER, g_get_user_name());
    }

  /*
  file_info->uid = getuid();
  file_info->gid = getgid();
  */

  g_debug("%s: free basename", G_STRFUNC);
  g_free(basename);

  g_debug("%s: leaving ...", G_STRFUNC);
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
        display_name = g_strdup(NAME_FILESYSTEM);
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

  /* just set owner to the user */
  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_OWNER_USER))
    {
      g_file_info_set_attribute_string (file_info, G_FILE_ATTRIBUTE_OWNER_USER, g_get_user_name());
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
  display_name = g_strdup(NAME_FILESYSTEM);

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
  gint index;
  HANDLE handle;

  g_debug("%s: getting info for filename %s", G_STRFUNC, filename);

  switch (index = get_location(filename, &location))
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
  handle = CeFindFirstFile(tempwstr, &entry);
  if(handle == INVALID_HANDLE_VALUE)
    {
      g_debug("%s: CeFindFirstFile failed", G_STRFUNC);

      error = g_error_from_rapi(NULL);
      g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
    }
  else
    {
      g_debug("%s: CeFindFirstFile succeeded", G_STRFUNC);

      get_file_attributes(synce_backend, info, index, &entry, matcher);

#if USE_FIND_CLOSE
      CeFindClose(handle);
#endif

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

  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  wide_path = wstr_from_utf8(location);

  g_debug("%s: CeCreateFile()", G_STRFUNC);
  handle = CeCreateFile
    (
     wide_path,
     GENERIC_READ,
     0,
     NULL,
     OPEN_EXISTING,
     FILE_ATTRIBUTE_NORMAL,
     0
     );

  if(handle == INVALID_HANDLE_VALUE) {
    error = g_error_from_rapi(NULL);
    g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
    g_error_free (error);
  } else {
    g_vfs_job_open_for_read_set_can_seek (job, TRUE);

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
  HANDLE synce_handle;
  GError *error = NULL;

  g_debug("%s: read file", G_STRFUNC);

  if (g_vfs_job_is_cancelled (G_VFS_JOB (job))) {
    g_vfs_job_failed (G_VFS_JOB (job),
		      G_IO_ERROR, G_IO_ERROR_CANCELLED,
		      _("Operation was cancelled"));
    g_debug("%s: cancelled ...", G_STRFUNC);
    return;
  }

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
  HANDLE synce_handle;
  GError *error = NULL;

  g_debug("%s: seek file", G_STRFUNC);

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

typedef struct {
  HANDLE handle;
  gchar *filename;
  gchar *tmp_filename;
  gchar *backup_filename;
} SynceWriteHandle;

static void
synce_write_handle_free (SynceWriteHandle *handle)
{
  g_free (handle->filename);
  g_free (handle->tmp_filename);
  g_free (handle->backup_filename);
  g_free (handle);
}



static void
synce_gvfs_create (GVfsBackend *backend,
		   GVfsJobOpenForWrite *job,
		   const char *filename,
		   GFileCreateFlags flags)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE(backend);

  g_debug("%s: create file %s", G_STRFUNC, filename);

  gchar *location = NULL;
  WCHAR *wide_path = NULL;
  HANDLE handle;
  GError *error = NULL;
  SynceWriteHandle *write_handle = NULL;

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

  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  wide_path = wstr_from_utf8(location);

  g_debug("%s: CeCreateFile()", G_STRFUNC);
  handle = CeCreateFile
    (
     wide_path,
     GENERIC_WRITE,
     0,
     NULL,
     CREATE_NEW,
     FILE_ATTRIBUTE_NORMAL,
     0
     );

  if(handle == INVALID_HANDLE_VALUE) {
    error = g_error_from_rapi(NULL);
    g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
    g_error_free (error);
  } else {
    write_handle = g_new0(SynceWriteHandle, 1);
    write_handle->handle = handle;

    g_vfs_job_open_for_write_set_can_seek (job, TRUE);

    g_vfs_job_open_for_write_set_handle (job, write_handle);

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
synce_gvfs_append_to (GVfsBackend *backend,
		      GVfsJobOpenForWrite *job,
		      const char *filename,
		      GFileCreateFlags flags)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE(backend);

  g_debug("%s: append to file %s", G_STRFUNC, filename);

  gchar *location = NULL;
  WCHAR *wide_path = NULL;
  HANDLE handle;
  DWORD retval;
  GError *error = NULL;
  SynceWriteHandle *write_handle = NULL;

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

  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  wide_path = wstr_from_utf8(location);

  g_debug("%s: CeCreateFile()", G_STRFUNC);
  handle = CeCreateFile
    (
     wide_path,
     GENERIC_WRITE,
     0,
     NULL,
     OPEN_ALWAYS,
     FILE_ATTRIBUTE_NORMAL,
     0
     );

  wstr_free_string(wide_path);

  if(handle == INVALID_HANDLE_VALUE) {
    error = g_error_from_rapi(NULL);
    g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
    g_error_free (error);
    MUTEX_UNLOCK (synce_backend->mutex);
    goto exit;
  }

  g_debug("%s: CeSetFilePointer()", G_STRFUNC);
  retval = CeSetFilePointer (handle,
                             0,
                             NULL,
                             FILE_END);

  if (retval == 0xFFFFFFFF) {
    error = g_error_from_rapi (NULL);
    g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
    g_error_free (error);
    MUTEX_UNLOCK (synce_backend->mutex);
    goto exit;
  }

  MUTEX_UNLOCK (synce_backend->mutex);

  write_handle = g_new0(SynceWriteHandle, 1);
  write_handle->handle = handle;

  g_vfs_job_open_for_write_set_handle (job, write_handle);

  g_vfs_job_open_for_write_set_can_seek (job, TRUE);
  g_vfs_job_open_for_write_set_initial_offset (job, retval);

  g_vfs_job_succeeded (G_VFS_JOB (job));

 exit:
  g_free(location);
  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}


/* ******************************************************************************** */

static void
random_chars (char *str, int len)
{
  int i;
  const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

  for (i = 0; i < len; i++)
    str[i] = chars[g_random_int_range (0, strlen(chars))];
}


static void
synce_gvfs_replace (GVfsBackend *backend,
		    GVfsJobOpenForWrite *job,
		    const char *filename,
		    const char *etag,
		    gboolean make_backup,
		    GFileCreateFlags flags)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE (backend);

  g_debug("%s: replace file %s", G_STRFUNC, filename);

  gchar *location = NULL;
  gchar *tmp_filename = NULL, *backup_filename = NULL, *current_etag = NULL;
  WCHAR *wide_path = NULL;
  HANDLE handle;
  HANDLE find_handle;
  GError *error = NULL;
  SynceWriteHandle *write_handle = NULL;
  CE_FIND_DATA entry;

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

  if (make_backup)
    backup_filename = g_strconcat (location, "~", NULL);
  else
    backup_filename = NULL;

  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  wide_path = wstr_from_utf8(location);

  g_debug("%s: initial CeCreateFile()", G_STRFUNC);
  handle = CeCreateFile
    (
     wide_path,
     GENERIC_WRITE,
     0,
     NULL,
     CREATE_NEW,
     FILE_ATTRIBUTE_NORMAL,
     0
     );

  if(handle == INVALID_HANDLE_VALUE) {
    error = g_error_from_rapi(NULL);

    if (error->code != G_IO_ERROR_EXISTS) {

      /* actual error */
      g_debug("%s: error is %s", G_STRFUNC, error->message);

      g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
      g_error_free (error);
      MUTEX_UNLOCK (synce_backend->mutex);
      wstr_free_string(wide_path);
      goto exit;
    }

    /* file exists */

    g_debug("%s: fie already exists", G_STRFUNC);

    if (etag != NULL)
      {
	g_debug("%s: check etags", G_STRFUNC);
	g_debug("%s: CeFindFirstFile()", G_STRFUNC);

	find_handle = CeFindFirstFile(wide_path, &entry);
	if (find_handle == INVALID_HANDLE_VALUE)
	  {
	    g_debug("%s: CeFindFirstFile failed", G_STRFUNC);

	    error = g_error_from_rapi(NULL);
	    g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
	    g_error_free (error);
	    MUTEX_UNLOCK (synce_backend->mutex);
	    wstr_free_string(wide_path);
	    goto exit;
	  }
	g_debug("%s: CeFindFirstFile succeeded", G_STRFUNC);

#if USE_FIND_CLOSE
	CeFindClose(find_handle);
#endif

	current_etag = create_etag (&entry);
	if (strcmp (etag, current_etag) != 0)
	  {
	    g_debug("%s: etags dont match", G_STRFUNC);

	    g_free (current_etag);
	    g_vfs_job_failed (G_VFS_JOB (job),
			      G_IO_ERROR, G_IO_ERROR_WRONG_ETAG,
			      _("The file was externally modified"));
	    MUTEX_UNLOCK (synce_backend->mutex);
	    wstr_free_string(wide_path);
	    goto exit;
	  }
	g_free (current_etag);

	g_debug("%s: etags match", G_STRFUNC);
      }

    wstr_free_string(wide_path);

    g_debug("%s: starting backup strategy", G_STRFUNC);

    /* Backup strategy:
     *
     * By default we:
     *  1) save to a tmp file (that doesn't exist already)
     *  2) rename orig file to backup file
     *     (or delete it if not backing up)
     *  3) rename tmp file to orig file
     *
     * However, this can fail if we can't write to the directory.
     * In that case we just truncate the file, after having 
     * copied directly to the backup filename.
     */

    gchar *tmpfile = g_strdup("~gvfXXXX.tmp");

    gchar *dir = g_path_get_dirname(location);

    do {
      random_chars (tmpfile + 4, 4);
      tmp_filename = g_strconcat (dir, tmpfile, NULL);

      wide_path = wstr_from_utf8(tmp_filename);

      g_debug("%s: try temp file CeCreateFile()", G_STRFUNC);
      handle = CeCreateFile
	(
	 wide_path,
	 GENERIC_WRITE,
	 0,
	 NULL,
	 CREATE_NEW,
	 FILE_ATTRIBUTE_NORMAL,
	 0
	 );

      wstr_free_string(wide_path);

      if(handle == INVALID_HANDLE_VALUE) {
	g_free(tmp_filename);
	error = g_error_from_rapi(NULL);

	if (error->code != G_IO_ERROR_EXISTS) {

	  /* actual error */

	  g_error_free (error);
	  break;
	} else {
	  g_error_free(error);
	}
      }

    } while (handle == INVALID_HANDLE_VALUE);

    g_free(tmpfile);
    g_free(dir);

    if (handle == INVALID_HANDLE_VALUE) {

      if (make_backup)
	{
	  if (g_vfs_job_is_cancelled (G_VFS_JOB (job))) {
	    g_vfs_job_failed (G_VFS_JOB (job),
			      G_IO_ERROR, G_IO_ERROR_CANCELLED,
			      _("Operation was cancelled"));
	    g_free (backup_filename);
	    goto exit;
	  }

	  BOOL copied = FALSE;
	  WCHAR *wide_backup = wstr_from_utf8(backup_filename);
	  wide_path = wstr_from_utf8(location);

	  copied = CeCopyFile(wide_path, wide_backup, TRUE);

	  wstr_free_string(wide_path);
	  wstr_free_string(wide_backup);

	  if (!copied)
	    {
	      error = g_error_from_rapi(NULL);
	      g_error_free(error);

	      g_vfs_job_failed (G_VFS_JOB (job),
				G_IO_ERROR, G_IO_ERROR_CANT_CREATE_BACKUP,
				_("Backup file creation failed"));
	      g_free (backup_filename);
	      goto exit;
	    }
	  g_free (backup_filename);
	  backup_filename = NULL;
	}

      wide_path = wstr_from_utf8(location);

      g_debug("%s: CeCreateFile()", G_STRFUNC);
      handle = CeCreateFile
	(
	 wide_path,
	 GENERIC_WRITE,
	 0,
	 NULL,
	 CREATE_ALWAYS,
	 FILE_ATTRIBUTE_NORMAL,
	 0
	 );
      wstr_free_string(wide_path);

      if(handle == INVALID_HANDLE_VALUE) {
	error = g_error_from_rapi(NULL);

	g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
	g_error_free (error);
	MUTEX_UNLOCK (synce_backend->mutex);
	goto exit;
      }
    }

  } else {
    /* file doesn't exist, just write */

    g_debug("%s: file doesn't already exist", G_STRFUNC);

    wstr_free_string(wide_path);
    g_free(backup_filename);
  }

  write_handle = g_new0(SynceWriteHandle, 1);
  write_handle->handle = handle;
  write_handle->filename = g_strdup(location);
  write_handle->tmp_filename = tmp_filename;
  write_handle->backup_filename = backup_filename;

  g_vfs_job_open_for_write_set_can_seek (job, TRUE);
  g_vfs_job_open_for_write_set_handle (job, write_handle);
  g_vfs_job_succeeded (G_VFS_JOB (job));

  MUTEX_UNLOCK (synce_backend->mutex);

 exit:
  g_free(location);
  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}

/* ******************************************************************************** */


static void
synce_gvfs_close_write (GVfsBackend *backend,
			GVfsJobCloseWrite *job,
			GVfsBackendHandle _handle)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE(backend);
  SynceWriteHandle *handle = _handle;
  GError *error = NULL;
  BOOL result;
  WCHAR *wide_path_1 = NULL;
  WCHAR *wide_path_2 = NULL;

  g_debug("%s: close file", G_STRFUNC);

  g_debug("%s: CeCloseHandle()", G_STRFUNC);
  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);
  result = CeCloseHandle(handle->handle);

  if (!result) {
    error = g_error_from_rapi(NULL);
    g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
    g_error_free (error);

    if (handle->tmp_filename) {
      wide_path_1 = wstr_from_utf8(handle->tmp_filename);

      g_debug("%s: CeDeleteFile()", G_STRFUNC);
      result = CeDeleteFile(wide_path_1);

      wstr_free_string(wide_path_1);
    }

    goto exit;
  }

  if (handle->tmp_filename)
    {
      if (handle->backup_filename)
	{
	  if (g_vfs_job_is_cancelled (G_VFS_JOB (job))) {
	    g_vfs_job_failed (G_VFS_JOB (job),
			      G_IO_ERROR, G_IO_ERROR_CANCELLED,
			      _("Operation was cancelled"));
	    goto exit;
	  }

	  wide_path_2 = wstr_from_utf8(handle->backup_filename);
	  wide_path_1 = wstr_from_utf8(handle->filename);

	  result = CeMoveFile(wide_path_1, wide_path_2);

	  wstr_free_string(wide_path_1);
	  wstr_free_string(wide_path_2);

	  if (!result)
	    {
	      error = g_error_from_rapi(NULL);
	      g_vfs_job_failed (G_VFS_JOB (job),
				G_IO_ERROR, G_IO_ERROR_CANT_CREATE_BACKUP,
				_("Backup file creation failed: %s"), error->message);
	      g_error_free (error);

	      wide_path_1 = wstr_from_utf8(handle->tmp_filename);
	      g_debug("%s: CeDeleteFile()", G_STRFUNC);
	      result = CeDeleteFile(wide_path_1);

	      wstr_free_string(wide_path_1);
	      goto exit;
	    }
	}
      else
	{
	  wide_path_1 = wstr_from_utf8(handle->filename);
	  g_debug("%s: CeDeleteFile()", G_STRFUNC);
	  result = CeDeleteFile(wide_path_1);
	  wstr_free_string(wide_path_1);
	}

      if (g_vfs_job_is_cancelled (G_VFS_JOB (job))) {
	g_vfs_job_failed (G_VFS_JOB (job),
			  G_IO_ERROR, G_IO_ERROR_CANCELLED,
			  _("Operation was cancelled"));
	goto exit;
      }

      wide_path_1 = wstr_from_utf8(handle->tmp_filename);
      wide_path_2 = wstr_from_utf8(handle->filename);

      result = CeMoveFile(wide_path_1, wide_path_2);

      wstr_free_string(wide_path_1);
      wstr_free_string(wide_path_2);

      if (!result)
	{
	  error = g_error_from_rapi(NULL);
	  g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
	  g_error_free (error);

	  wide_path_1 = wstr_from_utf8(handle->tmp_filename);
	  g_debug("%s: CeDeleteFile()", G_STRFUNC);
	  result = CeDeleteFile(wide_path_1);
	  wstr_free_string(wide_path_1);

	  goto exit;
	}
    }

  /* not sure if this is required
  
  if (stat_res == 0)
    {
      char *etag;
      etag = create_etag (&stat_at_close);
      g_vfs_job_close_write_set_etag (job, etag);
      g_free (etag);
    }

  */
  
  g_vfs_job_succeeded (G_VFS_JOB (job));

 exit:
  MUTEX_UNLOCK (synce_backend->mutex);
  synce_write_handle_free (handle);  
  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}


/* ******************************************************************************** */


static void
synce_gvfs_write (GVfsBackend *backend,
		  GVfsJobWrite *job,
		  GVfsBackendHandle _handle,
		  char *buffer,
		  gsize buffer_size)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE (backend);
  SynceWriteHandle *handle = _handle;

  GError *error = NULL;
  BOOL result;
  DWORD bytes_written;

  g_debug("%s: write file", G_STRFUNC);

  if (g_vfs_job_is_cancelled (G_VFS_JOB (job))) {
    g_vfs_job_failed (G_VFS_JOB (job),
		      G_IO_ERROR, G_IO_ERROR_CANCELLED,
		      _("Operation was cancelled"));
    g_debug("%s: cancelled ...", G_STRFUNC);
    return;
  }

  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  g_debug("%s: CeWriteFile()", G_STRFUNC);
  result = CeWriteFile
    (
     handle->handle,
     buffer,
     buffer_size,
     &bytes_written,
     NULL
     );


  if (!result) {
    error = g_error_from_rapi(NULL);
    g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
    g_error_free(error);
  } else {
    g_vfs_job_write_set_written_size (job, bytes_written);
    g_vfs_job_succeeded (G_VFS_JOB (job));
  }

  MUTEX_UNLOCK (synce_backend->mutex);

  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}


/* ******************************************************************************** */


static void
synce_gvfs_seek_on_write(GVfsBackend *backend,
			 GVfsJobSeekWrite *job,
			 GVfsBackendHandle _handle,
			 goffset    offset,
			 GSeekType  type)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE(backend);

  DWORD retval, move_method;
  GError *error = NULL;

  g_debug("%s: seek file", G_STRFUNC);

  SynceWriteHandle *handle = _handle;

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
  retval = CeSetFilePointer (handle->handle,
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
      g_vfs_job_seek_write_set_offset (job, retval);
      g_vfs_job_succeeded (G_VFS_JOB (job));
    }

  MUTEX_UNLOCK (synce_backend->mutex);

  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}


/* ******************************************************************************** */


static void
synce_gvfs_delete (GVfsBackend *backend,
		   GVfsJobDelete *job,
		   const char *filename)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE (backend);

  GError *error = NULL;
  gchar *location = NULL;
  WCHAR *tempwstr = NULL;
  CE_FIND_DATA entry;
  HANDLE handle;
  BOOL result;

  g_debug("%s: delete %s", G_STRFUNC, filename);

  switch (get_location(filename, &location))
    {
    case INDEX_DEVICE:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
			_("Not permitted"));
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
			_("Not permitted"));
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
			_("Not found"));
      goto exit;
    }

  if (!location) {
    g_critical("%s: NULL location, should not happen", G_STRFUNC);
    g_vfs_job_failed (G_VFS_JOB (job),
		      G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME,
		      _("Not found"));
    goto exit;
  }

  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  tempwstr = wstr_from_utf8(location);

  g_debug("%s: CeFindFirstFile()", G_STRFUNC);
  handle = CeFindFirstFile(tempwstr, &entry);
  if(handle == INVALID_HANDLE_VALUE) {
    error = g_error_from_rapi(NULL);
    g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
    g_error_free(error);
    wstr_free_string(tempwstr);
    goto exit;
  }

#if USE_FIND_CLOSE
  CeFindClose(handle);
#endif

  if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    result = CeRemoveDirectory(tempwstr);
  else
    result = CeDeleteFile(tempwstr);

  if (result)
    g_vfs_job_succeeded (G_VFS_JOB (job));
  else {
    error = g_error_from_rapi(NULL);
    g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
    g_error_free (error);
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
synce_gvfs_make_directory (GVfsBackend *backend,
			   GVfsJobMakeDirectory *job,
			   const char *filename)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE (backend);

  GError *error = NULL;
  gchar *location = NULL;
  WCHAR *tempwstr = NULL;
  BOOL result;

  g_debug("%s: make directory %s", G_STRFUNC, filename);

  switch (get_location(filename, &location))
    {
    case INDEX_DEVICE:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
			_("Not permitted"));
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
			_("Not permitted"));
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
			_("Not found"));
      goto exit;
    }

  if (!location) {
    g_critical("%s: NULL location, should not happen", G_STRFUNC);
    g_vfs_job_failed (G_VFS_JOB (job),
		      G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME,
		      _("Not found"));
    goto exit;
  }

  tempwstr = wstr_from_utf8(location);

  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  g_debug("%s: CeCreateDirectory()", G_STRFUNC);
  result = CeCreateDirectory(tempwstr, NULL);
  wstr_free_string(tempwstr);

  if(!result) {
    error = g_error_from_rapi(NULL);
    g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
    g_error_free(error);
  } else
    g_vfs_job_succeeded (G_VFS_JOB (job));

  MUTEX_UNLOCK (synce_backend->mutex);

 exit:
  g_free(location);
  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}


/* ******************************************************************************** */

static void
synce_gvfs_move (GVfsBackend *backend,
		 GVfsJobMove *job,
		 const char *source,
		 const char *destination,
		 GFileCopyFlags flags,
		 GFileProgressCallback progress_callback,
		 gpointer progress_callback_data)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE (backend);
  GError *error = NULL;
  gchar *source_loc, *dest_loc, *backup_name;
  gboolean destination_exist, source_is_dir, result;
  WCHAR *source_wstr = NULL, *dest_wstr = NULL, *backup_wstr = NULL;
  CE_FIND_DATA entry;
  HANDLE handle;

  g_debug("%s: move %s to %s", G_STRFUNC, source, destination);

  switch (get_location(source, &source_loc))
    {
    case INDEX_DEVICE:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
			_("Not permitted"));
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
			_("Not permitted"));
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
			_("Not found"));
      goto exit;
    }

  if (!source_loc) {
    g_critical("%s: NULL location, should not happen", G_STRFUNC);
    g_vfs_job_failed (G_VFS_JOB (job),
		      G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME,
		      _("Not found"));
    goto exit;
  }


  switch (get_location(destination, &dest_loc))
    {
    case INDEX_DEVICE:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
			_("Not permitted"));
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
			_("Not permitted"));
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
			_("Not found"));
      goto exit;
    }

  if (!dest_loc) {
    g_critical("%s: NULL location, should not happen", G_STRFUNC);
    g_vfs_job_failed (G_VFS_JOB (job),
		      G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME,
		      _("Not found"));
    goto exit;
  }

  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  source_wstr = wstr_from_utf8(source_loc);

  g_debug("%s: CeFindFirstFile()", G_STRFUNC);
  handle = CeFindFirstFile(source_wstr, &entry);
  if(handle == INVALID_HANDLE_VALUE) {
    error = g_error_from_rapi(NULL);

    g_vfs_job_failed (G_VFS_JOB (job),
		      G_IO_ERROR,
		      error->code,
		      _("Error moving file: %s"),
		      error->message);

    g_error_free(error);
    wstr_free_string(source_wstr);
    MUTEX_UNLOCK (synce_backend->mutex);
    goto exit;
  }

#if USE_FIND_CLOSE
  CeFindClose(handle);
#endif

  if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    source_is_dir = TRUE;

  destination_exist = FALSE;

  dest_wstr = wstr_from_utf8(dest_loc);

  g_debug("%s: CeFindFirstFile()", G_STRFUNC);
  handle = CeFindFirstFile(dest_wstr, &entry);

  if (handle != INVALID_HANDLE_VALUE)
    {
      destination_exist = TRUE; /* Target file exists */

      if (flags & G_FILE_COPY_OVERWRITE)
	{
	  /* Always fail on dirs, even with overwrite */
	  if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	    {
	      g_vfs_job_failed (G_VFS_JOB (job),
				G_IO_ERROR,
				G_IO_ERROR_WOULD_MERGE,
				_("Can't move directory over directory"));
	      MUTEX_UNLOCK (synce_backend->mutex);
	      goto exit;
	    }
	}
      else
	{
	  g_vfs_job_failed (G_VFS_JOB (job),
			    G_IO_ERROR,
			    G_IO_ERROR_EXISTS,
			    _("Target file already exists"));
	  MUTEX_UNLOCK (synce_backend->mutex);
	  goto exit;
	}
    } else
#if USE_FIND_CLOSE
      CeFindClose(handle);
#endif

  if (g_vfs_job_is_cancelled (G_VFS_JOB (job))) {
    g_vfs_job_failed (G_VFS_JOB (job),
		      G_IO_ERROR, G_IO_ERROR_CANCELLED,
		      _("Operation was cancelled"));
    g_debug("%s: cancelled ...", G_STRFUNC);
    MUTEX_UNLOCK (synce_backend->mutex);
    goto exit;
  }

  if (flags & G_FILE_COPY_BACKUP && destination_exist)
    {
      backup_name = g_strconcat (dest_loc, "~", NULL);
      backup_wstr = wstr_from_utf8(backup_name);

      result = CeMoveFile(dest_wstr, backup_wstr);

      wstr_free_string(backup_wstr);
      g_free(backup_name);

      if (!result)
	{
	  error = g_error_from_rapi(NULL);

	  g_vfs_job_failed (G_VFS_JOB (job),
			    G_IO_ERROR,
			    G_IO_ERROR_CANT_CREATE_BACKUP,
			    _("Backup file creation failed: %s"),
			    error->message);
	  g_error_free(error);
	  MUTEX_UNLOCK (synce_backend->mutex);
	  goto exit;
	}
      destination_exist = FALSE; /* It did, but no more */
    }

  if (source_is_dir && destination_exist && (flags & G_FILE_COPY_OVERWRITE))
    {
      /* Source is a dir, destination exists (and is not a dir, because that would have failed
	 earlier), and we're overwriting. Manually remove the target so we can do the rename. */

      result = CeDeleteFile(dest_wstr);

      if (!result)
	{
	  error = g_error_from_rapi(NULL);

	  g_vfs_job_failed (G_VFS_JOB (job), G_IO_ERROR,
			    error->code,
			    _("Error removing target file: %s"),
			    error->message);
	  g_error_free (error);
	  MUTEX_UNLOCK (synce_backend->mutex);
	  goto exit;
	}
    }

  if (g_vfs_job_is_cancelled (G_VFS_JOB (job))) {
    g_vfs_job_failed (G_VFS_JOB (job),
		      G_IO_ERROR, G_IO_ERROR_CANCELLED,
		      _("Operation was cancelled"));
    g_debug("%s: cancelled ...", G_STRFUNC);
    MUTEX_UNLOCK (synce_backend->mutex);
    goto exit;
  }

  result = CeMoveFile(source_wstr, dest_wstr);

  if (!result)
    {
      error = g_error_from_rapi(NULL);
      g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
      g_error_free(error);
    }
  else
    g_vfs_job_succeeded (G_VFS_JOB (job));

    MUTEX_UNLOCK (synce_backend->mutex);
exit:
  g_free(source_loc);
  g_free(dest_loc);
  wstr_free_string(source_wstr);
  wstr_free_string(dest_wstr);
  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}


/* ******************************************************************************** */


static void
synce_gvfs_query_fs_info (GVfsBackend *backend,
			  GVfsJobQueryFsInfo *job,
			  const char *filename,
			  GFileInfo *info,
			  GFileAttributeMatcher *attribute_matcher)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE (backend);

  g_debug("%s: query fs info at %s", G_STRFUNC, filename);

  GError *error = NULL;
  STORE_INFORMATION store;
  DWORD attributes;
  gchar *location = NULL;
  gchar *root_dir;
  LPWSTR wide_root_dir = NULL;
  gint index;
  gboolean other_storage = FALSE;
  ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;

  index = get_location(filename, &location);
  if (index == INDEX_INVALID) {
    g_vfs_job_failed (G_VFS_JOB (job),
		      G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
		      _("Not found"));
    goto exit;
  }
#if SHOW_APPLICATIONS
  if (index == INDEX_APPLICATIONS) {
    result = GNOME_VFS_ERROR_NOT_PERMITTED;
    goto exit;
  }
#endif

  g_file_info_set_attribute_string (info, G_FILE_ATTRIBUTE_FILESYSTEM_TYPE, "synce");
  g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY, FALSE);
  g_file_info_set_attribute_uint32 (info, G_FILE_ATTRIBUTE_FILESYSTEM_USE_PREVIEW, G_FILESYSTEM_PREVIEW_TYPE_NEVER);

  MUTEX_LOCK (synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  if (index == INDEX_FILESYSTEM) {
    gchar **split = g_strsplit(location, "\\", 0);

    if (split && split[0] && split[1]) {

      root_dir = g_strdup_printf("\\%s", split[1]);
      wide_root_dir = wstr_from_current(root_dir);
      attributes = CeGetFileAttributes(wide_root_dir);

      if ((attributes != 0xffffffff) && (attributes & FILE_ATTRIBUTE_TEMPORARY)) {

	other_storage = TRUE;

        if (CeGetDiskFreeSpaceEx(root_dir, 
                                 &FreeBytesAvailable, 
                                 &TotalNumberOfBytes, 
                                 &TotalNumberOfFreeBytes) != 0) {
                g_file_info_set_attribute_uint64 (info,
                                                  G_FILE_ATTRIBUTE_FILESYSTEM_SIZE,
                                                  TotalNumberOfBytes);

                g_file_info_set_attribute_uint64 (info,
                                                  G_FILE_ATTRIBUTE_FILESYSTEM_FREE,
                                                  TotalNumberOfFreeBytes);
        } else {
                error = g_error_from_rapi(NULL);
                g_critical("%s: Failed to get store information: %s", G_STRFUNC, error->message);
                g_error_free(error);
        }
      }

      wstr_free_string(wide_root_dir);
      g_free(root_dir);
    }
    g_strfreev(split);
  }

  if (!other_storage) {
    if (CeGetStoreInformation(&store)) {
      g_file_info_set_attribute_uint64 (info,
					G_FILE_ATTRIBUTE_FILESYSTEM_SIZE,
					store.dwStoreSize);

      g_file_info_set_attribute_uint64 (info,
					G_FILE_ATTRIBUTE_FILESYSTEM_FREE,
					store.dwFreeSize);
    } else {
      error = g_error_from_rapi(NULL);
      g_critical("%s: Failed to get store information: %s", G_STRFUNC, error->message);
      g_error_free(error);
    }

  }

  MUTEX_UNLOCK (synce_backend->mutex);

  g_vfs_job_succeeded (G_VFS_JOB (job));
 exit:
  g_free(location);
  g_debug("%s: leaving ...", G_STRFUNC);
  return;
}



/* ******************************************************************************** */


static void
synce_gvfs_set_display_name (GVfsBackend *backend,
			     GVfsJobSetDisplayName *job,
			     const char *filename,
			     const char *display_name)
{
  GVfsBackendSynce *synce_backend = G_VFS_BACKEND_SYNCE (backend);
  gchar *location, *dirname, *new_path, *new_location;
  WCHAR *from_wstr = NULL, *to_wstr = NULL;
  gboolean result;
  GError *error = NULL;

  g_debug("%s: rename %s to %s", G_STRFUNC, filename, display_name);

  switch (get_location(filename, &location))
    {
    case INDEX_DEVICE:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
			_("Not permitted"));
      goto exit;

#if SHOW_APPLICATIONS
    case INDEX_APPLICATIONS:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
			_("Not permitted"));
      goto exit;
#endif

    case INDEX_DOCUMENTS:
    case INDEX_FILESYSTEM:
      break;

    default:
      g_vfs_job_failed (G_VFS_JOB (job),
			G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
			_("Not found"));
      goto exit;
    }

  if (!location) {
    g_critical("%s: NULL location, should not happen", G_STRFUNC);
    g_vfs_job_failed (G_VFS_JOB (job),
		      G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME,
		      _("Not found"));
    goto exit;
  }

  dirname = g_path_get_dirname (filename);
  new_path = g_build_filename (dirname, display_name, NULL);
  get_location(new_path, &new_location);

  g_free(dirname);

  g_debug("%s: renaming %s to %s", G_STRFUNC, location, new_location);

  MUTEX_LOCK(synce_backend->mutex);
  rapi_connection_select(synce_backend->rapi_conn);

  from_wstr = wstr_from_utf8(location);
  to_wstr = wstr_from_utf8(new_location);

  result = CeMoveFile(from_wstr, to_wstr);

  wstr_free_string(from_wstr);
  wstr_free_string(to_wstr);

  if (!result)
    {
      error = g_error_from_rapi(NULL);
      g_vfs_job_failed_from_error (G_VFS_JOB (job), error);
      g_error_free(error);
    }
  else
    {
      g_vfs_job_set_display_name_set_new_path (job, new_path);
      g_vfs_job_succeeded (G_VFS_JOB (job));
    }

  MUTEX_UNLOCK(synce_backend->mutex);
  g_free(new_path);
  g_free(new_location);
 exit:
  g_free(location);
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
      goto error_exit;
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
      goto error_exit;
    }

  g_debug("%s: CeFindAllFiles() succeeded", G_STRFUNC);

  g_debug("%s: unlocking mutex", G_STRFUNC);
  MUTEX_UNLOCK (synce_backend->mutex);

  g_debug("%s: freeing wide string", G_STRFUNC);
  wstr_free_string(tempwstr);

  count = 0;

  g_debug("%s: found %d items", G_STRFUNC, itemcount);

  while (count < itemcount) {
    info = g_file_info_new ();
    g_debug("%s: running get_file_attributes for file %d", G_STRFUNC, count);
    get_file_attributes(synce_backend, info, index, data+count, matcher);

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

  g_debug("%s: g_vfs_job_succeeded()", G_STRFUNC);
  g_vfs_job_succeeded (G_VFS_JOB (job));

 exit:

  g_debug("%s: freeing files()", G_STRFUNC);
  if (files)
    {
      files = g_list_reverse (files);
      g_vfs_job_enumerate_add_infos (job, files);
      g_list_foreach (files, (GFunc)g_object_unref, NULL);
      g_list_free (files);
    }

  g_debug("%s: g_vfs_job_enumerate_done()", G_STRFUNC);
  g_vfs_job_enumerate_done (job);

 error_exit:
  g_free(location);
  g_string_free (fullpath, TRUE);

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

  g_vfs_backend_set_icon_name (backend, MOUNT_ICON_NAME);

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

  backend_class->create = synce_gvfs_create;
  backend_class->append_to = synce_gvfs_append_to;
  backend_class->replace = synce_gvfs_replace;
  backend_class->close_write = synce_gvfs_close_write;
  backend_class->write = synce_gvfs_write;
  backend_class->seek_on_write = synce_gvfs_seek_on_write;

  backend_class->delete = synce_gvfs_delete;
  backend_class->make_directory = synce_gvfs_make_directory;
  backend_class->move = synce_gvfs_move;
  backend_class->query_fs_info = synce_gvfs_query_fs_info;
  backend_class->set_display_name = synce_gvfs_set_display_name;

  /*
  backend_class->try_query_settable_attributes = try_query_settable_attributes;
  */

  /*
    backend_class->mount                            tested
    backend_class->unmount                          tested
    backend_class->mount_mountable =
    backend_class->unmount_mountable =
    backend_class->eject_mountable =                NO
    backend_class->open_for_read                    tested
    backend_class->close_read                       tested
    backend_class->read                             tested
    backend_class->seek_on_read                     tested
    backend_class->create                           in
    backend_class->append_to                        in
    backend_class->replace                          in
    backend_class->close_write                      in
    backend_class->write                            in
    backend_class->seek_on_write                    in
    backend_class->query_info =                     tested
    backend_class->query_fs_info =                  in
    backend_class->enumerate =                      tested
    backend_class->set_display_name =               in
    backend_class->delete =                         in
    backend_class->trash =                          NO
    backend_class->make_directory =                 in
    backend_class->make_symlink =                   NO
    backend_class->copy =
    backend_class->upload =
    backend_class->move =                           in
    backend_class->set_attribute =
    backend_class->create_dir_monitor =             NO
    backend_class->create_file_monitor =            NO
    backend_class->query_settable_attributes =
    backend_class->query_writable_namespaces =
  */

}


void
g_vfs_synce_daemon_init (void)
{
  g_set_application_name (_("Pocket PC Filesystem Service"));

  openlog(g_get_prgname(), LOG_PID, LOG_USER);
  g_log_set_default_handler(log_to_syslog, NULL);

  synce_log_use_syslog();
}
