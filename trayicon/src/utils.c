/*
Copyright (c) 2007 Mark Ellis <mark@mpellis.org.uk>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <rapi.h>

void synce_error_dialog(const gchar *format, ...)
{
  va_list ap;
  GtkWidget *dialog;
  gchar *tmpstr = NULL;

  va_start(ap, format);
  tmpstr = g_strdup_vprintf(format, ap);
  va_end(ap);

  dialog = gtk_message_dialog_new (NULL,
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_ERROR,
				   GTK_BUTTONS_OK,
				   "%s",
                                   tmpstr);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  g_free(tmpstr);
}

void synce_warning_dialog(const gchar *format, ...)
{
  va_list ap;
  GtkWidget *dialog;
  gchar *tmpstr = NULL;

  va_start(ap, format);
  tmpstr = g_strdup_vprintf(format, ap);
  va_end(ap);

  dialog = gtk_message_dialog_new (NULL,
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_WARNING,
				   GTK_BUTTONS_OK,
				   "%s",
                                   tmpstr);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  g_free(tmpstr);
}

void synce_info_dialog(const gchar *format, ...)
{
  va_list ap;
  GtkWidget *dialog;
  gchar *tmpstr = NULL;

  va_start(ap, format);
  tmpstr = g_strdup_vprintf(format, ap);
  va_end(ap);

  dialog = gtk_message_dialog_new (NULL,
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_INFO,
				   GTK_BUTTONS_OK,
				   "%s",
                                   tmpstr);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  g_free(tmpstr);
}


gchar *
get_device_name_via_rapi()
{
  LONG result;
  WCHAR* key_name = NULL;
  HKEY key_handle = 0;
  DWORD type = 0;
  DWORD size;
  WCHAR buffer[MAX_PATH];
  gchar *device_name;

  key_name = wstr_from_utf8("Ident");
  result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, 0, &key_handle);
  wstr_free_string(key_name);

  if (result != ERROR_SUCCESS) {
    g_critical("%s: CeRegOpenKeyEx failed getting device name: %s", G_STRFUNC, synce_strerror(result));
    return NULL;
  }

  key_name = wstr_from_utf8("Name");
  size = sizeof(buffer);

  result = CeRegQueryValueEx(key_handle, key_name, 0, &type, (LPBYTE)buffer, &size);
  wstr_free_string(key_name);

  if (result != ERROR_SUCCESS) {
    g_critical("%s: CeRegQueryValueEx failed getting device name: %s", G_STRFUNC, synce_strerror(result));
    return NULL;
  }

  if (type != REG_SZ) {
    g_critical("%s: Unexpected value type: 0x%08x = %i", G_STRFUNC, type, type);
    return NULL;
  }

  device_name = wstr_to_utf8(buffer);
  return device_name;
}
