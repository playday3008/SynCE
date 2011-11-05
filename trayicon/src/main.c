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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <errno.h>

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
	GError *error = NULL;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	if (!gtk_init_with_args(&argc, &argv,
				" - gnome control for synCE",
				options, GETTEXT_PACKAGE,
				&error))
	  {
	    g_error("%s: failed to initialise GTK: %s", G_STRFUNC, error->message);
	  }

        g_set_application_name(_("SynCE Tray Icon"));

 	synce_log_set_level(log_level);

	if (in_background)
	{
		synce_log_use_syslog();
		g_debug("Forking into background");
		if (daemon(0,0) == -1) {
                        g_critical("%s: failed to fork into background: %d: %s", G_STRFUNC, errno, g_strerror(errno));
                        return -1;
                }
	}
	else
	{
		g_debug("Running in foreground");
	}

	remove_obsolete_script();

	trayicon = g_object_new (SYNCE_TRAYICON_TYPE, NULL);

	gtk_main ();

	g_object_unref(trayicon);

	result = 0;
	return result;
}
