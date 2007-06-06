#include <gtk/gtk.h>
#include <libgnomeui/libgnomeui.h>

void synce_error_dialog(const char *message)
{
  GtkWidget *error_dialog;

  error_dialog = gnome_message_box_new (
      message,
      GNOME_MESSAGE_BOX_ERROR,
          GTK_STOCK_OK, 
      NULL);

  gnome_dialog_run (GNOME_DIALOG (error_dialog));
}
