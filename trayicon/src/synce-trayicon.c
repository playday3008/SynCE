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
#include <rapi.h>
#include <synce_log.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-about.h>
#include "eggtrayicon.h"
#include "config.h"

static bool in_background = true;
static bool is_connected = false;
static char* device_name = NULL;

static EggTrayIcon* tray_icon = NULL;
static GtkTooltips* tooltips = NULL;

static const char* get_battery_flag_string(unsigned flag)
{
	const char* name;
	
	switch (flag)
	{
		case BATTERY_FLAG_HIGH:        name = "High";       break;
		case BATTERY_FLAG_LOW:         name = "Low";        break;
		case BATTERY_FLAG_CRITICAL:    name = "Critical";   break;
		case BATTERY_FLAG_CHARGING:    name = "Charging";   break;
		case BATTERY_FLAG_NO_BATTERY:  name = "NoBattery";  break;

		default: name = "Unknown"; break;
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
		power_str = g_strdup("Unknown");
	}
	 
	memset(&store, 0, sizeof(store));
	if (CeGetStoreInformation(&store) && store.dwStoreSize != 0)
	{
		store_str = g_strdup_printf(
				"%i%% (%i megabytes)", 
				100 * store.dwFreeSize / store.dwStoreSize,
				store.dwFreeSize >> 20);
	}
	else
	{
		store_str = g_strdup("Unknown");
	}

	tooltip_str = g_strdup_printf(
		"Battery life:\t%s\n"
		"Free store:\t%s",
		power_str,
		store_str);

	gtk_tooltips_set_tip(tooltips, GTK_WIDGET(tray_icon), tooltip_str, NULL);

	g_free(power_str);
	g_free(store_str);
	g_free(tooltip_str);
}

static void menu_about (GtkWidget *button, EggTrayIcon *icon)
{
	GtkWidget *about;
	const gchar* authors[] = {
		"David Eriksson <twogood@users.sourceforge.net>",
		NULL
	};
	
	about = gnome_about_new (
			"SynCE Tray Icon",
			VERSION,
			"Copyright (c) 2002, David Eriksson",
			"Displays information about devices connected through SynCE",
			authors,
			NULL,
			NULL,
			NULL);

	gtk_widget_show(about);
}

static void menu_disconnect(GtkWidget *button, EggTrayIcon *icon)
{
	system("killall -HUP dccm");
}

static void menu_exit(GtkWidget *button, EggTrayIcon *icon)
{
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

	if (is_connected)
		snprintf(buffer, sizeof(buffer), "Disconnect from '%s'", device_name);
	else
		strcpy(buffer, "(No device connected)");
	
	entry = gtk_menu_item_new_with_label(buffer);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_disconnect), NULL);
	gtk_menu_append(GTK_MENU(menu), entry);

	entry = gtk_menu_item_new_with_label("About");
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_about), NULL);
	gtk_menu_append(GTK_MENU(menu), entry);

	entry = gtk_menu_item_new_with_label("Exit");
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
	GdkPixbuf* unscaled;
	char* filename = NULL;

	if (is_connected)
		filename = g_build_filename(DATADIR, "pixmaps", "synce", "synce-color-small.png", NULL);
	else
		filename = g_build_filename(DATADIR, "pixmaps", "synce", "synce-gray-small.png", NULL);

	unscaled = gdk_pixbuf_new_from_file(filename, NULL);
	gtk_image_set_from_pixbuf(GTK_IMAGE(icon), unscaled);
	g_object_unref(unscaled);

	g_free(filename);
	
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
		gtk_tooltips_set_tip(tooltips, GTK_WIDGET(tray_icon), "Not connected", NULL);
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
	gtk_idle_add(update_event, NULL);
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

static bool handle_parameters(int argc, char** argv)
{
	int c;
	int log_level = SYNCE_LOG_LEVEL_LOWEST;

	while ((c = getopt(argc, argv, "d:fh")) != -1)
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

			case 'h':
			default:
				show_usage(argv[0]);
				return false;
		}
	}

	synce_log_set_level(log_level);

	return true;
}


int
main (gint argc, gchar **argv)
{
	int result = 1;
	GtkWidget *box;

	write_script();

  gtk_init (&argc, &argv);

	if (!handle_parameters(argc, argv))
		goto exit;

	if (in_background)
	{
		synce_trace("Forking into background");
		daemon(0,0);
	}
	else
	{
		synce_trace("Running in foreground");
	}

  tray_icon = egg_tray_icon_new ("SynCE");
	box = gtk_event_box_new();
	icon = gtk_image_new();

	gtk_container_add(GTK_CONTAINER(box), icon);
	gtk_container_add(GTK_CONTAINER(tray_icon), box);
	gtk_widget_show_all(GTK_WIDGET(tray_icon));

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
