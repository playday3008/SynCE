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

#include <gnome.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gtk/gtksignal.h>
#include <libgnomeui/libgnomeui.h>
#include <glade/glade-xml.h>

#include "rapi_stuff.h"

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

static void synce_info_dialog(const char *message)
{
    GtkWidget *info_dialog;

    info_dialog = gnome_message_box_new (
            message,
            GNOME_MESSAGE_BOX_INFO,
            GTK_STOCK_OK,
            NULL);

    gnome_dialog_run (GNOME_DIALOG (info_dialog));
}

static void
programs_selection_changed (GtkTreeSelection *selection, gpointer user_data) 
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    gint number;
    gchar *program;

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
        gtk_tree_model_get (model, &iter, 1, &program,0,&number, -1);
        
        g_printf("You selected %d: %s\n", number, program);
        g_free (program);
    }
}

void setup_programs_treeview_store(GtkWidget *treeview) 
{
    GtkListStore *store = gtk_list_store_new (2, G_TYPE_INT, G_TYPE_STRING);
    GtkTreeIter iter;
    GladeXML *gladefile = NULL;
    GtkWidget *fetchwindow;
    GtkWidget *progressbar;
    GList *programlist = NULL;
    GList *tmplist = NULL;
    int i = 0;
	
    gladefile = glade_xml_new(SYNCE_SOFTWARE_MANAGER_GLADEFILE, 
                                "fetchwindow", NULL); 
    fetchwindow = glade_xml_get_widget(gladefile,"fetchwindow");
    progressbar = glade_xml_get_widget(gladefile,"fetch_progressbar");
    
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
    gtk_widget_show_all(fetchwindow);
    
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    programlist = setup_rapi_and_create_list(programlist, progressbar); 

    tmplist = programlist;
    
    /* i is not realy used except to insert a sequence number in the list */
    i = 0;
    while (tmplist != NULL) {

        gtk_list_store_append (store, &iter);  /* Acquire an iterator */

        gtk_list_store_set (store, &iter,
                0, i,
                1, tmplist->data,
                -1);

        tmplist = g_list_next(tmplist);    
        i++;
    }
    
#if 0
    gtk_list_store_append (store, &iter);  /* Acquire an iterator */

    gtk_list_store_set (store, &iter,
            0, 1,
            1, "Program 1",
            -1);
    gtk_list_store_append (store, &iter);  /* Acquire an iterator */
    gtk_list_store_set (store, &iter,
            0, 2,
            1, "Program 2",
            -1);

#endif

    gtk_tree_view_set_model (GTK_TREE_VIEW(treeview), GTK_TREE_MODEL (store));
    g_object_unref (G_OBJECT (store));
    
    while (tmplist != NULL) {

        g_free(tmplist->data);

        tmplist = g_list_next(tmplist);    
    }
    gtk_widget_destroy(fetchwindow);
}

void setup_programs_treeview(GtkWidget *treeview) 
{
    GtkListStore *store = gtk_list_store_new (2, G_TYPE_INT, G_TYPE_STRING);
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;

    gtk_tree_view_set_model (GTK_TREE_VIEW(treeview), GTK_TREE_MODEL (store));
    g_object_unref (G_OBJECT (store));
    column = gtk_tree_view_column_new ();
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer,
            "text", 0, NULL);

    gtk_tree_view_column_set_visible(GTK_TREE_VIEW_COLUMN(column), FALSE);
    gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (GTK_TREE_VIEW_COLUMN(column),"Program");
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer,
            "text", 1, NULL);

    gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    g_signal_connect (G_OBJECT (selection), "changed",
            G_CALLBACK (programs_selection_changed),NULL);

    gtk_widget_show (treeview);
}

void on_install_add_button_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *file_entry = (GtkWidget *) user_data;
    gchar *filepath;

    filepath = g_strdup(gtk_entry_get_text(GTK_ENTRY(file_entry)));
    if (!g_str_has_suffix(filepath,".cab") && !g_str_has_suffix(filepath,".CAB")) {
        g_free(filepath);
        synce_error_dialog(_("The file is not a valid cab-file, judging by the suffix."));
        return;
    }
    gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(button)));

    /* Do some install things */
    rapi_mkdir("/Windows/AppMgr");
    rapi_mkdir("/Windows/AppMgr/Install");
    if (rapi_copy(filepath,":/Windows/AppMgr/Install/synce-install.cab") != 0) {
        g_free(filepath);
        synce_error_dialog(_("Failed to install the program:\nCould not copy the file to the PDA."));
        return;
    }
    if (rapi_run("wceload.exe",NULL) != 0) {
        g_free(filepath);
        synce_error_dialog(_("Failed to install the program:\nCould not execute the installer on the PDA."));
        return;
    }

    synce_info_dialog(_("The installation was successful!\nCheck your PDA to see if any additional steps are required.\nYou must manually refresh the program list\nfor the new program to be listed."));
    g_free(filepath);
}

void on_install_cancel_button_clicked(GtkButton *button, gpointer user_data)
{
    gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(button)));
}

void on_add_button_clicked(GtkButton *button, gpointer user_data)
{
    GladeXML *gladefile = NULL;
    GtkWidget *installwindow;
    GtkWidget *install_add_button;
    GtkWidget *install_cancel_button;
    GtkWidget *file_entry;

    gladefile = glade_xml_new(SYNCE_SOFTWARE_MANAGER_GLADEFILE, 
                                "installwindow", NULL); 
    installwindow = glade_xml_get_widget(gladefile,"installwindow");
    install_add_button = glade_xml_get_widget(gladefile,"install_add_button");
    install_cancel_button = glade_xml_get_widget(gladefile,"install_cancel_button");
    file_entry = glade_xml_get_widget(gladefile,"file_entry");

    gtk_signal_connect (GTK_OBJECT(install_add_button), "clicked", 
                        GTK_SIGNAL_FUNC(on_install_add_button_clicked),
                        file_entry);
    gtk_signal_connect (GTK_OBJECT(install_cancel_button), "clicked", 
                        GTK_SIGNAL_FUNC(on_install_cancel_button_clicked),
                        NULL);
    gtk_widget_show_all(installwindow);
}

void on_remove_button_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *programs_treeview = (GtkWidget *) user_data;
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    gint number;
    gchar *program;
    gchar *tmpstr;
    
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(programs_treeview));
    if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
        gtk_tree_model_get (model, &iter, 1, &program,0,&number, -1);
        
        if (rapi_run("unload.exe",program) != 0) {
            tmpstr = g_strdup_printf(_("The program \"%s\"\ncould not be removed!"),program);
            synce_error_dialog(tmpstr);
            g_free(tmpstr);
        } else {
            tmpstr = g_strdup_printf(_("The program \"%s\"\nwas successfully removed!\nYou must manually refresh the program list\nto see any changes."),program);
            synce_info_dialog(tmpstr);
            g_free(tmpstr);

        }
        g_free (program);
    }
}

void on_quit_button_clicked(GtkButton *button, gpointer user_data)
{
    gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(button)));
}

void on_refresh_button_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *programs_treeview = (GtkWidget *) user_data;
    
    setup_programs_treeview_store(programs_treeview);
}
    
void on_mainwindow_destroy(GtkWidget *widget, gpointer user_data){
    gtk_main_quit();
}

static gboolean idle_populate_treeview(gpointer user_data) {
	GtkWidget *programs_treeview = (GtkWidget *) user_data;
	setup_programs_treeview_store(programs_treeview);

	return FALSE;
}

int main (int argc, char **argv)
{
    GladeXML *gladefile = NULL;
    GtkWidget *mainwindow;
    GtkWidget *programs_treeview;
    GtkWidget *add_button;
    GtkWidget *remove_button;
    GtkWidget *quit_button;
    GtkWidget *refresh_button;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

    gnome_program_init ("synce-software-manager", VERSION,
            LIBGNOMEUI_MODULE,
            argc, argv,
			GNOME_PARAM_HUMAN_READABLE_NAME,_("Synce Software Manager"),
            NULL);

    gtk_init (&argc, &argv);
	glade_init ();

	gladefile = glade_xml_new(SYNCE_SOFTWARE_MANAGER_GLADEFILE, 
                                "mainwindow", NULL); 

    mainwindow = glade_xml_get_widget(gladefile,"mainwindow");
    programs_treeview = glade_xml_get_widget(gladefile,"programs_treeview");
    add_button = glade_xml_get_widget(gladefile,"add_button");
    remove_button = glade_xml_get_widget(gladefile,"remove_button");
    quit_button = glade_xml_get_widget(gladefile,"quit_button");
    refresh_button = glade_xml_get_widget(gladefile,"refresh_button");
    
    gtk_signal_connect (GTK_OBJECT(add_button), "clicked", 
                        GTK_SIGNAL_FUNC(on_add_button_clicked),NULL);
    
    gtk_signal_connect (GTK_OBJECT(remove_button), "clicked", 
                        GTK_SIGNAL_FUNC(on_remove_button_clicked),
                        programs_treeview);
    
    gtk_signal_connect (GTK_OBJECT(quit_button), "clicked", 
                        GTK_SIGNAL_FUNC(on_quit_button_clicked),NULL);
   
    gtk_signal_connect (GTK_OBJECT(refresh_button), "clicked", 
                        GTK_SIGNAL_FUNC(on_refresh_button_clicked),
                        programs_treeview);
   
    gtk_signal_connect (GTK_OBJECT(mainwindow),"destroy",
            GTK_SIGNAL_FUNC(on_mainwindow_destroy),NULL);
    
    setup_programs_treeview(programs_treeview);
   
    gtk_widget_show_all(mainwindow);
	gtk_idle_add(idle_populate_treeview,programs_treeview);
	
	
    gtk_main ();
    return(0);
}
