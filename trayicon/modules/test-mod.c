#include "../src/synce-trayicon-module.h"

#include <glib.h>
#include <gmodule.h>
#include <synce_log.h>

void
module_connect_func (const gchar *device_name)
{
  g_debug("%s: running from trayicon test module for %s", G_STRFUNC, device_name);
  return;
}

void
module_disconnect_func (const gchar *device_name)
{
  g_debug("%s: running from trayicon test module for %s", G_STRFUNC, device_name);
  return;
}

const gchar*
g_module_check_init(GModule *module)
{
  g_debug("%s: running from trayicon test module for %s", G_STRFUNC, g_module_name(module));
  return NULL;
}

void
g_module_unload(GModule *module)
{
  g_debug("%s: running from trayicon test module for %s", G_STRFUNC, g_module_name(module));
  return;
}
