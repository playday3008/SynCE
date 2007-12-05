/*
 *  Copyright (C) 2007 Mark Ellis
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>
#include <rapi.h>

#include "registry-list.h"
#include "registry-access.h"
#include "misc.h"

enum {
  KEY_NAME_COLUMN,
  KEY_CLASS_COLUMN,
  KEY_N_COLUMNS
};

enum {
  VALUE_NAME_COLUMN,
  VALUE_TYPE_COLUMN,
  VALUE_DATA_COLUMN,
  VALUE_N_COLUMNS
};

static const gchar*
convert_value_data(REG_VALUE_INFO *reg_val_item, gchar **data_string)
{
  const gchar *type_string;
  gchar *data_str;

  switch(reg_val_item->type)
    {
    case REG_NONE:
      type_string = "REG_NONE";
      data_str = g_strdup("");
      break;
    case REG_SZ:
      type_string = "REG_SZ";
      data_str = wstr_to_utf8(reg_val_item->data);
      break;
    case REG_EXPAND_SZ:
      type_string = "REG_EXPAND_SZ";
      data_str = wstr_to_utf8(reg_val_item->data);
      break;
    case REG_BINARY:
      type_string = "REG_BINARY";
      data_str = g_strdup("");
      break;
    case REG_DWORD:
      type_string = "REG_DWORD";
      data_str = g_strdup_printf("%u", GUINT32_FROM_LE(*((guint32*)reg_val_item->data)));
      break;
    case REG_DWORD_BIG_ENDIAN:
      type_string = "REG_DWORD_BIG_ENDIAN";
      data_str = g_strdup_printf("%u", GUINT32_FROM_BE(*((guint32*)reg_val_item->data)));
      break;
    case REG_LINK:
      type_string = "REG_LINK";
      data_str = g_strdup("");
      break;
    case REG_MULTI_SZ:
      type_string = "REG_MULTI_SZ";
      data_str = g_strdup("");
      break;
    default:
      type_string = "Unknown";
      data_str = g_strdup("");
    }

  *data_string = data_str;

  return type_string;
}

static void
setup_registry_value_list_store(gchar *keyname, GtkTreeView *registry_value_listview)
{
  gchar *data_str;
  const gchar *type_str;

  GtkListStore *store = gtk_list_store_new (VALUE_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  GList *reg_val_list = NULL, *tmplist = NULL;
  REG_VALUE_INFO *reg_val_item;
  GtkTreeIter iter;

  reg_val_list = enum_registry_values(reg_val_list, keyname);

  tmplist = reg_val_list;

  while (tmplist != NULL) {
    reg_val_item = (REG_VALUE_INFO *)tmplist->data;

    type_str = convert_value_data(reg_val_item, &data_str);

    gtk_list_store_append (store, &iter);  /* Acquire an iterator */

    gtk_list_store_set (store, &iter,
			VALUE_NAME_COLUMN, reg_val_item->name,
			VALUE_TYPE_COLUMN, type_str,
			VALUE_DATA_COLUMN, data_str,
			-1);

    g_free(reg_val_item->name);
    g_free(reg_val_item->data);
    g_free((REG_VALUE_INFO *)tmplist->data);
    g_free(data_str);

    tmplist = g_list_next(tmplist);    
  }

  g_list_free(reg_val_list);

  gtk_tree_view_set_model (registry_value_listview, GTK_TREE_MODEL (store));
  g_object_unref (G_OBJECT (store));
}

static void
registry_value_list_selection_changed (GtkTreeSelection *selection, gpointer user_data) 
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *valuename;

  if (!(gtk_tree_selection_get_selected (selection, &model, &iter))) {
    return;
  }

  gtk_tree_model_get (model, &iter, VALUE_NAME_COLUMN, &valuename, -1);

  g_free (valuename);
}

void
setup_registry_value_list_view(GtkTreeView *registry_value_listview) 
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;

  GtkListStore *store = gtk_list_store_new (VALUE_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  gtk_tree_view_set_model (registry_value_listview, GTK_TREE_MODEL (store));
  g_object_unref (G_OBJECT (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Name",
						    renderer,
						    "text", VALUE_NAME_COLUMN,
						    NULL);
  gtk_tree_view_append_column (registry_value_listview, column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Type",
						    renderer,
						    "text", VALUE_TYPE_COLUMN,
						    NULL);
  gtk_tree_view_append_column (registry_value_listview, column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Value",
						    renderer,
						    "text", VALUE_DATA_COLUMN,
						    NULL);
  gtk_tree_view_append_column (registry_value_listview, column);

  selection = gtk_tree_view_get_selection (registry_value_listview);
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (registry_value_list_selection_changed), NULL);

  gtk_widget_show (GTK_WIDGET(registry_value_listview));
}


static void
registry_key_tree_selection_changed (GtkTreeSelection *selection, gpointer user_data) 
{
  GtkTreeIter iter, curr_iter, parent_iter;
  GtkTreeModel *model;
  GtkTreeView *registry_value_listview = GTK_TREE_VIEW(user_data);

  gchar *keyname, *tmpname, *keypath;

  if (!(gtk_tree_selection_get_selected (selection, &model, &iter))) {
    return;
  }

  gtk_tree_model_get (model, &iter, KEY_NAME_COLUMN, &keyname, -1);

  keypath = g_strdup_printf("%s", keyname);
  g_free (keyname);

  curr_iter = iter;

  while (gtk_tree_model_iter_parent (model, &parent_iter, &curr_iter))
    {
      gtk_tree_model_get (model, &parent_iter, KEY_NAME_COLUMN, &keyname, -1);
      tmpname = g_strdup_printf("%s\\%s", keyname, keypath);
      g_free(keypath);
      keypath = tmpname;

      curr_iter = parent_iter;
    }

  setup_registry_value_list_store(keypath, registry_value_listview);

  g_free(keypath);
}

static void
setup_registry_key_branch_store(GtkTreeStore *store,
				char *key_name,
				char *parent_key_name,
				GtkTreeIter *parent,
				GtkWidget *progressbar,
				guint depth)
{
  GList *registrylist = NULL;
  GList *tmplist = NULL;
  REG_KEY_INFO *reg_item;
  GtkTreeIter iter;
  char *full_key_name;

  depth--;

  if (parent_key_name && (strlen(parent_key_name) > 0))
    full_key_name = g_strjoin("\\", parent_key_name, key_name, NULL);
  else
    full_key_name = g_strdup(key_name);

  registrylist = enum_registry_key(registrylist, full_key_name, progressbar); 

  tmplist = registrylist;

  while (tmplist != NULL) {
    reg_item = (REG_KEY_INFO *)tmplist->data;

    gtk_tree_store_append (store, &iter, parent);  /* Acquire an iterator */

    gtk_tree_store_set (store, &iter,
			KEY_NAME_COLUMN, reg_item->name,
			KEY_CLASS_COLUMN, reg_item->class,
			-1);

    if (depth > 0)
      setup_registry_key_branch_store(store, reg_item->name, full_key_name, &iter, progressbar, depth);

    g_free(reg_item->name);
    g_free(reg_item->class);
    g_free((REG_KEY_INFO *)tmplist->data);

    tmplist = g_list_next(tmplist);    
  }

  g_list_free(registrylist);

  g_free(full_key_name);
}

void
setup_registry_key_tree_store(GtkTreeView *registry_key_treeview) 
{
  GladeXML *glade_fetch_window = NULL;
  GtkWidget *fetchwindow, *progressbar;
  GtkTreeIter iter;
  GtkTreeStore *store = gtk_tree_store_new (KEY_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  glade_fetch_window = glade_xml_new(SYNCE_DATA "synce-registry-tool.glade",
			    "fetchwindow", NULL); 
  fetchwindow = glade_xml_get_widget(glade_fetch_window, "fetchwindow");
  progressbar = glade_xml_get_widget(glade_fetch_window, "fetch_progressbar");

  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
  gtk_widget_show_all(fetchwindow);

  while (gtk_events_pending())
    gtk_main_iteration();

  gtk_tree_store_append (store, &iter, NULL);  /* Acquire an iterator */
  gtk_tree_store_set (store, &iter,
		      KEY_NAME_COLUMN, "HKEY_LOCAL_MACHINE",
		      KEY_CLASS_COLUMN, "",
		      -1);
  setup_registry_key_branch_store(store, "", "HKEY_LOCAL_MACHINE", &iter, progressbar, 1);

  gtk_tree_store_append (store, &iter, NULL);  /* Acquire an iterator */
  gtk_tree_store_set (store, &iter,
		      KEY_NAME_COLUMN, "HKEY_CLASSES_ROOT",
		      KEY_CLASS_COLUMN, "",
		      -1);
  setup_registry_key_branch_store(store, "", "HKEY_CLASSES_ROOT", &iter, progressbar, 1);

  gtk_tree_store_append (store, &iter, NULL);  /* Acquire an iterator */
  gtk_tree_store_set (store, &iter,
		      KEY_NAME_COLUMN, "HKEY_CURRENT_USER",
		      KEY_CLASS_COLUMN, "",
		      -1);
  setup_registry_key_branch_store(store, "", "HKEY_CURRENT_USER", &iter, progressbar, 1);

  gtk_tree_store_append (store, &iter, NULL);  /* Acquire an iterator */
  gtk_tree_store_set (store, &iter,
		      KEY_NAME_COLUMN, "HKEY_USERS",
		      KEY_CLASS_COLUMN, "",
		      -1);
  setup_registry_key_branch_store(store, "", "HKEY_USERS", &iter, progressbar, 1);

  gtk_tree_view_set_model (registry_key_treeview, GTK_TREE_MODEL (store));
  g_object_unref (G_OBJECT (store));

  gtk_widget_destroy(fetchwindow);
}


static void
registry_key_tree_view_row_expanded(GtkTreeView *treeview,
				    GtkTreeIter *iter,
				    GtkTreePath *path,
				    gpointer     user_data)
{
  GtkTreeIter curr_iter, parent_iter, child_iter;
  GtkTreeModel *model;
  GtkWidget *fetchwindow, *progressbar;
  gchar *keyname, *tmpname, *keypath;
  gboolean have_child;

  GladeXML *glade_fetch_window = glade_xml_new(SYNCE_DATA "synce-registry-tool.glade",
					       "fetchwindow", NULL); 
  fetchwindow = glade_xml_get_widget(glade_fetch_window, "fetchwindow");
  progressbar = glade_xml_get_widget(glade_fetch_window, "fetch_progressbar");
  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
  gtk_widget_show_all(fetchwindow);

  model = gtk_tree_view_get_model(treeview);
  gtk_tree_model_get (model, iter, KEY_NAME_COLUMN, &keyname, -1);

  keypath = g_strdup_printf("%s", keyname);
  g_free (keyname);

  curr_iter = *iter;

  while (gtk_tree_model_iter_parent (model, &parent_iter, &curr_iter))
    {
      gtk_tree_model_get (model, &parent_iter, KEY_NAME_COLUMN, &keyname, -1);
      tmpname = g_strdup_printf("%s\\%s", keyname, keypath);
      g_free(keypath);
      g_free(keyname);
      keypath = tmpname;

      curr_iter = parent_iter;
    }

  have_child = gtk_tree_model_iter_children(model, &child_iter, iter);
  while(have_child)
    {
      gtk_tree_model_get (model, &child_iter, KEY_NAME_COLUMN, &keyname, -1);

      setup_registry_key_branch_store((GtkTreeStore*)model, keyname, keypath, &child_iter, progressbar, 1);

      g_free(keyname);
      have_child = gtk_tree_model_iter_next(model, &child_iter);
    }

  g_free(keypath);
  gtk_widget_destroy(fetchwindow);

}


static void
registry_key_tree_remove_children(GtkTreeModel *model, GtkTreeIter *iter)
{
  gboolean have_child;
  GtkTreeIter curr_iter, parent_iter, child_iter;


  if (!(have_child = gtk_tree_model_iter_children(model, &child_iter, iter)))
    return;

  while(have_child)
    {
      have_child = gtk_tree_store_remove(GTK_TREE_STORE(model),
					 &child_iter);
    }

}

static void
registry_key_tree_view_row_collapsed(GtkTreeView *treeview,
				     GtkTreeIter *iter,
				     GtkTreePath *path,
				     gpointer     user_data)
{
  GtkTreeIter curr_iter, parent_iter, child_iter;
  GtkTreeModel *model;
  gchar *keyname, *tmpname, *keypath;
  gboolean have_child;

  model = gtk_tree_view_get_model(treeview);
  gtk_tree_model_get (model, iter, KEY_NAME_COLUMN, &keyname, -1);

  keypath = g_strdup_printf("%s", keyname);
  g_free (keyname);

  curr_iter = *iter;

  while (gtk_tree_model_iter_parent (model, &parent_iter, &curr_iter))
    {
      gtk_tree_model_get (model, &parent_iter, KEY_NAME_COLUMN, &keyname, -1);
      tmpname = g_strdup_printf("%s\\%s", keyname, keypath);
      g_free(keypath);
      g_free(keyname);
      keypath = tmpname;

      curr_iter = parent_iter;
    }

  have_child = gtk_tree_model_iter_children(model, &child_iter, iter);
  while(have_child)
    {
      registry_key_tree_remove_children(model, &child_iter);

      have_child = gtk_tree_model_iter_next(model, &child_iter);
    }

  g_free(keypath);
}


void
setup_registry_key_tree_view(GtkTreeView *registry_key_treeview, GtkTreeView *registry_value_listview) 
{
  GtkTreeStore *store = gtk_tree_store_new (KEY_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;

  gtk_tree_view_set_model (registry_key_treeview, GTK_TREE_MODEL (store));
  g_object_unref (G_OBJECT (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Key",
						    renderer,
						    "text", KEY_NAME_COLUMN,
						    NULL);
  gtk_tree_view_append_column (registry_key_treeview, column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Class",
						    renderer,
						    "text", KEY_CLASS_COLUMN,
						    NULL);
  gtk_tree_view_append_column (registry_key_treeview, column);

  selection = gtk_tree_view_get_selection (registry_key_treeview);
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect(G_OBJECT (selection), "changed",
		   G_CALLBACK (registry_key_tree_selection_changed), registry_value_listview);

  g_signal_connect(G_OBJECT(registry_key_treeview), "row-expanded",
		   G_CALLBACK(registry_key_tree_view_row_expanded), NULL);
  g_signal_connect(G_OBJECT(registry_key_treeview), "row-collapsed",
		   G_CALLBACK(registry_key_tree_view_row_collapsed), NULL);

  gtk_widget_show (GTK_WIDGET(registry_key_treeview));
}

