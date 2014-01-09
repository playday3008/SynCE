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
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#if !USE_GDBUS
#include <dbus/dbus-glib.h>
#endif
#include <synce.h>
#include <libnotify/notify.h>

#if HAVE_APP_INDICATOR
#include <libappindicator/app-indicator.h>
#endif

#ifndef NOTIFY_CHECK_VERSION
#define NOTIFY_CHECK_VERSION(x,y,z) 0
#endif

#include "synce-trayicon.h"
#include "properties.h"
#include "utils.h"
#include "keyring.h"
#include "dccm-client.h"
#if ENABLE_VDCCM_SUPPORT
#include "gtop_stuff.h"
#include "vdccm-client.h"
#endif
#if ENABLE_ODCCM_SUPPORT
#include "odccm-client.h"
#endif
#if ENABLE_UDEV_SUPPORT
#include "udev-client.h"
#endif
#include "device-manager.h"
#include "stock-icons.h"
#include "device-info.h"
#include "module.h"

G_DEFINE_TYPE (SynceTrayIcon, synce_trayicon, G_TYPE_OBJECT)

typedef struct _SynceTrayIconPrivate SynceTrayIconPrivate;
struct _SynceTrayIconPrivate {

  GtkStatusIcon *status_icon;
  GSettings *settings;
  gulong settings_watch_id;
#if ENABLE_UDEV_SUPPORT
  DccmClient *udev_client;
#endif
#if ENABLE_ODCCM_SUPPORT
  DccmClient *odccm_client;
#endif
#if ENABLE_VDCCM_SUPPORT
  DccmClient *vdccm_client;
#endif
  WmDeviceManager *device_list;
  GtkMenu *menu;
  NotifyNotification *notification;
#if HAVE_APP_INDICATOR
  AppIndicator *app_indicator;
#endif
  gboolean show_disconnected;

  gboolean disposed;
};

#define SYNCE_TRAYICON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SYNCE_TRAYICON_TYPE, SynceTrayIconPrivate))

     /* methods */


static gboolean
devices_ready(SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  if (wm_device_manager_device_connected_count(priv->device_list) > 0) {
    return TRUE;
  }
  return FALSE;
}

static gboolean
devices_connected(SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  if (wm_device_manager_device_all_count(priv->device_list) > 0) {
    return TRUE;
  }
  return FALSE;
}

static void
set_status_tooltip(SynceTrayIcon *self, GtkTooltip *tooltip)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  gchar *power_str = NULL;
  gchar *store_str = NULL;
  gchar *pdaname = NULL;
  gchar *tooltip_str = NULL;
  gchar *tmpstr = NULL;
  WmDevice *device;
  GList *device_names = NULL;
  GList *device_names_iter = NULL;

  if (!(devices_ready(self))) {
          gtk_tooltip_set_text(tooltip, _("Not connected"));
          return;
  }

  device_names = wm_device_manager_get_connected_names(priv->device_list);
  device_names_iter = device_names;

  while (device_names_iter) {
          if (!(device = wm_device_manager_find_by_name(priv->device_list, device_names_iter->data))) {
                  g_free(device_names_iter->data);
                  device_names_iter = g_list_next(device_names_iter);
                  continue;
          }

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

          g_free(device_names_iter->data);
          device_names_iter = g_list_next(device_names_iter);
  }

  g_list_free(device_names);
  gtk_tooltip_set_text(tooltip, tooltip_str);
  g_free(tooltip_str);
  return;
}

static gboolean
query_tooltip_cb(GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data)
{
  set_status_tooltip(SYNCE_TRAYICON(user_data), tooltip);

  /* show the tooltip */
  return TRUE;
}

static void
trayicon_update_menu(SynceTrayIcon *self);

static gboolean 
update_status(gpointer data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(data);
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  /*
   * for appindicator we need to change the menu before changing the status, or
   * the icon won't change, not sure why ?
   */
  trayicon_update_menu(self);

  if (devices_connected(self))
    gtk_status_icon_set_visible(priv->status_icon, TRUE);
  else
    gtk_status_icon_set_visible(priv->status_icon, priv->show_disconnected);

  if (devices_ready(self))
    gtk_status_icon_set_from_icon_name(priv->status_icon, SYNCE_STOCK_CONNECTED);
  else
    gtk_status_icon_set_from_icon_name(priv->status_icon, SYNCE_STOCK_DISCONNECTED);


#if HAVE_APP_INDICATOR
  if (!devices_connected(self)) {
    if (!(priv->show_disconnected))
      app_indicator_set_status(priv->app_indicator, APP_INDICATOR_STATUS_PASSIVE);
    else
      app_indicator_set_status(priv->app_indicator, APP_INDICATOR_STATUS_ACTIVE);
  } else if (devices_ready(self))
    app_indicator_set_status(priv->app_indicator, APP_INDICATOR_STATUS_ATTENTION);
  else
    app_indicator_set_status(priv->app_indicator, APP_INDICATOR_STATUS_ACTIVE);
#endif

  /* prevent function from running again when
   set with g_idle_add */
  return FALSE;
}


static void
event_notification(SynceTrayIcon *self, const gchar *summary, const gchar *body)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  if (priv->notification) {
    notify_notification_close(priv->notification, NULL);
    g_object_unref (priv->notification);
    priv->notification = NULL;
  }

    /* libnotify 0.7.0 and later has no support for attaching to widgets */
#if NOTIFY_CHECK_VERSION(0,7,0)
  priv->notification = notify_notification_new (summary, body, NULL);
  notify_notification_show (priv->notification, NULL);
#else
  if (gtk_status_icon_is_embedded(priv->status_icon)) {
    priv->notification = notify_notification_new_with_status_icon (summary, body, NULL, priv->status_icon);
    notify_notification_show (priv->notification, NULL);
  } else {
    priv->notification = notify_notification_new (summary, body, NULL);
    notify_notification_show (priv->notification, NULL);
  }
#endif
}


void device_password_dialog_entry_changed_cb (GtkWidget *widget, gpointer data)
{
  gchar *entry_string;

  entry_string = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  if (entry_string)
    gtk_widget_set_sensitive(GTK_WIDGET(data), TRUE);
  else
    gtk_widget_set_sensitive(GTK_WIDGET(data), FALSE);

  g_free(entry_string);
}

static void
trayicon_supply_password(SynceTrayIcon *self)
{
        SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

        GList *locked_devices = NULL;
        GList *tmplist = NULL;
        GtkBuilder *builder;
        GtkWidget *password_dialog, *password_dialog_entry;
        GtkWidget *password_dialog_ok, *password_dialog_pdaname, *password_dialog_save_pw;
        gchar *password = NULL;
        gint response;
        gchar *pdaname = NULL;
        gchar *notify_string = NULL;
        GnomeKeyringResult keyring_ret;
        WmDevice *device = NULL;
        gchar *dccm_type = NULL;
        GtkListStore *store = NULL;
        GtkTreeIter iter;
        GtkCellRenderer *renderer = NULL;


        /* get list of locked devices from device_manager */
        locked_devices = wm_device_manager_get_passwordreq_names(priv->device_list);

        if (!locked_devices)
                return;

        builder = gtk_builder_new();
        guint builder_res;
        GError *error = NULL;
        gchar *namelist[] = { "password_dialog", NULL };

        builder_res = gtk_builder_add_objects_from_file(builder,
                                                        SYNCE_DATA "synce_trayicon_properties.glade",
                                                        namelist,
                                                        &error);
        if (builder_res == 0) {
                g_critical("%s: failed to load interface file: %s", G_STRFUNC, error->message);
                g_error_free(error);
                error = NULL;
        }

        password_dialog = GTK_WIDGET(gtk_builder_get_object(builder, "password_dialog"));

        password_dialog_pdaname = GTK_WIDGET(gtk_builder_get_object(builder, "password_dialog_pdaname"));
        password_dialog_entry = GTK_WIDGET(gtk_builder_get_object(builder, "password_dialog_entry"));
        password_dialog_ok = GTK_WIDGET(gtk_builder_get_object(builder, "password_dialog_ok"));
        password_dialog_save_pw = GTK_WIDGET(gtk_builder_get_object(builder, "password_dialog_save_password"));

        g_object_unref(builder);

        store = gtk_list_store_new (1, G_TYPE_STRING);

        tmplist = locked_devices;
        while (tmplist) {
                gtk_list_store_append(store, &iter);  /* Acquire an iterator */

                gtk_list_store_set(store, &iter, 0, tmplist->data, -1);

                g_free(tmplist->data);
                tmplist = g_list_next(tmplist);
        }
        g_list_free(locked_devices);

        gtk_combo_box_set_model(GTK_COMBO_BOX(password_dialog_pdaname), GTK_TREE_MODEL (store));

        renderer = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(password_dialog_pdaname), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(password_dialog_pdaname), renderer,
                                        "text", 0, NULL);
        gtk_combo_box_set_active(GTK_COMBO_BOX(password_dialog_pdaname), 0);

        gtk_widget_set_sensitive(password_dialog_ok, FALSE);
        gtk_editable_delete_text(GTK_EDITABLE(password_dialog_entry), 0, -1);

        g_signal_connect (G_OBJECT (password_dialog_entry), "changed",
                          G_CALLBACK (device_password_dialog_entry_changed_cb), password_dialog_ok);

        gtk_widget_show_all (password_dialog);
        gtk_dialog_set_default_response(GTK_DIALOG(password_dialog), GTK_RESPONSE_OK);

        response = gtk_dialog_run(GTK_DIALOG(password_dialog));
        gtk_widget_hide(password_dialog);
        if (response == GTK_RESPONSE_OK) {
                if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(password_dialog_pdaname), &iter)) {
                        gtk_tree_model_get (GTK_TREE_MODEL(store),
                                            &iter,
                                            0, &pdaname,
                                            -1);
                        g_debug("%s: pda '%s' selected", G_STRFUNC, pdaname);
                }

                password = gtk_editable_get_chars(GTK_EDITABLE(password_dialog_entry), 0, -1);
        }
        gtk_editable_delete_text(GTK_EDITABLE(password_dialog_entry), 0, -1);

        if ((!password) || (!pdaname)) {
                g_debug("%s: No password provided or no pda selected", G_STRFUNC);
                g_free(pdaname);
                g_free(password);
                gtk_widget_destroy(password_dialog);
                return;
        }

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(password_dialog_save_pw))) {
                if ((keyring_ret = keyring_set_key(pdaname, password)) != GNOME_KEYRING_RESULT_OK) {
                        notify_string = g_strdup_printf("Unable to save password for device \"%s\" in your keyring: %s", pdaname, gnome_keyring_result_to_message(keyring_ret));
                        event_notification(self, "Save password failed", notify_string);
                        g_free(notify_string);
                }
        }

        gtk_widget_destroy(password_dialog);

        if (!(device = wm_device_manager_find_by_name(priv->device_list, pdaname))) {
                g_warning("%s: password provided for unknown device '%s'", G_STRFUNC, pdaname);
                g_free(password);
                return;
        }

        g_object_get(device, "dccm-type", &dccm_type, NULL);

#if ENABLE_UDEV_SUPPORT
        if (!(g_ascii_strcasecmp(dccm_type, "udev")))
                dccm_client_provide_password(priv->udev_client, pdaname, password);
#endif
#if ENABLE_ODCCM_SUPPORT
        if (!(g_ascii_strcasecmp(dccm_type, "odccm")))
                dccm_client_provide_password(priv->odccm_client, pdaname, password);
#endif
#if ENABLE_VDCCM_SUPPORT
        if (!(g_ascii_strcasecmp(dccm_type, "vdccm")))
                dccm_client_provide_password(priv->vdccm_client, pdaname, password);
#endif

        g_free(pdaname);
        g_free(password);
}

static void
menu_supply_password_cb(GtkWidget *menuitem, SynceTrayIcon *self)
{
  trayicon_supply_password(self);
}

static void
password_rejected_cb(DccmClient *comms_client, gchar *pdaname, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  GnomeKeyringResult keyring_ret;
  gchar *notify_string = NULL;

  g_debug("%s: removing password from keyring", G_STRFUNC);
  
  if ((keyring_ret = keyring_delete_key(pdaname)))
    g_warning("%s: failed to remove incorrect password from keyring: %d: %s", G_STRFUNC, keyring_ret, gnome_keyring_result_to_message(keyring_ret));

  notify_string = g_strdup_printf("Password for device \"%s\" was rejected", pdaname);
  event_notification(self, "Incorrect password", notify_string);
  g_free(notify_string);
}

static void
password_required_cb(DccmClient *comms_client, const gchar *pdaname, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);

  gchar *password = NULL;
  GnomeKeyringResult keyring_ret;
  gchar *notify_string = NULL;

  if ((keyring_ret = keyring_get_key(pdaname, &password)) == GNOME_KEYRING_RESULT_OK) {
          dccm_client_provide_password(comms_client, pdaname, password);
          g_free(password);
          return;
  }

  notify_string = g_strdup_printf("A password is required for device \"%s\", click here to provide a password", pdaname);
  event_notification(self, "Password required", notify_string);
  g_free(notify_string);
}

static void
password_required_on_device_cb(DccmClient *comms_client, gchar *pdaname, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  gchar *notify_string = NULL;

  notify_string = g_strdup_printf("The device %s is locked. Please unlock it by following instructions on the device", pdaname);
  event_notification(self, "Device locked", notify_string);
  g_free(notify_string);
}

static void
device_connected_cb(DccmClient *comms_client, gchar *pdaname, gpointer info, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
  WmDevice *new_device = WM_DEVICE(info);
  WmDevice *device;

  g_debug("%s: looking for preexisting device %s", G_STRFUNC, pdaname);
  if ((device = wm_device_manager_find_by_name(priv->device_list, pdaname))) {
    g_debug("%s: Ignoring connection message for \"%s\", already connected", G_STRFUNC, pdaname);
    return;
  }

  wm_device_manager_add(priv->device_list, new_device);
  g_idle_add(update_status, self);
}

static void
device_disconnected_cb(DccmClient *comms_client, gchar *pdaname, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  wm_device_manager_remove_by_name(priv->device_list, pdaname);
  return;
}

static void
device_unlocked_cb(DccmClient *comms_client, gchar *pdaname, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  wm_device_manager_unlocked(priv->device_list, pdaname);
  return;
}

#if ENABLE_VDCCM_SUPPORT
static void
stop_dccm ()
{
  send_signal_dccm(SIGTERM);
}

static void
uninit_vdccm_client_comms(SynceTrayIcon *self)
{
        SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

        gboolean enable_vdccm = g_settings_get_boolean(priv->settings, "enable-vdccm");
  
        if (!enable_vdccm)
                return;

        gboolean stop_vdccm = g_settings_get_boolean(priv->settings, "start-vdccm");

        if (stop_vdccm)
                stop_dccm();

        if (IS_DCCM_CLIENT(priv->vdccm_client)) {
                dccm_client_uninit_comms(priv->vdccm_client);
                g_object_unref(priv->vdccm_client);
                priv->vdccm_client = NULL;
        }

        wm_device_manager_remove_by_prop(priv->device_list, "dccm-type", "vdccm");

        return;
}
#endif

static void
service_starting_cb(DccmClient *comms_client, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);

#if ENABLE_UDEV_SUPPORT
  if (IS_UDEV_CLIENT(comms_client)) {
          event_notification(self, "Service starting", "Udev DCCM has signalled that it is starting");
          return;
  }
#endif

#if ENABLE_ODCCM_SUPPORT
  if (IS_ODCCM_CLIENT(comms_client)) {
          event_notification(self, "Service starting", "Odccm has signalled that it is starting");
          return;
  }
#endif

#if ENABLE_VDCCM_SUPPORT
  if (IS_VDCCM_CLIENT(comms_client)) {
          event_notification(self, "Service starting", "Vdccm has signalled that it is starting");
          return;
  }
#endif
}


static void
service_stopping_cb(DccmClient *comms_client, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

#if ENABLE_UDEV_SUPPORT
  if (IS_UDEV_CLIENT(comms_client)) {
          event_notification(self, "Service stopping", "Udev DCCM has signalled that it is stopping");
          wm_device_manager_remove_by_prop(priv->device_list, "dccm-type", "udev");
          return;
  }
#endif

#if ENABLE_ODCCM_SUPPORT
  if (IS_ODCCM_CLIENT(comms_client)) {
          event_notification(self, "Service stopping", "Odccm has signalled that it is stopping");
          wm_device_manager_remove_by_prop(priv->device_list, "dccm-type", "odccm");
          return;
  }
#endif

#if ENABLE_VDCCM_SUPPORT
  if (IS_VDCCM_CLIENT(comms_client)) {
          event_notification(self, "Service stopping", "Vdccm has signalled that it is stopping");
          uninit_vdccm_client_comms(self);
          return;
  }
#endif
}


static void
uninit_client_comms(SynceTrayIcon *self)
{
        SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

#if ENABLE_UDEV_SUPPORT
        if (IS_DCCM_CLIENT(priv->udev_client)) {
                dccm_client_uninit_comms(priv->udev_client);
                g_object_unref(priv->udev_client);
                priv->udev_client = NULL;
                wm_device_manager_remove_by_prop(priv->device_list, "dccm-type", "udev");
        }
#endif

#if ENABLE_ODCCM_SUPPORT
        if (IS_DCCM_CLIENT(priv->odccm_client)) {
                dccm_client_uninit_comms(priv->odccm_client);
                g_object_unref(priv->odccm_client);
                priv->odccm_client = NULL;
                wm_device_manager_remove_by_prop(priv->device_list, "dccm-type", "odccm");
        }
#endif

#if ENABLE_VDCCM_SUPPORT
        uninit_vdccm_client_comms(self);
#endif

        return;
}

#if ENABLE_VDCCM_SUPPORT
static gboolean
start_dccm (SynceTrayIcon *self)
{
  GError *error = NULL;

  gchar *argv[1] = {
          DCCM_BIN
  };

  if (!(dccm_is_running())) {
    g_debug("%s: starting %s", G_STRFUNC, DCCM_BIN);
    if (!g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)) {
            event_notification(self, "Service failure", _("Can't start vdccm which is needed to communicate with the PDA. Make sure it is installed and try again."));
            g_warning("%s: Failed to start %s: %s", G_STRFUNC, DCCM_BIN, error->message);
            g_error_free(error);
            return FALSE;
    }
  } else {
    g_warning("%s: %s is already running!", G_STRFUNC, DCCM_BIN);
  }

  sleep(2);
  return TRUE;
}


static void
init_vdccm_client_comms(SynceTrayIcon *self)
{
        SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
        DccmClient *comms_client = NULL;

        gboolean enable_vdccm = g_settings_get_boolean(priv->settings, "enable-vdccm");

        if (!enable_vdccm)
                return;

        gboolean start_vdccm = g_settings_get_boolean(priv->settings, "start-vdccm");

        if (start_vdccm)
                if (!(start_dccm(self))) {
                        g_critical("%s: Unable to start vdccm", G_STRFUNC);
                        return;
                }

        comms_client = DCCM_CLIENT(g_object_new(VDCCM_CLIENT_TYPE, NULL));

        g_signal_connect (G_OBJECT (comms_client), "password-rejected",
                          G_CALLBACK (password_rejected_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "password-required",
                          G_CALLBACK (password_required_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "password-required-on-device",
                          G_CALLBACK (password_required_on_device_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "service-starting",
                          G_CALLBACK (service_stopping_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "service-stopping",
                          G_CALLBACK (service_stopping_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "device-connected",
                          G_CALLBACK (device_connected_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "device-disconnected",
                          G_CALLBACK (device_disconnected_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "device-unlocked",
                          G_CALLBACK (device_unlocked_cb), self);

        if (!(dccm_client_init_comms(comms_client))) {
                g_critical("%s: Unable to initialise vdccm comms client", G_STRFUNC);
                event_notification(self, "Service failure", "Unable to contact VDCCM, check it is installed and set to run");
                g_object_unref(comms_client);
        }

        priv->vdccm_client = comms_client;

        return;
}
#endif

static void
init_client_comms(SynceTrayIcon *self)
{
        SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
        DccmClient *comms_client = NULL;

#if !USE_GDBUS
        dbus_g_thread_init();
#endif

#if ENABLE_UDEV_SUPPORT
        comms_client = DCCM_CLIENT(g_object_new(UDEV_CLIENT_TYPE, NULL));

        g_signal_connect (G_OBJECT (comms_client), "password-rejected",
                          G_CALLBACK (password_rejected_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "password-required",
                          G_CALLBACK (password_required_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "password-required-on-device",
                          G_CALLBACK (password_required_on_device_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "service-starting",
                          G_CALLBACK (service_starting_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "service-stopping",
                          G_CALLBACK (service_stopping_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "device-connected",
                          G_CALLBACK (device_connected_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "device-disconnected",
                          G_CALLBACK (device_disconnected_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "device-unlocked",
                          G_CALLBACK (device_unlocked_cb), self);

        if (!(dccm_client_init_comms(comms_client))) {
                g_critical("%s: Unable to initialise udev dccm comms client", G_STRFUNC);
                g_object_unref(comms_client);
        }

        priv->udev_client = comms_client;
#endif

#if ENABLE_ODCCM_SUPPORT
        comms_client = DCCM_CLIENT(g_object_new(ODCCM_CLIENT_TYPE, NULL));

        g_signal_connect (G_OBJECT (comms_client), "password-rejected",
                          G_CALLBACK (password_rejected_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "password-required",
                          G_CALLBACK (password_required_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "password-required-on-device",
                          G_CALLBACK (password_required_on_device_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "service-starting",
                          G_CALLBACK (service_starting_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "service-stopping",
                          G_CALLBACK (service_stopping_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "device-connected",
                          G_CALLBACK (device_connected_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "device-disconnected",
                          G_CALLBACK (device_disconnected_cb), self);

        g_signal_connect (G_OBJECT (comms_client), "device-unlocked",
                          G_CALLBACK (device_unlocked_cb), self);

        if (!(dccm_client_init_comms(comms_client))) {
                g_critical("%s: Unable to initialise odccm comms client", G_STRFUNC);
                g_object_unref(comms_client);
        }

        priv->odccm_client = comms_client;
#endif

#if ENABLE_VDCCM_SUPPORT
        init_vdccm_client_comms(self);
#endif
        return;
}


static void
device_added_cb(GObject *obj, gchar *name, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  WmDevice *new_device = NULL;
  gchar *model, *platform, *notify_string;

  new_device = wm_device_manager_find_by_name(priv->device_list, name);
  if (!new_device)
          return;

  g_idle_add(update_status, self);

  module_run_connect(name);

  g_object_get(new_device, "hardware", &model, NULL);
  g_object_get(new_device, "class", &platform, NULL);

  notify_string = g_strdup_printf("A %s %s '%s' just connected.", model, platform, name);
  event_notification(self, "PDA connected", notify_string);

  g_free(model);
  g_free(platform);
  g_free(notify_string);
}

static void
device_removed_cb(GObject *obj, gchar *name, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);

  gchar *notify_string;

  module_run_disconnect(name);

  notify_string = g_strdup_printf("'%s' just disconnected.", name);
  event_notification(self, "PDA disconnected", notify_string);
  g_free(notify_string);

  g_idle_add(update_status, self);
}


/* menu callbacks */

static void
menu_explore (GtkWidget *menu_item, SynceTrayIcon *self)
{
  const gchar *name = NULL;
  gchar *arg_str = NULL;
  GError *error = NULL;

  GtkWidget *device_menu = gtk_widget_get_parent(menu_item);
  name = gtk_menu_get_title(GTK_MENU(device_menu));

  arg_str = g_strdup_printf("synce://%s/", name);
  
  char *argv[4] = {
          "nautilus", "-n", arg_str, NULL
  };
  
  if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)) {
          g_warning("%s: failed to explore '%s': %s", G_STRFUNC, arg_str, error->message);
          g_error_free(error);
          return;
  }

  g_free(arg_str);
}

static void
menu_preferences (GtkWidget *button, SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
  run_prefs_dialog(priv->settings);
}

static void
device_info_closed_cb(WmDeviceInfo *device_info, gpointer user_data)
{
  g_object_unref(device_info);
}

static void
menu_device_info (GtkWidget *menu_item, SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
  const gchar *name = NULL;
  WmDevice *device = NULL;
  WmDeviceInfo *device_info = NULL;

  GtkWidget *device_menu = gtk_widget_get_parent(menu_item);
  name = gtk_menu_get_title(GTK_MENU(device_menu));

  if (!(device = wm_device_manager_find_by_name(priv->device_list, name))) {
          g_debug("%s: cannot display device info for unfound device '%s'", G_STRFUNC, name);
          return;
  }

  device_info = g_object_new(WM_DEVICE_INFO_TYPE, "device", device, NULL);

  g_signal_connect(G_OBJECT(device_info), "device-info-closed", G_CALLBACK(device_info_closed_cb), NULL);
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

  about = gtk_about_dialog_new();

  gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about), g_get_application_name());
  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), VERSION);
  gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), _("Copyright (c) 2002, David Eriksson\n"
							    "Copyright (c) 2007 - 2009, Mark Ellis"));
  gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about), _("Displays information about devices connected through SynCE"));
  gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about), "http://www.synce.org");
  gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), authors);
  gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about), SYNCE_STOCK_CONNECTED);

  gtk_dialog_run (GTK_DIALOG (about));
  gtk_widget_destroy (GTK_WIDGET(about));
}

#if ENABLE_VDCCM_SUPPORT
static void
menu_disconnect(GtkWidget *menu_item, SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);
  const gchar *name = NULL;

  GtkWidget *device_menu = gtk_widget_get_parent(menu_item);
  name = gtk_menu_get_title(GTK_MENU(device_menu));

  g_debug("%s: Asked to disconnect %s by user", G_STRFUNC, name);

  dccm_client_request_disconnect(priv->vdccm_client, name);
}

static void
menu_start_vdccm(GtkWidget *button, SynceTrayIcon *self)
{
  init_vdccm_client_comms(self);
}

static void
menu_stop_vdccm(GtkWidget *button, SynceTrayIcon *self)
{
  uninit_vdccm_client_comms(self);
}

static void
menu_restart_vdccm(GtkWidget *button, SynceTrayIcon *self)
{
  uninit_vdccm_client_comms(self);
  sleep(1);
  init_vdccm_client_comms(self);
}
#endif

static void
menu_exit(GtkWidget *button, SynceTrayIcon *self)
{
  gtk_main_quit();
}

static void
trayicon_update_menu(SynceTrayIcon *self)
{
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  GtkWidget *entry;
  WmDevice *device;
  GList *device_names = NULL;
  GList *device_names_iter = NULL;
  GList *locked_devices = NULL;

  if (priv->menu) {
    gtk_widget_destroy(GTK_WIDGET(priv->menu));
    g_object_unref(priv->menu);
  }

  priv->menu = GTK_MENU(gtk_menu_new());
  g_object_ref_sink(priv->menu);


  if (devices_connected(self)) {
          GtkWidget *device_menu;

	  /* devices needing password to be sent from us */

	  locked_devices = wm_device_manager_get_passwordreq_names(priv->device_list);
	  device_names_iter = locked_devices;

          while (device_names_iter) {

                  if (!(device = wm_device_manager_find_by_name(priv->device_list, device_names_iter->data))) {
                          g_free(device_names_iter->data);
                          device_names_iter = g_list_next(device_names_iter);
                          continue;
                  }

                  entry = gtk_menu_item_new_with_label(device_names_iter->data);
                  gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), entry);
		  gtk_widget_show(GTK_WIDGET(entry));

                  device_menu = gtk_menu_new();
                  gtk_menu_set_title(GTK_MENU(device_menu), device_names_iter->data);
                  gtk_menu_item_set_submenu(GTK_MENU_ITEM(entry), device_menu);

                  entry = gtk_menu_item_new_with_label(_("Unlock"));
                  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_supply_password_cb), self);
                  gtk_menu_shell_append(GTK_MENU_SHELL(device_menu), entry);
		  gtk_widget_show(GTK_WIDGET(entry));

                  g_free(device_names_iter->data);
                  device_names_iter = g_list_next(device_names_iter);
	  }
          g_list_free(locked_devices);

	  /* devices needing password to be entered on itself */

	  locked_devices = wm_device_manager_get_passwordreqondevice_names(priv->device_list);
	  device_names_iter = locked_devices;

          while (device_names_iter) {

                  if (!(device = wm_device_manager_find_by_name(priv->device_list, device_names_iter->data))) {
                          g_free(device_names_iter->data);
                          device_names_iter = g_list_next(device_names_iter);
                          continue;
                  }

                  entry = gtk_menu_item_new_with_label(device_names_iter->data);
                  gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), entry);
		  gtk_widget_set_sensitive(GTK_WIDGET(entry), FALSE);
		  gtk_widget_show(GTK_WIDGET(entry));

                  g_free(device_names_iter->data);
                  device_names_iter = g_list_next(device_names_iter);
	  }
          g_list_free(locked_devices);

	  /* unlocked devices */

          device_names = wm_device_manager_get_connected_names(priv->device_list);
          device_names_iter = device_names;

          while (device_names_iter) {

                  if (!(device = wm_device_manager_find_by_name(priv->device_list, device_names_iter->data))) {
                          g_free(device_names_iter->data);
                          device_names_iter = g_list_next(device_names_iter);
                          continue;
                  }

                  entry = gtk_menu_item_new_with_label(device_names_iter->data);
                  gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), entry);
		  gtk_widget_show(GTK_WIDGET(entry));

                  device_menu = gtk_menu_new();
                  gtk_menu_set_title(GTK_MENU(device_menu), device_names_iter->data);
                  gtk_menu_item_set_submenu(GTK_MENU_ITEM(entry), device_menu);

                  entry = gtk_menu_item_new_with_label(_("Explore with Filemanager"));
                  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_explore), self);
                  gtk_menu_shell_append(GTK_MENU_SHELL(device_menu), entry);
		  gtk_widget_show(GTK_WIDGET(entry));

                  entry = gtk_menu_item_new_with_label(_("View device status"));
                  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_device_info), self);
                  gtk_menu_shell_append(GTK_MENU_SHELL(device_menu), entry);
		  gtk_widget_show(GTK_WIDGET(entry));

#if ENABLE_VDCCM_SUPPORT
                  if (g_settings_get_boolean(priv->settings, "enable-vdccm")) {
                          entry = gtk_image_menu_item_new_from_stock (GTK_STOCK_DISCONNECT, NULL);
                          g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_disconnect), self);
                          gtk_menu_shell_append(GTK_MENU_SHELL(device_menu), entry);
			  gtk_widget_show(GTK_WIDGET(entry));
                  }
#endif

                  g_free(device_names_iter->data);
                  device_names_iter = g_list_next(device_names_iter);
          }
          g_list_free(device_names);
  } else {
    entry = gtk_menu_item_new_with_label(_("(No device connected)"));
    gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), entry);
    gtk_widget_show(GTK_WIDGET(entry));
  }

  entry = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), entry);
  gtk_widget_show(GTK_WIDGET(entry));

#if ENABLE_VDCCM_SUPPORT
  if (g_settings_get_boolean(priv->settings, "enable-vdccm")) {
    if (dccm_is_running()) {
      entry = gtk_menu_item_new_with_label(_("Stop DCCM"));
      g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_stop_vdccm), self);
      gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), entry);
      gtk_widget_set_sensitive(entry, !devices_ready(self));
      gtk_widget_show(GTK_WIDGET(entry));

      entry = gtk_menu_item_new_with_label(_("Restart DCCM"));
      g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_restart_vdccm), self);
      gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), entry);
      gtk_widget_set_sensitive(entry, !devices_ready(self));
      gtk_widget_show(GTK_WIDGET(entry));
    } else {
      entry = gtk_menu_item_new_with_label(_("Start DCCM"));
      g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_start_vdccm), self);
      gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), entry);
      gtk_widget_show(GTK_WIDGET(entry));
    }
    entry = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), entry);
    gtk_widget_show(GTK_WIDGET(entry));
  }
#endif

  entry = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES, NULL);
  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_preferences), self);
  gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), entry);
  gtk_widget_show(GTK_WIDGET(entry));

  entry = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_about), self);
  gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), entry);
  gtk_widget_show(GTK_WIDGET(entry));

  entry = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(menu_exit), self);
  gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), entry);
  gtk_widget_show(GTK_WIDGET(entry));

#if HAVE_APP_INDICATOR
  g_debug("%s: setting menu to app ind", G_STRFUNC);

  gtk_widget_show(GTK_WIDGET(priv->menu));

  app_indicator_set_menu(priv->app_indicator, GTK_MENU(priv->menu));

  app_indicator_set_status(priv->app_indicator, APP_INDICATOR_STATUS_ACTIVE);
#endif
}

static void
trayicon_activate_cb(GtkStatusIcon *status_icon, gpointer user_data)
{
  g_debug("%s: synce trayicon activated", G_STRFUNC);

  trayicon_supply_password(SYNCE_TRAYICON(user_data));

  return;
}

static void
trayicon_popup_menu_cb(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(user_data);
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  gtk_widget_show_all(GTK_WIDGET(priv->menu));
  gtk_menu_popup(GTK_MENU(priv->menu), NULL, NULL,
                 gtk_status_icon_position_menu, status_icon,
                 button, activate_time);
}

static void
prefs_changed_cb (GSettings *settings,
		  gchar *key, gpointer data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(data);
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

#if ENABLE_VDCCM_SUPPORT
  if (!(g_ascii_strcasecmp(key, "enable-vdccm"))) {
    gboolean enable_vdccm = g_settings_get_boolean(priv->settings, key);

    if (enable_vdccm)
            init_vdccm_client_comms(self);
    else
            uninit_vdccm_client_comms(self);

    return;
  }
#endif

  if (!(g_ascii_strcasecmp(key, "/apps/synce/trayicon/show-disconnected"))) {
    priv->show_disconnected = g_settings_get_boolean(priv->settings, key);
    g_idle_add(update_status, self);

    return;
  }

  return;
}

#if HAVE_APP_INDICATOR
static void
app_ind_connection_changed_cb(AppIndicator *appind, gboolean connected, gpointer data)
{
  SynceTrayIcon *self = SYNCE_TRAYICON(data);
  SynceTrayIconPrivate *priv = SYNCE_TRAYICON_GET_PRIVATE (self);

  if (connected)
    g_debug("%s: application indicator connection changed - now connected", G_STRFUNC);
  else
    g_debug("%s: application indicator connection changed - now disconnected", G_STRFUNC);

  return;
}
#endif

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

  priv->disposed = FALSE;
  priv->menu = NULL;

  priv->notification = NULL;

  if (!notify_is_initted ())
    notify_init ("synce-trayicon");

  /* 
   * initialise gsettings
   */
  priv->settings = g_settings_new("org.synce.SynceTrayicon");

  priv->settings_watch_id = g_signal_connect (G_OBJECT (priv->settings), "changed",
                                              (GCallback)prefs_changed_cb, self);

  priv->show_disconnected = g_settings_get_boolean(priv->settings, "show-disconnected");

  /* 
   * initialise device list
   */
  if (!(priv->device_list = g_object_new(WM_DEVICE_MANAGER_TYPE, NULL)))
    g_error("%s: Couldn't initialize device list", G_STRFUNC);

  g_signal_connect (G_OBJECT (priv->device_list), "device-added",
		    (GCallback)device_added_cb, self);

  g_signal_connect (G_OBJECT (priv->device_list), "device-removed",
		    (GCallback)device_removed_cb, self);


  /*
   * determine how we display status and menu, and initialise
   */

  priv->status_icon = gtk_status_icon_new();
  gboolean app_ind_connected = FALSE;

#if HAVE_APP_INDICATOR
  g_debug("%s: setting up application-indicator", G_STRFUNC);
  priv->app_indicator = app_indicator_new("synce-trayicon",
					  SYNCE_STOCK_DISCONNECTED,
					  APP_INDICATOR_CATEGORY_HARDWARE);

  app_indicator_set_attention_icon_full(priv->app_indicator, SYNCE_STOCK_CONNECTED, "SynCE device connected");
  g_signal_connect(G_OBJECT(priv->app_indicator), "connection-changed", G_CALLBACK(app_ind_connection_changed_cb), self);

  g_object_get(priv->app_indicator, "connected", &app_ind_connected, NULL);
  if (app_ind_connected)
    g_debug("%s: application indicator connected", G_STRFUNC);
  else
    g_debug("%s: application indicator not connected", G_STRFUNC);

#endif

  g_signal_connect(G_OBJECT(priv->status_icon), "activate", G_CALLBACK(trayicon_activate_cb), self);
  g_signal_connect(G_OBJECT(priv->status_icon), "popup-menu", G_CALLBACK(trayicon_popup_menu_cb), self);

  /* tooltip */
  gtk_status_icon_set_has_tooltip(priv->status_icon, TRUE);
  g_signal_connect(G_OBJECT(priv->status_icon), "query-tooltip", G_CALLBACK(query_tooltip_cb), self);

  /* module initialisation */
  module_load_all();

  /* set initial state */
  update_status(self);

  /* dccm comms */
  init_client_comms(self);
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

  g_object_unref(priv->status_icon);

#if HAVE_APP_INDICATOR
  g_object_unref(priv->app_indicator);
#endif

  if (priv->menu) {
    gtk_widget_destroy(GTK_WIDGET(priv->menu));
    g_object_unref(priv->menu);
  }

  /* gsettings */
  g_signal_handler_disconnect(priv->settings, priv->settings_watch_id);
  g_object_unref(priv->settings);

  /* dccm comms */
  uninit_client_comms(self);

  /* device list */
  wm_device_manager_remove_all(priv->device_list);
  g_object_unref(priv->device_list);

  /* notification */
  if (priv->notification)
    {
      notify_notification_close (priv->notification, NULL);
      g_object_unref (priv->notification);
    }
  notify_uninit();

  module_unload_all();

  if (G_OBJECT_CLASS (synce_trayicon_parent_class)->dispose)
    G_OBJECT_CLASS (synce_trayicon_parent_class)->dispose (obj);
}

static void
synce_trayicon_finalize (GObject *obj)
{
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
