#include <gtk/gtk.h>
#include <libgnomeui/libgnomeui.h>
#include <glade/glade.h>
#include <synce_log.h>

#include "keyring.h"

void synce_error_dialog(const char *message)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (NULL,
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_ERROR,
				   GTK_BUTTONS_OK,
				   message);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

void synce_warning_dialog(const char *message)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (NULL,
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_WARNING,
				   GTK_BUTTONS_OK,
				   message);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
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
  gtk_entry_set_text(GTK_ENTRY(password_dialog_entry), NULL);

  g_signal_connect (G_OBJECT (password_dialog_entry), "changed",
		    G_CALLBACK (device_password_dialog_entry_changed_cb), password_dialog_ok);

  gtk_widget_show_all (password_dialog);

  response = gtk_dialog_run(GTK_DIALOG(password_dialog));
  if (response == GTK_RESPONSE_OK)
    password = gtk_editable_get_chars(GTK_EDITABLE(password_dialog_entry), 0, -1);

  gtk_widget_destroy(password_dialog);
  return password;
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
    synce_debug("Failed to get password for device %s from user", pdaname);
  }
  return password;
}
