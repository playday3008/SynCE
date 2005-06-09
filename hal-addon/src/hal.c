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

#include <stdlib.h>
#include <string.h>
#include <libhal.h>

#include "hal.h"
#include "misc.h"

static DBusError err;
static LibHalContext *ctx = NULL;
static gboolean hal_initialized = FALSE;

extern gboolean hal_verbose;

static void
hal_init                   (void)
{
    SHC_INFO ("Initializing HAL library");

	dbus_error_init (&err);
	if ((ctx = libhal_ctx_init_direct (&err)) == NULL) {
		SHC_ERROR ("Unable to initialize HAL library");
		exit (1);
	}

    hal_initialized = TRUE;
	return;
}

void
hal_add_capability         (gchar     *udi,
			    gchar     *capability)
{
	if (!hal_initialized)
		hal_init ();

	if (!libhal_device_query_capability (ctx, udi, capability, &err))
		libhal_device_property_strlist_append (ctx, udi, "info.capabilities", capability, &err);
}

void
hal_set_property           (gchar     *udi,
			    guint     type,
			    gchar     *property,
			    gpointer  value)
{
	if (!hal_initialized)
		hal_init ();

	if (type == PROPERTY_TYPE_STRING) {
		SHC_INFO ("Setting %s=%s", property, (gchar *) value);
		libhal_device_set_property_string (ctx, udi, property, (gchar *) value, &err);
	}
	else if (type == PROPERTY_TYPE_BOOL) {
		SHC_INFO ("Setting %s=%s", property, (gboolean) value ? "TRUE" : "FALSE");
		libhal_device_set_property_bool (ctx, udi, property, (gboolean) value, &err);
	}
	else if (type == PROPERTY_TYPE_INT) {
		SHC_INFO ("Setting %s=%d", property, (gint) value);
		libhal_device_set_property_int (ctx, udi, property, (gint) value, &err);
	}
}

gchar *
hal_check_device           (gchar  *udi)
{
	if (! strlen (udi ? udi : ""))
		return NULL;
	
	if (! hal_initialized)
		hal_init ();
	
	/* check for "pda" capability */
	if (! libhal_device_query_capability (ctx, udi, "pda", &err))
		return NULL;
	
	if (libhal_device_property_exists (ctx, udi, "serial.interface", &err))
		return libhal_device_get_property_string (ctx, udi, "serial.interface", &err);
	else
		return NULL;
}

gchar *
hal_get_sysfs_path         (gchar  *udi)
{
	if (! strlen (udi ? udi : ""))
		return NULL;
	
	if (! hal_initialized)
		hal_init ();
	
	return libhal_device_get_property_string (ctx, udi, "linux.sysfs_path", NULL);
}
