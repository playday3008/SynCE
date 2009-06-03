#include <gtk/gtk.h>

void
synce_error_dialog(const gchar *message)
{
  GtkWidget *error_dialog;

  error_dialog = gtk_message_dialog_new(NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					"%s",
                                        message);
  gtk_dialog_run (GTK_DIALOG (error_dialog));
  gtk_widget_destroy (error_dialog);
}

void
synce_info_dialog(const gchar *message)
{
  GtkWidget *info_dialog;

  info_dialog = gtk_message_dialog_new(NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK,
                                       "%s",
                                       message);
  gtk_dialog_run (GTK_DIALOG (info_dialog));
  gtk_widget_destroy (info_dialog);
}

