/*
 *  Copyright (C) 2003 Mattias Eriksson
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
#include <libgnome/libgnome.h>
#include <glade/glade.h>

#include "software_manager.h"

static void
on_file_quit_activate(GtkMenuItem *menu_item, gpointer user_data)
{
  gtk_widget_destroy(GTK_WIDGET(user_data));
}

static void
on_mainwindow_destroy(GtkWidget *widget, gpointer user_data){
  gtk_main_quit();
}

int
main (int argc, char **argv)
{
    GtkWidget *mainwindow;
    GnomeProgram *app;
    SynceSoftwareManager *software_manager;
    gint result = 0;

#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    app = gnome_program_init(PACKAGE, VERSION,
			     LIBGNOME_MODULE,
			     argc, argv,
			     GNOME_PARAM_HUMAN_READABLE_NAME,_("Synce Software Manager"),
			     GNOME_PROGRAM_STANDARD_PROPERTIES,
			     NULL);

    gtk_init (&argc, &argv);
    glade_init ();

    mainwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mainwindow), gnome_program_get_human_readable_name(app));

    g_signal_connect (G_OBJECT(mainwindow),"destroy",
		      G_CALLBACK(on_mainwindow_destroy),NULL);

    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(mainwindow), vbox);

    GtkWidget *menu_bar = gtk_menu_bar_new();
    GtkWidget *file_menu_item = gtk_menu_item_new_with_mnemonic("_File");
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *file_quit_menu_item = gtk_menu_item_new_with_mnemonic("_Quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), file_quit_menu_item);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu_item), file_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_menu_item);

    g_signal_connect(G_OBJECT(file_quit_menu_item), "activate", 
		     G_CALLBACK(on_file_quit_activate), mainwindow);

    gtk_container_add(GTK_CONTAINER(vbox), menu_bar);

    if (! (software_manager = g_object_new(SYNCE_SOFTWARE_MANAGER_TYPE, NULL)) ) {
      result = 1;
      goto exit;
    }

    gtk_container_add(GTK_CONTAINER(vbox), GTK_WIDGET(software_manager));   
    gtk_widget_show_all(mainwindow);
    gtk_main ();

exit:
    return(result);
}
