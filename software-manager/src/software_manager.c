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
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <synce.h>
#include <synce_log.h>
#include <liborange.h>
#include <libunshield.h>
#include <stdio.h>
#include <rapi.h>

#include "software_manager.h"
#include "rapi_stuff.h"

G_DEFINE_TYPE (SynceSoftwareManager, synce_software_manager, GTK_TYPE_HBOX)

typedef struct _SynceSoftwareManagerPrivate SynceSoftwareManagerPrivate;
struct _SynceSoftwareManagerPrivate {

  gboolean connected;
  GladeXML *gladefile;

  gboolean disposed;
};

#define SYNCE_SOFTWARE_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SYNCE_SOFTWARE_MANAGER_TYPE, SynceSoftwareManagerPrivate))

enum
{
  INDEX_COLUMN,
  NAME_COLUMN,
  N_COLUMNS
};


static void
synce_error_dialog(const char *message)
{
  GtkWidget *error_dialog;

  error_dialog = gtk_message_dialog_new (NULL,
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 message);

  gtk_dialog_run (GTK_DIALOG (error_dialog));
  gtk_widget_destroy (error_dialog);
}

static void
synce_info_dialog(const char *message)
{
  GtkWidget *info_dialog;

  info_dialog = gtk_message_dialog_new (NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK,
					message);

  gtk_dialog_run (GTK_DIALOG (info_dialog));
  gtk_widget_destroy (info_dialog);
}

static void
programs_selection_changed (GtkTreeSelection *selection, gpointer user_data) 
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gint number;
  gchar *program;

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter,
			NAME_COLUMN, &program,
			INDEX_COLUMN, &number,
			-1);

    g_printf("You selected %d: %s\n", number, program);
    g_free (program);
  }
}

static void
setup_programs_treeview_store(GtkWidget *treeview) 
{
  GtkListStore *store = gtk_list_store_new (N_COLUMNS,
					    G_TYPE_INT,     /* index */
					    G_TYPE_STRING); /* program name */
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
  programlist = create_program_list(programlist, progressbar); 

  tmplist = programlist;

  /* i is not realy used except to insert a sequence number in the list */
  i = 0;
  while (tmplist != NULL) {
    gtk_list_store_append (store, &iter);  /* Acquire an iterator */

    gtk_list_store_set (store, &iter,
			INDEX_COLUMN, i,
			NAME_COLUMN, tmplist->data,
			-1);

    tmplist = g_list_next(tmplist);    
    i++;
  }

  gtk_tree_view_set_model (GTK_TREE_VIEW(treeview), GTK_TREE_MODEL (store));
  g_object_unref (G_OBJECT (store));

  tmplist = programlist;
  while (tmplist != NULL) {
    g_free(tmplist->data);
    tmplist = g_list_next(tmplist);    
  }
  g_list_free(programlist);
  gtk_widget_destroy(fetchwindow);
}

static void
setup_programs_treeview(GtkWidget *treeview) 
{
  GtkListStore *store = gtk_list_store_new (N_COLUMNS,
					    G_TYPE_INT,     /* index */
					    G_TYPE_STRING); /* program name */
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;

  gtk_tree_view_set_model (GTK_TREE_VIEW(treeview), GTK_TREE_MODEL (store));
  g_object_unref (G_OBJECT (store));

  column = gtk_tree_view_column_new ();
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
				       "text", INDEX_COLUMN, NULL);

  gtk_tree_view_column_set_visible(GTK_TREE_VIEW_COLUMN(column), FALSE);
  gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (GTK_TREE_VIEW_COLUMN(column),"Program");
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
				       "text", NAME_COLUMN, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (programs_selection_changed),NULL);

  gtk_widget_show (treeview);
}

typedef struct _orange_cookie
{
  gchar *output_directory;
  GList *file_list;
} orange_cookie;

gboolean
cab_copy(const gchar *source_file, const gchar *dest_file)
{
  FILE* input = NULL;
  FILE* output = NULL;
  size_t bytes;
  char buffer[1024];
  gboolean success = FALSE;

  if (!(input = fopen(source_file, "r"))) {
    g_critical("%s: Failed to open input file for copying: %s", G_STRFUNC, source_file);
    return FALSE;
  }

  if (!(output = fopen(dest_file, "w"))) {
    g_critical("%s: Failed to open output file for copying: %s", G_STRFUNC, dest_file);
    fclose(input);
    g_unlink(dest_file);
    return FALSE;
  }

  while (TRUE)
    {
      bytes = fread(buffer, 1, sizeof(buffer), input);
      if (bytes < sizeof(buffer)) {
	if (feof(input)) {
	  fwrite(buffer, 1, bytes, output);
	  success = TRUE;
	  break;
	}

	g_critical("%s: error reading input file for copying: %s", G_STRFUNC, source_file);
	g_unlink(dest_file);
	break;
      }

      if (fwrite(buffer, 1, bytes, output) < bytes) {
	g_critical("%s: error writing output file for copying: %s", G_STRFUNC, dest_file);
	g_unlink(dest_file);
	break;
      }
    }

  fclose(output);
  fclose(input);
  return success;
}

bool
orange_callback(const char* filename, CabInfo* info, void *cookie )
{
  gboolean success = FALSE;

  orange_cookie *data_exchange = (orange_cookie *) cookie;

  gchar *output_filename = g_strdup_printf("%s/%s", data_exchange->output_directory, ( g_strrstr(filename, "/") + 1 ));
  g_debug("%s: squeezing out %s for processor type %d", G_STRFUNC, output_filename, info->processor);

  if (cab_copy(filename, output_filename)) {
    data_exchange->file_list = g_list_append(data_exchange->file_list, output_filename);
    success = TRUE;
  } else {
    g_debug("%s: Failed to copy from '%s' to '%s'", G_STRFUNC, filename, output_filename);
    g_free(output_filename);
  }

  return success;
}

static GList *
extract_with_orange(const gchar *arch_file, const gchar *dest_dir)
{
  GList *return_list = NULL;

  orange_cookie *cookie = g_malloc0(sizeof(orange_cookie));
  cookie->output_directory = g_strdup(dest_dir);

  orange_squeeze_file(arch_file, orange_callback, cookie);

  g_free(cookie->output_directory);
  return_list = cookie->file_list;
  g_free(cookie);

  return return_list;
}

void
install_file(const gchar *filepath)
{
  GladeXML *gladefile = glade_xml_new(SYNCE_SOFTWARE_MANAGER_GLADEFILE, 
				      "fetchwindow", NULL); 
  GtkWidget *fetchwindow = glade_xml_get_widget(gladefile,"fetchwindow");
  GtkWidget *progressbar = glade_xml_get_widget(gladefile,"fetch_progressbar");

  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
  gtk_widget_show_all(fetchwindow);

  GList *cab_list = extract_with_orange(filepath, "/tmp");
  if (!cab_list) {
    synce_error_dialog(_("Failed to install the program:\nNo Cabinet files found."));
    goto exit;
  }

  GList *tmplist = g_list_first(cab_list);
  while(tmplist)
    {
      g_debug("%s: found cab file %s", G_STRFUNC, (gchar *)(tmplist->data));
      tmplist = g_list_next(tmplist);
    }

  /* Do some install things */

  if (rapi_mkdir("/Windows/AppMgr") != 0) {
    g_warning("%s: unable to create directory /Windows/AppMgr on device", G_STRFUNC);
    synce_error_dialog(_("Failed to create the installation\ndirectory on the device."));
    goto exit;
  }
  if (rapi_mkdir("/Windows/AppMgr/Install") != 0) {
    g_warning("%s: unable to create directory /Windows/AppMgr/Install on device", G_STRFUNC);
    synce_error_dialog(_("Failed to create the installation\ndirectory on the device."));
    goto exit;
  }

  tmplist = g_list_first(cab_list);
  while(tmplist) {
    gchar *cab_file_base = g_strrstr(tmplist->data, "/") + 1;
    g_debug("%s: copying file %s to device", G_STRFUNC, cab_file_base);
    
    gchar *device_filename = g_strdup_printf(":/Windows/AppMgr/Install/%s", cab_file_base);

    if (rapi_copy((gchar *)(tmplist->data), device_filename, progressbar) != 0) {
      synce_error_dialog(_("Failed to install the program:\nCould not copy the file to the PDA."));
      goto exit;
    }

    tmplist = g_list_next(tmplist);
  }

  if (rapi_run("wceload.exe",NULL) != 0) {
    synce_error_dialog(_("Failed to install the program:\nCould not execute the installer on the PDA."));
    goto exit;
  }

  synce_info_dialog(_("The installation was successful!\nCheck your PDA to see if any additional steps are required.\nYou must manually refresh the program list\nfor the new program to be listed."));

exit:
  gtk_widget_destroy(fetchwindow);

  tmplist = g_list_first(cab_list);
  while(tmplist)
    {
      g_unlink((gchar *)(tmplist->data));
      tmplist = g_list_next(tmplist);
    }
}

void
on_add_button_clicked(GtkButton *button, gpointer user_data)
{
  SynceSoftwareManager *self = SYNCE_SOFTWARE_MANAGER(user_data);
  SynceSoftwareManagerPrivate *priv = SYNCE_SOFTWARE_MANAGER_GET_PRIVATE (self);

  GladeXML *gladefile = NULL;
  GtkWidget *installdialog;
  GtkWidget *file_entry;
  gchar *filepath;
  gint result;

  gladefile = glade_xml_new(SYNCE_SOFTWARE_MANAGER_GLADEFILE, 
			    "installdialog", NULL); 

  installdialog = glade_xml_get_widget(gladefile, "installdialog");
  file_entry = glade_xml_get_widget(gladefile, "file_entry");

  result = gtk_dialog_run (GTK_DIALOG (installdialog));
  filepath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_entry));
  gtk_widget_hide_all (installdialog);

  if (result == GTK_RESPONSE_APPLY) {
    g_debug("%s: selected apply", G_STRFUNC);
    g_debug("%s: selected file %s", G_STRFUNC, filepath);
    install_file(filepath);
  } else {
    g_debug("%s: didn't select apply", G_STRFUNC);
  }

  gtk_widget_destroy (installdialog);
  g_free(filepath);
}

void
on_remove_button_clicked(GtkButton *button, gpointer user_data)
{
  SynceSoftwareManager *self = SYNCE_SOFTWARE_MANAGER(user_data);
  SynceSoftwareManagerPrivate *priv = SYNCE_SOFTWARE_MANAGER_GET_PRIVATE (self);

  GtkWidget *programs_treeview = glade_xml_get_widget(priv->gladefile,"programs_treeview");
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  gint number;
  gchar *program;
  gchar *tmpstr;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(programs_treeview));
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter,
			  NAME_COLUMN, &program,
			  INDEX_COLUMN, &number,
			  -1);

      if (rapi_run("unload.exe", program) != 0) {
	tmpstr = g_strdup_printf(_("The program \"%s\"\ncould not be removed!"),program);
	synce_error_dialog(tmpstr);
	g_free(tmpstr);
      } else {
	g_debug(_("The program \"%s\"\nwas successfully removed!\nYou must manually refresh the program list\nto see any changes."), program);
      }
      g_free (program);
    }
  setup_programs_treeview_store(programs_treeview);
}

void
on_refresh_button_clicked(GtkButton *button, gpointer user_data)
{
  SynceSoftwareManager *self = SYNCE_SOFTWARE_MANAGER(user_data);
  SynceSoftwareManagerPrivate *priv = SYNCE_SOFTWARE_MANAGER_GET_PRIVATE (self);

  GtkWidget *programs_treeview = glade_xml_get_widget(priv->gladefile,"programs_treeview");
  GtkWidget *add_button = glade_xml_get_widget(priv->gladefile,"add_button");
  GtkWidget *remove_button = glade_xml_get_widget(priv->gladefile,"remove_button");

  if (!(priv->connected)) {
    HRESULT hr;
    hr = CeRapiInit();
    if (FAILED(hr)) {
      g_warning("%s: Unable to initialize RAPI: %s",
		G_STRFUNC, 
		synce_strerror(hr));
      priv->connected = FALSE;
      return;
    }
    priv->connected = TRUE;
    gtk_widget_set_sensitive(add_button, priv->connected);
    gtk_widget_set_sensitive(remove_button, priv->connected);
  }

  setup_programs_treeview_store(programs_treeview);
}

static gboolean
idle_populate_treeview(gpointer user_data)
{
  SynceSoftwareManager *self = SYNCE_SOFTWARE_MANAGER(user_data);
  SynceSoftwareManagerPrivate *priv = SYNCE_SOFTWARE_MANAGER_GET_PRIVATE (self);

  GtkWidget *programs_treeview = programs_treeview = glade_xml_get_widget(priv->gladefile,"programs_treeview");
  setup_programs_treeview_store(programs_treeview);

  return FALSE;
}


/* class & instance functions */

static void
synce_software_manager_init(SynceSoftwareManager *self)
{
  SynceSoftwareManagerPrivate *priv = SYNCE_SOFTWARE_MANAGER_GET_PRIVATE (self);

  priv->disposed = FALSE;
  priv->connected = FALSE;

  priv->gladefile = glade_xml_new(SYNCE_SOFTWARE_MANAGER_GLADEFILE, 
			    "software_manager", NULL); 

  GtkWidget *software_manager = NULL, *programs_treeview,
    *add_button, *remove_button, *refresh_button;

  software_manager = glade_xml_get_widget(priv->gladefile,"software_manager");

  programs_treeview = glade_xml_get_widget(priv->gladefile,"programs_treeview");
  add_button = glade_xml_get_widget(priv->gladefile,"add_button");
  remove_button = glade_xml_get_widget(priv->gladefile,"remove_button");
  refresh_button = glade_xml_get_widget(priv->gladefile,"refresh_button");

  g_signal_connect(G_OBJECT(add_button), "clicked",
		   G_CALLBACK(on_add_button_clicked),
		   self);

  g_signal_connect(G_OBJECT(remove_button), "clicked",
		   G_CALLBACK(on_remove_button_clicked),
		   self);

  g_signal_connect(G_OBJECT(refresh_button), "clicked",
		   G_CALLBACK(on_refresh_button_clicked),
		   self);

  gtk_container_add(GTK_CONTAINER(self), software_manager);

  synce_log_set_level(SYNCE_LOG_LEVEL_WARNING);
  unshield_set_log_level(UNSHIELD_LOG_LEVEL_WARNING);

  HRESULT hr;
  hr = CeRapiInit();
  if (FAILED(hr)) {
    g_warning("%s: Unable to initialize RAPI: %s",
	      G_STRFUNC, 
	      synce_strerror(hr));
    priv->connected = FALSE;
  } else {
    priv->connected = TRUE;
  }
  gtk_widget_set_sensitive(add_button, priv->connected);
  gtk_widget_set_sensitive(remove_button, priv->connected);

  setup_programs_treeview(programs_treeview);

  if (priv->connected)
    g_idle_add(idle_populate_treeview, self);

}

static void
synce_software_manager_dispose (GObject *obj)
{
  SynceSoftwareManager *self = SYNCE_SOFTWARE_MANAGER(obj);
  SynceSoftwareManagerPrivate *priv = SYNCE_SOFTWARE_MANAGER_GET_PRIVATE (self);

  if (priv->disposed) {
    return;
  }
  priv->disposed = TRUE;

  /* unref other objects */

  if (priv->connected) {
    CeRapiUninit();
    priv->connected = FALSE;
  }

  if (G_OBJECT_CLASS (synce_software_manager_parent_class)->dispose)
    G_OBJECT_CLASS (synce_software_manager_parent_class)->dispose (obj);
}

static void
synce_software_manager_finalize (GObject *obj)
{
  if (G_OBJECT_CLASS (synce_software_manager_parent_class)->finalize)
    G_OBJECT_CLASS (synce_software_manager_parent_class)->finalize (obj);
}

static void
synce_software_manager_class_init (SynceSoftwareManagerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (SynceSoftwareManagerPrivate));

  gobject_class->dispose = synce_software_manager_dispose;
  gobject_class->finalize = synce_software_manager_finalize;
}


