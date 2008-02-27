/*
 *  Copyright (C) 2008 Mark Ellis
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <synce.h>
#include <liborange.h>
#include <rapi.h>
#include <string.h>
#include <errno.h>

#include "synce_app_man.h"

#define BUFFER_SIZE (64*1024)

GQuark
synce_app_man_error_quark (void)
{
  return g_quark_from_string ("synce-app-man-error-quark");
}


gboolean
synce_app_man_create_program_list(GList **list, SynceAppManBusyFunc busy_func, gpointer busy_data, GError **error)
{
  LONG rapi_result;
  gboolean result = TRUE;
  HKEY parent_key;
  gchar *parent_key_name_ascii = NULL;
  WCHAR* parent_key_name_wide = NULL;
  WCHAR* value_name = NULL;
  DWORD i;
  gboolean smartphone = false;
  GList *tmp_list = NULL;
  gchar *app_name = NULL;

  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (busy_func)
    (*busy_func)(busy_data);

  /* Path on SmartPhone 2002 */
  parent_key_name_ascii = "Security\\AppInstall";
  parent_key_name_wide = wstr_from_ascii(parent_key_name_ascii);

  rapi_result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, parent_key_name_wide, 0, 0, &parent_key);

  if (ERROR_SUCCESS == rapi_result) {
    smartphone = true;
  } else {
    smartphone = false;
    wstr_free_string(parent_key_name_wide);

    /* Path on Pocket PC 2002 */
    parent_key_name_ascii = "Software\\Apps";
    parent_key_name_wide = wstr_from_ascii(parent_key_name_ascii);

    rapi_result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, parent_key_name_wide, 0, 0, &parent_key);

    if (ERROR_SUCCESS != rapi_result) {
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI,
		  _("Unable to open applications registry key: %s"),
		  synce_strerror(rapi_result));
      result = FALSE;
      goto exit;
    }
  }
  value_name = wstr_from_ascii("Instl");

  if (busy_func)
    (*busy_func)(busy_data);

  for (i = 0; ; i++) {
    WCHAR wide_name[MAX_PATH];
    DWORD name_size = sizeof(wide_name);
    HKEY program_key;
    DWORD installed = 0;
    DWORD value_size = sizeof(installed);

    rapi_result = CeRegEnumKeyEx(parent_key, i, wide_name, &name_size, 
				 NULL, NULL, NULL, NULL);
    if (rapi_result == ERROR_NO_MORE_ITEMS)
      break;

    if (rapi_result != ERROR_SUCCESS) {
      g_warning("%s: Unable to enumerate registry key '%s': %s", G_STRFUNC, parent_key_name_ascii, synce_strerror(rapi_result));
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI,
		  _("Unable to enumerate registry key '%s': %s"),
		  parent_key_name_ascii,
		  synce_strerror(rapi_result));
      result = FALSE;
      break;
    }

    if (busy_func)
      (*busy_func)(busy_data);

    if (smartphone) {
      app_name = wstr_to_utf8(wide_name);
      tmp_list = g_list_append(tmp_list, app_name);
    } else {
      rapi_result = CeRegOpenKeyEx(parent_key, wide_name, 0, 0, &program_key);
      if (ERROR_SUCCESS != rapi_result)
	continue;

      rapi_result = CeRegQueryValueEx(program_key, value_name, NULL, NULL,
				 (LPBYTE)&installed, &value_size);

      if (ERROR_SUCCESS == rapi_result && installed) {
	app_name = wstr_to_utf8(wide_name);
	tmp_list = g_list_append(tmp_list, app_name);
      }
      CeRegCloseKey(program_key);
    }

  }

  CeRegCloseKey(parent_key);

exit:
  if (!result) {
    GList *list_iter = g_list_first(tmp_list);
    while (list_iter != NULL) {
      g_free(list_iter->data);
      list_iter = g_list_next(list_iter);
    }
    g_list_free(tmp_list);
  } else {
    *list = tmp_list;
  }
  return result;
}

typedef struct _orange_cookie
{
  DWORD processor_type;
  gchar *output_directory;
  GList *file_list;
} orange_cookie;


static gboolean
cab_copy(const gchar *source_file, const gchar *dest_file)
{
  FILE* input = NULL;
  FILE* output = NULL;
  size_t bytes;
  char buffer[1024];
  gboolean success = FALSE;

  if (!(input = fopen(source_file, "r"))) {
    g_critical(_("%s: Failed to open input file for copying: %s"), G_STRFUNC, source_file);
    return FALSE;
  }

  if (!(output = fopen(dest_file, "w"))) {
    g_critical(_("%s: Failed to open output file for copying: %s"), G_STRFUNC, dest_file);
    fclose(input);
    g_unlink(dest_file);
    return FALSE;
  }

  while (TRUE)
    {
      bytes = fread(buffer, 1, sizeof(buffer), input);
      if (bytes < sizeof(buffer)) {
	if (feof(input)) {
	  fwrite(buffer, 1, bytes, output);
	  success = TRUE;
	  break;
	}

	g_critical(_("%s: error reading input file for copying: %s"), G_STRFUNC, source_file);
	g_unlink(dest_file);
	break;
      }

      if (fwrite(buffer, 1, bytes, output) < bytes) {
	g_critical(_("%s: error writing output file for copying: %s"), G_STRFUNC, dest_file);
	g_unlink(dest_file);
	break;
      }
    }

  fclose(output);
  fclose(input);
  return success;
}

static bool
orange_callback(const char* filename, CabInfo* info, void *cookie )
{
  gboolean success = FALSE;

  orange_cookie *data_exchange = (orange_cookie *) cookie;

  gchar *output_filename = g_strdup_printf("%s/%s", data_exchange->output_directory, ( g_strrstr(filename, "/") + 1 ));
  g_debug(_("%s: squeezing out %s for processor type %d"), G_STRFUNC, output_filename, info->processor);

  if (info->processor != ((orange_cookie *)cookie)->processor_type)
    return TRUE;

  if (cab_copy(filename, output_filename)) {
    data_exchange->file_list = g_list_append(data_exchange->file_list, output_filename);
    success = TRUE;
  } else {
    g_warning(_("%s: Failed to copy from '%s' to '%s'"), G_STRFUNC, filename, output_filename);
    g_free(output_filename);
  }

  return success;
}

static GList *
extract_with_orange(const gchar *arch_file, const gchar *dest_dir, DWORD processor_type)
{
  GList *return_list = NULL;

  orange_cookie *cookie = g_malloc0(sizeof(orange_cookie));
  cookie->output_directory = g_strdup(dest_dir);
  cookie->processor_type = processor_type;

  orange_squeeze_file(arch_file, orange_callback, cookie);

  g_free(cookie->output_directory);
  return_list = cookie->file_list;
  g_free(cookie);

  return return_list;
}

static gchar*
get_install_dir(GError **error)
{
  WCHAR* wide_path = NULL;
  gchar *path = NULL;
  gchar *tmppath = NULL;
  BOOL result;
  CE_FIND_DATA entry;
  HANDLE handle;
  HRESULT hr;
  DWORD rapi_error;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  path = g_strdup("\\Storage\\Windows\\AppMgr");

  wide_path = wstr_from_utf8(path);
  handle = CeFindFirstFile(wide_path, &entry);
  wstr_free_string(wide_path);

  if (handle != INVALID_HANDLE_VALUE) {
    CeFindClose(handle);
  } else {
    g_free(path);

    if (FAILED(hr = CeRapiGetError())) {
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI_TERM,
		  _("Unable to determine installer directory: %s"),
		  synce_strerror(hr));
      return NULL;
    }

    rapi_error = CeGetLastError();
    if (rapi_error != ERROR_PATH_NOT_FOUND) {
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI,
		  _("Unable to determine installer directory: %s"),
		  synce_strerror(rapi_error));
      return NULL;
    }

    path = g_strdup("\\Windows\\AppMgr");

    wide_path = wstr_from_utf8(path);
    handle = CeFindFirstFile(wide_path, &entry);
    wstr_free_string(wide_path);

    if (handle != INVALID_HANDLE_VALUE) {
      CeFindClose(handle);
    } else {
      if (FAILED(hr = CeRapiGetError())) {
	g_free(path);
	g_set_error(error,
		    SYNCE_APP_MAN_ERROR,
		    SYNCE_APP_MAN_ERROR_RAPI_TERM,
		    _("Unable to determine installer directory: %s"),
		    synce_strerror(hr));
	return NULL;
      }

      rapi_error = CeGetLastError();
      if (rapi_error != ERROR_PATH_NOT_FOUND) {
	g_free(path);
	g_set_error(error,
		    SYNCE_APP_MAN_ERROR,
		    SYNCE_APP_MAN_ERROR_RAPI,
		    _("Unable to determine installer directory: %s"),
		    synce_strerror(rapi_error));
	return NULL;
      }

      wide_path = wstr_from_utf8(path);

      result = CeCreateDirectory(wide_path, NULL);
      wstr_free_string(wide_path);
      if (!result) {
	if (FAILED(hr = CeRapiGetError())) {
	  g_free(path);
	  g_set_error(error,
		      SYNCE_APP_MAN_ERROR,
		      SYNCE_APP_MAN_ERROR_RAPI_TERM,
		      _("Unable to determine installer directory: %s"),
		      synce_strerror(hr));
	  return NULL;
	}

	rapi_error = CeGetLastError();
	g_free(path);
	g_set_error(error,
		    SYNCE_APP_MAN_ERROR,
		    SYNCE_APP_MAN_ERROR_RAPI,
		    _("Unable to determine installer directory: %s"),
		    synce_strerror(rapi_error));
	return NULL;
      }
    }
  }

  tmppath = g_strdup_printf("%s\\Install", path);
  g_free(path);
  path = tmppath;

  wide_path = wstr_from_utf8(path);
  handle = CeFindFirstFile(wide_path, &entry);
  wstr_free_string(wide_path);

  if (handle != INVALID_HANDLE_VALUE) {
    CeFindClose(handle);
  } else {
    if (FAILED(hr = CeRapiGetError())) {
      g_free(path);
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI_TERM,
		  _("Unable to determine installer directory: %s"),
		  synce_strerror(hr));
      return NULL;
    }

    rapi_error = CeGetLastError();
    if (rapi_error != ERROR_PATH_NOT_FOUND) {
      g_free(path);
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI,
		  _("Unable to determine installer directory: %s"),
		  synce_strerror(rapi_error));
      return NULL;
    }

    wide_path = wstr_from_utf8(path);

    result = CeCreateDirectory(wide_path, NULL);
    wstr_free_string(wide_path);
    if (!result) {
      if (FAILED(hr = CeRapiGetError())) {
	g_free(path);
	g_set_error(error,
		    SYNCE_APP_MAN_ERROR,
		    SYNCE_APP_MAN_ERROR_RAPI_TERM,
		    _("Unable to determine installer directory: %s"),
		    synce_strerror(hr));
	return NULL;
      }

      rapi_error = CeGetLastError();
      g_free(path);
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI,
		  _("Unable to determine installer directory: %s"),
		  synce_strerror(rapi_error));
      return NULL;
    }
  }

  g_debug("%s: install dir is %s", G_STRFUNC, path);
  return path;
}


static gboolean
copy_to_device(const gchar *source, const gchar *dest_dir, SynceAppManBusyFunc busy_func, gpointer busy_data, GError **error)
{
  gboolean result = TRUE;
  gsize bytes_read;
  guchar* buffer = NULL;
  WCHAR* wide_filename = NULL;
  HANDLE dest_handle;
  HRESULT hr;
  DWORD rapi_error;
  BOOL retval;
  DWORD bytes_written;;
  FILE *src_handle;
  gint errnum;

  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (!(buffer = (guchar *) g_malloc(BUFFER_SIZE))) {
    g_warning("%s: Failed to allocate file copy buffer", G_STRFUNC);
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_FAILED,
		_("Failed to copy file \"%s\" to device: failed to allocate buffer"),
		source);
    result = FALSE;
    goto exit;
  }

  gchar *source_base = g_strrstr(source, "/") + 1;
  gchar *device_filename = g_strdup_printf("%s/%s", dest_dir, source_base);
  wide_filename = wstr_from_utf8(device_filename);
  g_free(device_filename);

  dest_handle = CeCreateFile(wide_filename, GENERIC_WRITE, 0, NULL,
			     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  wstr_free_string(wide_filename);
  if (INVALID_HANDLE_VALUE == dest_handle) {
    if (FAILED(hr = CeRapiGetError())) {
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI_TERM,
		  _("Failed to copy file \"%s\" to device: failed to open destination file: %s"),
		  source,
		  synce_strerror(hr));
      result = FALSE;
      goto exit;
    }

    rapi_error = CeGetLastError();
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_RAPI,
		_("Failed to copy file \"%s\" to device: failed to open destination file: %s"),
		source,
		synce_strerror(rapi_error));
    result = FALSE;
    goto exit;
  }

  src_handle = fopen(source, "r");
  if (src_handle == NULL) {
    errnum = errno;
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_FAILED,
		_("Failed to copy file \"%s\" to device: failed to open source file: %s"),
		source,
		g_strerror(errnum));
    result = FALSE;
    goto exit;
  }

  while(feof(src_handle) == 0)
    {
      if (busy_func)
	(*busy_func)(busy_data);

      bytes_read = fread(buffer, 1, BUFFER_SIZE, src_handle);
      if (bytes_read != BUFFER_SIZE) {
	if (ferror(src_handle)) {
	  g_set_error(error,
		      SYNCE_APP_MAN_ERROR,
		      SYNCE_APP_MAN_ERROR_FAILED,
		      _("Failed to copy file \"%s\" to device: failed to read from source file"),
		      source);
	  result = FALSE;
	  goto exit;
	}

	/* End of file */
      }
    
      retval = CeWriteFile(dest_handle, buffer, bytes_read, &bytes_written, NULL);
      if (!retval) {
	if (FAILED(hr = CeRapiGetError())) {
	  g_set_error(error,
		      SYNCE_APP_MAN_ERROR,
		      SYNCE_APP_MAN_ERROR_RAPI_TERM,
		      _("Failed to copy file \"%s\" to device: failed to write to destination file: %s"),
		      source,
		      synce_strerror(hr));
	  result = FALSE;
	  goto exit;
	}

	rapi_error = CeGetLastError();
	g_set_error(error,
		    SYNCE_APP_MAN_ERROR,
		    SYNCE_APP_MAN_ERROR_RAPI,
		    _("Failed to copy file \"%s\" to device: failed to write to destination file: %s"),
		    source,
		    synce_strerror(rapi_error));
	result = FALSE;
	goto exit;
      }

      if (bytes_written != bytes_read) {
	g_set_error(error,
		    SYNCE_APP_MAN_ERROR,
		    SYNCE_APP_MAN_ERROR_FAILED,
		    _("Failed to copy file \"%s\" to device: failed to write to destination file"),
		    source);
	result = FALSE;
	goto exit;
      }
    }

exit:
  g_free(buffer);

  if (src_handle)
    fclose(src_handle);

  if (dest_handle != INVALID_HANDLE_VALUE)
    CeCloseHandle(dest_handle);

  return result;
}


gboolean
synce_app_man_install(const gchar *filepath, SynceAppManBusyFunc busy_func, gpointer busy_data, GError **error)
{
  gchar *install_path = NULL;
  SYSTEM_INFO system;
  gboolean result;
  WCHAR* wide_program = NULL;
  PROCESS_INFORMATION info;
  BOOL rapi_res;
  HRESULT hr;
  DWORD rapi_error;

  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (busy_func)
    (*busy_func)(busy_data);

  memset(&system, 0, sizeof(system));
  CeGetSystemInfo(&system);

  GList *cab_list = extract_with_orange(filepath, "/tmp", system.dwProcessorType);
  if (!cab_list) {
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_INVALID_INSTALL_FILE,
		_("No CAB files found"));
    return FALSE;
  }

  /* Do some install things */

  install_path = get_install_dir(error);
  if (!install_path) {
    result = FALSE;
    goto exit;
  }

  GList *tmplist = g_list_first(cab_list);
  while(tmplist) {
    gchar *cabname = tmplist->data;
    g_debug("%s: copying file %s to device", G_STRFUNC, cabname);

    if (!copy_to_device(cabname, install_path, busy_func, busy_data, error)) {
      result = FALSE;
      goto exit;
    }

    tmplist = g_list_next(tmplist);
  }

  wide_program = wstr_from_utf8("wceload.exe");
  memset(&info, 0, sizeof(info));

  rapi_res = CeCreateProcess(wide_program, NULL, 
			     NULL, NULL, false, 0,
			     NULL, NULL, NULL, 
			     &info);
  wstr_free_string(wide_program);
  CeCloseHandle(info.hProcess);
  CeCloseHandle(info.hThread);

  if (!rapi_res) {
    if (FAILED(hr = CeRapiGetError())) {
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI_TERM,
		  _("Failed to execute installer: %s"),
		  synce_strerror(hr));
      result = FALSE;
      goto exit;
    }

    rapi_error = CeGetLastError();
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_RAPI,
		_("Failed to execute installer: %s"),
		synce_strerror(rapi_error));
    result = FALSE;
    goto exit;
  }

  g_debug("%s: successfully installed %s on device", G_STRFUNC, filepath);
  result = TRUE;

exit:

  tmplist = g_list_first(cab_list);
  while(tmplist)
    {
      g_unlink((gchar *)(tmplist->data));
      tmplist = g_list_next(tmplist);
    }
  return result;
}

gboolean
synce_app_man_uninstall(const gchar *program, GError **error)
{
  HRESULT hr;
  DWORD rapi_error;
  BOOL result;
  gchar *tmp = NULL;

  /* unload.exe */
  WCHAR* wide_command = NULL;
  WCHAR* wide_parameters = NULL;
  PROCESS_INFORMATION info;
  gchar *command = NULL;

  /* process config */
  gchar *configXML = NULL;
  LPWSTR config = NULL;
  DWORD flags = CONFIG_PROCESS_DOCUMENT;
  LPWSTR reply = NULL;

  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  g_debug("%s: requested removal of program %s", G_STRFUNC, program);

  /* try unload.exe */

  command = g_strdup("unload.exe");
  wide_command = wstr_from_utf8(command);
  wide_parameters = wstr_from_utf8(program);
  memset(&info, 0, sizeof(info));

  result = CeCreateProcess(wide_command, wide_parameters,
			   NULL, NULL, false, 0, NULL, NULL, NULL,
			   &info);
  CeCloseHandle(info.hProcess);
  CeCloseHandle(info.hThread);
  wstr_free_string(wide_command);
  wstr_free_string(wide_parameters);
  g_free(command);

  if (result)
    return TRUE;

  if (FAILED(hr = CeRapiGetError())) {
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_RAPI_TERM,
		_("Failed to execute uninstaller: %s"),
		synce_strerror(hr));
    return FALSE;
  }

  if ((rapi_error = CeGetLastError()) != ERROR_FILE_NOT_FOUND) {
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_RAPI,
		_("Failed to execute uninstaller: %s"),
		synce_strerror(rapi_error));
    return FALSE;
  }

  /* Failed to start unload.exe, trying to uninstall via Configuration Service Provider ... */

  configXML = g_strdup_printf("%s%s%s",
			      "<wap-provisioningdoc>\n\r<characteristic type=\"UnInstall\">\n\r<characteristic type=\"",
			      program,
			      "\">\n\r<parm name=\"uninstall\" value=\"1\"/>\n\r</characteristic>\n\r</characteristic>\n\r</wap-provisioningdoc>\n\r");
  config = wstr_from_current(configXML);

  hr = CeProcessConfig(config, flags, &reply);
  wstr_free_string(config);
  g_free(configXML);

  if (SUCCEEDED(hr)) {
    wstr_free_string(reply);
    return TRUE;
  }

  tmp = wstr_to_current(reply);
  g_set_error(error,
	      SYNCE_APP_MAN_ERROR,
	      SYNCE_APP_MAN_ERROR_RAPI,
	      _("Failed to uninstall application: %s"),
	      tmp);
  g_free(tmp);
  wstr_free_string(reply);
  return FALSE;
}
