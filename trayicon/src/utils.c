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
#include <glade/glade.h>
#include <rapi.h>

#include "keyring.h"

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

void device_password_dialog_entry_changed_cb (GtkWidget *widget, gpointer data)
{
  gchar *entry_string;

  entry_string = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  if (entry_string)
    gtk_widget_set_sensitive(GTK_WIDGET(data), TRUE);
  else
    gtk_widget_set_sensitive(GTK_WIDGET(data), FALSE);

  g_free(entry_string);
}


gchar *
device_do_password_dialog (gchar *pdaname)
{
  GladeXML *xml;
  GtkWidget *password_dialog, *password_dialog_entry, *password_dialog_cancel;
  GtkWidget *password_dialog_ok, *password_dialog_pdaname ;
  gchar *password = NULL;
  gint response;

  xml = glade_xml_new (SYNCE_DATA "synce_trayicon_properties.glade", "password_dialog", NULL);

  password_dialog = glade_xml_get_widget (xml, "password_dialog");

  password_dialog_pdaname = glade_xml_get_widget (xml, "password_dialog_pdaname");
  password_dialog_entry = glade_xml_get_widget (xml, "password_dialog_entry");
  password_dialog_ok = glade_xml_get_widget (xml, "password_dialog_ok");
  password_dialog_cancel = glade_xml_get_widget (xml, "password_dialog_cancel");

  gtk_label_set_text(GTK_LABEL(password_dialog_pdaname), pdaname);

  gtk_widget_set_sensitive(password_dialog_ok, FALSE);
  gtk_editable_delete_text(GTK_EDITABLE(password_dialog_entry), 0, -1);

  g_signal_connect (G_OBJECT (password_dialog_entry), "changed",
		    G_CALLBACK (device_password_dialog_entry_changed_cb), password_dialog_ok);

  gtk_widget_show_all (password_dialog);

  response = gtk_dialog_run(GTK_DIALOG(password_dialog));
  if (response == GTK_RESPONSE_OK)
    password = gtk_editable_get_chars(GTK_EDITABLE(password_dialog_entry), 0, -1);
  gtk_editable_delete_text(GTK_EDITABLE(password_dialog_entry), 0, -1);

  gtk_widget_destroy(password_dialog);
  return password;
}

void
device_do_password_on_device_dialog (gchar *pdaname)
{
  GtkWidget *password_dialog;

  password_dialog = gtk_message_dialog_new (NULL,
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_INFO,
				   GTK_BUTTONS_OK,
				   "The device %s is locked. Please unlock it by following instructions on the device",
				   pdaname);
  gtk_dialog_run (GTK_DIALOG (password_dialog));
  gtk_widget_destroy (password_dialog);
}

gchar *
device_get_password(gchar *pdaname)
{
  gchar *password;
  GtkWidget *dialog;
  GnomeKeyringResult keyring_ret;

  if ((keyring_ret = keyring_get_key(pdaname, &password)) == GNOME_KEYRING_RESULT_OK)
    return password;

  password = device_do_password_dialog(pdaname);

  if (password) {
    if ((keyring_ret = keyring_set_key(pdaname, password)) != GNOME_KEYRING_RESULT_OK) {
      dialog = gtk_message_dialog_new (NULL,
				       GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_WARNING,
				       GTK_BUTTONS_CLOSE,
				       "Unable to save password for device \"%s\" in your keyring: %s",
				       pdaname, keyring_strerror(keyring_ret));
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    }
  } else {
    g_debug("Failed to get password for device %s from user", pdaname);
  }
  return password;
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

  key_name = wstr_from_ascii("Ident");
  result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, 0, &key_handle);
  wstr_free_string(key_name);

  if (result != ERROR_SUCCESS) {
    g_critical("%s: CeRegOpenKeyEx failed getting device name: %s", G_STRFUNC, synce_strerror(result));
    return NULL;
  }

  key_name = wstr_from_ascii("Name");
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

  device_name = wstr_to_ascii(buffer);
  return device_name;
}
