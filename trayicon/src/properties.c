#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gconf/gconf-client.h>

#include "properties.h"

GtkBuilder *builder = NULL;

static void
prefs_changed_cb (GConfClient *client, guint id,
		  GConfEntry *entry, gpointer data)
{
  const gchar *key;
  const GConfValue *value;

  key = gconf_entry_get_key(entry);
  value = gconf_entry_get_value(entry);

#if ENABLE_VDCCM_SUPPORT
  if (!(g_ascii_strcasecmp(key, "/apps/synce/trayicon/enable_vdccm"))) {
    gboolean enable_vdccm = gconf_value_get_bool(value);
    GtkWidget *prefs_enable_vdccm = GTK_WIDGET(gtk_builder_get_object(builder, "prefs_enable_vdccm"));

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_enable_vdccm), enable_vdccm);

    return;
  }

  if (!(g_ascii_strcasecmp(key, "/apps/synce/trayicon/start_vdccm"))) {
    gboolean start_vdccm = gconf_value_get_bool(value);
    GtkWidget *prefs_start_stop_vdccm = GTK_WIDGET(gtk_builder_get_object(builder, "prefs_start_stop_vdccm"));

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_start_stop_vdccm), start_vdccm);

    return;
  }
#endif

  if (!(g_ascii_strcasecmp(key, "/apps/synce/trayicon/show_disconnected"))) {
    gboolean show_disconnected = gconf_value_get_bool(value);
    GtkWidget *prefs_show_disconnected = GTK_WIDGET(gtk_builder_get_object(builder, "prefs_show_disconnected"));

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_show_disconnected), show_disconnected);

    return;
  }
}

#if ENABLE_VDCCM_SUPPORT
static void
prefs_enable_vdccm_toggled_cb (GtkWidget *widget, gpointer data)
{
        gboolean state;
        GError *error = NULL;
        GtkWidget *prefs_start_stop_vdccm = GTK_WIDGET(data);
        GConfClient *conf_client = gconf_client_get_default();

        state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    
        if (!(gconf_client_set_bool (conf_client, "/apps/synce/trayicon/enable_vdccm", state, &error))) {
                g_warning("%s: setting '/apps/synce/trayicon/enable_vdccm' in gconf failed: %s", G_STRFUNC, error->message);
                g_error_free(error);
                error = NULL;
        }

        gtk_widget_set_sensitive(prefs_start_stop_vdccm, state);

        g_object_unref(conf_client);
}


static void
prefs_start_stop_vdccm_toggled_cb (GtkWidget *widget, gpointer data)
{
        GError *error = NULL;
        GConfClient *conf_client = gconf_client_get_default();

        gboolean state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

        if (!(gconf_client_set_bool (conf_client, "/apps/synce/trayicon/start_vdccm", state, &error))) {
                g_warning("%s: setting '/apps/synce/trayicon/start_vdccm' in gconf failed: %s", G_STRFUNC, error->message);
                g_error_free(error);
                error = NULL;
        }

        g_object_unref(conf_client);
}
#endif

static void
prefs_show_disconnected_toggled_cb (GtkWidget *widget, gpointer data)
{
        gboolean state;
        GError *error = NULL;
        GConfClient *conf_client = gconf_client_get_default();

        state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

        if (!(gconf_client_set_bool (conf_client, "/apps/synce/trayicon/show_disconnected", state, &error))) {
                g_warning("%s: setting '/apps/synce/trayicon/show_disconnected' in gconf failed: %s", G_STRFUNC, error->message);
                g_error_free(error);
                error = NULL;
        }

        g_object_unref(conf_client);
}                            


static void
prefs_close_button_clicked_cb (GtkWidget *widget, gpointer data)
{
        gtk_widget_destroy(gtk_widget_get_toplevel(widget));
}


static void
prefs_window_destroy_cb(GtkObject *object, gpointer user_data)
{
        g_object_unref(builder);

        GConfClient *conf_client = gconf_client_get_default();
        gconf_client_notify_remove(conf_client, GPOINTER_TO_UINT(user_data));
}


GtkWidget *
run_prefs_dialog (SynceTrayIcon *trayicon)
{
  GtkWidget *prefs_window, *prefs_show_disconnected, *close_button;
  gboolean show_disconnected;
#if ENABLE_VDCCM_SUPPORT
  GtkWidget *prefs_enable_vdccm, *prefs_start_stop_vdccm;
  gboolean enable_vdccm, start_stop_vdccm;
  gchar *prefs_window_name = "prefs_window_vdccm";
#else
  gchar *prefs_window_name = "prefs_window";
#endif
  GError *error = NULL;

  GConfClient *conf_client = gconf_client_get_default();

  builder = gtk_builder_new();
  guint builder_res;
#if GTK_CHECK_VERSION(2,14,0)
  gchar *namelist[] = { prefs_window_name, NULL };

  builder_res = gtk_builder_add_objects_from_file(builder,
                                                  SYNCE_DATA "synce_trayicon_properties.glade",
                                                  namelist,
                                                  &error);
#else
  builder_res = gtk_builder_add_from_file(builder,
                                                  SYNCE_DATA "synce_trayicon_properties.glade",
                                                  &error);
#endif
  if (builder_res == 0) {
          g_critical("%s: failed to load interface file: %s", G_STRFUNC, error->message);
          g_error_free(error);
          error = NULL;
  }

  prefs_window = GTK_WIDGET(gtk_builder_get_object(builder, prefs_window_name));
  prefs_show_disconnected = GTK_WIDGET(gtk_builder_get_object(builder, "prefs_show_disconnected"));
  close_button = GTK_WIDGET(gtk_builder_get_object(builder, "prefs_closebutton"));

#if ENABLE_VDCCM_SUPPORT
  prefs_enable_vdccm = GTK_WIDGET(gtk_builder_get_object(builder, "prefs_enable_vdccm"));
  prefs_start_stop_vdccm = GTK_WIDGET(gtk_builder_get_object(builder, "prefs_start_stop_vdccm"));

  enable_vdccm = gconf_client_get_bool (conf_client, "/apps/synce/trayicon/enable_vdccm", &error);
  if (error) {
          g_warning("%s: Getting '/apps/synce/trayicon/enable_vdccm' from gconf failed: %s", G_STRFUNC, error->message);
          g_error_free(error);
          error = NULL;
  }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_enable_vdccm), enable_vdccm);
  gtk_widget_set_sensitive(prefs_start_stop_vdccm, enable_vdccm);
  if (enable_vdccm) {
          start_stop_vdccm = gconf_client_get_bool (conf_client, "/apps/synce/trayicon/start_vdccm", &error);
          if (error) {
                  g_warning("%s: Getting '/apps/synce/trayicon/start_vdccm' from gconf failed: %s", G_STRFUNC, error->message);
                  g_error_free(error);
                  error = NULL;
          }
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_start_stop_vdccm), start_stop_vdccm);
  }

  g_signal_connect (G_OBJECT (prefs_enable_vdccm), "toggled",
		      G_CALLBACK (prefs_enable_vdccm_toggled_cb), prefs_start_stop_vdccm);
  g_signal_connect (G_OBJECT (prefs_start_stop_vdccm), "toggled",
		      G_CALLBACK (prefs_start_stop_vdccm_toggled_cb), NULL);
#endif

  show_disconnected = gconf_client_get_bool (conf_client, "/apps/synce/trayicon/show_disconnected", &error);
  if (error) {
          g_warning("%s: Getting '/apps/synce/trayicon/show_disconnected' from gconf failed: %s", G_STRFUNC, error->message);
          g_error_free(error);
          error = NULL;
  }
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_show_disconnected), show_disconnected);

  g_signal_connect (G_OBJECT (prefs_show_disconnected), "toggled",
		      G_CALLBACK (prefs_show_disconnected_toggled_cb), NULL);

  guint id = gconf_client_notify_add (conf_client, 
			   "/apps/synce/trayicon", 
			   prefs_changed_cb, trayicon, NULL, &error);
  if (error) {                                                                                                                                                             
          g_warning("%s: failed to add watch to gconf dir '/apps/synce/trayicon': %s", G_STRFUNC, error->message);
          g_error_free(error);
          error = NULL;
  }                  

  g_signal_connect (G_OBJECT (close_button), "clicked",
		    G_CALLBACK (prefs_close_button_clicked_cb), NULL);

  g_signal_connect (G_OBJECT (prefs_window), "destroy",
		    G_CALLBACK (prefs_window_destroy_cb), GUINT_TO_POINTER(id));

  gtk_widget_show_all (prefs_window);

  return prefs_window;
}
