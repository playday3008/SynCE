#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnome.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>

extern GConfClient *synce_conf_client;
GladeXML *xml;

static void
enable_passwd_updated (GConfClient *client, guint id, GConfEntry *entry,
			        gpointer data)
{
	GtkWidget *password_label;
	GtkWidget *password_entry;
	gboolean enable_password;
	enable_password = gconf_client_get_bool (synce_conf_client,
			                "/apps/synce/trayicon/enable_password", NULL);

    password_label = glade_xml_get_widget (xml, "password_label");    
    password_entry = glade_xml_get_widget (xml, "password_entry");    

	gtk_widget_set_sensitive(password_label, enable_password);
	gtk_widget_set_sensitive(password_entry, enable_password);
}


void password_checkbutton_toggled_cb (GtkWidget *widget, gpointer data)
{
    gboolean state;
                                                                                
    state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    gconf_client_set_bool (synce_conf_client,
            "/apps/synce/trayicon/enable_password", state, NULL);
}

void password_entry_activate_cb (GtkWidget *widget, gpointer data)
{
    gchar *passwd;
                                                                                
    passwd = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
    gconf_client_set_string (synce_conf_client,
            "/apps/synce/trayicon/password", passwd, NULL);
	g_free(passwd);
}

void close_button_clicked_cb (GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(gtk_widget_get_toplevel(widget));
}


GtkWidget
*init_prefgui (void)
{
    GtkWidget *window, *password_checkbutton, *password_entry, *close_button;
	GtkWidget *password_label;
	gboolean enable_password;
                                                                                
    xml = glade_xml_new (SYNCE_DATA "synce_trayicon_properties.glade", NULL, NULL);
                                                                                
    window = glade_xml_get_widget (xml, "window");

    password_checkbutton = glade_xml_get_widget (xml, "password_checkbutton");	
	enable_password = gconf_client_get_bool (synce_conf_client,
			                "/apps/synce/trayicon/enable_password", NULL);
							
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (password_checkbutton), enable_password);
    g_signal_connect (G_OBJECT (password_checkbutton), "toggled",
            G_CALLBACK (password_checkbutton_toggled_cb), NULL);
                                                            
                    
    password_label = glade_xml_get_widget (xml, "password_entry");    
    password_entry = glade_xml_get_widget (xml, "password_entry");    
    gtk_entry_set_text(GTK_ENTRY(password_entry),gconf_client_get_string (synce_conf_client,
                "/apps/synce/trayicon/password", NULL));                                                                             
    g_signal_connect (G_OBJECT (password_entry), "activate",
            G_CALLBACK (password_entry_activate_cb), NULL);

	gtk_widget_set_sensitive(password_label, enable_password);
	gtk_widget_set_sensitive(password_entry, enable_password);
	
    close_button = glade_xml_get_widget (xml, "closebutton1");    
    g_signal_connect (G_OBJECT (close_button), "clicked",
            G_CALLBACK (close_button_clicked_cb), NULL);
	
	gconf_client_notify_add (synce_conf_client, 
			"/apps/synce/trayicon/enable_password", 
			enable_passwd_updated, NULL, NULL, NULL);
	
    gtk_widget_show_all (window);
                                                                                
    return window;
}

