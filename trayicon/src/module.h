#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>

gboolean
module_load_all();

gboolean
module_unload_all();

void
module_run_connect(const gchar *device_name);

void
module_run_disconnect(const gchar *device_name);
