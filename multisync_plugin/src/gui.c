#include <stdlib.h>
#include <glib.h>
#include <gmodule.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gnome.h>
#include <rapi.h>
#include <synce_log.h>
#include "syncengine.h"
#include "gui.h"
#include "multisync_plugin.h"

GtkWidget *syncewindow = NULL;
Synce_Partner synce_partner_1;
Synce_Partner synce_partner_2;
GtkWidget *partner_option_menu;
GtkWidget *partner_menu;
GtkWidget *get_all_button;

uint32_t current_partner;
SynceConnection *connection = NULL;

static void synce_error_dialog(const char *message)
{
  GtkWidget *error_dialog;

  error_dialog = gnome_message_box_new (
      message,
      GNOME_MESSAGE_BOX_ERROR,
      GNOME_STOCK_BUTTON_OK,
      NULL);

  gnome_dialog_run (GNOME_DIALOG (error_dialog));
}

GtkWidget* open_option_window(sync_pair *pair, connection_type type)
{
  GtkWidget *v_box;
  GtkWidget *h_box;
  GtkWidget *button_box;
  GtkWidget *separator;

  GtkWidget *label;
  
  GtkWidget *new_partner_button;
  GtkWidget *replace_partner_button;

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
      synce_error_dialog("Failed to connect to PDA.");
      goto exit;
    }

    connection->rra = rra_new();

    if(!rra_partner_get_current(connection->rra, &current_partner))
    {
      synce_error_dialog("Failed to get current partner index.");
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

    syncewindow = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_window_set_title (GTK_WINDOW(syncewindow), "SynCE Options");
    gtk_window_set_policy(GTK_WINDOW(syncewindow), FALSE, FALSE, TRUE);
    gtk_signal_connect(GTK_OBJECT(syncewindow), "delete_event", GTK_SIGNAL_FUNC(synce_window_closed), NULL);

    v_box = gtk_vbox_new(TRUE, 5);
    gtk_container_add(GTK_CONTAINER(syncewindow), v_box);

    h_box = gtk_hbox_new(FALSE, 5);		
    label = gtk_label_new("Current partner:");
    partner_option_menu = gtk_option_menu_new();
    synce_build_partner_menu();
    gtk_box_pack_start(GTK_BOX(h_box), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(h_box), partner_option_menu, TRUE, TRUE, 0);
    gtk_widget_show_all(h_box);
    gtk_box_pack_start(GTK_BOX(v_box), h_box, TRUE, TRUE, 0);

    /*		button_box =  gtk_hbutton_box_new();		
          replace_partner_button = gtk_button_new_with_label("Replace old");
          gtk_signal_connect (GTK_OBJECT(replace_partner_button), "clicked", GTK_SIGNAL_FUNC(synce_replace_old_partner), NULL);
          new_partner_button = gtk_button_new_with_label("Create new");
          gtk_signal_connect (GTK_OBJECT(new_partner_button), "clicked", GTK_SIGNAL_FUNC(synce_create_new_partner), NULL);
          gtk_box_pack_start(GTK_BOX(button_box), replace_partner_button, FALSE, FALSE, 5);
          gtk_box_pack_start(GTK_BOX(button_box), new_partner_button, FALSE, FALSE, 5);
          gtk_widget_show_all(button_box);
          gtk_box_pack_start(GTK_BOX(v_box), button_box, TRUE, TRUE, 0);*/

    separator = gtk_hseparator_new();
    gtk_widget_show(separator);
    gtk_box_pack_start(GTK_BOX(v_box), separator, TRUE, TRUE, 0);

    button_box =  gtk_hbutton_box_new();
    get_all_button = gtk_check_button_new_with_label("Get all items from PDA on next synchronization");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(get_all_button), connection->get_all);
    gtk_box_pack_start(GTK_BOX(button_box), get_all_button, FALSE, FALSE, 5);
    gtk_widget_show_all(button_box);
    gtk_box_pack_start(GTK_BOX(v_box), button_box, TRUE, TRUE, 0);

    separator = gtk_hseparator_new();
    gtk_widget_show(separator);
    gtk_box_pack_start(GTK_BOX(v_box), separator, TRUE, TRUE, 0);

    button_box =  gtk_hbutton_box_new();
    ok_button = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
    cancel_button = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
    gtk_signal_connect (GTK_OBJECT(ok_button), "clicked", GTK_SIGNAL_FUNC(synce_ok_button), NULL);
    gtk_signal_connect (GTK_OBJECT(cancel_button), "clicked", GTK_SIGNAL_FUNC(synce_cancel_button), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), ok_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(button_box), cancel_button, FALSE, FALSE, 5);
    gtk_widget_show_all(button_box);
    gtk_box_pack_start(GTK_BOX(v_box), button_box, TRUE, TRUE, 0);

    gtk_widget_show(v_box);
    gtk_container_set_border_width(GTK_CONTAINER(syncewindow), 10);
    gtk_widget_realize(syncewindow);
    gtk_widget_show(syncewindow);
  }

exit:
  return syncewindow;
}

void synce_build_partner_menu()
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

void synce_replace_old_partner(GtkButton *button, gpointer user_data)
{
}

void synce_create_new_partner(GtkButton *button, gpointer user_data)
{
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
      synce_error_dialog("Failed to set current partner index.");
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
