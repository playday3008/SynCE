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
#include <rapi2.h>
#include <string.h>
#include <errno.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "synce_app_man.h"

#define BUFFER_SIZE (64*1024)
#define REG_PATH_SMARTPHONE_2002 "Security\\AppInstall"
#define REG_PATH_POCKETPC_2002 "Software\\Apps"

GQuark
synce_app_man_error_quark (void)
{
  return g_quark_from_string ("synce-app-man-error-quark");
}


static gboolean
create_program_list(IRAPISession *session, GList **list, SynceAppManBusyFunc busy_func, gpointer busy_data, GError **error)
{
  GList *prog_list = NULL;

  xmlDocPtr doc = NULL;
  xmlDocPtr reply_doc = NULL;
  xmlNodePtr parent = NULL;
  xmlNodePtr root_node = NULL, node = NULL, reply_node = NULL;
  xmlChar *doc_string = NULL;
  gint doc_size;

  LPWSTR config_w = NULL;
  LPWSTR reply_w = NULL;
  HRESULT result;

  gchar *reply = NULL;
  gchar *prop = NULL;

  doc = xmlNewDoc((xmlChar *)"1.0");
  root_node = xmlNewNode(NULL, (xmlChar *)"wap-provisioningdoc");
  xmlDocSetRootElement(doc, root_node);
  parent = root_node;

  node = xmlNewNode(NULL, (xmlChar *)"characteristic-query");
  xmlNewProp(node, (xmlChar *)"recursive", (xmlChar *)"false");
  xmlNewProp(node, (xmlChar *)"type", (xmlChar *)"UnInstall");
  xmlAddChild(parent, node);

  xmlDocDumpMemoryEnc(doc, &doc_string, &doc_size, "utf-8");
  g_debug("%s: CeProcessConfig request is \n%s", G_STRFUNC, doc_string);

  config_w = wstr_from_utf8((char *)doc_string);
  xmlFree(doc_string);
  xmlFreeDoc(doc);

  result = IRAPISession_CeProcessConfig(session, config_w, CONFIG_PROCESS_DOCUMENT, &reply_w);

  wstr_free_string(config_w);

  if (result != 0) {
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_RAPI,
		_("Unable to obtain application list: %s"),
		synce_strerror(result));
    return FALSE;
  }

  reply = wstr_to_utf8(reply_w);
  wstr_free_string(reply_w);

  reply_doc = xmlReadMemory(reply, strlen(reply), "reply.xml", NULL, 0);

  xmlDocDumpMemoryEnc(reply_doc, &doc_string, &doc_size, "utf-8");
  g_debug("%s: CeProcessConfig response is \n%s", G_STRFUNC, doc_string);
  xmlFree(doc_string);

  reply_node = NULL;
  node = xmlDocGetRootElement(reply_doc);
  node = node->children;

  while(node)
    {
      if (node->type == XML_ELEMENT_NODE)
	{
	  reply_node = node;
	}
      node = node->next;
    }
  if (!reply_node) {
    xmlFreeDoc(reply_doc);
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_RAPI,
		_("Unable to obtain application list: Unexpected reply XML structure"));

    return FALSE;
  }

  node = reply_node->children;
  while (node) {
    prop = (gchar *)xmlGetProp(node, (xmlChar *)"type");
    g_strstrip(prop);
    prog_list = g_list_append(prog_list, prop);

    node = node->next;
  }
  xmlFreeDoc(reply_doc);

  *list = prog_list;
  return TRUE;
}

static gboolean
create_program_list_legacy(IRAPISession *session, GList **list, SynceAppManBusyFunc busy_func, gpointer busy_data, GError **error)
{
  LONG rapi_result;
  gboolean result = TRUE;
  HKEY app_parent_key;
  const gchar *app_parent_key_name_utf8 = NULL;
  WCHAR* app_parent_key_name_wide = NULL;
  WCHAR* value_name = NULL;
  DWORD app_count;
  gboolean smartphone = FALSE;
  GList *tmp_list = NULL;
  gchar *app_name = NULL;
  WCHAR app_name_wide[MAX_PATH];
  DWORD app_name_size = MAX_PATH;
  HKEY app_key;
  DWORD installed = 0;
  DWORD value_size = sizeof(installed);


  if (busy_func)
    (*busy_func)(busy_data);

  app_parent_key_name_utf8 = REG_PATH_SMARTPHONE_2002;
  app_parent_key_name_wide = wstr_from_utf8(app_parent_key_name_utf8);

  rapi_result = IRAPISession_CeRegOpenKeyEx(session, HKEY_LOCAL_MACHINE, app_parent_key_name_wide, 0, 0, &app_parent_key);
  wstr_free_string(app_parent_key_name_wide);

  if (ERROR_SUCCESS == rapi_result) {
    smartphone = TRUE;
  } else {
    smartphone = FALSE;

    app_parent_key_name_utf8 = REG_PATH_POCKETPC_2002;
    app_parent_key_name_wide = wstr_from_utf8(app_parent_key_name_utf8);

    rapi_result = IRAPISession_CeRegOpenKeyEx(session, HKEY_LOCAL_MACHINE, app_parent_key_name_wide, 0, 0, &app_parent_key);
    wstr_free_string(app_parent_key_name_wide);

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

  value_name = wstr_from_utf8("Instl");

  if (busy_func)
    (*busy_func)(busy_data);

  for (app_count = 0; ; app_count++) {
    app_name_size = MAX_PATH;
    installed = 0;
    value_size = sizeof(installed);

    rapi_result = IRAPISession_CeRegEnumKeyEx(session, app_parent_key, app_count, app_name_wide, &app_name_size, 
				 NULL, NULL, NULL, NULL);
    if (rapi_result == ERROR_NO_MORE_ITEMS)
      break;

    if (rapi_result != ERROR_SUCCESS) {
      g_warning("%s: Unable to enumerate registry key '%s': %s", G_STRFUNC, app_parent_key_name_utf8, synce_strerror(rapi_result));
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI,
		  _("Unable to enumerate registry key '%s': %s"),
		  app_parent_key_name_utf8,
		  synce_strerror(rapi_result));
      result = FALSE;
      break;
    }

    if (busy_func)
      (*busy_func)(busy_data);

    if (smartphone) {
      app_name = wstr_to_utf8(app_name_wide);
      tmp_list = g_list_append(tmp_list, app_name);
    } else {
      rapi_result = IRAPISession_CeRegOpenKeyEx(session, app_parent_key, app_name_wide, 0, 0, &app_key);
      if (ERROR_SUCCESS != rapi_result)
	continue;

      rapi_result = IRAPISession_CeRegQueryValueEx(session, app_key, value_name, NULL, NULL,
				 (LPBYTE)&installed, &value_size);

      if ((ERROR_SUCCESS == rapi_result) && installed) {
	app_name = wstr_to_utf8(app_name_wide);
	tmp_list = g_list_append(tmp_list, app_name);
      }
      IRAPISession_CeRegCloseKey(session, app_key);
    }

  }

  IRAPISession_CeRegCloseKey(session, app_parent_key);
  wstr_free_string(value_name);

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

gboolean
synce_app_man_create_program_list(IRAPISession *session, GList **list, SynceAppManBusyFunc busy_func, gpointer busy_data, GError **error)
{
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  CEOSVERSIONINFO version_info;
  HRESULT hr;
  DWORD last_error;

  version_info.dwOSVersionInfoSize = sizeof(CEOSVERSIONINFO);
  if (!IRAPISession_CeGetVersionEx(session, &version_info)) {
    if (FAILED(hr = IRAPISession_CeRapiGetError(session))) {
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI_TERM,
		  _("Unable to obtain application list: Unable to determine OS version: %s"),
		  synce_strerror(hr));
      return FALSE;
    }

    last_error = IRAPISession_CeGetLastError(session);
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_RAPI,
		_("Unable to obtain application list: Unable to determine OS version: %s"),
		synce_strerror(last_error));
    return FALSE;
  }

  if ((version_info.dwMajorVersion < 5) || ((version_info.dwMajorVersion == 5) && (version_info.dwMinorVersion == 0))) {
    return create_program_list_legacy(session, list, busy_func, busy_data, error);
  }

  return create_program_list(session, list, busy_func, busy_data, error);
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
                if (fwrite(buffer, 1, bytes, output) < bytes) {
                        g_critical(_("%s: error writing output file for copying: %s"), G_STRFUNC, dest_file);
                        g_unlink(dest_file);
                } else {
                        success = TRUE;
                }
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
  gchar *processor_name = NULL;

  orange_cookie *data_exchange = (orange_cookie *) cookie;

  gchar *output_filename = g_strdup_printf("%s/%s", data_exchange->output_directory, ( g_strrstr(filename, "/") + 1 ));

  switch (info->processor)
          {
          case PROCESSOR_STRONGARM:    processor_name = "StrongARM";  break;
          case PROCESSOR_MIPS_R4000:   processor_name = "MIPS R4000"; break;
          case PROCESSOR_HITACHI_SH3:  processor_name = "SH3";        break;

          default:
                  processor_name = "Unknown";
                  break;
          }

  g_debug(_("%s: squeezing out %s for processor type %d (%s)"), G_STRFUNC, output_filename, info->processor, processor_name);

  if ((info->processor != ((orange_cookie *)cookie)->processor_type) && (info->processor != 0)) {
          g_debug(_("%s: ignoring %s for incorrect processor type %d (%s)"), G_STRFUNC, output_filename, info->processor, processor_name);
          g_free(output_filename);
          return TRUE;
  }

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
get_install_dir(IRAPISession *session, GError **error)
{
  WCHAR* wide_path = NULL;
  gchar *path = NULL;
  gchar *tmppath = NULL;
  BOOL result;
  CE_FIND_DATA entry;
  HANDLE handle;
  HRESULT hr;
  DWORD last_error;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  path = g_strdup("\\Storage\\Windows\\AppMgr");

  wide_path = wstr_from_utf8(path);
  handle = IRAPISession_CeFindFirstFile(session, wide_path, &entry);
  wstr_free_string(wide_path);

  if (handle != INVALID_HANDLE_VALUE) {
    IRAPISession_CeFindClose(session, handle);
  } else {
    g_free(path);

    if (FAILED(hr = IRAPISession_CeRapiGetError(session))) {
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI_TERM,
		  _("Unable to determine installer directory: %s"),
		  synce_strerror(hr));
      return NULL;
    }

    last_error = IRAPISession_CeGetLastError(session);
    if ((last_error != ERROR_PATH_NOT_FOUND) && (last_error != ERROR_NO_MORE_FILES)) {
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI,
		  _("Unable to determine installer directory: %s"),
		  synce_strerror(last_error));
      return NULL;
    }

    path = g_strdup("\\Windows\\AppMgr");

    wide_path = wstr_from_utf8(path);
    handle = IRAPISession_CeFindFirstFile(session, wide_path, &entry);
    wstr_free_string(wide_path);

    if (handle != INVALID_HANDLE_VALUE) {
      IRAPISession_CeFindClose(session, handle);
    } else {
      if (FAILED(hr = IRAPISession_CeRapiGetError(session))) {
	g_free(path);
	g_set_error(error,
		    SYNCE_APP_MAN_ERROR,
		    SYNCE_APP_MAN_ERROR_RAPI_TERM,
		    _("Unable to determine installer directory: %s"),
		    synce_strerror(hr));
	return NULL;
      }

      last_error = IRAPISession_CeGetLastError(session);
      if ((last_error != ERROR_PATH_NOT_FOUND) && (last_error != ERROR_NO_MORE_FILES)) {
	g_free(path);
	g_set_error(error,
		    SYNCE_APP_MAN_ERROR,
		    SYNCE_APP_MAN_ERROR_RAPI,
		    _("Unable to determine installer directory: %s"),
		    synce_strerror(last_error));
	return NULL;
      }

      wide_path = wstr_from_utf8(path);

      result = IRAPISession_CeCreateDirectory(session, wide_path, NULL);
      wstr_free_string(wide_path);
      if (!result) {
	if (FAILED(hr = IRAPISession_CeRapiGetError(session))) {
	  g_free(path);
	  g_set_error(error,
		      SYNCE_APP_MAN_ERROR,
		      SYNCE_APP_MAN_ERROR_RAPI_TERM,
		      _("Unable to determine installer directory: %s"),
		      synce_strerror(hr));
	  return NULL;
	}

	last_error = IRAPISession_CeGetLastError(session);
	g_free(path);
	g_set_error(error,
		    SYNCE_APP_MAN_ERROR,
		    SYNCE_APP_MAN_ERROR_RAPI,
		    _("Unable to determine installer directory: %s"),
		    synce_strerror(last_error));
	return NULL;
      }
    }
  }

  tmppath = g_strdup_printf("%s\\Install", path);
  g_free(path);
  path = tmppath;

  wide_path = wstr_from_utf8(path);
  handle = IRAPISession_CeFindFirstFile(session, wide_path, &entry);
  wstr_free_string(wide_path);

  if (handle != INVALID_HANDLE_VALUE) {
    IRAPISession_CeFindClose(session, handle);
  } else {
    if (FAILED(hr = IRAPISession_CeRapiGetError(session))) {
      g_free(path);
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI_TERM,
		  _("Unable to determine installer directory: %s"),
		  synce_strerror(hr));
      return NULL;
    }

    last_error = IRAPISession_CeGetLastError(session);
    if ((last_error != ERROR_PATH_NOT_FOUND) && (last_error != ERROR_NO_MORE_FILES)) {
      g_free(path);
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI,
		  _("Unable to determine installer directory: %s"),
		  synce_strerror(last_error));
      return NULL;
    }

    wide_path = wstr_from_utf8(path);

    result = IRAPISession_CeCreateDirectory(session, wide_path, NULL);
    wstr_free_string(wide_path);
    if (!result) {
      if (FAILED(hr = IRAPISession_CeRapiGetError(session))) {
	g_free(path);
	g_set_error(error,
		    SYNCE_APP_MAN_ERROR,
		    SYNCE_APP_MAN_ERROR_RAPI_TERM,
		    _("Unable to determine installer directory: %s"),
		    synce_strerror(hr));
	return NULL;
      }

      last_error = IRAPISession_CeGetLastError(session);
      g_free(path);
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI,
		  _("Unable to determine installer directory: %s"),
		  synce_strerror(last_error));
      return NULL;
    }
  }

  g_debug("%s: install dir is %s", G_STRFUNC, path);
  return path;
}


static gboolean
copy_to_device(IRAPISession *session, const gchar *source, const gchar *dest_dir, SynceAppManBusyFunc busy_func, gpointer busy_data, GError **error)
{
  gboolean result = TRUE;
  gsize bytes_read;
  guchar* buffer = NULL;
  WCHAR* wide_filename = NULL;
  HANDLE dest_handle;
  HRESULT hr;
  DWORD last_error;
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

  dest_handle = IRAPISession_CeCreateFile(session, wide_filename, GENERIC_WRITE, 0, NULL,
			     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  wstr_free_string(wide_filename);
  if (INVALID_HANDLE_VALUE == dest_handle) {
    if (FAILED(hr = IRAPISession_CeRapiGetError(session))) {
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI_TERM,
		  _("Failed to copy file \"%s\" to device: failed to open destination file: %s"),
		  source,
		  synce_strerror(hr));
      result = FALSE;
      goto exit;
    }

    last_error = IRAPISession_CeGetLastError(session);
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_RAPI,
		_("Failed to copy file \"%s\" to device: failed to open destination file: %s"),
		source,
		synce_strerror(last_error));
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
    
      retval = IRAPISession_CeWriteFile(session, dest_handle, buffer, bytes_read, &bytes_written, NULL);
      if (!retval) {
	if (FAILED(hr = IRAPISession_CeRapiGetError(session))) {
	  g_set_error(error,
		      SYNCE_APP_MAN_ERROR,
		      SYNCE_APP_MAN_ERROR_RAPI_TERM,
		      _("Failed to copy file \"%s\" to device: failed to write to destination file: %s"),
		      source,
		      synce_strerror(hr));
	  result = FALSE;
	  goto exit;
	}

	last_error = IRAPISession_CeGetLastError(session);
	g_set_error(error,
		    SYNCE_APP_MAN_ERROR,
		    SYNCE_APP_MAN_ERROR_RAPI,
		    _("Failed to copy file \"%s\" to device: failed to write to destination file: %s"),
		    source,
		    synce_strerror(last_error));
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
    IRAPISession_CeCloseHandle(session, dest_handle);

  return result;
}


gboolean
synce_app_man_install(IRAPISession *session, const gchar *filepath, SynceAppManBusyFunc busy_func, gpointer busy_data, GError **error)
{
  gchar *install_path = NULL;
  SYSTEM_INFO system;
  gboolean result;
  WCHAR* wide_program = NULL;
  PROCESS_INFORMATION info;
  BOOL rapi_res;
  HRESULT hr;
  DWORD last_error;
  gchar *tmpdir = NULL;
  GList *cab_list = NULL;

  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (busy_func)
    (*busy_func)(busy_data);

  memset(&system, 0, sizeof(system));
  IRAPISession_CeGetSystemInfo(session, &system);

#if GLIB_CHECK_VERSION(2,30,0)
  tmpdir = g_dir_make_tmp("sti_XXXXXX", error);
  if (!tmpdir) {
    g_prefix_error(error, _("Failed to create temp directory '%s': "), tmpdir);
    result = FALSE;
    goto exit;
  }
  g_chmod(tmpdir, 0700);
#else
  tmpdir = tempnam(g_get_tmp_dir(), "sti");
  if (g_mkdir(tmpdir, 0700) != 0) {
    g_set_error(error,
		G_FILE_ERROR,
		g_file_error_from_errno(errno),
		_("Failed to create temp directory '%s': %s"),
                tmpdir,
		g_strerror(errno));
    result = FALSE;
    goto exit;
  }
#endif

  cab_list = extract_with_orange(filepath, tmpdir, system.dwProcessorType);
  if (!cab_list) {
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_INVALID_INSTALL_FILE,
		_("No CAB files found"));
    return FALSE;
  }

  /* Do some install things */

  install_path = get_install_dir(session, error);
  if (!install_path) {
    result = FALSE;
    goto exit;
  }

  GList *tmplist = g_list_first(cab_list);
  while(tmplist) {
    gchar *cabname = tmplist->data;
    g_debug("%s: copying file %s to device", G_STRFUNC, cabname);

    if (!copy_to_device(session, cabname, install_path, busy_func, busy_data, error)) {
      result = FALSE;
      goto exit;
    }

    tmplist = g_list_next(tmplist);
  }

  wide_program = wstr_from_utf8("wceload.exe");
  memset(&info, 0, sizeof(info));

  rapi_res = IRAPISession_CeCreateProcess(session, wide_program, NULL, 
			     NULL, NULL, false, 0,
			     NULL, NULL, NULL, 
			     &info);
  wstr_free_string(wide_program);
  IRAPISession_CeCloseHandle(session, info.hProcess);
  IRAPISession_CeCloseHandle(session, info.hThread);

  if (!rapi_res) {
    if (FAILED(hr = IRAPISession_CeRapiGetError(session))) {
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI_TERM,
		  _("Failed to execute installer: %s"),
		  synce_strerror(hr));
      result = FALSE;
      goto exit;
    }

    last_error = IRAPISession_CeGetLastError(session);
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_RAPI,
		_("Failed to execute installer: %s"),
		synce_strerror(last_error));
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

  if (tmpdir) {
          g_rmdir(tmpdir);
          g_free(tmpdir);
  }

  return result;
}

static gboolean
app_uninstall(IRAPISession *session, const gchar *program, GError **error)
{
  xmlChar *config = NULL;
  LPWSTR config_w = NULL;
  DWORD flags = CONFIG_PROCESS_DOCUMENT;
  LPWSTR reply_w = NULL;
  gchar *reply = NULL;
  HRESULT hr;

  xmlDocPtr doc = NULL;
  xmlNodePtr parent = NULL, node = NULL;
  gint config_size;

  doc = xmlNewDoc((xmlChar *)"1.0");
  parent = xmlNewNode(NULL, (xmlChar *)"wap-provisioningdoc");
  xmlDocSetRootElement(doc, parent);

  node = xmlNewNode(NULL, (xmlChar *)"characteristic");
  xmlNewProp(node, (xmlChar *)"type", (xmlChar *)"UnInstall");
  xmlAddChild(parent, node);
  parent = node;

  node = xmlNewNode(NULL, (xmlChar *)"characteristic");
  xmlNewProp(node, (xmlChar *)"type", (xmlChar *)program);
  xmlAddChild(parent, node);
  parent = node;

  node = xmlNewNode(NULL, (xmlChar *)"parm");
  xmlNewProp(node, (xmlChar *)"name", (xmlChar *)"uninstall");
  xmlNewProp(node, (xmlChar *)"value", (xmlChar *)"1");
  xmlAddChild(parent, node);

  xmlDocDumpMemoryEnc(doc, &config, &config_size, "utf-8");
  xmlFreeDoc(doc);
  g_debug("%s: config doc: %s", G_STRFUNC, config);

  config_w = wstr_from_utf8((gchar *)config);
  g_free(config);
  hr = IRAPISession_CeProcessConfig(session, config_w, flags, &reply_w);
  wstr_free_string(config_w);

  if (SUCCEEDED(hr)) {
    wstr_free_string(reply_w);
    return TRUE;
  }

  reply = wstr_to_utf8(reply_w);
  g_debug("%s: reply doc: %s", G_STRFUNC, reply);
  g_set_error(error,
	      SYNCE_APP_MAN_ERROR,
	      SYNCE_APP_MAN_ERROR_RAPI,
	      _("Failed to uninstall application: %s"),
	      reply);
  g_free(reply);
  wstr_free_string(reply_w);
  return FALSE;
}

static gboolean
app_uninstall_legacy(IRAPISession *session, const gchar *program, GError **error)
{
  HRESULT hr;
  DWORD last_error;
  BOOL result;

  WCHAR* wide_command = NULL;
  WCHAR* wide_parameters = NULL;
  PROCESS_INFORMATION info;
  gchar *command = NULL;

  command = g_strdup("unload.exe");
  wide_command = wstr_from_utf8(command);
  wide_parameters = wstr_from_utf8(program);
  memset(&info, 0, sizeof(info));

  result = IRAPISession_CeCreateProcess(session, wide_command, wide_parameters,
			   NULL, NULL, FALSE, 0, NULL, NULL, NULL,
			   &info);
  IRAPISession_CeCloseHandle(session, info.hProcess);
  IRAPISession_CeCloseHandle(session, info.hThread);
  wstr_free_string(wide_command);
  wstr_free_string(wide_parameters);
  g_free(command);

  if (result)
    return TRUE;

  if (FAILED(hr = IRAPISession_CeRapiGetError(session))) {
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_RAPI_TERM,
		_("Failed to execute uninstaller: %s"),
		synce_strerror(hr));
    return FALSE;
  }

  last_error = IRAPISession_CeGetLastError(session);
  g_set_error(error,
	      SYNCE_APP_MAN_ERROR,
	      SYNCE_APP_MAN_ERROR_RAPI,
	      _("Failed to execute uninstaller: %s"),
	      synce_strerror(last_error));
  return FALSE;
}

gboolean
synce_app_man_uninstall(IRAPISession *session, const gchar *program, GError **error)
{
  CEOSVERSIONINFO version_info;
  HRESULT hr;
  DWORD last_error;

  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  g_debug("%s: requested removal of program %s", G_STRFUNC, program);

  version_info.dwOSVersionInfoSize = sizeof(CEOSVERSIONINFO);
  if (!IRAPISession_CeGetVersionEx(session, &version_info)) {
    if (FAILED(hr = IRAPISession_CeRapiGetError(session))) {
      g_set_error(error,
		  SYNCE_APP_MAN_ERROR,
		  SYNCE_APP_MAN_ERROR_RAPI_TERM,
		  _("Unable to uninstall application: Unable to determine OS version: %s"),
		  synce_strerror(hr));
      return FALSE;
    }

    last_error = IRAPISession_CeGetLastError(session);
    g_set_error(error,
		SYNCE_APP_MAN_ERROR,
		SYNCE_APP_MAN_ERROR_RAPI,
		_("Unable to uninstall application: Unable to determine OS version: %s"),
		synce_strerror(last_error));
    return FALSE;
  }

  if ((version_info.dwMajorVersion < 5) || ((version_info.dwMajorVersion == 5) && (version_info.dwMinorVersion == 0))) {
    return app_uninstall_legacy(session, program, error);
  }

  return app_uninstall(session, program, error);
}

