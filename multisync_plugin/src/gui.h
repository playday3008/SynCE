#ifndef GUI_H
#define GUI_H

#include "syncengine.h"

GtkWidget* open_option_window(sync_pair *pair, connection_type type);
#include <stdlib.h>
#include <glib.h>
#include <gmodule.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gnome.h>
#include <rapi.h>
#include <synce_log.h>
#include "syncengine.h"
#include "gui.h"
#include "multisync_plugin.h"

typedef struct _Synce_Partner
{
	gchar *name;
	uint32_t id;
	gboolean current;
	gboolean changed;
	gboolean exist;
} Synce_Partner;

GtkWidget* open_option_window(sync_pair *pair, connection_type type);

void synce_build_partner_menu();
void synce_partner_menu_response_1(GtkMenuItem *menuitem, gpointer data);
void synce_partner_menu_response_2(GtkMenuItem *menuitem, gpointer data);

void synce_replace_old_partner(GtkButton *button, gpointer user_data);
void synce_create_new_partner(GtkButton *button, gpointer user_data);

void synce_ok_button(GtkButton *button, gpointer user_data);
void synce_cancel_button(GtkButton *button, gpointer user_data);
void synce_window_closed(void);

#endif
