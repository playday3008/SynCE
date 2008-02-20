
#include <rapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "rapi_stuff.h"

#include "pcommon.h"

GList*
create_program_list(GList *list, GtkWidget *progressbar)
{
  LONG result;
  HKEY parent_key;
  WCHAR* parent_key_name = NULL;
  WCHAR* value_name = NULL;
  DWORD i;
  gboolean smartphone = false;

  /* Update the progressbar */
  if (progressbar != NULL) {
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
    while (gtk_events_pending()) {
      gtk_main_iteration();
    }
  }
  /* Path on SmartPhone 2002 */
  parent_key_name = wstr_from_ascii("Security\\AppInstall");

  result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, parent_key_name, 0, 0, &parent_key);

  if (ERROR_SUCCESS == result) {
    smartphone = true;
  } else {
    smartphone = false;
    wstr_free_string(parent_key_name);

    /* Path on Pocket PC 2002 */
    parent_key_name = wstr_from_ascii("Software\\Apps");

    result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, parent_key_name, 0, 0, &parent_key);

    if (ERROR_SUCCESS != result) {
      g_warning("%s: Unable to open parent registry key: %s",
		G_STRFUNC,
		synce_strerror(result));
      goto exit;
    }
  }
  value_name = wstr_from_ascii("Instl");

  /* Update the progressbar */
  if (progressbar != NULL) {
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
    while (gtk_events_pending()) {
      gtk_main_iteration();
    }
  }

  for (i = 0; ; i++) {
    WCHAR wide_name[MAX_PATH];
    DWORD name_size = sizeof(wide_name);
    HKEY program_key;
    DWORD installed = 0;
    DWORD value_size = sizeof(installed);

    result = CeRegEnumKeyEx(parent_key, i, wide_name, &name_size, 
			    NULL, NULL, NULL, NULL);

    /* Update the progressbar */
    if (progressbar != NULL) {
      gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
      while (gtk_events_pending()) {
	gtk_main_iteration();
      }
    }
    if (ERROR_SUCCESS != result)
      break;

    if (smartphone) {
      gchar* name = wstr_to_utf8(wide_name);
      list = g_list_append(list,g_strdup(name));
      wstr_free_string(name);
    } else {
      result = CeRegOpenKeyEx(parent_key, wide_name, 0, 0, &program_key);
      if (ERROR_SUCCESS != result)
	continue;

      result = CeRegQueryValueEx(program_key, value_name, NULL, NULL,
				 (LPBYTE)&installed, &value_size);

      if (ERROR_SUCCESS == result && installed) {
	gchar* name = wstr_to_utf8(wide_name);
	list = g_list_append(list,g_strdup(name));
	wstr_free_string(name);
      }
      CeRegCloseKey(program_key);
    }

  }

  CeRegCloseKey(parent_key);

exit:
  return (list);
}

#define ANYFILE_BUFFER_SIZE (64*1024)

static gboolean
anyfile_copy(const gchar* source_ascii, const gchar* dest_ascii, gsize* bytes_copied, GtkWidget *progressbar, gchar **error_return)
{
  gboolean success = false;
  gsize bytes_read;
  gsize bytes_written;
  guchar* buffer = NULL;
  AnyFile* source = NULL;
  AnyFile* dest   = NULL;

  if (!(buffer = (guchar *) g_malloc(ANYFILE_BUFFER_SIZE))) {
    g_warning("%s: Failed to allocate buffer of size %i", G_STRFUNC, ANYFILE_BUFFER_SIZE);
    *error_return = g_strdup("Failed to allocate buffer");
    goto exit;
  }

  if (!(source = anyfile_open(source_ascii, READ))) {
    g_warning("%s: Failed to open source file '%s'", G_STRFUNC, source_ascii);
    *error_return = g_strdup("Failed to open source file");
    goto exit;
  }

  if (!(dest = anyfile_open(dest_ascii, WRITE))) {
    g_warning("%s: Failed to open destination file '%s'", G_STRFUNC, dest_ascii);
    *error_return = g_strdup("Failed to open destination file");
    goto exit;
  }

  for(;;)
    {
      /* Update the progressbar */
      if (progressbar != NULL) {
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
	while (gtk_events_pending()) {
	  gtk_main_iteration();
	}
      }

      if (!anyfile_read(source, buffer, ANYFILE_BUFFER_SIZE, &bytes_read)) {
	g_warning("%s: Failed to read from source file '%s'", G_STRFUNC, source_ascii);
	*error_return = g_strdup("Failed to read from source file");
	goto exit;
      }

      if (0 == bytes_read) {
	/* End of file */
	break;
      }

      if (!anyfile_write(dest, buffer, bytes_read, &bytes_written)) {
	g_warning("%s: Failed to write to destination file '%s'", G_STRFUNC, dest_ascii);
	*error_return = g_strdup("Failed to write to destination file");
	goto exit;
      }

      if (bytes_written != bytes_read) {
	g_warning("%s: Only wrote %i bytes of %i to destination file '%s'\n", G_STRFUNC,
		  (int)bytes_written, (int)bytes_read, dest_ascii);
	*error_return = g_strdup("Failed to write to destination file");
	goto exit;
      }

      *bytes_copied += bytes_written;
    }

  success = true;

exit:
  g_free(buffer);

  if (source) {
    anyfile_close(source);
    free(source);
  }

  if (dest) {
    anyfile_close(dest);
    free(dest);
  }

  return success;
}

gint
rapi_copy_to_device(const gchar *source, const gchar *dest, GtkWidget *progressbar, gchar **error_return)
{
  gint result = 1;
  gsize bytes_copied = 0;

  if ((!source) || (!dest)) {
    g_error("%s: Source or destination for copy is (null)", G_STRFUNC);
  }

  gchar *file_base = g_strrstr(source, "/") + 1;
  gchar *device_filename = g_strdup_printf(":%s/%s", dest, file_base);

  if (anyfile_copy(source, device_filename, &bytes_copied, progressbar, error_return))
    result = 0;

  return result;
}

gint
rapi_run(const gchar *program, const gchar *parameters, gchar **error_return)
{
  gint result = 1;
  WCHAR* wide_program = NULL;
  WCHAR* wide_parameters = NULL;
  PROCESS_INFORMATION info;
  gchar *tmpprogram = NULL;

  tmpprogram = (gchar *) g_strdup(program);

  convert_to_backward_slashes(tmpprogram);
  wide_program = wstr_from_utf8(tmpprogram);
  if (parameters)
    wide_parameters = wstr_from_utf8(parameters);

  memset(&info, 0, sizeof(info));

  if (!CeCreateProcess(wide_program,
		       wide_parameters,
		       NULL,
		       NULL,
		       false,
		       0,
		       NULL,
		       NULL,
		       NULL,
		       &info
		       )) {
    const gchar *error_str = synce_strerror(CeGetLastError());
    g_warning("%s: Failed to execute '%s': %s",
	      G_STRFUNC,
	      tmpprogram,
	      error_str);
    if (error_return)
      *error_return = g_strdup(error_str);
    goto exit;
  }

  CeCloseHandle(info.hProcess);
  CeCloseHandle(info.hThread);

  result = 0;

exit:
  wstr_free_string(wide_program);
  wstr_free_string(wide_parameters);

  g_free(tmpprogram);

  return result;
}

