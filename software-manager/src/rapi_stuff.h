#include <glib.h>
#include <gtk/gtk.h>

GList* create_program_list(GList *list, GtkWidget *progressbar);
gint rapi_mkdir(const gchar *path);
gint rapi_copy(const gchar *source, const gchar *dest, GtkWidget *progressbar);
gint rapi_run(const gchar *program, const gchar *parameters);

