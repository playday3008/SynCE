/***************************************************************************
 * Copyright (c) 2009 Mark Ellis <mark_ellis@users.sourceforge.net>         *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/
#include <config.h>

#include "cescreen.h"
#include <glib/gi18n.h>
#include <synce_log.h>

static gint log_level = SYNCE_LOG_LEVEL_WARNING;
static gboolean synce = TRUE;
static gboolean forceinstall = FALSE;
static CeScreen *ce_screen = NULL;

static GOptionEntry options[] =
        {
                { "debug", 'd', 0, G_OPTION_ARG_INT, &log_level, N_("Set debug level 1-5"), "level" },
                { "nosynce", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &synce, N_("the pdaname is an IP-Address/Hostname"), NULL },
                { "forceinstall", 0, 0, G_OPTION_ARG_NONE, &forceinstall, N_("force the installation of the pda component"), NULL },

                { NULL }
        };


static void
pda_error_cb(CeScreen *ce_screen, gpointer user_data)
{
        g_critical("%s: Could not contact PDA %s", G_STRFUNC, (gchar*)user_data);
        gtk_main_quit();
        gtk_widget_destroy(GTK_WIDGET(ce_screen));
}

gint
main(gint argc, gchar *argv[])
{
#ifdef ENABLE_NLS
        bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);
#endif

        GOptionContext *option_context = g_option_context_new (" - gtk remote viewer for synCE");
        g_option_context_add_main_entries (option_context, options, GETTEXT_PACKAGE);


        if (!gtk_init_with_args(&argc,
                                &argv,
                                " - gtk remote viewer for synCE",
                                options,
                                NULL,
                                NULL))
                g_error("%s: failed to initialise GTK", G_STRFUNC);

        g_set_application_name(_("SynCE GCEMirror"));

        synce_log_set_level(log_level);

        if (argc < 2)
                g_error("%s: no device specified", G_STRFUNC);


        gchar *pda_name = g_strdup(argv[1]);

        g_debug("%s: Synce: %d", G_STRFUNC, synce);
        g_debug("%s: ForceInstall: %d", G_STRFUNC, forceinstall);
        g_debug("%s: device: %s", G_STRFUNC, pda_name);

        ce_screen = g_object_new(CE_SCREEN_TYPE, NULL);

        g_signal_connect(G_OBJECT(ce_screen), "pda-error", G_CALLBACK(pda_error_cb), pda_name);

        ce_screen_connect(ce_screen, pda_name, synce, forceinstall);

        gtk_widget_show_all(GTK_WIDGET(ce_screen));
        gtk_main();

        g_free(pda_name);
        return 0;
}

