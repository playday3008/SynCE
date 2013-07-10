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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>
#include <synce_log.h>
#include <rapi2.h>

#include "registry-list.h"
#include "misc.h"

static gint log_level = 2;

static GOptionEntry options[] =
  {
    { "debug", 'd', 0, G_OPTION_ARG_INT, &log_level, N_("Set debug level 1-5 (default 2)"), "level" },
    { NULL }
  };


static void
on_quit_button_clicked(GtkButton *button, gpointer user_data)
{
  gtk_widget_destroy(GTK_WIDGET(user_data));
}

static void
on_mainwindow_destroy(GtkWidget *widget, gpointer user_data)
{
  gtk_main_quit();
}

static void
on_refresh_button_clicked(GtkButton *button, gpointer user_data)
{
  setup_registry_key_tree_store(user_data);
}
    
static gboolean
idle_populate_key_treeview(gpointer user_data)
{
  setup_registry_key_tree_store(user_data);

  return FALSE;
}

int
main (int argc, char **argv)
{
  GtkWidget *mainwindow,
    *tool_button_quit, *tool_button_refresh;
  struct reg_info *registry_info = NULL;

  HRESULT hr;
  IRAPIDesktop *desktop = NULL;
  IRAPIEnumDevices *enumdev = NULL;
  IRAPIDevice *device = NULL;
  IRAPISession *session = NULL;
  RAPI_DEVICEINFO devinfo;

  GtkBuilder *builder = NULL;
  guint builder_res;
  GError *error = NULL;
  gint result = -1;
  gchar *dev_name = NULL;

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  if (!gtk_init_with_args(&argc,
                          &argv,
                          " - gtk registry tool for synCE",
                          options,
                          NULL,
                          NULL))
          g_error("%s: failed to initialise GTK", G_STRFUNC);

  g_set_application_name(_("SynCE Registry Tool"));

  synce_log_set_level(log_level);

  registry_info = g_malloc0(sizeof(struct reg_info));

  gchar *namelist[] = { "mainwindow", NULL };
  builder = gtk_builder_new();

  builder_res = gtk_builder_add_objects_from_file(builder,
                                                  SYNCE_DATA "synce-registry-tool.glade",
                                                  namelist,
                                                  &error);
  if (builder_res == 0) {
          g_error("%s: failed to load interface file: %s", G_STRFUNC, error->message);
          g_error_free(error);
          error = NULL;
  }

  mainwindow = GTK_WIDGET(gtk_builder_get_object(builder, "mainwindow"));

  tool_button_quit = GTK_WIDGET(gtk_builder_get_object(builder, "tool_button_quit"));
  tool_button_refresh = GTK_WIDGET(gtk_builder_get_object(builder, "tool_button_refresh"));
  registry_info->registry_key_treeview = GTK_WIDGET(gtk_builder_get_object(builder, "registry_key_treeview"));
  registry_info->registry_value_listview = GTK_WIDGET(gtk_builder_get_object(builder, "registry_value_listview"));

  g_object_unref(builder);

  g_signal_connect(G_OBJECT(tool_button_quit), "clicked",
		   G_CALLBACK(on_quit_button_clicked),
		   mainwindow);

  g_signal_connect(G_OBJECT(tool_button_refresh), "clicked", 
		   G_CALLBACK(on_refresh_button_clicked),
		   registry_info);

  g_signal_connect(G_OBJECT(mainwindow),"destroy",
		   G_CALLBACK(on_mainwindow_destroy),
		   NULL);

  if (FAILED(hr = IRAPIDesktop_Get(&desktop))) {
    g_critical("%s: failed to initialise RAPI: %d: %s\n", 
               argv[0], hr, synce_strerror_from_hresult(hr));
    goto exit;
  }

  if (FAILED(hr = IRAPIDesktop_EnumDevices(desktop, &enumdev))) {
    g_critical("%s: failed to get connected devices: %d: %s\n", 
               argv[0], hr, synce_strerror_from_hresult(hr));
    goto exit;
  }

  while (SUCCEEDED(hr = IRAPIEnumDevices_Next(enumdev, &device))) {
    if (dev_name == NULL)
      break;

    if (FAILED(IRAPIDevice_GetDeviceInfo(device, &devinfo))) {
      g_critical("%s: failure to get device info\n", argv[0]);
      goto exit;
    }
    if (strcmp(dev_name, devinfo.bstrName) == 0)
      break;
  }

  if (FAILED(hr)) {
    g_critical("%s: Could not find device '%s': %08x: %s\n", 
               argv[0],
               dev_name?dev_name:"(Default)", hr, synce_strerror_from_hresult(hr));
    device = NULL;
    goto exit;
  }

  IRAPIDevice_AddRef(device);
  IRAPIEnumDevices_Release(enumdev);
  enumdev = NULL;

  if (FAILED(hr = IRAPIDevice_CreateSession(device, &session))) {
    g_critical("%s: Could not create a session to device: %08x: %s\n", 
               argv[0], hr, synce_strerror_from_hresult(hr));
    goto exit;
  }

  if (FAILED(hr = IRAPISession_CeRapiInit(session))) {
    g_critical("%s: Unable to initialize connection to device: %08x: %s\n", 
               argv[0], hr, synce_strerror_from_hresult(hr));
    goto exit;
  }

  registry_info->session = session;

  setup_registry_value_list_view(registry_info);
  setup_registry_key_tree_view(registry_info);

  gtk_widget_show_all(mainwindow);
  g_idle_add(idle_populate_key_treeview, registry_info);

  gtk_main ();

  result = 0;

exit:
  if (session) {
    IRAPISession_CeRapiUninit(session);
    IRAPISession_Release(session);
  }
  if (device) IRAPIDevice_Release(device);
  if (enumdev) IRAPIEnumDevices_Release(enumdev);
  if (desktop) IRAPIDesktop_Release(desktop);
  return result;
}
