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

#define SYNCE_SOFTWARE_MANAGER "synce-software-manager"

GConfClient *synce_conf_client = NULL;

static bool in_background = true;
static bool is_connected = false;
static char* device_name = NULL;

static EggTrayIcon* tray_icon = NULL;
static GtkTooltips* tooltips = NULL;

static const struct poptOption options[] = {
	{NULL, 'd', POPT_ARG_INT, NULL, 0, N_("debug"), NULL},
	{NULL, 'f', POPT_ARG_NONE, NULL, 0, N_("debug"), NULL},
	{NULL, '\0', 0, NULL, 0} /* end the list */
};

static const char* get_battery_flag_string(unsigned flag)
{
	const char* name;
	
	switch (flag)
	{
		case BATTERY_FLAG_HIGH:        name = _("High");       break;
		case BATTERY_FLAG_LOW:         name = _("Low");        break;
		case BATTERY_FLAG_CRITICAL:    name = _("Critical");   break;
		case BATTERY_FLAG_CHARGING:    name = _("Charging");   break;
		case BATTERY_FLAG_NO_BATTERY:  name = _("NoBattery");  break;

		default: name = _("Unknown"); break;
	}

	return name;
}

static void set_status_tooltips()
{
	SYSTEM_POWER_STATUS_EX power;
	STORE_INFORMATION store;
	char* power_str = NULL;
	char* store_str = NULL;
	char* tooltip_str = NULL;
	
	memset(&power, 0, sizeof(SYSTEM_POWER_STATUS_EX));
	if (CeGetSystemPowerStatusEx(&power, false) &&
			BATTERY_PERCENTAGE_UNKNOWN != power.BatteryLifePercent)
	{
		power_str = g_strdup_printf(
				"%i%% (%s)", 
				power.BatteryLifePercent, 
				get_battery_flag_string(power.BatteryFlag));
	}
	else
	{
		power_str = g_strdup(_("Unknown"));
	}
	 
	memset(&store, 0, sizeof(store));
	if (CeGetStoreInformation(&store) && store.dwStoreSize != 0)
	{
		store_str = g_strdup_printf(
				_("%i%% (%i megabytes)"), 
				100 * store.dwFreeSize / store.dwStoreSize,
				store.dwFreeSize >> 20);
	}
	else
	{
		store_str = g_strdup(_("Unknown"));
	}

	tooltip_str = g_strdup_printf(
		_("Battery life:\t%s\nFree store:\t%s"),
		power_str,
		store_str);

	gtk_tooltips_set_tip(tooltips, GTK_WIDGET(tray_icon), tooltip_str, NULL);

	g_free(power_str);
	g_free(store_str);
	g_free(tooltip_str);
}

void start_dccm ()
{
	char *argv[3] = {
		"dccm","-p",""
	};
	char argc = 1;

	if (gconf_client_get_bool (synce_conf_client,
			                "/apps/synce/trayicon/enable_password", NULL)) {

#ifdef WITH_GNOME_KEYRING
		if (!(argv[2] = get_key_from_keyring(NULL)))
		{
			argc = 1;
		} else {
			argc = 3;
		}
#else
		argv[2] = gconf_client_get_string (synce_conf_client,
					"/apps/synce/trayicon/password", NULL);
		argc = 3;
#endif
	}

	if (!dccm_is_running()) {
		synce_trace("starting dccm");
		if (gnome_execute_async(NULL,argc, argv) == -1) {
			synce_error_dialog(_("Can't start dccm which is needed to comunicate \nwith the PDA. Make sure it is installed and try again."));
			synce_trace("Failed to start dccm");
		}
	} else {
		synce_trace("dccm is already running!");
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
	init_prefgui();
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

static void menu_disconnect(GtkWidget *button, EggTrayIcon *icon)
{
	send_signal_dccm(SIGHUP);
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

	if (menu) {
		gtk_widget_destroy(menu);
	}

	menu = gtk_menu_new();

	entry = gtk_menu_item_new_with_label(_("Explore with Filemanager"));
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_explore), NULL);
	gtk_widget_set_sensitive(entry, is_connected);
	gtk_menu_append(GTK_MENU(menu), entry);
	
	entry = gtk_menu_item_new_with_label(_("Add/Remove Programs"));
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_software), NULL);
	gtk_menu_append(GTK_MENU(menu), entry);
	if (g_find_program_in_path(SYNCE_SOFTWARE_MANAGER) != NULL && 
			is_connected) {
		gtk_widget_set_sensitive(entry, TRUE);
	} else {
		gtk_widget_set_sensitive(entry, FALSE);
	}
	
	entry = gtk_separator_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), entry);
		
	if (is_connected)
		snprintf(buffer, sizeof(buffer), _("Disconnect from '%s'"), device_name);
	else
		strcpy(buffer, _("(No device connected)"));
	
	entry = gtk_menu_item_new_with_label(buffer);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_disconnect), NULL);
	gtk_menu_append(GTK_MENU(menu), entry);
	
	entry = gtk_separator_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), entry);

	if (dccm_is_running()) {
	  entry = gtk_menu_item_new_with_label(_("Stop DCCM"));
	  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_stop_dccm), NULL);
	  gtk_menu_append(GTK_MENU(menu), entry);
	  gtk_widget_set_sensitive(entry, !is_connected);

	  entry = gtk_menu_item_new_with_label(_("Restart DCCM"));
	  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_restart_dccm), NULL);
	  gtk_menu_append(GTK_MENU(menu), entry);
	  gtk_widget_set_sensitive(entry, !is_connected);
	} else {
	  entry = gtk_menu_item_new_with_label(_("Start DCCM"));
	  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_start_dccm), NULL);
	  gtk_menu_append(GTK_MENU(menu), entry);
	}

	entry = gtk_separator_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), entry);

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
	if (is_connected)
	  gtk_image_set_from_icon_name(GTK_IMAGE(icon), "synce-color-small", GTK_ICON_SIZE_SMALL_TOOLBAR);
	else
	  gtk_image_set_from_icon_name(GTK_IMAGE(icon), "synce-gray-small", GTK_ICON_SIZE_SMALL_TOOLBAR);
	
	while (gtk_events_pending ())
		gtk_main_iteration ();
}

static void update()
{
	HRESULT hr;
	LONG result;
	WCHAR* tmp = NULL;
	HKEY key = 0;
	DWORD type = 0;
	DWORD size;
	WCHAR buffer[MAX_PATH];

	hr = CeRapiInit();
	if (FAILED(hr))
	{
		is_connected = false;
		set_icon();
		gtk_tooltips_set_tip(tooltips, GTK_WIDGET(tray_icon), _("Not connected"), NULL);
		return;
	}

	is_connected = true;
	set_icon();

	tmp = wstr_from_ascii("Ident");
	result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, tmp, 0, 0, &key);
	wstr_free_string(tmp);

	if (ERROR_SUCCESS != result)
	{
		synce_error("CeRegOpenKeyEx failed");
		goto exit;
	}

	tmp = wstr_from_ascii("Name");
	size = sizeof(buffer);

	result = CeRegQueryValueEx(key, tmp, 0, &type, (LPBYTE)buffer, &size);
	
	wstr_free_string(tmp);

	if (ERROR_SUCCESS != result)
	{
		synce_error("CeRegQueryValueEx failed");
		goto exit;
	}
 
	if (REG_SZ != type)
	{
		synce_error("Unexpected value type: 0x%08x = %i", type, type);
		goto exit;
	}
	 
	wstr_free_string(device_name);
	device_name = wstr_to_ascii(buffer);

	set_status_tooltips();

exit:
	CeRapiUninit();
}

static gboolean update_event(gpointer data)
{
	synce_trace("gotta update");
	update();
	return FALSE; /* prevent function from running again, right? */
}

static void sighup_handler()
{
	synce_trace("SIGHUP");
	g_idle_add(update_event, NULL);
}

static bool write_script()
{
	bool success = false;
	char* directory = NULL;
	char buffer[MAX_PATH];
	FILE* file = NULL;
	
	if (!synce_get_script_directory(&directory))
		goto exit;

	snprintf(buffer, sizeof(buffer), "%s/trayicon.sh", directory);
	
	file = fopen(buffer, "w");
	if (!file)
		goto exit;

	/* XXX: killall does not exist everywhere... */
	fprintf(file,
			"#!/bin/sh\n"
			"# The contents of this file will be lost next time you run synce-trayicon\n"
			"\n"
			"OS=`uname -s`\n"
			"if [ \"$OS\" = \"SunOS\" ]; then\n"
			"\tkiller=pkill\n"
			"else\n"
			"\tkiller=killall\n"
			"fi\n"
			"$killer -HUP synce-trayicon\n"
			);

	fclose(file); 
	file = NULL;

	chmod(buffer, 0700);

	success = true;

exit:
	if (file)
		fclose(file);
	
	if (directory)
		free(directory);

	return success;
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

int
main (gint argc, gchar **argv)
{
	int result = 1;
	GtkWidget *box;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	write_script();

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

	/* Gconf */
	synce_conf_client = gconf_client_get_default ();

	gconf_client_add_dir (synce_conf_client,
			"/apps/synce/trayicon",
			GCONF_CLIENT_PRELOAD_ONELEVEL,
			NULL);

	
	start_dccm();
	tray_icon = egg_tray_icon_new ("SynCE");
	box = gtk_event_box_new();
	icon = gtk_image_new();

	gtk_container_add(GTK_CONTAINER(box), icon);
	gtk_container_add(GTK_CONTAINER(tray_icon), box);
	gtk_widget_show_all(GTK_WIDGET(tray_icon));

	init_sm();
	g_signal_connect(G_OBJECT(box), "button-press-event", G_CALLBACK(trayicon_clicked), NULL);

	tooltips = gtk_tooltips_new();
	
	set_icon();
	update();

	signal(SIGHUP, sighup_handler);
  
	gtk_main ();

	result = 0;
 
exit:
  return result;
}
