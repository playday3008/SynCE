#include <glib.h>
#include <gtk/gtk.h>

GList* setup_rapi_and_create_list(GList *list,GtkWidget *progressbar); 
int rapi_mkdir(char *path);
int rapi_copy(char *source, char *dest);
int rapi_run(char *program, char *parameters);

