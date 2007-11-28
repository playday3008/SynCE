#include <glib.h>
#include <gtk/gtk.h>

GList* create_program_list(GList *list, GtkWidget *progressbar);
gint rapi_mkdir(const gchar *path, gchar **error_return);
gint rapi_copy_to_device(const gchar *source, const gchar *dest, GtkWidget *progressbar, gchar **error_return);
gint rapi_run(const gchar *program, const gchar *parameters, gchar **error_return);

