#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <glibtop.h>
#include <glibtop/proclist.h>
#include <glibtop/procstate.h>
#include <glibtop/procargs.h>
#include <unistd.h>

char *
get_process_name (gchar *cmd, gchar *args)
{
    gchar *command = NULL;
    gint i, n = 0, len, newlen;
    gboolean done = FALSE;
	char *name;
                                                                                
    /* strip the absolute path from the arguments */
    if (args)
    {
        len = strlen (args);
        i = len;
        while (!done)
        {
            /* no / in string */
            if (i == 0) {
                n = 0;
                done = TRUE;
            }
            if (args[i] == '/') {
                done = TRUE;
                n = i + 1;
            }
            i--;
        }
        newlen = len - n;
        command = g_new (gchar, newlen + 1);
        for (i = 0; i < newlen; i++) {
            command[i] = args[i + n];
        }
        command[newlen] = '\0';
    }
                                                                                
    if (command)
        name = g_strdup (command);
    else if (!command)
        name = g_strdup (cmd);
                                                                                
    if (command)
        g_free (command);

	return name;
                                                                        
}


gboolean
dccm_is_running () {
	glibtop_proclist proclist;
	unsigned *pid_list;
	gint i = 0;
	gint n = 0;
	gint which, arg;
	gboolean result = FALSE;

	glibtop_init ();
	
	/* dccm skall köras som user */
	which = GLIBTOP_KERN_PROC_UID;
	arg = getuid ();
	pid_list = glibtop_get_proclist (&proclist, which, arg);
	n = proclist.number;
	
	
	while (i < n)
	 {
		glibtop_proc_args procargs;
		glibtop_proc_state procstate;
		gchar *arguments;
		gchar *name;

		glibtop_get_proc_state (&procstate, pid_list[i]);
		arguments = glibtop_get_proc_args (&procargs, pid_list[i], 0);
		name = get_process_name (procstate.cmd, arguments);
		if (g_ascii_strncasecmp("dccm", name, 4) == 0) {
			result = TRUE;
			g_free(name);
			g_free(arguments);
			break;
		}	
		g_free(name);
		g_free(arguments);

		i++;
	 }

	g_free (pid_list);

	return(result);
}

