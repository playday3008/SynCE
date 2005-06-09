/* 
 * Copyright (c) 2005 Andrei Yurkevich <urruru@ru.ru>
 * Copyright (c) 2002 David Eriksson <twogood@users.sourceforge.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
 
#include "udevinfo.h"
#include "misc.h"

extern short int hal_verbose;

static gchar *
udevinfo_run_query (gchar **argv)
{
	gint retval;
	gchar *stdout, *stderr;
	GError *err = NULL;

	if (g_spawn_sync (NULL, argv, NULL, 0, NULL, NULL, &stdout, &stderr, &retval, &err)) {
		if (retval != 0)
			return (NULL);
		else {
			if (stderr)
				g_free (stderr);
			if (stdout) {
				gchar *p = stdout;
				do {
					if (*p == '\n') {
						*p = '\0';
					}
				} while (*p++);
				return (stdout);
			}
			else {
				return (NULL);
			}
		}
	}
	else {
		SHC_ERROR ("could not run udevinfo: %s", err->message);
		return (NULL);
	}
}

gchar *
udevinfo_get_node (gchar *sysfs_path)
{
    gchar *argv_name[] = { UDEVINFO, "-q", "name", "-p", sysfs_path, NULL };
    gchar *argv_root[] = { UDEVINFO, "-r", NULL };

    gchar *root = udevinfo_run_query(argv_root);
    gchar *name = udevinfo_run_query (argv_name);

    if (root == NULL || name == NULL)
	return NULL;
    else {
	gchar *node = g_strdup_printf ("%s/%s", root, name);
	SHC_INFO ("device node for %s is %s", sysfs_path, node);
	return (node);
    }
}
