/*
Copyright (c) 2007 Mark Ellis <mark@mpellis.org.uk>

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
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gnome.h>
#include <gconf/gconf-client.h>
#include <dbus/dbus-glib.h>
#include <synce.h>
#ifdef ENABLE_NOTIFY
#include <libnotify/notify.h>
#endif /* ENABLE_NOTIFY */

#include "synce-trayicon.h"
#include "gtop_stuff.h"
#include "properties.h"
#include "utils.h"
#include "keyring.h"
#include "dccm-client.h"
#include "vdccm-client.h"
#include "odccm-client.h"
#include "device-manager.h"
#include "stock-icons.h"

G_DEFINE_TYPE (SynceTrayIcon, synce_trayicon, EGG_TYPE_TRAY_ICON)

typedef struct _SynceTrayIconPrivate SynceTrayIconPrivate;
struct _SynceTrayIconPrivate {

  guint which_dccm;
  GConfClient *conf_client;
  DccmClient *comms_client;
  WmDeviceManager *device_list;
  GtkTooltips* tooltips;
  GtkWidget *icon;
  GtkWidget *menu;

#ifdef ENABLE_NOTIFY
  NotifyNotification *notification;
#endif /* ENABLE_NOTIFY */

  gboolean disposed;
};

#define SYNCE_TRAYICON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SYNCE_TRAYICON_TYPE, SynceTrayIconPrivate))

enum {
  USE_ODCCM,
  USE_VDCCM
};

#define SYNCE_SOFTWARE_MANAGER "synce-software-manager"

     /* method declarations */

static gboolean start_dccm ();
static void stop_dccm ();
static DccmClient *init_client_comms(SynceTrayIcon *self);
static void uninit_client_comms(SynceTrayIcon *self);
static void password_rejected_cb(DccmClient *comms_client, gchar *pdaname, gpointer user_data);
static void password_required_cb(DccmClient *comms_client, gchar *pdaname, gpointer user_data);
static void device_connected_cb(DccmClient *comms_client, gchar *pdaname, gpointer info, gpointer user_data);
static void device_disconnected_cb(DccmClient *comms_client, gchar *pdaname, gpointer user_data);
static void service_stopping_cb(DccmClient *comms_client, gpointer user_data);
static gboolean is_connected(SynceTrayIcon *self);
static void set_status_tooltips(SynceTrayIcon *self);
static void set_icon(SynceTrayIcon *self);
static gboolean update(gpointer data);
static void device_added_cb(GObject *obj, gpointer user_data);
static void device_removed_cb(GObject *obj, gpointer user_data);
static void trayicon_clicked(GtkWidget *button, GdkEventButton *event, gpointer user_data);

#ifdef ENABLE_NOTIFY
static void event_notification(SynceTrayIcon *self, const gchar *summary, const gchar *body);
#endif /* ENABLE_NOTIFY */


     /* methods */


static gboolean
is_connected(SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  if (wm_device_manager_device_count(priv->device_list) > 0) {
    return TRUE;
  }
  return FALSE;
}

static void
set_status_tooltips(SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  gchar *power_str = NULL;
  gchar *store_str = NULL;
  gchar *pdaname = NULL;
  gchar *tooltip_str = NULL;
  gchar *tmpstr = NULL;
  guint device_count, i = 0;
  WmDevice *device;

  if (!(is_connected(self))) {
    gtk_tooltips_set_tip(priv->tooltips, GTK_WIDGET(self), _("Not connected"), NULL);
    return;
  }

  device_count = wm_device_manager_device_count(priv->device_list);

  for (i = 0; i < device_count; i++) {
    device = wm_device_manager_find_by_index(priv->device_list,i);

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

  gtk_tooltips_set_tip(priv->tooltips, GTK_WIDGET(self), tooltip_str, NULL);
  g_free(tooltip_str);
  return;
}

static void
set_icon(SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  if (is_connected(self))
    gtk_image_set_from_icon_name(GTK_IMAGE(priv->icon), SYNCE_STOCK_CONNECTED, GTK_ICON_SIZE_SMALL_TOOLBAR);
  else
    gtk_image_set_from_icon_name(GTK_IMAGE(priv->icon), SYNCE_STOCK_DISCONNECTED, GTK_ICON_SIZE_SMALL_TOOLBAR);
	
  while (gtk_events_pending ())
    gtk_main_iteration ();
}

static gboolean 
update(gpointer data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(data);

  set_icon(self);
  set_status_tooltips(self);

  /* prevent function from running again when
   set with g_idle_add */
  return FALSE;
}


static void
password_rejected_cb(DccmClient *comms_client, gchar *pdaname, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  GtkWidget *dialog;

  GnomeKeyringResult keyring_ret;

  g_debug("%s: removing password from keyring", G_STRFUNC);

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

static void
password_required_cb(DccmClient *comms_client, gchar *pdaname, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  gchar *password;

  password = device_get_password(pdaname);

  dccm_client_provide_password(comms_client, pdaname, password);
  g_free(password);
}

static void
device_connected_cb(DccmClient *comms_client, gchar *pdaname, gpointer info, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
  WmDevice *new_device = WM_DEVICE(info);
  WmDevice *device;
#ifdef ENABLE_NOTIFY
  gchar *model, *platform, *name, *notify_string;
#endif /* ENABLE_NOTIFY */

  g_debug("%s: looking for preexisting device %s", G_STRFUNC, pdaname);
  if ((device = wm_device_manager_find_by_name(priv->device_list, pdaname))) {
    g_debug("%s: Ignoring connection message for \"%s\", already connected", G_STRFUNC, pdaname);
    g_object_unref(device);
    return;
  }

  wm_device_manager_add(priv->device_list, new_device);

#ifdef ENABLE_NOTIFY
  g_object_get(new_device, "name", &name, NULL);
  g_object_get(new_device, "hardware", &model, NULL);
  g_object_get(new_device, "class", &platform, NULL);

  notify_string = g_strdup_printf("A %s %s '%s' just connected.", model, platform, name);
  event_notification(self, "PDA connected", notify_string);

  g_free(name);
  g_free(model);
  g_free(platform);
  g_free(notify_string);
#endif /* ENABLE_NOTIFY */
}

static void
device_disconnected_cb(DccmClient *comms_client, gchar *pdaname, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
  WmDevice *device;
#ifdef ENABLE_NOTIFY
  gchar *name, *notify_string;
#endif /* ENABLE_NOTIFY */

  if (!(device = wm_device_manager_remove_by_name(priv->device_list, pdaname))) {
    g_debug("%s: Ignoring disconnection message for \"%s\", not connected", G_STRFUNC, pdaname);
    return;
  }

#ifdef ENABLE_NOTIFY
  g_object_get(device, "name", &name, NULL);

  notify_string = g_strdup_printf("'%s' just disconnected.", name);
  event_notification(self, "PDA disconnected", notify_string);

  g_free(name);
  g_free(notify_string);
#endif /* ENABLE_NOTIFY */

  g_object_unref(device);
}

static void
service_stopping_cb(DccmClient *comms_client, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);

  synce_warning_dialog("VDCCM has signalled that it is stopping");

  uninit_client_comms(self);
}


static void
stop_dccm ()
{
  send_signal_dccm(SIGTERM);
}

static void
uninit_client_comms(SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  if (priv->which_dccm == USE_VDCCM) {
    if (gconf_client_get_bool (priv->conf_client,
			       "/apps/synce/trayicon/start_vdccm", NULL))
      stop_dccm();
  }

  if (IS_DCCM_CLIENT(priv->comms_client)) {
    dccm_client_uninit_comms(priv->comms_client);
    g_object_unref(priv->comms_client);
    priv->comms_client = NULL;
  }

  wm_device_manager_remove_all(priv->device_list);

  return;
}

static gboolean
start_dccm ()
{
  gchar *argv[3] = {
    DCCM_BIN,""
  };
  gint argc = 1;

  if (!(dccm_is_running())) {
    g_debug("%s: starting %s", G_STRFUNC, DCCM_BIN);
    if (gnome_execute_async(NULL, argc, argv) == -1) {
      synce_error_dialog(_("Can't start vdccm which is needed to communicate \nwith the PDA. Make sure it is installed and try again."));
      g_warning("%s: Failed to start %s", G_STRFUNC, DCCM_BIN);
      return FALSE;
    }
  } else {
    g_warning("%s: %s is already running!", G_STRFUNC, DCCM_BIN);
  }

  sleep(2);
  return TRUE;
}

static DccmClient *
init_client_comms(SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
  DccmClient *comms_client;

  if (priv->which_dccm == USE_VDCCM) {
    if (gconf_client_get_bool (priv->conf_client, "/apps/synce/trayicon/start_vdccm", NULL))
      if (!(start_dccm()))
	return NULL;
    comms_client = DCCM_CLIENT(g_object_new(VDCCM_CLIENT_TYPE, NULL));
  }
  else
    {
      dbus_g_thread_init();
      comms_client = DCCM_CLIENT(g_object_new(ODCCM_CLIENT_TYPE, NULL));
    }

  if (!(comms_client)) {
    g_critical("%s: Unable to create vdccm comms client", G_STRFUNC);
    goto error;
  }
  if (!(dccm_client_init_comms(comms_client))) {
    g_critical("%s: Unable to initialise dccm comms client", G_STRFUNC);
    g_object_unref(comms_client);
    goto error;
  }

  g_signal_connect (G_OBJECT (comms_client), "password-rejected",
            GTK_SIGNAL_FUNC (password_rejected_cb), self);

  g_signal_connect (G_OBJECT (comms_client), "password-required",
            GTK_SIGNAL_FUNC (password_required_cb), self);

  g_signal_connect (G_OBJECT (comms_client), "service-stopping",
            GTK_SIGNAL_FUNC (service_stopping_cb), self);

  g_signal_connect (G_OBJECT (comms_client), "device-connected",
            GTK_SIGNAL_FUNC (device_connected_cb), self);

  g_signal_connect (G_OBJECT (comms_client), "device-disconnected",
            GTK_SIGNAL_FUNC (device_disconnected_cb), self);

  return comms_client;

error:
  if (priv->which_dccm == USE_VDCCM)
    synce_warning_dialog("Unable to contact VDCCM, check it is installed and set to run");
  else
    synce_warning_dialog("Unable to contact ODCCM, check it is installed and running");
  return NULL;
}


static void
device_added_cb(GObject *obj, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  g_idle_add(update, self);
}

static void
device_removed_cb(GObject *obj, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  g_idle_add(update, self);
}


/* menu callbacks */

static void
menu_explore (GtkWidget *button, SynceTrayIcon *self)
{
  char *argv[2] = {
    "nautilus","synce:"
  };
  if (gnome_execute_async(NULL,2, argv) == -1) {
    synce_error_dialog(_("Can't explore the PDA with the filemanager,\nmake sure you have nautilus and the synce gnome-vfs module installed"));
  }
}

static void
menu_software (GtkWidget *button, SynceTrayIcon *self)
{
  char *argv[1] = {
    SYNCE_SOFTWARE_MANAGER
  };
  if (gnome_execute_async(NULL,1, argv) == -1) {
    synce_error_dialog(_("Can't open the software manager\nmake sure you have synce-software-manager installed"));
  }
}

static void
menu_preferences (GtkWidget *button, SynceTrayIcon *self)
{
  run_prefs_dialog(self);
}

static void
menu_about (GtkWidget *button, SynceTrayIcon *icon)
{
  GtkWidget *about;
  const gchar* authors[] = {
    "David Eriksson <twogood@users.sourceforge.net>",
    "Mattias Eriksson <snaggen@users.sourceforge.net>",
    "Mark Ellis <mark_ellis@users.sourceforge.net>",
    NULL
  };

  GnomeProgram *prog_info = gnome_program_get ();
  about = gtk_about_dialog_new();

  gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about), gnome_program_get_human_readable_name (prog_info));
  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), gnome_program_get_app_version (prog_info));
  gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), _("Copyright (c) 2002, David Eriksson"));
  gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about), _("Displays information about devices connected through SynCE"));
  gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about), "http://www.synce.org");
  gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), authors);
  gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about), SYNCE_STOCK_CONNECTED);

  gtk_dialog_run (GTK_DIALOG (about));
  gtk_widget_destroy (GTK_WIDGET(about));
}

static void
menu_disconnect(GtkWidget *button, SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
  const gchar *label = NULL;

  GList *child_list = gtk_container_get_children(GTK_CONTAINER(button));

  GList *child = g_list_first(child_list);
  while (child) {
    if (GTK_IS_LABEL(child->data)) {
      label = gtk_label_get_text(GTK_LABEL(child->data));    
      break;
    }
    child = g_list_next(child);
  }
  if (!(label)) {
    g_critical("%s: failed to find device name", G_STRFUNC);
    g_list_free(child_list);
    return;
  }

  gchar len = strlen(label);
  gchar *name_start = g_strstr_len(label, len, "'") + 1;
  gchar *name = g_strndup(name_start, strlen(name_start) - 1);

  g_debug("%s: Asked to disconnect %s by user", G_STRFUNC, name);

  dccm_client_request_disconnect(priv->comms_client, name);
  g_free(name);
  g_list_free(child_list);
}

static void
menu_start_vdccm(GtkWidget *button, SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
  priv->comms_client = init_client_comms(self);
}

static void
menu_stop_vdccm(GtkWidget *button, SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
  uninit_client_comms(self);
}

static void
menu_restart_vdccm(GtkWidget *button, SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
  uninit_client_comms(self);
  sleep(1);
  priv->comms_client = init_client_comms(self);
}

static void
menu_exit(GtkWidget *button, SynceTrayIcon *self)
{
  gtk_main_quit();
}

static void
trayicon_menu(GdkEventButton *event, SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  GtkWidget *entry;
  char buffer[MAX_PATH];
  gchar *device_name;
  gint i;
  WmDevice *device;

  if (priv->menu)
    gtk_widget_destroy(priv->menu);

  priv->menu = gtk_menu_new();

  entry = gtk_menu_item_new_with_label(_("Explore with Filemanager"));
  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_explore), self);
  gtk_widget_set_sensitive(entry, is_connected(self));
  gtk_menu_append(GTK_MENU(priv->menu), entry);
	
  entry = gtk_menu_item_new_with_label(_("Add/Remove Programs"));
  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_software), self);
  gtk_menu_append(GTK_MENU(priv->menu), entry);
  if (g_find_program_in_path(SYNCE_SOFTWARE_MANAGER) != NULL && 
      is_connected(self)) {
    gtk_widget_set_sensitive(entry, TRUE);
  } else {
    gtk_widget_set_sensitive(entry, FALSE);
  }
	
  entry = gtk_separator_menu_item_new();
  gtk_menu_append(GTK_MENU(priv->menu), entry);
		
  if (is_connected(self)) {

    if (priv->which_dccm == USE_VDCCM) {
      for (i = 0; i < wm_device_manager_device_count(priv->device_list); i++) {
	device = wm_device_manager_find_by_index(priv->device_list, i);
	device_name = wm_device_get_name(device);
	g_snprintf(buffer, sizeof(buffer), _("Disconnect from '%s'"), device_name);

	entry = gtk_menu_item_new_with_label(buffer);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_disconnect), self);
	gtk_menu_append(GTK_MENU(priv->menu), entry);

	g_free(device_name);
      }
      entry = gtk_separator_menu_item_new();
      gtk_menu_append(GTK_MENU(priv->menu), entry);
    }

  } else {
    strcpy(buffer, _("(No device connected)"));
    entry = gtk_menu_item_new_with_label(buffer);
    gtk_menu_append(GTK_MENU(priv->menu), entry);

    entry = gtk_separator_menu_item_new();
    gtk_menu_append(GTK_MENU(priv->menu), entry);
  }

  if (priv->which_dccm == USE_VDCCM) {
    if (dccm_is_running()) {
      entry = gtk_menu_item_new_with_label(_("Stop DCCM"));
      g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_stop_vdccm), self);
      gtk_menu_append(GTK_MENU(priv->menu), entry);
      gtk_widget_set_sensitive(entry, !is_connected(self));

      entry = gtk_menu_item_new_with_label(_("Restart DCCM"));
      g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_restart_vdccm), self);
      gtk_menu_append(GTK_MENU(priv->menu), entry);
      gtk_widget_set_sensitive(entry, !is_connected(self));
    } else {
      entry = gtk_menu_item_new_with_label(_("Start DCCM"));
      g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_start_vdccm), self);
      gtk_menu_append(GTK_MENU(priv->menu), entry);
    }
    entry = gtk_separator_menu_item_new();
    gtk_menu_append(GTK_MENU(priv->menu), entry);
  }

  entry = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES,
					      NULL);
  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_preferences), self);
  gtk_menu_append(GTK_MENU(priv->menu), entry);
	
  entry = gtk_image_menu_item_new_from_stock (GNOME_STOCK_ABOUT,
					      NULL);
  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_about), self);
  gtk_menu_append(GTK_MENU(priv->menu), entry);

  entry = gtk_image_menu_item_new_from_stock (GNOME_STOCK_MENU_EXIT,
					      NULL);
  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_exit), self);
  gtk_menu_append(GTK_MENU(priv->menu), entry);

  gtk_widget_show_all(priv->menu);
  gtk_menu_popup(GTK_MENU(priv->menu), NULL, NULL, NULL, NULL, event->button, event->time);
}

static void
trayicon_clicked(GtkWidget *button, GdkEventButton *event, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);

  if (event->type != GDK_BUTTON_PRESS)
    return;

  switch (event->button) {
  case 1:
    break;
  case 2:
    break;
  case 3:
    trayicon_menu(event, self);
    break;
  }
}

#ifdef ENABLE_NOTIFY

static void
event_notification(SynceTrayIcon *self, const gchar *summary, const gchar *body)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  if (priv->notification) {
    notify_notification_close(priv->notification, NULL);
    g_object_unref (priv->notification);
  }

  priv->notification = notify_notification_new (summary, body, NULL, GTK_WIDGET (self));
  notify_notification_show (priv->notification, NULL);
}

#endif /* ENABLE_NOTIFY */

/*

static void
synce_trayicon_ (SynceTrayIcon *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }

  return;
}

void
synce_trayicon_ (SynceTrayIcon *self)
{
  return SYNCE_TRAYICON_GET_CLASS (self)->synce_traticon_ (self);
}


void
synce_trayicon__impl(SYnceTrayIcon *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return 0;
  }
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return 0;
  }

  return;
}

*/

/* class & instance functions */

static void
synce_trayicon_init(SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
  GError *error = NULL;

  priv->disposed = FALSE;
  priv->menu = NULL;

#ifdef ENABLE_NOTIFY
  priv->notification = NULL;

  if (!notify_is_initted ())
    notify_init ("synce-trayicon");
#endif /* ENABLE_NOTIFY */

  /* gconf */
  priv->conf_client = gconf_client_get_default();
  gconf_client_add_dir (priv->conf_client,
                        "/apps/synce/trayicon",
                        GCONF_CLIENT_PRELOAD_ONELEVEL,
                        NULL);

  gchar *tmpstr = gconf_client_get_string (priv->conf_client,
					   "/apps/synce/trayicon/dccm",
					   &error);
  if (error)
    g_error("%s: Error contacting gconf: %s", G_STRFUNC, error->message);
  if (!tmpstr)
    priv->which_dccm = USE_ODCCM;
  else {
    if (!(g_ascii_strcasecmp(tmpstr, "v")))
      priv->which_dccm = USE_VDCCM;
    else
      priv->which_dccm = USE_ODCCM;
    g_free(tmpstr);
  }

  /* device list */
  if (!(priv->device_list = g_object_new(WM_DEVICE_MANAGER_TYPE, NULL)))
    g_error("%s: Couldn't initialize device list", G_STRFUNC);

  g_signal_connect (G_OBJECT (priv->device_list), "device-added",
		    (GCallback)device_added_cb, self);

  g_signal_connect (G_OBJECT (priv->device_list), "device-removed",
		    (GCallback)device_removed_cb, self);

  /* tooltip */
  priv->tooltips = gtk_tooltips_new();

  /* dccm comms */
  priv->comms_client = init_client_comms(self);

  /* visible icon */
  priv->icon = gtk_image_new();
  GtkWidget *box = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(box), priv->icon);
  gtk_container_add(GTK_CONTAINER(self), box);
  gtk_widget_show_all(GTK_WIDGET(self));
  g_signal_connect(G_OBJECT(box), "button-press-event", G_CALLBACK(trayicon_clicked), self);

  /* set initial state */
  update(self);
}

static void
synce_trayicon_dispose (GObject *obj)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(obj);
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  if (priv->disposed) {
    return;
  }
  priv->disposed = TRUE;

  /* unref other objects */

  /* gconf */
  gconf_client_remove_dir(priv->conf_client,
			  "/apps/synce/trayicon",
			  NULL);
  g_object_unref(priv->conf_client);

  /* device list */
  wm_device_manager_remove_all(priv->device_list);
  g_object_unref(priv->device_list);

  /* dccm comms */
  uninit_client_comms(self);

#ifdef ENABLE_NOTIFY
  /* notification */
  if (priv->notification)
    {
      notify_notification_close (priv->notification, NULL);
      g_object_unref (priv->notification);
    }
  notify_uninit();
#endif /* ENABLE_NOTIFY */

  if (G_OBJECT_CLASS (synce_trayicon_parent_class)->dispose)
    G_OBJECT_CLASS (synce_trayicon_parent_class)->dispose (obj);
}

static void
synce_trayicon_finalize (GObject *obj)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(obj);
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);


  /*
  priv->tooltips = NULL;
  priv->icon = NULL;
  */


  if (G_OBJECT_CLASS (synce_trayicon_parent_class)->finalize)
    G_OBJECT_CLASS (synce_trayicon_parent_class)->finalize (obj);
}

static void
synce_trayicon_class_init (SynceTrayIconClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (SynceTrayIconPrivate));
  
  gobject_class->dispose = synce_trayicon_dispose;
  gobject_class->finalize = synce_trayicon_finalize;
}
