/*

	 Copyright (c) 2002 David Eriksson <twogood@users.sourceforge.net>

	 Permission is hereby granted, free of charge, to any person obtaining a copy
	 of this software and associated documentation files (the "Software"), to
	 deal in the Software without restriction, including without limitation the
	 rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	 sell copies of the Software, and to permit persons to whom the Software is
	 furnished to do so, subject to the following conditions:

	 The above copyright notice and this permission notice shall be included in
	 all copies or substantial portions of the Software.

	 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
	 IN THE SOFTWARE.


	 Some info:

	 This is quite a hack, so feel free to submit patches! :-)
	 
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rapi.h>
#include <synce_log.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gnome.h>
#include <glade/glade.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-about.h>
#include <gconf/gconf-client.h>

#include "eggtrayicon.h"
#include "gtop_stuff.h"
#include "properties.h"
#include "utils.h"
#ifdef WITH_GNOME_KEYRING
#include "keyring.h"
#endif
#include "dccm-client.h"
#include "vdccm-client.h"
#include "odccm-client.h"
#include "device-manager.h"

#define SYNCE_SOFTWARE_MANAGER "synce-software-manager"

enum {
  USE_ODCCM,
  USE_VDCCM
};

static guint which_dccm = USE_ODCCM;

GConfClient *synce_conf_client = NULL;

static bool in_background = true;
DccmClient *comms_client = NULL;
WmDeviceManager *device_list = NULL;


static EggTrayIcon* tray_icon = NULL;
static GtkTooltips* tooltips = NULL;

static const struct poptOption options[] = {
	{NULL, 'd', POPT_ARG_INT, NULL, 0, N_("debug"), NULL},
	{NULL, 'f', POPT_ARG_NONE, NULL, 0, N_("debug"), NULL},
	{NULL, '\0', 0, NULL, 0} /* end the list */
};


static bool is_connected() {
  if (wm_device_manager_device_count(device_list) > 0) {
    return TRUE;
  }
  return FALSE;
}


static void set_status_tooltips()
{
  gchar *power_str = NULL;
  gchar *store_str = NULL;
  gchar *pdaname = NULL;
  gchar *tooltip_str = NULL;
  gchar *tmpstr = NULL;
  guint device_count, i = 0;
  WmDevice *device;

  if (!is_connected()) {
    gtk_tooltips_set_tip(tooltips, GTK_WIDGET(tray_icon), _("Not connected"), NULL);
    return;
  }

  device_count = wm_device_manager_device_count(device_list);

  for (i = 0; i < device_count; i++) {
    device = wm_device_manager_find_by_index(device_list,i);

    power_str = wm_device_get_power_status(device);
    store_str = wm_device_get_store_status(device);
    pdaname = wm_device_get_name(device);
    if (tooltip_str)
      tmpstr = g_strdup_printf(_("%s\n%s Battery life: %s  Free store: %s"),
			       tooltip_str, pdaname, power_str, store_str);
    else
      tmpstr = g_strdup_printf(_("%s Battery life: %s  Free store: %s"),
			       pdaname, power_str, store_str);


    g_free(tooltip_str);
    g_free(power_str);
    g_free(store_str);
    g_free(pdaname);
    tooltip_str = tmpstr;
  }

  gtk_tooltips_set_tip(tooltips, GTK_WIDGET(tray_icon), tooltip_str, NULL);
  g_free(tooltip_str);
  return;
}

void start_dccm ()
{
  char *argv[3] = {
    DCCM_BIN,""
  };
  char argc = 1;

  if (!dccm_is_running()) {
    synce_trace("starting %s", DCCM_BIN);
    if (gnome_execute_async(NULL,argc, argv) == -1) {
      synce_error_dialog(_("Can't start vdccm which is needed to comunicate \nwith the PDA. Make sure it is installed and try again."));
      synce_trace("Failed to start %s", DCCM_BIN);
    }
  } else {
    synce_trace("%s is already running!", DCCM_BIN);
  }
}


void stop_dccm ()
{
  send_signal_dccm(SIGTERM);
}


static void menu_explore (GtkWidget *button, EggTrayIcon *icon)
{	
	char *argv[2] = {
		"nautilus","synce:"
	};
	if (gnome_execute_async(NULL,2, argv) == -1) {
		synce_error_dialog(_("Can't explore the PDA with the filemanager,\nmake sure you have nautilus and the synce gnome-vfs module installed"));
	}
}

static void menu_software (GtkWidget *button, EggTrayIcon *icon)
{	
	char *argv[1] = {
		SYNCE_SOFTWARE_MANAGER
	};
	if (gnome_execute_async(NULL,1, argv) == -1) {
		synce_error_dialog(_("Can't open the software manager\nmake sure you have synce-software-manager installed"));
	}
}

static void menu_preferences (GtkWidget *button, EggTrayIcon *icon)
{
  run_prefs_dialog();
}

static void menu_about (GtkWidget *button, EggTrayIcon *icon)
{
	GtkWidget *about;
	const gchar* authors[] = {
		"David Eriksson <twogood@users.sourceforge.net>",
		"Mattias Eriksson <snaggen@users.sourceforge.net>",
		NULL
	};
	
	about = gnome_about_new (
			_("SynCE Tray Icon"),
			VERSION,
			_("Copyright (c) 2002, David Eriksson"),
			_("Displays information about devices connected through SynCE"),
			authors,
			NULL,
			NULL,
			NULL);

	gtk_widget_show(about);
}

static void menu_disconnect(GtkWidget *button, gpointer data)
{
  WmDevice *device = WM_DEVICE(data);
  gchar *name = wm_device_get_name(device);

  synce_debug("***** Asked to disconnect %s by user ******", name);

  dccm_client_request_disconnect(comms_client, name);
  g_free(name);
}

static void menu_start_dccm(GtkWidget *button, EggTrayIcon *icon)
{
	start_dccm();
}

static void menu_stop_dccm(GtkWidget *button, EggTrayIcon *icon)
{
	stop_dccm();
}

static void menu_restart_dccm(GtkWidget *button, EggTrayIcon *icon)
{
	stop_dccm();
	sleep(1);
	start_dccm();
}

static void
unreg_sm ()
{
    GnomeClient *master;
    GnomeClientFlags flags;
                                                                                
    master = gnome_master_client ();
    flags = gnome_client_get_flags (master);
    if (flags & GNOME_CLIENT_IS_CONNECTED) {
        gnome_client_set_restart_style (master,
                GNOME_RESTART_NEVER);
        gnome_client_flush (master);
    }
}

static void menu_exit(GtkWidget *button, EggTrayIcon *icon)
{
	unreg_sm();
	gtk_main_quit();
}
	
static void trayicon_menu(GdkEventButton *event)
{
	static GtkWidget *menu = NULL;
	GtkWidget *entry;
	char buffer[MAX_PATH];
	gchar *device_name;
	gint i;
	WmDevice *device;

	if (menu) {
		gtk_widget_destroy(menu);
	}

	menu = gtk_menu_new();

	entry = gtk_menu_item_new_with_label(_("Explore with Filemanager"));
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_explore), NULL);
	gtk_widget_set_sensitive(entry, is_connected());
	gtk_menu_append(GTK_MENU(menu), entry);
	
	entry = gtk_menu_item_new_with_label(_("Add/Remove Programs"));
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_software), NULL);
	gtk_menu_append(GTK_MENU(menu), entry);
	if (g_find_program_in_path(SYNCE_SOFTWARE_MANAGER) != NULL && 
			is_connected()) {
		gtk_widget_set_sensitive(entry, TRUE);
	} else {
		gtk_widget_set_sensitive(entry, FALSE);
	}
	
	entry = gtk_separator_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), entry);
		
	if (is_connected()) {
	  for (i = 0; i < wm_device_manager_device_count(device_list); i++) {
	    device = wm_device_manager_find_by_index(device_list, i);
	    device_name = wm_device_get_device_name(device);
	    g_snprintf(buffer, sizeof(buffer), _("Disconnect from '%s'"), device_name);

	    entry = gtk_menu_item_new_with_label(buffer);
	    g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_disconnect), (gpointer) device);
	    gtk_menu_append(GTK_MENU(menu), entry);

	    g_free(device_name);
	  }
	} else {
	  strcpy(buffer, _("(No device connected)"));
	  entry = gtk_menu_item_new_with_label(buffer);
	  gtk_menu_append(GTK_MENU(menu), entry);
	}

	entry = gtk_separator_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), entry);

	if (which_dccm == USE_VDCCM) {
	  if (dccm_is_running()) {
	    entry = gtk_menu_item_new_with_label(_("Stop DCCM"));
	    g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_stop_dccm), NULL);
	    gtk_menu_append(GTK_MENU(menu), entry);
	    gtk_widget_set_sensitive(entry, !is_connected());

	    entry = gtk_menu_item_new_with_label(_("Restart DCCM"));
	    g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_restart_dccm), NULL);
	    gtk_menu_append(GTK_MENU(menu), entry);
	    gtk_widget_set_sensitive(entry, !is_connected());
	  } else {
	    entry = gtk_menu_item_new_with_label(_("Start DCCM"));
	    g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_start_dccm), NULL);
	    gtk_menu_append(GTK_MENU(menu), entry);
	  }
	  entry = gtk_separator_menu_item_new();
	  gtk_menu_append(GTK_MENU(menu), entry);
	}

	entry = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES,
			             NULL);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_preferences), NULL);
	gtk_menu_append(GTK_MENU(menu), entry);
	
	entry = gtk_image_menu_item_new_from_stock (GNOME_STOCK_ABOUT,
			             NULL);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_about), NULL);
	gtk_menu_append(GTK_MENU(menu), entry);

	entry = gtk_image_menu_item_new_from_stock (GNOME_STOCK_MENU_EXIT,
			             NULL);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_exit), NULL);
	gtk_menu_append(GTK_MENU(menu), entry);

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
}

static void trayicon_clicked(GtkWidget *button, GdkEventButton *event, void *data)
{
	if (event->type != GDK_BUTTON_PRESS)
		return;

	switch (event->button) {
		case 1:
			break;
		case 2:
			break;
		case 3:
			trayicon_menu(event);
			break;
	}
}
	
static GtkWidget *icon = NULL;

static void set_icon()
{
	if (is_connected())
	  gtk_image_set_from_icon_name(GTK_IMAGE(icon), "synce-color-small", GTK_ICON_SIZE_SMALL_TOOLBAR);
	else
	  gtk_image_set_from_icon_name(GTK_IMAGE(icon), "synce-gray-small", GTK_ICON_SIZE_SMALL_TOOLBAR);
	
	while (gtk_events_pending ())
		gtk_main_iteration ();
}

static gboolean update(gpointer data)
{
  set_icon();
  set_status_tooltips();

  /* prevent function from running again when
   set with g_idle_add */
  return FALSE;
}


/**
 * Write help message to stderr
 */
#if 0
static void show_usage(char *name)
{
	fprintf(
			stderr, 
			"Syntax:\n"
			"\n"
			"\t%s [-d LEVEL] [-f] [-h]\n"
			"\n"
			"\t-d LEVEL     Set debug log level\n"
			"\t                 0 - No logging (default)\n"
			"\t                 1 - Errors only\n"
			"\t                 2 - Errors and warnings\n"
			"\t                 3 - Everything\n"
			"\t-f           Run in foreground (default is to run in background)\n"
			"\t-h           Show this help message\n",
			name);
}
#endif 

static bool handle_parameters(int argc, char** argv)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;

	while ((c = getopt(argc, argv, "d:f")) != -1)
	{
		switch (c)
		{
			case 'd':
				log_level = atoi(optarg);
				break;
			
			/*
			 * The -f parameter specifies that we want to run in the foreground
			 */
			case 'f':
				in_background = false;
				break;
		}
	}

	synce_log_set_level(log_level);

	return true;
}


static void
init_sm ()
{
    GnomeClient *master;
    GnomeClientFlags flags;
                                                                                
    master = gnome_master_client ();
    flags = gnome_client_get_flags (master);
    if (flags & GNOME_CLIENT_IS_CONNECTED) {
        gnome_client_set_restart_style (master,
                GNOME_RESTART_IMMEDIATELY);
        gnome_client_flush (master);
    }
                                                                                
    g_signal_connect (GTK_OBJECT (master), "die",
            GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
}

void
device_added_cb(GObject *obj, gpointer user_data)
{
  g_idle_add(update, NULL);
}

void
device_removed_cb(GObject *obj, gpointer user_data)
{
  g_idle_add(update, NULL);
}

void
password_rejected_cb(DccmClient *comms_client, gchar *pdaname, gpointer user_data)
{
  GtkWidget *dialog;

  GnomeKeyringResult keyring_ret;

  keyring_ret = keyring_delete_key(pdaname);

  dialog = gtk_message_dialog_new (NULL,
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_WARNING,
				   GTK_BUTTONS_CLOSE,
				   "Password for device \"%s\" was rejected",
				   pdaname);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

void
password_required_cb(DccmClient *comms_client, gchar *pdaname, gpointer user_data)
{
  gchar *password;

  password = device_get_password(pdaname);

  dccm_client_provide_password(comms_client, pdaname, password);
  g_free(password);
}

void
device_connected_cb(DccmClient *comms_client, gchar *pdaname, gpointer info, gpointer user_data)
{
  WmDevice *device;
  SynceInfo * synce_info = (SynceInfo *)info;

  synce_debug("looking for preexisting device");
  if ((device = wm_device_manager_find_by_name(device_list, pdaname))) {
    synce_debug("Ignoring connection message for \"%s\", already connected", pdaname);
    synce_info_destroy(synce_info);
    return;
  }

  device = g_object_new(WM_DEVICE_TYPE, NULL);
  device = wm_device_from_synce_info(device, synce_info);
  wm_device_manager_add(device_list, device);

  synce_info_destroy(synce_info);
}

void
device_disconnected_cb(DccmClient *comms_client, gchar *pdaname, gpointer user_data)
{
  WmDevice *device;

  if (!(device = wm_device_manager_remove_by_name(device_list, pdaname))) {
    synce_debug("Ignoring disconnection message for \"%s\", not connected", pdaname);
    return;
  }

  g_object_unref(device);
}

bool
uninit_client_comms(DccmClient *comms_client)
{
  if (comms_client) {
    dccm_client_uninit_comms(comms_client);
    g_object_unref(comms_client);
  }

  return TRUE;
}

void
service_stopping_cb(DccmClient *comms_client, gpointer user_data)
{
  synce_debug("**** Entering service_stopping_cb() ******");
  synce_warning_dialog("VDCCM has signalled that it is stopping");

  uninit_client_comms(comms_client);
  g_object_unref(comms_client);
  comms_client = NULL;

  wm_device_manager_remove_all(device_list);

  synce_debug("**** Leaving service_stopping_cb() ******");
}

DccmClient *
init_client_comms()
{
  DccmClient *comms_client;

  if (which_dccm == USE_VDCCM)
    comms_client = DCCM_CLIENT(g_object_new(VDCCM_CLIENT_TYPE, NULL));
  else
    comms_client = DCCM_CLIENT(g_object_new(ODCCM_CLIENT_TYPE, NULL));

  if (!(comms_client)) {
    synce_error("Unable to create vdccm comms client");
    return NULL;
  }
  if (!(dccm_client_init_comms(comms_client))) {
    synce_error("Unable to initialise dccm comms client");
    g_object_unref(comms_client);
    return NULL;
  }

  g_signal_connect (G_OBJECT (comms_client), "password-rejected",
            GTK_SIGNAL_FUNC (password_rejected_cb), NULL);

  g_signal_connect (G_OBJECT (comms_client), "password-required",
            GTK_SIGNAL_FUNC (password_required_cb), NULL);

  g_signal_connect (G_OBJECT (comms_client), "service-stopping",
            GTK_SIGNAL_FUNC (service_stopping_cb), NULL);

  g_signal_connect (G_OBJECT (comms_client), "device-connected",
            GTK_SIGNAL_FUNC (device_connected_cb), NULL);

  g_signal_connect (G_OBJECT (comms_client), "device-disconnected",
            GTK_SIGNAL_FUNC (device_disconnected_cb), NULL);

  return comms_client;
}

int
main (gint argc, gchar **argv)
{
	int result = 1;
	GtkWidget *box;
	GError *error = NULL;
	gchar *synce_dir, *script_path = NULL;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gnome_program_init ("synce-trayicon", VERSION,
			LIBGNOMEUI_MODULE,
			argc, argv,
			GNOME_PARAM_POPT_TABLE,options,
			GNOME_PARAM_HUMAN_READABLE_NAME,_("Synce TrayIcon"),
			NULL);

	glade_gnome_init ();

	if (!handle_parameters(argc, argv))
		goto exit;

	if (in_background)
	{
	  synce_log_use_syslog();
		synce_trace("Forking into background");
		daemon(0,0);
	}
	else
	{
		synce_trace("Running in foreground");
	}

	/* remove obsolete script */
	if (!(synce_get_script_directory(&synce_dir)))
	  g_error("Cannot obtain synce script dir: %s", G_STRFUNC);
	script_path = g_strdup_printf("%s/trayicon.sh", synce_dir);
	g_unlink(script_path);
	g_free(script_path);

	/* Gconf */
	synce_conf_client = gconf_client_get_default ();

	gconf_client_add_dir (synce_conf_client,
			"/apps/synce/trayicon",
			GCONF_CLIENT_PRELOAD_ONELEVEL,
			NULL);

       	if (!(device_list = g_object_new(WM_DEVICE_MANAGER_TYPE, NULL)))
	  g_error("Couldn't initialize device list: %s", G_STRFUNC);

	g_signal_connect (G_OBJECT (device_list), "device-added",
			  (GCallback)device_added_cb, NULL);

	g_signal_connect (G_OBJECT (device_list), "device-removed",
			  (GCallback)device_removed_cb, NULL);

	gchar *dccm_tmp = gconf_client_get_string (synce_conf_client,
					"/apps/synce/trayicon/dccm", NULL);
	if (!dccm_tmp) {
	  if (error)
	    g_error("Error contacting gconf: %s: %s", error->message, G_STRFUNC);
	  which_dccm = USE_ODCCM;
	} else {
	  if (!(g_ascii_strcasecmp(dccm_tmp, "v")))
	    which_dccm = USE_VDCCM;
	  else
	    which_dccm = USE_ODCCM;
	  g_free(dccm_tmp);
	}

	if (which_dccm == USE_VDCCM) {
	  if (gconf_client_get_bool (synce_conf_client,
				     "/apps/synce/trayicon/start_vdccm", NULL)) {
	    start_dccm();
	    sleep(3);
	  }
	}

	if (!(comms_client = init_client_comms())) {
	  if (which_dccm == USE_VDCCM)
	    synce_warning_dialog("Unable to contact VDCCM, check it is installed and set to run");
	  else
	    synce_warning_dialog("Unable to contact ODCCM, check it is installed and running");
	}

	tray_icon = egg_tray_icon_new ("SynCE");
	box = gtk_event_box_new();
	icon = gtk_image_new();

	gtk_container_add(GTK_CONTAINER(box), icon);
	gtk_container_add(GTK_CONTAINER(tray_icon), box);
	gtk_widget_show_all(GTK_WIDGET(tray_icon));

	init_sm();
	g_signal_connect(G_OBJECT(box), "button-press-event", G_CALLBACK(trayicon_clicked), NULL);

	tooltips = gtk_tooltips_new();
	
	/* set initial state */
	update(NULL);

	gtk_main ();

	uninit_client_comms(comms_client);
	g_object_unref(device_list);

	result = 0;
exit:
	if (which_dccm == USE_VDCCM) {
	  if (gconf_client_get_bool (synce_conf_client,
					"/apps/synce/trayicon/start_vdccm", NULL))
	    stop_dccm();
	}

	return result;
}
