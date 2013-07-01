#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gio/gio.h>
#include <gtk/gtk.h>
#include "properties.h"

static void
prefs_close_button_clicked_cb (GtkWidget *widget, gpointer data)
{
        gtk_widget_destroy(gtk_widget_get_toplevel(widget));
}

static void
prefs_window_destroy_cb(GtkWidget *widget, gpointer user_data)
{
}

GtkWidget *
run_prefs_dialog(GSettings *settings)
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
  GtkBuilder *builder = NULL;

  builder = gtk_builder_new();
  guint builder_res;
  gchar *namelist[] = { prefs_window_name, NULL };

  builder_res = gtk_builder_add_objects_from_file(builder,
                                                  SYNCE_DATA "synce_trayicon_properties.glade",
                                                  namelist,
                                                  &error);
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

  enable_vdccm = g_settings_get_boolean(settings, "enable-vdccm");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_enable_vdccm), enable_vdccm);
  g_settings_bind(settings, "enable-vdccm", prefs_enable_vdccm, "active", G_SETTINGS_BIND_DEFAULT);

  start_stop_vdccm = g_settings_get_boolean(settings, "start-vdccm");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_start_stop_vdccm), start_stop_vdccm);
  g_settings_bind(settings, "start-vdccm", prefs_start_stop_vdccm, "active", G_SETTINGS_BIND_DEFAULT|G_SETTINGS_BIND_NO_SENSITIVITY);

  gtk_widget_set_sensitive(prefs_start_stop_vdccm, enable_vdccm);
  g_settings_bind(settings, "enable-vdccm", prefs_start_stop_vdccm, "sensitive", G_SETTINGS_BIND_GET|G_SETTINGS_BIND_NO_SENSITIVITY);
#endif

  show_disconnected = g_settings_get_boolean(settings, "show-disconnected");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_show_disconnected), show_disconnected);
  g_settings_bind(settings, "show-disconnected", prefs_show_disconnected, "active", G_SETTINGS_BIND_DEFAULT);


  g_signal_connect (G_OBJECT (close_button), "clicked",
		    G_CALLBACK (prefs_close_button_clicked_cb), NULL);

  g_signal_connect (G_OBJECT (prefs_window), "destroy",
		    G_CALLBACK (prefs_window_destroy_cb), NULL);

  g_object_unref(builder);
  builder = NULL;

  gtk_widget_show_all (prefs_window);

  return prefs_window;
}
