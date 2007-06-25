#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnome.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>

#include "properties.h"

extern GConfClient *synce_conf_client;
GladeXML *xml;

static void
prefs_changed_cb (GConfClient *client, guint id,
		  GConfEntry *entry, gpointer data)
{
  const gchar *key;
  const GConfValue *value;

  key = gconf_entry_get_key(entry);
  value = gconf_entry_get_value(entry);

  if (!(g_ascii_strcasecmp(key, "/apps/synce/trayicon/dccm"))) {
    const gchar *which_dccm = gconf_value_get_string(value);
    GtkWidget *prefs_use_odccm = glade_xml_get_widget (xml, "prefs_use_odccm");	
    GtkWidget *prefs_use_vdccm = glade_xml_get_widget (xml, "prefs_use_vdccm");	
    GtkWidget *prefs_start_stop_vdccm = glade_xml_get_widget (xml, "prefs_start_stop_vdccm");	

    if (!(g_ascii_strcasecmp(which_dccm, "v"))) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_use_vdccm), TRUE);
      gtk_widget_set_sensitive(prefs_start_stop_vdccm, TRUE);
    } else if (!(g_ascii_strcasecmp(which_dccm, "o"))) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_use_odccm), TRUE);
      gtk_widget_set_sensitive(prefs_start_stop_vdccm, FALSE);
    } else {
      gconf_client_set_string (synce_conf_client,
			    "/apps/synce/trayicon/dccm", "o", NULL);
    }
  }
}


static void
prefs_use_odccm_toggled_cb (GtkWidget *widget, gpointer data)
{
    gboolean state;
    GtkWidget *prefs_start_stop_vdccm = GTK_WIDGET(data);

    state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    if (state) {
      gconf_client_set_string (synce_conf_client,
            "/apps/synce/trayicon/dccm", "o", NULL);

    } else {
      gconf_client_set_string (synce_conf_client,
            "/apps/synce/trayicon/dccm", "v", NULL);
    }
    gtk_widget_set_sensitive(prefs_start_stop_vdccm, !state);
}

static void
prefs_start_stop_vdccm_toggled_cb (GtkWidget *widget, gpointer data)
{
    gboolean state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

    gconf_client_set_bool (synce_conf_client,
            "/apps/synce/trayicon/start_vdccm", state, NULL);
}


static void
prefs_close_button_clicked_cb (GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(gtk_widget_get_toplevel(widget));
}


GtkWidget *
run_prefs_dialog (void)
{
  GtkWidget *prefs_window, *prefs_use_odccm, *prefs_use_vdccm, *prefs_start_stop_vdccm, *close_button;
  gchar *which_dccm;
  GError *error = NULL;

  xml = glade_xml_new (SYNCE_DATA "synce_trayicon_properties.glade", "prefs_window", NULL);

  prefs_window = glade_xml_get_widget (xml, "prefs_window");

  prefs_use_odccm = glade_xml_get_widget (xml, "prefs_use_odccm");	
  prefs_use_vdccm = glade_xml_get_widget (xml, "prefs_use_vdccm");	
  prefs_start_stop_vdccm = glade_xml_get_widget (xml, "prefs_start_stop_vdccm");	

  if (!(which_dccm = gconf_client_get_string (synce_conf_client,
					      "/apps/synce/trayicon/dccm", &error))) {
    which_dccm = g_strdup("o");
    if (error) {
      g_warning("Get dccm type from gconf failed: %s", error->message);
      g_error_free(error);
    }
  }

  if (!(g_ascii_strcasecmp(which_dccm, "o"))) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_use_odccm), TRUE);
    gtk_widget_set_sensitive(prefs_start_stop_vdccm, FALSE);
  } else {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_use_vdccm), TRUE);
    gtk_widget_set_sensitive(prefs_start_stop_vdccm, TRUE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_start_stop_vdccm), 
				  gconf_client_get_bool (synce_conf_client,
							 "/apps/synce/trayicon/start_vdccm", NULL));
  }

  g_signal_connect (G_OBJECT (prefs_use_odccm), "toggled",
		      G_CALLBACK (prefs_use_odccm_toggled_cb), prefs_start_stop_vdccm);
  g_signal_connect (G_OBJECT (prefs_start_stop_vdccm), "toggled",
		      G_CALLBACK (prefs_start_stop_vdccm_toggled_cb), NULL);

  close_button = glade_xml_get_widget (xml, "prefs_closebutton");    
  g_signal_connect (G_OBJECT (close_button), "clicked",
		    G_CALLBACK (prefs_close_button_clicked_cb), NULL);

  gconf_client_notify_add (synce_conf_client, 
			   "/apps/synce/trayicon", 
			   prefs_changed_cb, NULL, NULL, NULL);

  gtk_widget_show_all (prefs_window);

  return prefs_window;
}
