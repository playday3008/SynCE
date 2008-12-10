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

#include <synce_log.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gnome.h>
#include <glade/glade.h>

#include "synce-trayicon.h"

static bool in_background = true;
static gint log_level = SYNCE_LOG_LEVEL_LOWEST;

static GOptionEntry options[] = 
  {
    { "debug", 'd', 0, G_OPTION_ARG_INT, &log_level, N_("Set debug level 1-5"), "level" },
    { "foreground", 'f', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &in_background, "Run in foreground", NULL },
    { NULL }
  };


static void
init_sm (gboolean restart_immediately)
{
    GnomeClient *master;
    GnomeClientFlags flags;
                                                                                
    master = gnome_master_client ();
    flags = gnome_client_get_flags (master);
    if (flags & GNOME_CLIENT_IS_CONNECTED) {
       if (restart_immediately)
          gnome_client_set_restart_style (master, GNOME_RESTART_IMMEDIATELY);
       else
          gnome_client_set_restart_style (master, GNOME_RESTART_NEVER);
        gnome_client_flush (master);
    }
                                                                                
    g_signal_connect (G_OBJECT (master), "die",
            G_CALLBACK (gtk_main_quit), NULL);
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

static void
remove_obsolete_script()
{
  gchar *synce_dir = NULL, *script_path = NULL;

  if (!(synce_get_script_directory(&synce_dir)))
    g_error("%s: Cannot obtain synce script dir", G_STRFUNC);
  script_path = g_strdup_printf("%s/trayicon.sh", synce_dir);
  g_unlink(script_path);
  g_free(script_path);
}

int
main (gint argc, gchar **argv)
{
	int result = 1;
	SynceTrayIcon *trayicon;
	gboolean restart = TRUE;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	GOptionContext *option_context = g_option_context_new (" - gnome control for synCE");
	g_option_context_add_main_entries (option_context, options, GETTEXT_PACKAGE);
	gnome_program_init (PACKAGE, VERSION,
			LIBGNOMEUI_MODULE,
			argc, argv,
			GNOME_PARAM_GOPTION_CONTEXT, option_context,
			GNOME_PARAM_HUMAN_READABLE_NAME, _("SynCE Tray Icon"),
			GNOME_PROGRAM_STANDARD_PROPERTIES,
			NULL);

 	synce_log_set_level(log_level);

	glade_gnome_init ();

	if (in_background)
	{
		synce_log_use_syslog();
		g_debug("Forking into background");
		daemon(0,0);
	}
	else
	{
		g_debug("Running in foreground");
		restart = FALSE;
	}

	remove_obsolete_script();

	init_sm (restart);

	trayicon = g_object_new (SYNCE_TRAYICON_TYPE, NULL);

	gtk_main ();

	g_object_unref(trayicon);

	unreg_sm();

	result = 0;
	return result;
}
