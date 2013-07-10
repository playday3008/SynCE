#include <gtk/gtk.h>
#include <rapi2.h>

struct reg_info {
  IRAPISession *session;
  GtkWidget *registry_key_treeview;
  GtkWidget *registry_value_listview;
};


void
setup_registry_value_list_view(struct reg_info *registry_info);

void
setup_registry_key_tree_store(struct reg_info *registry_info);

void
setup_registry_key_tree_view(struct reg_info *registry_info);
