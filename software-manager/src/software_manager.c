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

#include <stdarg.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <synce.h>
#include <synce_log.h>
#include <rapi.h>
#include <libunshield.h>

#include "software_manager.h"
#include "synce_app_man.h"

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
synce_error_dialog(const gchar *format, ...)
{
  va_list ap;
  GtkWidget *error_dialog;
  gchar *tmpstr = NULL;

  va_start(ap, format);
  tmpstr = g_strdup_vprintf(format, ap);
  va_end(ap);

  error_dialog = gtk_message_dialog_new (NULL,
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 tmpstr);

  gtk_dialog_run (GTK_DIALOG (error_dialog));
  gtk_widget_destroy (error_dialog);
  g_free(tmpstr);
}

static void
synce_info_dialog(const char *format, ...)
{
  va_list ap;
  GtkWidget *info_dialog;
  gchar *tmpstr = NULL;

  va_start(ap, format);
  tmpstr= g_strdup_vprintf(format, ap);
  va_end(ap);

  info_dialog = gtk_message_dialog_new (NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK,
					tmpstr);

  gtk_dialog_run (GTK_DIALOG (info_dialog));
  gtk_widget_destroy (info_dialog);
  g_free(tmpstr);
}

static void
progress_bar_pulse(gpointer data)
{
  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(data));
  while (gtk_events_pending()) {
    gtk_main_iteration_do(FALSE);
  }
}

GtkWidget *
busy_window_new(const gchar *title, const gchar *message, GtkWidget **progressbar)
{
  GtkWidget *window, *hbox, *image, *vbox, *label;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), title);
  gtk_container_set_border_width(GTK_CONTAINER(window), 14);

  hbox = gtk_hbox_new(FALSE, 10);
  gtk_container_add(GTK_CONTAINER(window), hbox);

  image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
  gtk_container_add(GTK_CONTAINER(hbox), image);

  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_add(GTK_CONTAINER(hbox), vbox);

  label = gtk_label_new(message);
  gtk_container_add(GTK_CONTAINER(vbox), label);

  *progressbar = gtk_progress_bar_new();
  gtk_container_add(GTK_CONTAINER(vbox), *progressbar);

  return window;
}

static void
programs_selection_changed (GtkTreeSelection *selection, gpointer user_data) 
{
  SynceSoftwareManager *self = SYNCE_SOFTWARE_MANAGER(user_data);
  SynceSoftwareManagerPrivate *priv = SYNCE_SOFTWARE_MANAGER_GET_PRIVATE (self);

  GtkTreeIter iter;
  GtkTreeModel *model;
  gint number;
  gchar *program;

  GtkWidget *remove_button = glade_xml_get_widget(priv->gladefile, "remove_button");

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter,
			NAME_COLUMN, &program,
			INDEX_COLUMN, &number,
			-1);
    g_debug("%s: selected %d: %s", G_STRFUNC, number, program);
    g_free (program);
    gtk_widget_set_sensitive(remove_button, TRUE);
  } else {
    gtk_widget_set_sensitive(remove_button, FALSE);
  }
}

static void
setup_programs_treeview_store(SynceSoftwareManager *self) 
{
  SynceSoftwareManagerPrivate *priv = SYNCE_SOFTWARE_MANAGER_GET_PRIVATE (self);
  GtkWidget *programs_treeview = glade_xml_get_widget(priv->gladefile, "programs_treeview");
  GtkWidget *remove_button = glade_xml_get_widget(priv->gladefile, "remove_button");

  GtkListStore *store = gtk_list_store_new (N_COLUMNS,
					    G_TYPE_INT,     /* index */
					    G_TYPE_STRING); /* program name */
  GtkTreeIter iter;
  GtkWidget *fetchwindow, *progressbar;
  GList *programlist = NULL;
  GList *tmplist = NULL;
  int i = 0;
  GError *error = NULL;
  gboolean result;

  fetchwindow = busy_window_new(_("Fetching information..."), _("Fetching the list of installed applications from the PDA."), &progressbar);

  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
  gtk_widget_show_all(fetchwindow);

  while (gtk_events_pending()) {
    gtk_main_iteration_do(FALSE);
  }

  result = synce_app_man_create_program_list(&programlist, progress_bar_pulse, progressbar, &error);
  if (!result) {
    synce_error_dialog(_("Couldn't fetch the list of applications from the device: %s"), error->message);
    g_error_free(error);
  }
  gtk_widget_destroy(fetchwindow);

  tmplist = programlist;

  /* i is not really used except to insert a sequence number in the list */
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

  gtk_tree_view_set_model (GTK_TREE_VIEW(programs_treeview), GTK_TREE_MODEL (store));
  g_object_unref (G_OBJECT (store));

  tmplist = programlist;
  while (tmplist != NULL) {
    g_free(tmplist->data);
    tmplist = g_list_next(tmplist);    
  }
  g_list_free(programlist);

  gtk_widget_set_sensitive(remove_button, FALSE);
}

static void
setup_programs_treeview(SynceSoftwareManager *self) 
{
  SynceSoftwareManagerPrivate *priv = SYNCE_SOFTWARE_MANAGER_GET_PRIVATE (self);
  GtkWidget *programs_treeview = glade_xml_get_widget(priv->gladefile,"programs_treeview");

  GtkListStore *store = gtk_list_store_new (N_COLUMNS,
					    G_TYPE_INT,     /* index */
					    G_TYPE_STRING); /* program name */
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;

  gtk_tree_view_set_model (GTK_TREE_VIEW(programs_treeview), GTK_TREE_MODEL (store));
  g_object_unref (G_OBJECT (store));

  column = gtk_tree_view_column_new ();
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
				       "text", INDEX_COLUMN, NULL);

  gtk_tree_view_column_set_visible(GTK_TREE_VIEW_COLUMN(column), FALSE);
  gtk_tree_view_append_column (GTK_TREE_VIEW(programs_treeview), column);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (GTK_TREE_VIEW_COLUMN(column),"Program");
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
				       "text", NAME_COLUMN, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW(programs_treeview), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (programs_treeview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (programs_selection_changed), self);

  gtk_widget_show (programs_treeview);
}


static void
on_add_button_clicked(GtkButton *button, gpointer user_data)
{
  SynceSoftwareManager *self = SYNCE_SOFTWARE_MANAGER(user_data);
  SynceSoftwareManagerPrivate *priv = SYNCE_SOFTWARE_MANAGER_GET_PRIVATE (self);

  GtkWidget *installdialog;
  GtkWidget *fetchwindow, *progressbar;
  gchar *filepath;
  gint response;
  GError *error = NULL;
  gchar *message = NULL;
  gboolean result;

  installdialog = gtk_file_chooser_dialog_new (_("Select installation file"),
					       NULL,
					       GTK_FILE_CHOOSER_ACTION_OPEN,
					       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					       GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					       NULL);
  response = gtk_dialog_run (GTK_DIALOG (installdialog));
  filepath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(installdialog));
  gtk_widget_destroy (installdialog);

  if (response == GTK_RESPONSE_ACCEPT) {
    message = g_strdup_printf(_("Installing from file \"%s\"..."), filepath);
    fetchwindow = busy_window_new(_("Installing Software..."), message, &progressbar);
    g_free(message);

    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
    gtk_widget_show_all(fetchwindow);

    while (gtk_events_pending()) {
      gtk_main_iteration_do(FALSE);
    }

    g_debug("%s: requested installation of file %s", G_STRFUNC, filepath);
    result = synce_app_man_install(filepath, progress_bar_pulse, progressbar, &error);

    gtk_widget_hide_all(fetchwindow);
    gtk_widget_destroy(fetchwindow);
    if (!result) {
      synce_error_dialog(_("Failed to install from file %s: %s"), filepath, error->message);
      g_error_free(error);
    } else {
      synce_info_dialog(_("Successfully started installation of file \"%s\". Check your device to see if any additional steps are required. The program list must be manually refreshed after installation has completed."), filepath);
    }
  }

  g_free(filepath);
}


void
on_remove_button_clicked(GtkButton *button, gpointer user_data)
{
  SynceSoftwareManager *self = SYNCE_SOFTWARE_MANAGER(user_data);
  SynceSoftwareManagerPrivate *priv = SYNCE_SOFTWARE_MANAGER_GET_PRIVATE (self);

  GtkWidget *programs_treeview = glade_xml_get_widget(priv->gladefile, "programs_treeview");
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  gint number;
  gchar *program = NULL;
  GError *error = NULL;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(programs_treeview));
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter,
			  NAME_COLUMN, &program,
			  INDEX_COLUMN, &number,
			  -1);

      if (synce_app_man_uninstall(program, &error)) {
	g_debug("%s: successfully removed program %s", G_STRFUNC, program);
	synce_info_dialog(_("The program \"%s\" was successfully removed."), program);
	setup_programs_treeview_store(self);
      } else {
	g_warning("%s: failed to remove program %s: %s", G_STRFUNC, program, error->message);
	synce_error_dialog(_("The program \"%s\" could not be removed: %s"), program, error->message);
	g_error_free(error);
      }
      g_free (program);
    }
}

void
on_refresh_button_clicked(GtkButton *button, gpointer user_data)
{
  SynceSoftwareManager *self = SYNCE_SOFTWARE_MANAGER(user_data);
  SynceSoftwareManagerPrivate *priv = SYNCE_SOFTWARE_MANAGER_GET_PRIVATE (self);

  GtkWidget *add_button = glade_xml_get_widget(priv->gladefile,"add_button");

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
  }

  setup_programs_treeview_store(self);
}

static gboolean
idle_populate_treeview(gpointer user_data)
{
  setup_programs_treeview_store(SYNCE_SOFTWARE_MANAGER(user_data));
  return FALSE;
}


/* class & instance functions */

static void
synce_software_manager_init(SynceSoftwareManager *self)
{
  SynceSoftwareManagerPrivate *priv = SYNCE_SOFTWARE_MANAGER_GET_PRIVATE (self);

  priv->disposed = FALSE;
  priv->connected = FALSE;
  priv->gladefile = glade_xml_new(SYNCE_SOFTWARE_MANAGER_GLADEFILE, "software_manager", NULL); 

  GtkWidget *software_manager = NULL, *programs_treeview,
    *add_button, *remove_button, *refresh_button;

  software_manager = glade_xml_get_widget(priv->gladefile, "software_manager");

  programs_treeview = glade_xml_get_widget(priv->gladefile, "programs_treeview");
  add_button = glade_xml_get_widget(priv->gladefile, "add_button");
  remove_button = glade_xml_get_widget(priv->gladefile, "remove_button");
  refresh_button = glade_xml_get_widget(priv->gladefile, "refresh_button");

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

  setup_programs_treeview(self);

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

  g_object_unref(priv->gladefile);
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


