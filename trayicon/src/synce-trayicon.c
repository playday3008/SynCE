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
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-about.h>
#include "eggtrayicon.h"

static bool is_connected = false;
static char* device_name = NULL;

static void menu_about (GtkWidget *button, EggTrayIcon *icon)
{
	GtkWidget *about;
	const gchar* authors[] = {
		"David Eriksson <twogood@users.sourceforge.net>",
		NULL
	};
	
	about = gnome_about_new (
			"SynCE Tray Icon",
			"0.1",
			"Copyright (c) 2002, David Eriksson",
			"Displays information about devices connected through SynCE",
			authors,
			NULL,
			NULL,
			NULL);

	gtk_widget_show(about);
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
		snprintf(buffer, sizeof(buffer), "Connected to '%s'", device_name);
	else
		strcpy(buffer, "(No device connected)");
	
	entry = gtk_menu_item_new_with_label(buffer);
/*	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(auto_login), NULL);*/
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
		filename = "/tmp/synce-color.png";
	else
		filename = "/tmp/synce-gray.png";

	unscaled = gdk_pixbuf_new_from_file(filename, NULL);
	gtk_image_set_from_pixbuf(GTK_IMAGE(icon), unscaled);
	g_object_unref(unscaled);
	
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

gint
main (gint argc, gchar **argv)
{
  GtkWidget *button;
  EggTrayIcon *tray_icon;
	GtkWidget *box;

	write_script();

  gtk_init (&argc, &argv);

  tray_icon = egg_tray_icon_new ("SynCE");
	box = gtk_event_box_new();
	icon = gtk_image_new();

	gtk_container_add(GTK_CONTAINER(box), icon);
	gtk_container_add(GTK_CONTAINER(tray_icon), box);
	gtk_widget_show_all(GTK_WIDGET(tray_icon));

	g_signal_connect(G_OBJECT(box), "button-press-event", G_CALLBACK(trayicon_clicked), NULL);

	set_icon("/tmp/synce-red.png");
	update();

	signal(SIGHUP, sighup_handler);
	
#if 0	
  button = gtk_button_new_with_label ("This is a cool\ntray icon");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (first_button_pressed), tray_icon);
  gtk_container_add (GTK_CONTAINER (tray_icon), button);
  gtk_widget_show_all (GTK_WIDGET (tray_icon));

  tray_icon = egg_tray_icon_new ("Our other cool tray icon");
  button = gtk_button_new_with_label ("This is a another\ncool tray icon");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (second_button_pressed), tray_icon);

  gtk_container_add (GTK_CONTAINER (tray_icon), button);
  gtk_widget_show_all (GTK_WIDGET (tray_icon));
#endif

  gtk_main ();
  
  return 0;
}
