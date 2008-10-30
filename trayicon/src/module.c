#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <gmodule.h>
#include <libgnome/libgnome.h>


typedef void (* ModuleConnectFunc) (const gchar *device_name);
typedef void (* ModuleDisconnectFunc) (const gchar *device_name);


typedef struct _module_data
{
  GModule *module;
  gchar *name;
  ModuleConnectFunc connect;
  ModuleDisconnectFunc disconnect;
} module_data;

GList * module_list = NULL;


gboolean
module_load_all()
{
  GError *error = NULL;
  gchar *dir_path, *file_path;
  const gchar *filename;
  GModule *module;

  if (!g_module_supported())
    return FALSE;

  dir_path = gnome_program_locate_file(NULL,
				       GNOME_FILE_DOMAIN_APP_LIBDIR,
				       "synce-trayicon/modules",
				       TRUE, /* only_if_exists */
				       NULL);
  if (!dir_path) {
    g_critical("%s: module dir path not found", G_STRFUNC);
    return FALSE;
  }

  GDir *module_dir = g_dir_open(dir_path, 0, &error);
  if (!module_dir) {
    if (error) {
      g_critical("%s: Error opening module dir %s: %s", G_STRFUNC, dir_path, error->message);
      g_error_free(error);    
    } else {
      g_critical("%s: Unknown error opening module dir %s", G_STRFUNC, dir_path);
    }
    g_free(dir_path);
    return FALSE;
  }

  while((filename = g_dir_read_name(module_dir))) {
    if (!g_str_has_suffix(filename, ".so"))
      continue;

    file_path = g_strdup_printf("%s/%s", dir_path, filename);

    g_debug("%s: loading module %s", G_STRFUNC, file_path);

    module = g_module_open(file_path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
    if (!module) {
      g_critical("%s: error opening module %s: %s", G_STRFUNC, file_path, g_module_error ());
      g_free(file_path);
      continue;
    }

    module_data *mod_data = g_malloc0(sizeof(module_data));

    mod_data->module = module;
    mod_data->name = g_strdup(filename);

    gpointer func_addr = NULL;

    if (!g_module_symbol(module, "module_connect_func", &func_addr )) {
      g_debug("%s: no connect_func for module %s", G_STRFUNC, filename);
      mod_data->connect = NULL;
    } else {
      mod_data->connect = (ModuleConnectFunc) func_addr;
    }
   
    if (!g_module_symbol(module, "module_disconnect_func", &func_addr )) {
      g_debug("%s: no disconnect_func for module %s", G_STRFUNC, filename);
      mod_data->disconnect = NULL;
    } else {
      mod_data->disconnect = (ModuleDisconnectFunc) func_addr;
    }

    module_list = g_list_prepend(module_list, (gpointer) mod_data);

    g_free(file_path);
  }
  g_dir_close(module_dir);
  g_free(dir_path);

  return TRUE;
}

gboolean
module_unload_all()
{
  module_data *mod_data;
  GList *curr_mod = module_list;

  while(curr_mod) {
    mod_data = curr_mod->data;

    if (!g_module_close(mod_data->module))
      g_critical("%s: error closing module %s: %s", G_STRFUNC, mod_data->name, g_module_error ());

    g_free(mod_data->name);
    mod_data->connect = NULL;
    mod_data->disconnect = NULL;
    g_free(mod_data);
    curr_mod = g_list_next(curr_mod);
  }

  g_list_free(module_list);

  return TRUE;
}


void
module_run_connect(const gchar *device_name)
{
  module_data *module = NULL;

  GList *current_module = module_list;

  while(current_module) {
    module = current_module->data;

    if (module->connect)
      module->connect(device_name);

    current_module = g_list_next(current_module);
  }
  return;
}

void
module_run_disconnect(const gchar *device_name)
{
  module_data *module = NULL;

  GList *current_module = module_list;

  while(current_module) {
    module = current_module->data;

    if (module->disconnect)
      module->disconnect(device_name);

    current_module = g_list_next(current_module);
  }
  return;
}

