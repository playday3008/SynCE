#ifdef HAVE_CONFIG_H
#include "multisync_plugin_config.h"
#endif
#include <stdlib.h>
#include <glib.h>
#include <gmodule.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <libgnomeui/libgnomeui.h>
#include <glade/glade-xml.h>
#include <rapi.h>
#include <synce_log.h>
#include "multisync_plugin.h"
#include "gui.h"

GtkWidget *syncewindow = NULL;
Synce_Partner synce_partner_1;
Synce_Partner synce_partner_2;
GtkWidget *partner_menu = NULL;
GtkWidget *partner_option_menu = NULL;
GtkWidget *get_all_button = NULL;
int new_partner_index = 0;
int replace = 0;

uint32_t current_partner;
SynceConnection *connection = NULL;

static void synce_error_dialog(const char *message)
{
  GtkWidget *error_dialog;

  error_dialog = gnome_message_box_new (
      message,
      GNOME_MESSAGE_BOX_ERROR,
          GTK_STOCK_OK, 
      NULL);

  gnome_dialog_run (GNOME_DIALOG (error_dialog));
}

GtkWidget* open_option_window(sync_pair *pair, connection_type type)
{
    GladeXML *gladefile = NULL;
  
  GtkWidget *new_partner_button;

  GtkWidget *ok_button;
  GtkWidget *cancel_button;

  HRESULT hr;
  uint32_t id;
  char *name;
        
  connection = synce_connection_new(pair, type);
  synce_connection_load_state(connection);

  if(!syncewindow)
  {
    synce_partner_1.name = NULL;
    synce_partner_1.id = 0;
    synce_partner_1.current = FALSE;
    synce_partner_1.changed = FALSE;
    synce_partner_1.exist = FALSE;

    synce_partner_2.name = NULL;
    synce_partner_2.id = 0;
    synce_partner_2.current = FALSE;
    synce_partner_2.changed = FALSE;
    synce_partner_2.exist = FALSE;

    hr = CeRapiInit();
    if(FAILED(hr))
    {
      synce_error_dialog(_("Failed to connect to PDA."));
      goto exit;
    }

    connection->rra = rra_new();

    if(!rra_partner_get_current(connection->rra, &current_partner))
    {
      synce_error_dialog(_("Failed to get current partner index."));
      goto exit;
    }

    if(rra_partner_get_name(connection->rra, 1, &name))
    {
      synce_partner_1.name = name;
      if(current_partner == 1)
        synce_partner_1.current = TRUE;
      else
        synce_partner_1.current = FALSE;
      synce_partner_1.changed = FALSE;
      synce_partner_1.exist = TRUE;
    }
    if (rra_partner_get_id(connection->rra, 1, &id))
    {
      synce_partner_1.id = id;
    }

    if(rra_partner_get_name(connection->rra, 2, &name))
    {
      synce_partner_2.name = name;
      if(current_partner == 2)
        synce_partner_2.current = TRUE;
      else
        synce_partner_2.current = FALSE;
      synce_partner_2.changed = FALSE;
      synce_partner_2.exist = TRUE;
    }
    if (rra_partner_get_id(connection->rra, 2, &id))
    {
      synce_partner_2.id = id;
    }

    if(current_partner == 1)
    {
      synce_partner_1.current = TRUE;
      synce_partner_2.current = FALSE;
    }
    else if(current_partner == 2)
    {
      synce_partner_1.current = FALSE;
      synce_partner_2.current = TRUE;
    }

    gladefile = glade_xml_new(SYNCE_MULTISYNC_GLADEFILE, "syncewindow", NULL); 
    syncewindow = glade_xml_get_widget(gladefile, "syncewindow");
    gtk_signal_connect(GTK_OBJECT(syncewindow), "delete_event", GTK_SIGNAL_FUNC(synce_window_closed), NULL);

    partner_option_menu = glade_xml_get_widget(gladefile,"partner_option_menu");
    synce_build_partner_menu(partner_option_menu);

    get_all_button = glade_xml_get_widget(gladefile,"get_all_button");

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(get_all_button), connection->get_all);

    new_partner_button = glade_xml_get_widget(gladefile,"new_partner_button");
    ok_button = glade_xml_get_widget(gladefile,"ok_button");
    cancel_button = glade_xml_get_widget(gladefile,"cancel_button");

    gtk_signal_connect (GTK_OBJECT(new_partner_button), "clicked", GTK_SIGNAL_FUNC(synce_new_partner_button), NULL);
    gtk_signal_connect (GTK_OBJECT(ok_button), "clicked", GTK_SIGNAL_FUNC(synce_ok_button), NULL);
    gtk_signal_connect (GTK_OBJECT(cancel_button), "clicked", GTK_SIGNAL_FUNC(synce_cancel_button), NULL);
    
    /*gtk_widget_realize(syncewindow); */
    gtk_widget_show_all(syncewindow);
  }

exit:
  return syncewindow;
}

void synce_build_partner_menu(GtkWidget *partner_option_menu)
{
  GtkWidget *partner_item;

  gtk_widget_hide(partner_option_menu);

  partner_menu = gtk_menu_new();

  if(synce_partner_1.exist == TRUE)
  {
    partner_item = gtk_menu_item_new_with_label(synce_partner_1.name);
    gtk_menu_append(GTK_MENU(partner_menu), partner_item);
    gtk_signal_connect_object (GTK_OBJECT(partner_item), "activate", GTK_SIGNAL_FUNC(synce_partner_menu_response_1), NULL);
  }

  if(synce_partner_2.exist == TRUE)
  {
    partner_item = gtk_menu_item_new_with_label(synce_partner_2.name);
    gtk_menu_append(GTK_MENU(partner_menu), partner_item);
    gtk_signal_connect_object (GTK_OBJECT(partner_item), "activate", GTK_SIGNAL_FUNC(synce_partner_menu_response_2), NULL);
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(partner_option_menu), partner_menu);

  if(synce_partner_1.current == TRUE)
    gtk_option_menu_set_history(GTK_OPTION_MENU(partner_option_menu), 0);
  else	if(synce_partner_2.current == TRUE)
    gtk_option_menu_set_history(GTK_OPTION_MENU(partner_option_menu), 1);

  gtk_widget_show(partner_menu);
  gtk_widget_show(partner_option_menu);

}

void synce_partner_menu_response_1(GtkMenuItem *menuitem, gpointer data)
{
  synce_partner_1.current = TRUE;
  synce_partner_2.current = FALSE;
}

void synce_partner_menu_response_2(GtkMenuItem *menuitem, gpointer data)
{
  synce_partner_1.current = FALSE;
  synce_partner_2.current = TRUE;
}

gboolean synce_jump_to_page (GnomeDruidPage *druidpage,
                                 GtkWidget *widget,
                                 gpointer user_data) {
    GnomeDruidPage *name_druidpage = (GnomeDruidPage *) user_data;
    
    gnome_druid_set_page (GNOME_DRUID(widget), name_druidpage);
    return TRUE;
}

static void
synce_tree_selection_changed (GtkTreeSelection *selection, gpointer user_data) 
{
    GtkWidget *druid = (GtkWidget *) user_data;
    GtkTreeIter iter;
    GtkTreeModel *model;
    gint number;
    gchar *author;

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
        gtk_tree_model_get (model, &iter, 1, &author,0,&number, -1);

        new_partner_index = number;
       	replace = 1; 
/* to be used to set next insensitive until something is 
 * selected, it doesn't work for some reason */
#if 0
        gnome_druid_set_buttons_sensitive
            (GNOME_DRUID(druid),
             TRUE,
             TRUE,
             TRUE,
             TRUE);
#endif 
        g_free (author);
    }
}

void synce_setup_replace_treeview(GtkWidget *replace_treeview, GtkWidget *druid) {
        
    GtkListStore *store = gtk_list_store_new (2, G_TYPE_INT, G_TYPE_STRING);
        GtkTreeIter iter;
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        GtkTreeSelection *selection;

        
        gtk_list_store_append (store, &iter);  /* Acquire an iterator */

        gtk_list_store_set (store, &iter,
                0, 1,
                1, synce_partner_1.name,
                -1);
        gtk_list_store_append (store, &iter);  /* Acquire an iterator */
        gtk_list_store_set (store, &iter,
                0, 2,
                1, synce_partner_2.name,
                -1);

        gtk_tree_view_set_model (GTK_TREE_VIEW(replace_treeview), GTK_TREE_MODEL (store));
        g_object_unref (G_OBJECT (store));
        
        
        column = gtk_tree_view_column_new ();
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                             "text", 0, NULL);
       
        gtk_tree_view_column_set_visible(GTK_TREE_VIEW_COLUMN(column), false);
        gtk_tree_view_append_column (GTK_TREE_VIEW(replace_treeview), column);
        
        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_set_title (GTK_TREE_VIEW_COLUMN(column),"Name");
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                             "text", 1, NULL);
       
        gtk_tree_view_append_column (GTK_TREE_VIEW(replace_treeview), column);
       
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (replace_treeview));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
        g_signal_connect (G_OBJECT (selection), "changed",
                G_CALLBACK (synce_tree_selection_changed), druid);
        
        gtk_widget_show (replace_treeview);

}

/* to be used to set next insensitive until something is 
 * selected, it doesn't work for some reason */
#if 0
void  synce_prepare_replace_page (GnomeDruidPage *druidpage,
        GtkWidget *widget,
        gpointer user_data) 
{
    GtkWidget *replace_treeview = (GtkWidget *) user_data;
    GtkTreeSelection *selection;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (replace_treeview));
    if (!gtk_tree_selection_count_selected_rows(selection)) {
        printf ("gnome_druid_set_buttons_sensitive stuff\n"); 
        gnome_druid_set_buttons_sensitive (
                GNOME_DRUID(widget),
                TRUE,
                FALSE,
                TRUE,
                TRUE);
    } else {
        printf ("Keep all buttons sensitive\n"); 

    }

}
#endif

void synce_cancel_druid (GnomeDruid *druid,
        gpointer user_data) {
	new_partner_index=0;
    gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(druid)));
}

void synce_finish_partnership_druid (GnomeDruidPage *druidpage,
        GtkWidget *widget,
        gpointer user_data)
{
    if (new_partner_index == 0) {
        /* Error */
        synce_error_dialog (_("You must select a partnership to replace!\nGo back and select one of the existing\npartnerships from the list, or click on\n the Cancel button if you want to keep\nyour current setup."));
        return;
    }
   
  /* FIXME: The primary task left is to add the code to create a 
   * new (or replace an old) partnership. Which one to create or 
   * is stored in new_parner_index (global). The name is stored 
   * in name.
   */
	if (replace) {
		rra_partner_replace(connection->rra,new_partner_index);
	} else {
		rra_partner_create(connection->rra,&new_partner_index);
	}
    rra_partner_set_current(connection->rra, new_partner_index);

    gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(druidpage)));
}

/* Since this function manipulates the menu, the druid must be destroyed
 * before the option window. If the option window is destroyed before a 
 * segfault may occur. We should add some mechanism to ensure this doesn't 
 * happen. 
 */
void on_newpartnershipwindow_destroy (GtkObject *object, gpointer user_data) 
{
	if (new_partner_index != 0) {
		gtk_option_menu_set_history(GTK_OPTION_MENU(partner_option_menu), 
									new_partner_index-1);
	}
}

void synce_new_partner_button(GtkButton *button, gpointer user_data)
{
    GladeXML *gladefile = NULL;
    GtkWidget *newpartnershipwindow;
    GtkWidget *replace_druidpage;
    GtkWidget *start_druidpage;
    GtkWidget *finish_druidpage;
    GtkWidget *replace_treeview;
    GtkWidget *partnershipdruid;
    
        gladefile = glade_xml_new(SYNCE_MULTISYNC_GLADEFILE, "newpartnershipwindow", NULL); 

    newpartnershipwindow = glade_xml_get_widget(gladefile,"newpartnershipwindow");
    partnershipdruid = glade_xml_get_widget(gladefile,"partnershipdruid");
    gtk_signal_connect(GTK_OBJECT(partnershipdruid), "cancel", GTK_SIGNAL_FUNC(synce_cancel_druid),NULL);
    replace_treeview = glade_xml_get_widget(gladefile,"replace_treeview");
    
    replace_druidpage = glade_xml_get_widget(gladefile,"replace_druidpage");
    start_druidpage = glade_xml_get_widget(gladefile,"start_druidpage");
    finish_druidpage = glade_xml_get_widget(gladefile,"finish_druidpage");

    gtk_signal_connect(GTK_OBJECT(finish_druidpage), "finish", GTK_SIGNAL_FUNC(synce_finish_partnership_druid),NULL);
    
/* to be used to set next insensitive until something is 
 * selected, it doesn't work for some reason */
#if 0
    gtk_signal_connect(GTK_OBJECT(replace_druidpage), "prepare", GTK_SIGNAL_FUNC(synce_prepare_replace_page), replace_treeview);
#endif

    if (synce_partner_1.exist && synce_partner_2.exist) {
        /* Must replace a partner */
        synce_setup_replace_treeview(replace_treeview, partnershipdruid);
        
    } else {
        gtk_signal_connect(GTK_OBJECT(start_druidpage), "next", GTK_SIGNAL_FUNC(synce_jump_to_page), finish_druidpage);
        gtk_signal_connect(GTK_OBJECT(finish_druidpage), "back", GTK_SIGNAL_FUNC(synce_jump_to_page), start_druidpage);
        if (synce_partner_1.exist) {
            /* If only partner 1 exists, create new partner 2 */
            new_partner_index = 2;

        } else {
            /* If only partner 2 exists, create new partner 1 */
            new_partner_index = 1;

        }
    }

	gtk_signal_connect(GTK_OBJECT(newpartnershipwindow), "destroy", GTK_SIGNAL_FUNC(on_newpartnershipwindow_destroy), NULL);
    gtk_widget_show_all(newpartnershipwindow);
}

void synce_ok_button(GtkButton *button, gpointer user_data)
{
  uint32_t new_partner = 0;

  if (synce_partner_1.current)
    new_partner = 1;
  else if (synce_partner_2.current)
    new_partner = 2;

  if (new_partner && new_partner != current_partner)
  {
    if(!rra_partner_set_current(connection->rra, new_partner))
    {
      synce_error_dialog(_("Failed to set current partner index."));
    }			
  }
    
  connection->get_all = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(get_all_button));
  
  synce_connection_save_state(connection);

  synce_window_closed();
}

void synce_cancel_button(GtkButton *button, gpointer user_data)
{
  synce_window_closed();
}

void synce_window_closed(void)
{
  free(synce_partner_1.name);
  free(synce_partner_2.name);
  gtk_widget_destroy(syncewindow);
  sync_plugin_window_closed();
  syncewindow = NULL;

  synce_connection_destroy(connection);
}
