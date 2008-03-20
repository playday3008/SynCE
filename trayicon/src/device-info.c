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

#include <gnome.h>
#include <glade/glade.h>
#include <rapi.h>
#include <rra/matchmaker.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include "device-info.h"
#include "sync-engine-glue.h"

G_DEFINE_TYPE (WmDeviceInfo, wm_device_info, G_TYPE_OBJECT)

typedef struct _WmDeviceInfoPrivate WmDeviceInfoPrivate;
struct _WmDeviceInfoPrivate {
  WmDevice *device;
  GtkWidget *dialog;
  GladeXML *xml;

  gboolean disposed;
};

#define WM_DEVICE_INFO_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), WM_DEVICE_INFO_TYPE, WmDeviceInfoPrivate))

/* properties */
enum
  {
    PROP_DEVICE = 1,

    LAST_PROPERTY
  };

enum
{
  RRA_CURRENT_COLUMN,
  RRA_INDEX_COLUMN,
  RRA_ID_COLUMN,
  RRA_NAME_COLUMN,
  RRA_N_COLUMNS
};

enum
{
  SYNCENG_CURRENT_COLUMN,
  SYNCENG_INDEX_COLUMN,
  SYNCENG_ID_COLUMN,
  SYNCENG_GUID_COLUMN,
  SYNCENG_NAME_COLUMN,
  SYNCENG_ACTIVEITEM_COLUMN,
  SYNCENG_HOSTNAME_COLUMN,
  SYNCENG_DEVICENAME_COLUMN,
  SYNCENG_N_COLUMNS
};

enum
{
  SYNCITEM_INDEX_COLUMN,
  SYNCITEM_SELECTED_COLUMN,
  SYNCITEM_NAME_COLUMN,
  SYNCITEM_N_COLUMNS
};


/* partnership */

static void
partners_create_button_clicked_rra_cb (GtkWidget *widget, gpointer data)
{
  WmDeviceInfo *self = WM_DEVICE_INFO(data);
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  guint32 index;
  gint id;
  gchar *name;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkWidget *device_info_dialog = priv->dialog;
  GtkWidget *partners_list_view = glade_xml_get_widget (priv->xml, "partners_list");	
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (partners_list_view));

  g_debug("%s: create button_clicked", G_STRFUNC);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model,
			  &iter,
			  RRA_INDEX_COLUMN, &index,
			  RRA_ID_COLUMN, &id,
			  RRA_NAME_COLUMN, &name,
			  -1);
    }
  else
    {
      g_warning("%s: Failed to get selection", G_STRFUNC);
      return;
    }

  g_free (name);

  RRA_Matchmaker* matchmaker = NULL;

  if (!(matchmaker = rra_matchmaker_new())) {
    g_critical("%s: Failed to create match-maker", G_STRFUNC);
    return;
  }

  if (rra_matchmaker_create_partnership(matchmaker, &index)) {
    g_debug("%s: Partnership creation succeeded for index %d", G_STRFUNC, index);
  } else {
    g_warning("%s: Partnership creation failed for index %d", G_STRFUNC, index);
    GtkWidget *failed_dialog = gtk_message_dialog_new(GTK_WINDOW(device_info_dialog),
						     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_MESSAGE_WARNING,
						     GTK_BUTTONS_OK,
						     "Creation of a new partnership was unsuccessful");

    gtk_dialog_run(GTK_DIALOG(failed_dialog));
    gtk_widget_destroy (failed_dialog);
  }
  rra_matchmaker_destroy(matchmaker);
}

static void
partners_remove_button_clicked_rra_cb (GtkWidget *widget, gpointer data)
{
  WmDeviceInfo *self = WM_DEVICE_INFO(data);
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  gint index, id;
  gchar *name;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkWidget *partners_list_view = glade_xml_get_widget (priv->xml, "partners_list");	
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (partners_list_view));

  g_debug("%s: remove button_clicked", G_STRFUNC);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model,
			  &iter,
			  RRA_INDEX_COLUMN, &index,
			  RRA_ID_COLUMN, &id,
			  RRA_NAME_COLUMN, &name,
			  -1);
    }
  else
    {
      g_warning("%s: Failed to get selection", G_STRFUNC);
      return;
    }

  GtkWidget *device_info_dialog = priv->dialog;
  GtkWidget *confirm_dialog = gtk_message_dialog_new(GTK_WINDOW(device_info_dialog),
						     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_MESSAGE_QUESTION,
						     GTK_BUTTONS_YES_NO,
						     "Are you sure you want to remove the partnership ID %d with host %s ?",
						     id, name);

  gint result = gtk_dialog_run(GTK_DIALOG(confirm_dialog));
  gtk_widget_destroy (confirm_dialog);
  g_free (name);
  switch (result)
    {
    case GTK_RESPONSE_YES:
      break;
    default:
      return;
      break;
    }

  RRA_Matchmaker* matchmaker = NULL;

  if (!(matchmaker = rra_matchmaker_new())) {
    g_critical("%s: Failed to create match-maker", G_STRFUNC);
    return;
  }

  if (rra_matchmaker_clear_partnership(matchmaker, index)) {
    g_debug("%s: Partnership cleaning succeeded for index %d", G_STRFUNC, index);
  } else {
    g_warning("%s: Partnership cleaning failed for index %d", G_STRFUNC, index);
  }
  rra_matchmaker_destroy(matchmaker);
}


static void
partners_selection_changed_rra_cb (GtkTreeSelection *selection, gpointer data)
{
  WmDeviceInfo *self = WM_DEVICE_INFO(data);
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  GtkTreeIter iter;
  GtkTreeModel *model;
  gint index, id;
  gchar *name;
  GtkWidget *partners_create_button, *partners_remove_button;
  guint32 os_major = 0;

  partners_create_button = glade_xml_get_widget (priv->xml, "partners_create_button");	
  partners_remove_button = glade_xml_get_widget (priv->xml, "partners_remove_button");	

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model,
			  &iter,
			  RRA_INDEX_COLUMN, &index,
			  RRA_ID_COLUMN, &id,
			  RRA_NAME_COLUMN, &name,
			  -1);

      g_debug("%s: You selected index %d, id %d, name %s", G_STRFUNC, index, id, name);
      g_free (name);

      if (id == 0) {
	gtk_widget_set_sensitive(partners_create_button, TRUE);
	gtk_widget_set_sensitive(partners_remove_button, FALSE);
      } else {
	gtk_widget_set_sensitive(partners_create_button, FALSE);
	gtk_widget_set_sensitive(partners_remove_button, TRUE);
      }

    }
}


static void
partners_setup_view_store_synceng(WmDeviceInfo *self);


static void
setup_sync_item_store(gpointer key, gpointer value, gpointer user_data)
{
  GtkListStore *store = GTK_LIST_STORE(user_data);
  GtkTreeIter iter;

  gtk_list_store_append (store, &iter);  /* Acquire an iterator */

  gtk_list_store_set (store, &iter,
		      SYNCITEM_INDEX_COLUMN, key,
		      SYNCITEM_SELECTED_COLUMN, FALSE,
		      SYNCITEM_NAME_COLUMN, (gchar *)value,
		      -1);
}

static void
partners_create_button_clicked_synceng_cb (GtkWidget *widget, gpointer data)
{
  WmDeviceInfo *self = WM_DEVICE_INFO(data);
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  gint response;
  guint32 index, item;
  gboolean active, result;
  guint id;
  gchar *name = NULL;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkWidget *device_info_dialog = priv->dialog;
  GtkWidget *create_pship_dialog = glade_xml_get_widget (priv->xml, "create_partnership_dialog"); 
  GtkWidget *pship_name_entry = glade_xml_get_widget (priv->xml, "pship_name_entry");
  GtkWidget *sync_items_listview = glade_xml_get_widget (priv->xml, "sync_items_listview");
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (sync_items_listview));
  GArray *sync_items_required = NULL;
  GHashTable *sync_items = NULL;
  GError *error = NULL;
  DBusGConnection *dbus_connection = NULL;
  DBusGProxy *sync_engine_proxy = NULL;

  g_debug("%s: create button_clicked", G_STRFUNC);

  dbus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (dbus_connection == NULL) {
    g_critical("%s: Failed to open connection to bus: %s", G_STRFUNC, error->message);

    GtkWidget *failed_dialog = gtk_message_dialog_new(GTK_WINDOW(device_info_dialog),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_OK,
						      "Creation of a new partnership was unsuccessful: %s",
						      error->message);

    gtk_dialog_run(GTK_DIALOG(failed_dialog));
    gtk_widget_destroy (failed_dialog);

    g_error_free (error);
    goto exit;
  }

  sync_engine_proxy = dbus_g_proxy_new_for_name (dbus_connection,
						 "org.synce.SyncEngine",
						 "/org/synce/SyncEngine",
						 "org.synce.SyncEngine");
  if (sync_engine_proxy == NULL) {
    g_critical("%s: Failed to create proxy to sync engine", G_STRFUNC);

    GtkWidget *failed_dialog = gtk_message_dialog_new(GTK_WINDOW(device_info_dialog),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_OK,
						      "Creation of a new partnership was unsuccessful: Failed to connect to SyncEngine");

    gtk_dialog_run(GTK_DIALOG(failed_dialog));
    gtk_widget_destroy (failed_dialog);

    goto exit;
  }

  result = org_synce_syncengine_get_item_types(sync_engine_proxy, &sync_items, &error);
  if (!result) {
    g_critical("%s: Error creating partnership via sync-engine: %s", G_STRFUNC, error->message);

    GtkWidget *failed_dialog = gtk_message_dialog_new(GTK_WINDOW(device_info_dialog),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_OK,
						      "Creation of a new partnership was unsuccessful: %s",
						      error->message);

    gtk_dialog_run(GTK_DIALOG(failed_dialog));
    gtk_widget_destroy (failed_dialog);

    g_error_free(error);
    goto exit;
  }

  GtkListStore *store = gtk_list_store_new (SYNCITEM_N_COLUMNS,
                                            G_TYPE_INT,     /* index */
                                            G_TYPE_BOOLEAN,     /* selected */
                                            G_TYPE_STRING); /* program name */

  g_hash_table_foreach(sync_items, setup_sync_item_store, store);
  g_hash_table_destroy(sync_items);

  gtk_tree_view_set_model (GTK_TREE_VIEW(sync_items_listview), GTK_TREE_MODEL(store));
  g_object_unref (G_OBJECT (store));

  renderer = gtk_cell_renderer_toggle_new ();
  gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer), FALSE);
  column = gtk_tree_view_column_new_with_attributes("Selected",
                                                    renderer,
                                                    "active", SYNCITEM_SELECTED_COLUMN,
                                                    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(sync_items_listview), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("SyncItem",
                                                    renderer,
                                                    "text", SYNCITEM_NAME_COLUMN,
                                                    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(sync_items_listview), column);

  response = gtk_dialog_run(GTK_DIALOG(create_pship_dialog));
  if (response != GTK_RESPONSE_OK) {
    gtk_widget_destroy(create_pship_dialog);
    return;
  }

  gtk_widget_hide(create_pship_dialog);

  name = g_strdup(gtk_entry_get_text(GTK_ENTRY(pship_name_entry)));

  sync_items_required = g_array_new(FALSE, TRUE, sizeof(guint32));

  store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(sync_items_listview)));
  if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
    goto exit;

  gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
		     SYNCITEM_INDEX_COLUMN, &item,
		     SYNCITEM_SELECTED_COLUMN, &active,
		     -1);
  if (active)
    g_array_append_val(sync_items_required, item);

  while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter)) {
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
		       SYNCITEM_INDEX_COLUMN, &item,
		       SYNCITEM_SELECTED_COLUMN, &active,
		       -1);
    if (active)
      g_array_append_val(sync_items_required, item);
  }

  g_debug("%s: partnership name: %s", G_STRFUNC, name);
  gint i;
  for (i = 0; i < sync_items_required->len; i++) {
    g_debug("%s:     sync_items %d: %d", G_STRFUNC, i, g_array_index(sync_items_required, guint, i));
  }

  result = org_synce_syncengine_create_partnership(sync_engine_proxy, name, sync_items_required, &id, &error);
  if (!result) {
    g_critical("%s: Error creating partnership via sync-engine: %s", G_STRFUNC, error->message);

    GtkWidget *failed_dialog = gtk_message_dialog_new(GTK_WINDOW(device_info_dialog),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_OK,
						      "Creation of a new partnership was unsuccessful: %s",
						      error->message);

    gtk_dialog_run(GTK_DIALOG(failed_dialog));
    gtk_widget_destroy (failed_dialog);

    g_error_free(error);
    goto exit;
  }

  partners_setup_view_store_synceng(self);
exit:
  if (sync_engine_proxy) g_object_unref(sync_engine_proxy);
  if (dbus_connection) dbus_g_connection_unref(dbus_connection);
  if (create_pship_dialog) gtk_widget_destroy(create_pship_dialog);

  g_free(name);
}


static void
partners_remove_button_clicked_synceng_cb (GtkWidget *widget, gpointer data)
{
  WmDeviceInfo *self = WM_DEVICE_INFO(data);
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  guint index, id;
  gchar *guid = NULL;
  gchar *name;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkWidget *partners_list_view = glade_xml_get_widget (priv->xml, "partners_list");	
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (partners_list_view));
  GError *error = NULL;
  DBusGConnection *dbus_connection = NULL;
  DBusGProxy *sync_engine_proxy = NULL;

  g_debug("%s: remove button_clicked", G_STRFUNC);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model,
			  &iter,
			  SYNCENG_INDEX_COLUMN, &index,
			  SYNCENG_ID_COLUMN, &id,
			  SYNCENG_GUID_COLUMN, &guid,
			  SYNCENG_NAME_COLUMN, &name,
			  -1);
    }
  else
    {
      g_warning("%s: Failed to get selection", G_STRFUNC);
      return;
    }

  GtkWidget *device_info_dialog = priv->dialog;
  GtkWidget *confirm_dialog = gtk_message_dialog_new(GTK_WINDOW(device_info_dialog),
						     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_MESSAGE_QUESTION,
						     GTK_BUTTONS_YES_NO,
						     "Are you sure you want to remove the partnership '%s' ?",
						     name);

  gint result = gtk_dialog_run(GTK_DIALOG(confirm_dialog));
  gtk_widget_destroy (confirm_dialog);
  switch (result)
    {
    case GTK_RESPONSE_YES:
      break;
    default:
      return;
      break;
    }

  dbus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (dbus_connection == NULL) {
    g_critical("%s: Failed to open connection to bus: %s", G_STRFUNC, error->message);

    GtkWidget *failed_dialog = gtk_message_dialog_new(GTK_WINDOW(device_info_dialog),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_OK,
						      "Deletion of partnership was unsuccessful: %s",
						      error->message);

    gtk_dialog_run(GTK_DIALOG(failed_dialog));
    gtk_widget_destroy (failed_dialog);

    g_error_free (error);
    goto exit;
  }

  sync_engine_proxy = dbus_g_proxy_new_for_name (dbus_connection,
						 "org.synce.SyncEngine",
						 "/org/synce/SyncEngine",
						 "org.synce.SyncEngine");
  if (sync_engine_proxy == NULL) {
    g_critical("%s: Failed to create proxy to sync engine", G_STRFUNC);

    GtkWidget *failed_dialog = gtk_message_dialog_new(GTK_WINDOW(device_info_dialog),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_OK,
						      "Deletion of partnership was unsuccessful: Failed to connect to SyncEngine");

    gtk_dialog_run(GTK_DIALOG(failed_dialog));
    gtk_widget_destroy (failed_dialog);

    goto exit;
  }

  result = org_synce_syncengine_delete_partnership(sync_engine_proxy, id, guid, &error);
  if (!result) {
    g_critical("%s: Error deleting partnership via sync-engine: %s", G_STRFUNC, error->message);

    GtkWidget *failed_dialog = gtk_message_dialog_new(GTK_WINDOW(device_info_dialog),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_OK,
						      "Deletion of partnership was unsuccessful: %s",
						      error->message);

    gtk_dialog_run(GTK_DIALOG(failed_dialog));
    gtk_widget_destroy (failed_dialog);

    g_error_free(error);
    goto exit;
  }

  partners_setup_view_store_synceng(self);
exit:
  if (sync_engine_proxy) g_object_unref(sync_engine_proxy);
  if (dbus_connection) dbus_g_connection_unref(dbus_connection);
  g_free(guid);
  g_free (name);
}


static void
partners_selection_changed_synceng_cb (GtkTreeSelection *selection, gpointer data)
{
  WmDeviceInfo *self = WM_DEVICE_INFO(data);
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  GtkTreeIter iter;
  GtkTreeModel *model;
  gint index, id;
  gchar *name;
  GtkWidget *partners_create_button, *partners_remove_button;
  guint32 os_major = 0;

  partners_remove_button = glade_xml_get_widget (priv->xml, "partners_remove_button");	

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model,
			  &iter,
			  SYNCENG_INDEX_COLUMN, &index,
			  SYNCENG_ID_COLUMN, &id,
			  SYNCENG_NAME_COLUMN, &name,
			  -1);

      g_debug("%s: You selected partnership index %d, id %d, name %s", G_STRFUNC, index, id, name);
      g_free (name);

      gtk_widget_set_sensitive(partners_remove_button, TRUE);
    }
}


static gint
sync_item_sort(gconstpointer a, gconstpointer b)
{
  return GPOINTER_TO_UINT(a) - GPOINTER_TO_UINT(b);
}


#if GLIB_MINOR_VERSION < 14
get_sync_item_keys(gpointer key,
		   gpointer value,
		   gpointer user_data)
{
  *(GList **)user_data = g_list_append(*(GList **)user_data, key);
}
#endif

static void
partners_setup_view_store_synceng(WmDeviceInfo *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  GtkWidget *partners_list_view = glade_xml_get_widget (priv->xml, "partners_list");	
  GtkTreeIter iter, sub_iter;
  guint32 index;
  gboolean result;
  GPtrArray* partnership_list;
  GError *error = NULL;
  GtkTreeStore *store = NULL;
  DBusGConnection *dbus_connection = NULL;
  DBusGProxy *sync_engine_proxy = NULL;
  GValueArray *partnership = NULL;
  GArray *sync_items_active = NULL;
  GHashTable *sync_items = NULL;
  GList *item_keys = NULL;

  dbus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (dbus_connection == NULL) {
    g_critical("%s: Failed to open connection to bus: %s", G_STRFUNC, error->message);

    GtkWidget *failed_dialog = gtk_message_dialog_new(GTK_WINDOW(priv->dialog),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_OK,
						      "Failed to retrieve partnership information: %s",
						      error->message);

    gtk_dialog_run(GTK_DIALOG(failed_dialog));
    gtk_widget_destroy (failed_dialog);

    g_error_free (error);
    goto exit;
  }

  sync_engine_proxy = dbus_g_proxy_new_for_name (dbus_connection,
						 "org.synce.SyncEngine",
						 "/org/synce/SyncEngine",
						 "org.synce.SyncEngine");
  if (sync_engine_proxy == NULL) {
    g_critical("%s: Failed to create proxy to sync engine", G_STRFUNC);

    GtkWidget *failed_dialog = gtk_message_dialog_new(GTK_WINDOW(priv->dialog),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_OK,
						      "Failed to retrieve partnership information: Failed to connect to SyncEngine");

    gtk_dialog_run(GTK_DIALOG(failed_dialog));
    gtk_widget_destroy (failed_dialog);

    goto exit;
  }

  result = org_synce_syncengine_get_item_types(sync_engine_proxy, &sync_items, &error);
  if (!result) {
    g_critical("%s: Error fetching sync item list: %s", G_STRFUNC, error->message);

    GtkWidget *failed_dialog = gtk_message_dialog_new(GTK_WINDOW(priv->dialog),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_OK,
						      "Failed to retrieve partnership information: %s",
						      error->message);

    gtk_dialog_run(GTK_DIALOG(failed_dialog));
    gtk_widget_destroy (failed_dialog);

    g_error_free(error);
    goto exit;
  }

  result = org_synce_syncengine_get_partnerships (sync_engine_proxy, &partnership_list, &error);
  if (!result) {
    g_critical("%s: Error getting partnership list from sync-engine: %s", G_STRFUNC, error->message);

    GtkWidget *failed_dialog = gtk_message_dialog_new(GTK_WINDOW(priv->dialog),
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_OK,
						      "Failed to retrieve partnership information: %s",
						      error->message);

    gtk_dialog_run(GTK_DIALOG(failed_dialog));
    gtk_widget_destroy (failed_dialog);

    g_error_free(error);
    goto exit;
  }

  store = gtk_tree_store_new (SYNCENG_N_COLUMNS,
			      G_TYPE_BOOLEAN, /* current partner */
			      G_TYPE_INT,     /* partnee index */
			      G_TYPE_UINT,    /* partner id */
			      G_TYPE_STRING,  /* partner guid */
			      G_TYPE_STRING,  /* partnership name */
			      G_TYPE_BOOLEAN, /* active for sync items */
			      G_TYPE_STRING,  /* host name */
			      G_TYPE_STRING   /* device name */
			      );

  index = 0;
  while(index < partnership_list->len)
    {
      gtk_tree_store_append (store, &iter, NULL);  /* Acquire an iterator */

      partnership = g_ptr_array_index(partnership_list, index);

      guint partnership_id = g_value_get_uint(g_value_array_get_nth (partnership, 0));
      const gchar *partnership_guid = g_value_get_string(g_value_array_get_nth (partnership, 1));
      const gchar *partnership_name = g_value_get_string(g_value_array_get_nth (partnership, 2));
      const gchar *hostname = g_value_get_string(g_value_array_get_nth (partnership, 3));
      const gchar *device_name = g_value_get_string(g_value_array_get_nth (partnership, 4));

      /* an array of sync item ids - au - array of uint32 */
      sync_items_active = g_value_get_boxed(g_value_array_get_nth (partnership, 5));

      g_debug("%s: partnership %d id: %d", G_STRFUNC, index, partnership_id);
      g_debug("%s: partnership %d guid: %s", G_STRFUNC, index, partnership_guid);
      g_debug("%s: partnership %d name: %s", G_STRFUNC, index, partnership_name);
      g_debug("%s: partnership %d host name: %s", G_STRFUNC, index, hostname);
      g_debug("%s: partnership %d device name: %s", G_STRFUNC, index, device_name);

      gint i;
      for (i = 0; i < sync_items_active->len; i++) {
        g_debug("%s: partnership %d   sync_items %d: %d", G_STRFUNC, index, i, g_array_index(sync_items_active, guint, i));
      }

      gtk_tree_store_set (store, &iter,
			  SYNCENG_CURRENT_COLUMN, FALSE,
			  SYNCENG_INDEX_COLUMN, index,
			  SYNCENG_ID_COLUMN, partnership_id,
			  SYNCENG_GUID_COLUMN, partnership_guid,
			  SYNCENG_NAME_COLUMN, partnership_name,
			  SYNCENG_ACTIVEITEM_COLUMN, FALSE,
			  SYNCENG_HOSTNAME_COLUMN, hostname,
			  SYNCENG_DEVICENAME_COLUMN, device_name,
			  -1);

#if GLIB_MINOR_VERSION < 14
      g_hash_table_foreach(sync_items,
			   get_sync_item_keys,
			   &item_keys);
#else
      item_keys = g_hash_table_get_keys(sync_items);
#endif

      item_keys = g_list_sort(item_keys, sync_item_sort);
      GList *tmp_list = item_keys;

      gboolean active;

      while (tmp_list) {
	gtk_tree_store_append (store, &sub_iter, &iter);

	active = FALSE;
	gint active_index;
	for (active_index = 0; active_index < sync_items_active->len; active_index++) {
	  if (g_array_index(sync_items_active, guint, active_index) == (*(guint*)tmp_list->data)) {
	    active = TRUE;
	    break;
	  }
	}

	gtk_tree_store_set (store, &sub_iter,
			    SYNCENG_CURRENT_COLUMN, FALSE,
			    SYNCENG_INDEX_COLUMN, 0,
			    SYNCENG_ID_COLUMN, 0,
			    SYNCENG_GUID_COLUMN, NULL,
			    SYNCENG_NAME_COLUMN, g_hash_table_lookup(sync_items, tmp_list->data),
			    SYNCENG_ACTIVEITEM_COLUMN, active,
			    SYNCENG_HOSTNAME_COLUMN, NULL,
			    SYNCENG_DEVICENAME_COLUMN, NULL,
			    -1);
      }

      g_list_free(item_keys);

      index++;
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW(partners_list_view), GTK_TREE_MODEL (store));

  index = 0;
  while(index < partnership_list->len) {
    partnership = g_ptr_array_index(partnership_list, index);
    g_value_array_free(partnership);

    index++;
  }
  g_ptr_array_free(partnership_list, TRUE);

exit:
  if (store) g_object_unref (G_OBJECT (store));
  if (sync_items) g_hash_table_destroy(sync_items);

  if (sync_engine_proxy) g_object_unref(sync_engine_proxy);
  if (dbus_connection) dbus_g_connection_unref(dbus_connection);

  return;
}


static void
partners_setup_view_synceng(WmDeviceInfo *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  GtkWidget *partners_create_button, *partners_remove_button, *partners_list;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  guint32 os_major = 0;

  wm_device_rapi_select(priv->device);

  partners_create_button = glade_xml_get_widget (priv->xml, "partners_create_button");
  partners_remove_button = glade_xml_get_widget (priv->xml, "partners_remove_button");	
  partners_list = glade_xml_get_widget (priv->xml, "partners_list");

  gtk_widget_set_sensitive(partners_create_button, TRUE);
  gtk_widget_set_sensitive(partners_remove_button, FALSE);

  g_signal_connect (G_OBJECT (partners_create_button), "clicked",
		    G_CALLBACK (partners_create_button_clicked_synceng_cb), self);
  g_signal_connect (G_OBJECT (partners_remove_button), "clicked",
		    G_CALLBACK (partners_remove_button_clicked_synceng_cb), self);

  partners_setup_view_store_synceng(self);

  renderer = gtk_cell_renderer_toggle_new ();
  gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer), TRUE);
  column = gtk_tree_view_column_new_with_attributes("C",                                         /* title */
						    renderer,                                    /* renderer to use */
						    "active", SYNCENG_CURRENT_COLUMN,            /* attribute to use and column of store to get it from */
						    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(partners_list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Index",
						    renderer,
						    "text", SYNCENG_INDEX_COLUMN,
						    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(partners_list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Partnership Name",
						    renderer,
						    "text", SYNCENG_NAME_COLUMN,
						    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(partners_list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Host Name",
						    renderer,
						    "text", SYNCENG_HOSTNAME_COLUMN,
						    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(partners_list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Device Name",
						    renderer,
						    "text", SYNCENG_DEVICENAME_COLUMN,
						    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(partners_list), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (partners_list));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (partners_selection_changed_synceng_cb), self);

  gtk_widget_show (partners_list);
}


static void
partners_setup_view_store_rra(WmDeviceInfo *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  GtkWidget *partners_list_view = glade_xml_get_widget (priv->xml, "partners_list");	
  GtkTreeIter iter;
  RRA_Matchmaker* matchmaker = NULL;
  guint32 curr_partner = 0, partner_id, i;
  gchar *partner_name;
  gboolean is_current;
  guint32 os_major = 0;

  GtkListStore *store = gtk_list_store_new (RRA_N_COLUMNS,
					    G_TYPE_BOOLEAN, /* current partner ? */
					    G_TYPE_INT,     /* partnee index */
					    G_TYPE_UINT,    /* partner id */
					    G_TYPE_STRING); /* partner name */

  /* WM5 not yet supported */
  g_object_get(priv->device, "os-major", &os_major, NULL);
  if (os_major > 4) {
    goto exit;
  }

  wm_device_rapi_select(priv->device);

  if (!(matchmaker = rra_matchmaker_new())) {
    g_critical("%s: Failed to create match-maker", G_STRFUNC);
    goto exit;
  }
  rra_matchmaker_get_current_partner(matchmaker, &curr_partner);

  for (i = 1; i <= 2; i++)
    {
      gtk_list_store_append (store, &iter);  /* Acquire an iterator */

      if (!(rra_matchmaker_get_partner_id(matchmaker, i, &partner_id))) {
	g_critical("%s: Failed to get partner %d id", G_STRFUNC, i);
	continue;
      }

      if (!(rra_matchmaker_get_partner_name(matchmaker, i, &partner_name))) {
	g_critical("%s: Failed to get partner %d name", G_STRFUNC, i);
	continue;
      }

      if (i == curr_partner)
	is_current = TRUE;
      else
	is_current = FALSE;

      gtk_list_store_set (store, &iter,
			  RRA_CURRENT_COLUMN, is_current,
			  RRA_INDEX_COLUMN, i,
			  RRA_ID_COLUMN, partner_id,
			  RRA_NAME_COLUMN, partner_name,
			  -1);

      rra_matchmaker_free_partner_name(partner_name);
    }

exit:
  gtk_tree_view_set_model (GTK_TREE_VIEW(partners_list_view), GTK_TREE_MODEL (store));
  g_object_unref (G_OBJECT (store));

  if (matchmaker) rra_matchmaker_destroy(matchmaker);

  return;
}


static void
partners_setup_view_rra(WmDeviceInfo *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  GtkWidget *partners_create_button, *partners_remove_button, *partners_list;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  guint32 os_major = 0;

  wm_device_rapi_select(priv->device);

  partners_create_button = glade_xml_get_widget (priv->xml, "partners_create_button");
  partners_remove_button = glade_xml_get_widget (priv->xml, "partners_remove_button");	
  partners_list = glade_xml_get_widget (priv->xml, "partners_list");

  gtk_widget_set_sensitive(partners_create_button, FALSE);
  gtk_widget_set_sensitive(partners_remove_button, FALSE);

  g_signal_connect (G_OBJECT (partners_create_button), "clicked",
		    G_CALLBACK (partners_create_button_clicked_rra_cb), self);
  g_signal_connect (G_OBJECT (partners_remove_button), "clicked",
		    G_CALLBACK (partners_remove_button_clicked_rra_cb), self);

  partners_setup_view_store_rra(self);

  renderer = gtk_cell_renderer_toggle_new ();
  gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer), TRUE);
  column = gtk_tree_view_column_new_with_attributes("C",
						    renderer,
						    "active", RRA_CURRENT_COLUMN,
						    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(partners_list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Index",
						    renderer,
						    "text", RRA_INDEX_COLUMN,
						    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(partners_list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Partner Id",
						    renderer,
						    "text", RRA_ID_COLUMN,
						    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(partners_list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Partner Name",
						    renderer,
						    "text", RRA_NAME_COLUMN,
						    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(partners_list), column);


  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (partners_list));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (partners_selection_changed_rra_cb), self);

  gtk_widget_show (partners_list);
}

static void
partners_setup_view(WmDeviceInfo *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  /* WM5 uses sync-engine, older uses rra */
  guint32 os_major = 0;
  g_object_get(priv->device, "os-major", &os_major, NULL);
  if (os_major > 4) {
    partners_setup_view_synceng(self);
  } else {
    partners_setup_view_rra(self);
  }
}


/* system info */

/* from librapi2 pstatus */
static const char*
version_string(CEOSVERSIONINFO* version)
{
  const char* result = "Unknown";

  if (version->dwMajorVersion == 4)
    {
      if (version->dwMinorVersion == 20 && version->dwBuildNumber == 1081)
	result = "Ozone: Pocket PC 2003 (?)";
      else if (version->dwMinorVersion == 21 && version->dwBuildNumber == 1088)
	result = "Microsoft Windows Mobile 2003 Pocket PC Phone Edition";
    }
  else if (version->dwMajorVersion == 3 &&
	   version->dwMinorVersion == 0)
    {
      switch (version->dwBuildNumber)
	{
	case 9348:  result = "Rapier: Pocket PC"; break;
	case 11171: result = "Merlin: Pocket PC 2002"; break;

	  /*
	   * From:     Jonathan McDowell
	   * To:       SynCE-Devel
	   * Subject:  Re: [Synce-devel] Smartphone & installing CABs.
	   * Date:     Mon, 26 May 2003 19:12:10 +0100  (20:12 CEST)
	   */
	case 12255: result = "Stinger: Smart Phone 2002"; break;

	  /* My Qtek 7070 */
	case 13121: result = "Stinger: Smart Phone 2002"; break;
	}
    }
  else if (version->dwMajorVersion == 2 &&
	   version->dwMinorVersion == 1)
    {
    result =
      "Gryphon: Windows CE for P/PC V1 (Palm-size PC)"
      " / "
      "Apollo: Windows CE for A/PC V1 (Auto PC)";
    }

  return result;
}

#define PROCESSOR_ARCHITECTURE_COUNT 8

static const char* architecture[] = {
  "Intel",
  "MIPS",
  "Alpha",
  "PPC",
  "SHX",
  "ARM",
  "IA64",
  "ALPHA64"
};


static const gchar*
processor(gint n)
{
  const gchar* result;

  switch (n)
    {
    case PROCESSOR_STRONGARM:    result = "StrongARM";  break;
    case PROCESSOR_MIPS_R4000:   result = "MIPS R4000"; break;
    case PROCESSOR_HITACHI_SH3:  result = "SH3";        break;

    default:
      result = "Unknown";
      g_debug("%s: Unknown processor type, please send your device details to synce-devel@lists.sourceforge.net", G_STRFUNC);
      break;
    }

  return result;
}


static void
system_info_setup_view_store(WmDeviceInfo *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  GtkWidget *sys_info_store_size, *sys_info_store_free,
    *sys_info_store_ram, *sys_info_store_storage;
  STORE_INFORMATION store;
  DWORD storage_pages = 0, ram_pages = 0, page_size = 0;

  wm_device_rapi_select(priv->device);

  memset(&store, 0, sizeof(store));

  sys_info_store_size = glade_xml_get_widget (priv->xml, "sys_info_store_size");
  sys_info_store_free = glade_xml_get_widget (priv->xml, "sys_info_store_free");
  sys_info_store_ram = glade_xml_get_widget (priv->xml, "sys_info_store_ram");
  sys_info_store_storage = glade_xml_get_widget (priv->xml, "sys_info_store_storage");

  if (CeGetStoreInformation(&store)) {
    gchar *store_size = g_strdup_printf("%i bytes (%i MB)", store.dwStoreSize, store.dwStoreSize / (1024*1024));
    gchar *free_space = g_strdup_printf("%i bytes (%i MB)", store.dwFreeSize,  store.dwFreeSize  / (1024*1024));

    gtk_label_set_text(GTK_LABEL(sys_info_store_size), store_size);
    gtk_label_set_text(GTK_LABEL(sys_info_store_free), free_space);

    g_free(store_size);
    g_free(free_space);
  } else {
    g_warning("%s: Failed to get store information: %s",
	      G_STRFUNC,
	      synce_strerror(CeGetLastError()));
  }

  if (CeGetSystemMemoryDivision(&storage_pages, &ram_pages, &page_size)) {
    gchar *storage_size = g_strdup_printf("%i bytes (%i MB)", storage_pages * page_size, storage_pages * page_size / (1024*1024));
    gchar *ram_size = g_strdup_printf("%i bytes (%i MB)", ram_pages * page_size, ram_pages * page_size / (1024*1024));

    gtk_label_set_text(GTK_LABEL(sys_info_store_ram), ram_size);
    gtk_label_set_text(GTK_LABEL(sys_info_store_storage), storage_size);

    g_free(storage_size);
    g_free(ram_size);
  }
}

static void
system_info_setup_view(WmDeviceInfo *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  GtkWidget *sys_info_model, *sys_info_version, *sys_info_platform, *sys_info_details,
    *sys_info_proc_arch, *sys_info_proc_type, *sys_info_page_size;
  CEOSVERSIONINFO version;
  SYSTEM_INFO system;
  gchar *class, *hardware, *model_str;

  wm_device_rapi_select(priv->device);

  sys_info_model = glade_xml_get_widget (priv->xml, "sys_info_model");
  g_object_get(priv->device, "class", &class, NULL);
  g_object_get(priv->device, "hardware", &hardware, NULL);
  model_str = g_strdup_printf("%s (%s)", hardware, class);
  gtk_label_set_text(GTK_LABEL(sys_info_model), model_str);
  g_free(class);
  g_free(hardware);
  g_free(model_str);

  /* Version */

  sys_info_version = glade_xml_get_widget (priv->xml, "sys_info_version");
  sys_info_platform = glade_xml_get_widget (priv->xml, "sys_info_platform");
  sys_info_details = glade_xml_get_widget (priv->xml, "sys_info_details");

  memset(&version, 0, sizeof(version));
  version.dwOSVersionInfoSize = sizeof(version);

  if (CeGetVersionEx(&version)) {
    char *details = wstr_to_current(version.szCSDVersion);
    char *platform = NULL;

    if (VER_PLATFORM_WIN32_CE == version.dwPlatformId)
      platform = "(Windows CE)";

    gchar *version_str = g_strdup_printf("%i.%i.%i (%s)",
					 version.dwMajorVersion,
					 version.dwMinorVersion,
					 version.dwBuildNumber,
					 version_string(&version));

    gchar *platform_str = g_strdup_printf("%i %s",
					  version.dwPlatformId,
					  platform ? platform : "");

    gtk_label_set_text(GTK_LABEL(sys_info_version), version_str);
    gtk_label_set_text(GTK_LABEL(sys_info_platform), platform_str);
    gtk_label_set_text(GTK_LABEL(sys_info_details), details);

    g_free(version_str);
    g_free(platform_str);
    wstr_free_string(details);
  } else {
    g_warning("%s: Failed to get version information: %s",
	       G_STRFUNC,
	       synce_strerror(CeGetLastError()));
  }

  /* platform */

  sys_info_proc_arch = glade_xml_get_widget (priv->xml, "sys_info_proc_arch");
  sys_info_proc_type = glade_xml_get_widget (priv->xml, "sys_info_proc_type");
  sys_info_page_size = glade_xml_get_widget (priv->xml, "sys_info_page_size");

  memset(&system, 0, sizeof(system));

  CeGetSystemInfo(&system);
  {
    gchar *proc_arch_str = g_strdup_printf("%i (%s)",
					   system.wProcessorArchitecture,
					   (system.wProcessorArchitecture < PROCESSOR_ARCHITECTURE_COUNT) ?
					   architecture[system.wProcessorArchitecture] : "Unknown");

    gchar *proc_type_str = g_strdup_printf("%i (%s)",
					   system.dwProcessorType,
					   processor(system.dwProcessorType));

    gchar *page_size_str = g_strdup_printf("0x%x",
					   system.dwAllocationGranularity);

    gtk_label_set_text(GTK_LABEL(sys_info_proc_arch), proc_arch_str);
    gtk_label_set_text(GTK_LABEL(sys_info_proc_type), proc_type_str);
    gtk_label_set_text(GTK_LABEL(sys_info_page_size), page_size_str);

    g_free(proc_arch_str);
    g_free(proc_type_str);
    g_free(page_size_str);
  }

  /* store */

  system_info_setup_view_store(self);

}


/* power */

static const gchar*
get_ACLineStatus_string(unsigned ACLineStatus)
{
  const gchar* status;

  switch (ACLineStatus)
    {
    case AC_LINE_OFFLINE:       status = "Offline";       break;
    case AC_LINE_ONLINE:        status = "Online";        break;
    case AC_LINE_BACKUP_POWER:  status = "Backup Power";  break;
    case AC_LINE_UNKNOWN:       status = "Unknown";       break;
    default:                    status = "Invalid";       break;
    }

  return status;
}

static const gchar*
get_battery_flag_string(unsigned flag)
{
  const gchar* name;

  switch (flag)
    {
    case BATTERY_FLAG_HIGH:        name = "High";       break;
    case BATTERY_FLAG_LOW:         name = "Low";        break;
    case BATTERY_FLAG_CRITICAL:    name = "Critical";   break;
    case BATTERY_FLAG_CHARGING:    name = "Charging";   break;
    case BATTERY_FLAG_NO_BATTERY:  name = "No Battery"; break;

    default: name = "Unknown"; break;
    }

  return name;
}


static void
system_power_setup_view(WmDeviceInfo *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  wm_device_rapi_select(priv->device);

  SYSTEM_POWER_STATUS_EX power;
  GtkWidget *sys_info_ac_status,
    *sys_info_main_batt_lifetime, *sys_info_main_batt_fulllife, *sys_info_main_batt_bar,
    *sys_info_bup_batt_lifetime, *sys_info_bup_batt_fulllife, *sys_info_bup_batt_bar;
  gchar *lifetime, *fulllife, *lifepercent;

  sys_info_ac_status = glade_xml_get_widget (priv->xml, "sys_info_ac_status");

  sys_info_main_batt_lifetime = glade_xml_get_widget (priv->xml, "sys_info_main_batt_lifetime");
  sys_info_main_batt_fulllife = glade_xml_get_widget (priv->xml, "sys_info_main_batt_fulllife");
  sys_info_main_batt_bar = glade_xml_get_widget (priv->xml, "sys_info_main_batt_bar");

  sys_info_bup_batt_lifetime = glade_xml_get_widget (priv->xml, "sys_info_bup_batt_lifetime");
  sys_info_bup_batt_fulllife = glade_xml_get_widget (priv->xml, "sys_info_bup_batt_fulllife");
  sys_info_bup_batt_bar = glade_xml_get_widget (priv->xml, "sys_info_bup_batt_bar");

  memset(&power, 0, sizeof(SYSTEM_POWER_STATUS_EX));

  if (!(CeGetSystemPowerStatusEx(&power, false))) {
    g_warning("%s: Failed to get battery status: %s",
	      G_STRFUNC,
	      synce_strerror(CeGetLastError()));
    return;
  }

  gchar *acstatus = g_strdup_printf("%02x (%s)", power.ACLineStatus, get_ACLineStatus_string(power.ACLineStatus));
  gtk_label_set_text(GTK_LABEL(sys_info_ac_status), acstatus);
  g_free(acstatus);

  if (BATTERY_LIFE_UNKNOWN == power.BatteryLifeTime)
    lifetime = g_strdup("Unknown");
  else
    lifetime = g_strdup_printf("%i", power.BatteryLifeTime);
  gtk_label_set_text(GTK_LABEL(sys_info_main_batt_lifetime), lifetime);
  g_free(lifetime);

  if (BATTERY_LIFE_UNKNOWN == power.BatteryFullLifeTime)
    fulllife = g_strdup("Unknown");
  else
    fulllife = g_strdup_printf("%i", power.BatteryFullLifeTime);
  gtk_label_set_text(GTK_LABEL(sys_info_main_batt_fulllife), fulllife);
  g_free(fulllife);

  if (BATTERY_PERCENTAGE_UNKNOWN == power.BatteryLifePercent) {
    lifepercent = g_strdup_printf("%s (%% Unknown)", get_battery_flag_string(power.BatteryFlag));
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sys_info_main_batt_bar), lifepercent);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sys_info_main_batt_bar), 0.0);
  } else {
    lifepercent = g_strdup_printf("%s (%i%%)", get_battery_flag_string(power.BatteryFlag), power.BatteryLifePercent);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sys_info_main_batt_bar), lifepercent);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sys_info_main_batt_bar), (gdouble)(power.BatteryLifePercent / 100.0));
  }
  g_free(lifepercent);

  if (BATTERY_LIFE_UNKNOWN == power.BackupBatteryLifeTime)
    lifetime = g_strdup("Unknown");
  else
    lifetime = g_strdup_printf("%i", power.BackupBatteryLifeTime);
  gtk_label_set_text(GTK_LABEL(sys_info_bup_batt_lifetime), lifetime);
  g_free(lifetime);

  if (BATTERY_LIFE_UNKNOWN == power.BackupBatteryFullLifeTime)
    fulllife = g_strdup("Unknown");
  else
    fulllife = g_strdup_printf("%i", power.BackupBatteryFullLifeTime);
  gtk_label_set_text(GTK_LABEL(sys_info_bup_batt_fulllife), fulllife);
  g_free(fulllife);

  if (BATTERY_PERCENTAGE_UNKNOWN == power.BackupBatteryLifePercent) {
    lifepercent = g_strdup_printf("%s (%% Unknown)", get_battery_flag_string(power.BackupBatteryFlag));
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sys_info_bup_batt_bar), lifepercent);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sys_info_bup_batt_bar), 0.0);
  } else {
    lifepercent = g_strdup_printf("%s (%i%%)", get_battery_flag_string(power.BackupBatteryFlag), power.BackupBatteryLifePercent);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sys_info_bup_batt_bar), lifepercent);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sys_info_bup_batt_bar), (gdouble)(power.BackupBatteryLifePercent / 100.0));
  }
  g_free(lifepercent);

}


static void
device_info_refresh_button_clicked_cb (GtkWidget *widget, gpointer data)
{
  WmDeviceInfo *self = WM_DEVICE_INFO(data);
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  guint32 os_major = 0;

  /* WM5 uses sync-engine, older uses rra */
  g_object_get(priv->device, "os-major", &os_major, NULL);
  if (os_major > 4) {
    partners_setup_view_store_synceng(self);
  } else {
    partners_setup_view_store_rra(self);
  }

  system_info_setup_view_store(self);
  system_power_setup_view(self);
}


static void
device_info_close_button_clicked_cb (GtkWidget *widget, gpointer data)
{
  WmDeviceInfo *self = WM_DEVICE_INFO(data);
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }
  gtk_widget_hide(priv->dialog);
  g_signal_emit (self, WM_DEVICE_INFO_GET_CLASS (self)->signals[DEVICE_INFO_CLOSED], 0);
}

static void
device_info_setup_dialog (WmDeviceInfo *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  GtkWidget *device_info_dialog_close, *device_info_dialog_refresh;
  gchar *device_name, *title;

  priv->xml = glade_xml_new (SYNCE_DATA "synce_trayicon_properties.glade", "device_info_dialog", NULL);
  priv->dialog = glade_xml_get_widget (priv->xml, "device_info_dialog");

  device_info_dialog_close = glade_xml_get_widget (priv->xml, "device_info_dialog_close");
  device_info_dialog_refresh = glade_xml_get_widget (priv->xml, "device_info_dialog_refresh");

  g_signal_connect (G_OBJECT (device_info_dialog_close), "clicked",
		    G_CALLBACK (device_info_close_button_clicked_cb), self);

  g_signal_connect (G_OBJECT (device_info_dialog_refresh), "clicked",
		    G_CALLBACK (device_info_refresh_button_clicked_cb), self);

  g_object_get(priv->device, "name", &device_name, NULL);
  title = g_strdup_printf("%s Information", device_name);
  gtk_window_set_title(GTK_WINDOW(priv->dialog), title);
  g_free(device_name);
  g_free(title);

  partners_setup_view(self);
  system_info_setup_view(self);
  system_power_setup_view(self);

  gtk_widget_show_all (priv->dialog);

  return;
}

void
device_removed(gpointer data, GObject *removed_device)
{
  WmDeviceInfo *self = WM_DEVICE_INFO(data);
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);
  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }
  gtk_widget_hide(priv->dialog);
  g_signal_emit (self, WM_DEVICE_INFO_GET_CLASS (self)->signals[DEVICE_INFO_CLOSED], 0);
}


/* class & instance functions */

static void
wm_device_info_init(WmDeviceInfo *self)
{
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);

  priv->disposed = FALSE;
  priv->device = NULL;
}


static void
wm_device_info_get_property (GObject    *obj,
			     guint       property_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
  WmDeviceInfo *self = WM_DEVICE_INFO (obj);
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);

  switch (property_id) {

  case PROP_DEVICE:
    g_value_set_pointer (value, priv->device);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
    break;
  }
}


static void
wm_device_info_set_property (GObject      *obj,
			     guint         property_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  WmDeviceInfo *self = WM_DEVICE_INFO (obj);
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);

  switch (property_id) {

  case PROP_DEVICE:
    priv->device = g_value_get_pointer (value);
    g_object_weak_ref(G_OBJECT(priv->device),
		      device_removed,
		      self);
    device_info_setup_dialog(self);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
    break;
  }
}

static void
wm_device_info_dispose (GObject *obj)
{
  WmDeviceInfo *self = WM_DEVICE_INFO(obj);
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);

  if (priv->disposed) {
    return;
  }
  priv->disposed = TRUE;

  /* unref other objects */

  g_object_weak_unref(G_OBJECT(priv->device),
		      device_removed,
		      self);
  priv->device = NULL;
  gtk_widget_destroy(priv->dialog);
  g_object_unref(priv->xml);

  if (G_OBJECT_CLASS (wm_device_info_parent_class)->dispose)
    G_OBJECT_CLASS (wm_device_info_parent_class)->dispose (obj);
}


static void
wm_device_info_finalize (GObject *obj)
{
  WmDeviceInfo *self = WM_DEVICE_INFO(obj);
  WmDeviceInfoPrivate *priv = WM_DEVICE_INFO_GET_PRIVATE (self);

  if (G_OBJECT_CLASS (wm_device_info_parent_class)->finalize)
    G_OBJECT_CLASS (wm_device_info_parent_class)->finalize (obj);
}

static void
wm_device_info_class_init (WmDeviceInfoClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *param_spec;

  g_type_class_add_private (klass, sizeof (WmDeviceInfoPrivate));
  
  gobject_class->get_property = wm_device_info_get_property;
  gobject_class->set_property = wm_device_info_set_property;

  gobject_class->dispose = wm_device_info_dispose;
  gobject_class->finalize = wm_device_info_finalize;

  klass->signals[DEVICE_INFO_CLOSED] = g_signal_new ("device-info-closed",
						     G_TYPE_FROM_CLASS (klass),
						     G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
						     0,
						     NULL, NULL,
						     g_cclosure_marshal_VOID__VOID,
						     G_TYPE_NONE, 0);
  klass->signals[DEVICE_INFO_LAST_SIGNAL] = 0;

  param_spec = g_param_spec_pointer ("device", "Device",
                                     "The device object",
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_STATIC_NICK |
                                     G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_DEVICE, param_spec);
}
